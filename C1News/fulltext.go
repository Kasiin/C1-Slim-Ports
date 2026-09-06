package main

import (
	"bytes"
	"context"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"regexp"
	"strings"
	"sync"
	"time"

	"golang.org/x/net/html"
)

var publicPostPath = regexp.MustCompile(`^/post/[0-9]+/?$`)

func allowedPost(raw string) bool {
	u, e := url.Parse(raw)
	return e == nil && u.Scheme == "https" && u.Host == "sspai.com" && u.User == nil && publicPostPath.MatchString(u.Path)
}
func hasClass(n *html.Node, wanted string) bool {
	for _, a := range n.Attr {
		if a.Key == "class" {
			for _, s := range strings.Fields(a.Val) {
				if s == wanted {
					return true
				}
			}
		}
	}
	return false
}
func invisible(n *html.Node) bool {
	if n.Type != html.ElementNode {
		return false
	}
	switch n.Data {
	case "script", "style", "noscript", "iframe", "video", "audio", "button", "form":
		return true
	}
	for _, a := range n.Attr {
		v := strings.ToLower(strings.ReplaceAll(a.Val, " ", ""))
		if a.Key == "hidden" || a.Key == "aria-hidden" && v == "true" || a.Key == "style" && (strings.Contains(v, "display:none") || strings.Contains(v, "visibility:hidden")) {
			return true
		}
	}
	return false
}

// Only visible, server-rendered article content. Never extract serialized app
// state, hidden content, comments, recommendations, or protected API responses.
func publicBody(data []byte) (string, error) {
	doc, e := html.Parse(bytes.NewReader(data))
	if e != nil {
		return "", e
	}
	var content *html.Node
	var locate func(*html.Node)
	locate = func(n *html.Node) {
		if content != nil || invisible(n) {
			return
		}
		if hasClass(n, "article__main__wrapper") {
			content = n
			return
		}
		for c := n.FirstChild; c != nil; c = c.NextSibling {
			locate(c)
		}
	}
	locate(doc)
	if content == nil {
		return "", fmt.Errorf("public article container missing")
	}
	blocked := false
	var clean func(*html.Node)
	clean = func(n *html.Node) {
		for _, a := range n.Attr {
			if a.Key == "class" {
				for _, c := range strings.Fields(a.Val) {
					if c == "paywall" || strings.Contains(c, "pay__wall") || strings.Contains(c, "purchase__") || strings.Contains(c, "paid__mask") {
						blocked = true
					}
				}
			}
		}
		for c := n.FirstChild; c != nil; {
			next := c.NextSibling
			if invisible(c) {
				n.RemoveChild(c)
			} else {
				clean(c)
			}
			c = next
		}
	}
	clean(content)
	var b bytes.Buffer
	if e = html.Render(&b, content); e != nil {
		return "", e
	}
	body := plain(b.String())
	for _, s := range []string{"订阅后阅读全文", "购买后阅读全文", "登录后阅读全文", "付费后阅读全文", "购买后解锁全文", "本文为付费内容"} {
		if strings.Contains(body, s) {
			blocked = true
		}
	}
	if blocked || len([]rune(body)) < 160 {
		return "", fmt.Errorf("article is restricted or too short to verify")
	}
	return limitText(body, 30000), nil
}
func fetchPublicBody(ctx context.Context, raw string) (string, error) {
	if !allowedPost(raw) {
		return "", fmt.Errorf("not a public SSPAI article URL")
	}
	client := &http.Client{Timeout: 18 * time.Second, CheckRedirect: func(req *http.Request, via []*http.Request) error {
		if len(via) > 3 || !allowedPost(req.URL.String()) {
			return fmt.Errorf("unexpected article redirect")
		}
		return nil
	}}
	req, e := http.NewRequestWithContext(ctx, "GET", raw, nil)
	if e != nil {
		return "", e
	}
	req.Header.Set("User-Agent", "C1News/1.2 (personal RSS reader)")
	resp, e := client.Do(req)
	if e != nil {
		return "", e
	}
	defer resp.Body.Close()
	if resp.StatusCode != 200 {
		return "", fmt.Errorf("article HTTP %d", resp.StatusCode)
	}
	data, e := io.ReadAll(io.LimitReader(resp.Body, maxResponse+1))
	if e != nil {
		return "", e
	}
	if len(data) > maxResponse {
		return "", fmt.Errorf("article too large")
	}
	return publicBody(data)
}
func enrichSSPAI(ctx context.Context, f Feed, old Feed, fetcher func(context.Context, string) (string, error)) Feed {
	existing := map[string]Article{}
	for _, a := range old.Articles {
		if !a.Summary && allowedPost(a.URL) {
			existing[a.URL] = a
		}
	}
	jobs := make(chan int, len(f.Articles))
	for i, a := range f.Articles {
		if !a.Summary || !allowedPost(a.URL) {
			continue
		}
		if cached, ok := existing[a.URL]; ok && cached.Title == a.Title {
			f.Articles[i].Body = cached.Body
			f.Articles[i].Summary = false
			continue
		}
		jobs <- i
	}
	close(jobs)
	var wg sync.WaitGroup
	for worker := 0; worker < 2; worker++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for i := range jobs {
				if ctx.Err() != nil {
					return
				}
				body, e := fetcher(ctx, f.Articles[i].URL)
				if e == nil && len([]rune(body)) > len([]rune(f.Articles[i].Body)) {
					f.Articles[i].Body = body
					f.Articles[i].Summary = false
				}
			}
		}()
	}
	wg.Wait()
	return f
}
func refreshFeed(ctx context.Context, src Source, dir string) (Feed, error) {
	f, e := fetch(ctx, src)
	if e != nil || src.ID != "sspai" {
		return f, e
	}
	old, _ := loadFeed(dir, src.ID)
	// Independent source worker, bounded concurrency and time; UI stays responsive.
	articleCtx, cancel := context.WithTimeout(ctx, 45*time.Second)
	defer cancel()
	return enrichSSPAI(articleCtx, f, old, fetchPublicBody), nil
}
