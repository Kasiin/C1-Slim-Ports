package main

import (
	"context"
	"encoding/json"
	"encoding/xml"
	"fmt"
	"io"
	"net/http"
	"os"
	"path/filepath"
	"strings"
	"time"
	"unicode"

	"golang.org/x/net/html"
)

type Source struct{ ID, Name, URL string }

var sources = []Source{
	{"60s", "60秒早报", "https://60s.viki.moe/v2/60s"},
	{"sspai", "少数派", "https://sspai.com/feed"},
	{"ifanr", "爱范儿", "https://www.ifanr.com/feed"},
}

type Article struct {
	Title, Body, Date, URL string
	Summary                bool
}
type Feed struct {
	Version     int
	ID, Fetched string
	Articles    []Article
}

const maxResponse = 4 << 20

// The HTML tokenizer removes images/scripts without fetching any article assets.
func plain(s string) string {
	z := html.NewTokenizer(strings.NewReader(s))
	var b strings.Builder
	skip := 0
	for {
		switch z.Next() {
		case html.ErrorToken:
			lines := strings.Split(b.String(), "\n")
			out := []string{}
			for _, line := range lines {
				line = strings.Join(strings.FieldsFunc(line, unicode.IsSpace), " ")
				if line != "" {
					out = append(out, line)
				}
			}
			return strings.Join(out, "\n")
		case html.StartTagToken, html.SelfClosingTagToken:
			t := z.Token()
			if t.Data == "script" || t.Data == "style" {
				skip++
			}
			if skip == 0 && (t.Data == "p" || t.Data == "br" || t.Data == "div" || t.Data == "li" || strings.HasPrefix(t.Data, "h") && len(t.Data) == 2) {
				b.WriteByte('\n')
			}
		case html.EndTagToken:
			t := z.Token()
			if t.Data == "script" || t.Data == "style" {
				if skip > 0 {
					skip--
				}
			}
			if skip == 0 && (t.Data == "p" || t.Data == "div" || t.Data == "li" || t.Data == "blockquote") {
				b.WriteByte('\n')
			}
		case html.TextToken:
			if skip == 0 {
				b.Write(z.Text())
			}
		}
	}
}
func limitText(s string, n int) string {
	r := []rune(s)
	if len(r) > n {
		return string(r[:n]) + "\n[内容过长，已截断]"
	}
	return s
}
func dateText(s string) string {
	for _, layout := range []string{time.RFC1123Z, time.RFC1123, time.RFC3339, "2006-01-02"} {
		if t, e := time.Parse(layout, s); e == nil {
			return t.Format("2006-01-02")
		}
	}
	return limitText(strings.TrimSpace(s), 24)
}
func parseFeed(id string, data []byte) (Feed, error) {
	f := Feed{Version: 1, ID: id, Fetched: time.Now().UTC().Format(time.RFC3339)}
	if id == "60s" {
		var v struct {
			Code int
			Data struct {
				Date      string
				News      []string
				Tip, Link string
			}
		}
		if e := json.Unmarshal(data, &v); e != nil {
			return f, e
		}
		if v.Code != 200 || len(v.Data.News) == 0 {
			return f, fmt.Errorf("empty 60s response")
		}
		if _, e := time.Parse("2006-01-02", v.Data.Date); e != nil {
			return f, fmt.Errorf("invalid news date")
		}
		for i, s := range v.Data.News {
			if strings.TrimSpace(s) != "" {
				f.Articles = append(f.Articles, Article{Title: fmt.Sprintf("%02d %s", i+1, limitText(s, 200)), Body: limitText(s, 3000), Date: v.Data.Date, URL: v.Data.Link})
			}
		}
		if v.Data.Tip != "" {
			f.Articles = append(f.Articles, Article{Title: "每日微语", Body: limitText(v.Data.Tip, 1000), Date: v.Data.Date, URL: v.Data.Link})
		}
	} else {
		// Tags are explicit: RSS field names are case-sensitive.
		var raw struct {
			XMLName xml.Name `xml:"rss"`
			Channel struct {
				Items []struct {
					Title       string `xml:"title"`
					Link        string `xml:"link"`
					Description string `xml:"description"`
					Content     string `xml:"encoded"`
					Date        string `xml:"pubDate"`
				} `xml:"item"`
			} `xml:"channel"`
		}
		if e := xml.Unmarshal(data, &raw); e != nil {
			return f, e
		}
		seen := map[string]bool{}
		for _, v := range raw.Channel.Items {
			title := plain(v.Title)
			if title == "" || seen[title] {
				continue
			}
			seen[title] = true
			body := v.Content
			summary := body == ""
			if summary {
				body = v.Description
			}
			body = strings.TrimSpace(strings.TrimSuffix(plain(body), "查看全文"))
			if body == "" {
				body = "此订阅源只提供标题。"
			}
			f.Articles = append(f.Articles, Article{limitText(title, 300), limitText(body, 30000), dateText(v.Date), v.Link, summary})
			if len(f.Articles) >= 30 {
				break
			}
		}
	}
	if len(f.Articles) == 0 {
		return f, fmt.Errorf("no articles")
	}
	return f, nil
}
func fetch(ctx context.Context, src Source) (Feed, error) {
	req, e := http.NewRequestWithContext(ctx, "GET", src.URL, nil)
	if e != nil {
		return Feed{}, e
	}
	req.Header.Set("User-Agent", "C1News/1.0 (personal RSS reader)")
	client := &http.Client{Timeout: 22 * time.Second}
	resp, e := client.Do(req)
	if e != nil {
		return Feed{}, e
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		return Feed{}, fmt.Errorf("HTTP %d", resp.StatusCode)
	}
	data, e := io.ReadAll(io.LimitReader(resp.Body, maxResponse+1))
	if e != nil {
		return Feed{}, e
	}
	if len(data) > maxResponse {
		return Feed{}, fmt.Errorf("feed too large")
	}
	return parseFeed(src.ID, data)
}
func loadFeed(dir, id string) (Feed, error) {
	var f Feed
	file, e := os.Open(filepath.Join(dir, id+".json"))
	if e != nil {
		return f, e
	}
	defer file.Close()
	data, e := io.ReadAll(io.LimitReader(file, maxResponse+1))
	if e != nil || len(data) > maxResponse {
		return f, fmt.Errorf("invalid cache size")
	}
	if e = json.Unmarshal(data, &f); e != nil {
		return f, e
	}
	if f.Version != 1 || f.ID != id || len(f.Articles) == 0 || len(f.Articles) > 40 {
		return f, fmt.Errorf("invalid cache")
	}
	if _, e = time.Parse(time.RFC3339, f.Fetched); e != nil {
		return f, e
	}
	for _, a := range f.Articles {
		if a.Title == "" || len([]rune(a.Title)) > 400 || len([]rune(a.Body)) > 31000 {
			return f, fmt.Errorf("invalid cached article")
		}
	}
	return f, nil
}
func saveFeed(dir string, f Feed) error {
	if e := os.MkdirAll(dir, 0755); e != nil {
		return e
	}
	data, e := json.Marshal(f)
	if e != nil {
		return e
	}
	tmp, e := os.CreateTemp(dir, ".news-*")
	if e != nil {
		return e
	}
	name := tmp.Name()
	defer os.Remove(name)
	if _, e = tmp.Write(data); e != nil {
		tmp.Close()
		return e
	}
	if e = tmp.Sync(); e != nil {
		tmp.Close()
		return e
	}
	if e = tmp.Close(); e != nil {
		return e
	}
	return os.Rename(name, filepath.Join(dir, f.ID+".json"))
}
