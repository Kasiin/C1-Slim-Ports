package main

import (
	"context"
	"fmt"
	"net/http"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
)

func TestPlain(t *testing.T) {
	s := plain(`<p>中文 &amp; A</p><script>bad()</script><style>evil</style><p>下一行<br>结束<img src="https://invalid"></p>`)
	if s != "中文 & A\n下一行\n结束" {
		t.Fatal(s)
	}
	if plain("2 &lt; 3") != "2 < 3" {
		t.Fatal("escaped less-than lost")
	}
}
func TestRSS(t *testing.T) {
	s := `<rss xmlns:content="http://purl.org/rss/1.0/modules/content/"><channel><item><title>标题 &amp; 测试</title><link>https://example.com</link><description>摘要</description><content:encoded><![CDATA[<p>真正正文</p>]]></content:encoded><pubDate>Fri, 04 Sep 2026 10:00:00 +0800</pubDate></item><item><title>标题 &amp; 测试</title></item></channel></rss>`
	f, e := parseFeed("ifanr", []byte(s))
	if e != nil || len(f.Articles) != 1 {
		t.Fatal(f, e)
	}
	a := f.Articles[0]
	if a.Title != "标题 & 测试" || a.Body != "真正正文" || a.Date != "2026-09-04" || a.Summary {
		t.Fatal(a)
	}
	if _, e := parseFeed("ifanr", []byte("<html>blocked</html>")); e == nil {
		t.Fatal("accepted error page")
	}
}
func Test60s(t *testing.T) {
	f, e := parseFeed("60s", []byte(`{"code":200,"data":{"date":"2026-09-04","news":["测试新闻"],"tip":"微语"}}`))
	if e != nil || len(f.Articles) != 2 {
		t.Fatal(f, e)
	}
	if _, e = parseFeed("60s", []byte(`{"code":500}`)); e == nil {
		t.Fatal("accepted error")
	}
}
func TestCache(t *testing.T) {
	dir := t.TempDir()
	f, _ := parseFeed("60s", []byte(`{"code":200,"data":{"date":"2026-09-04","news":["新闻"]}}`))
	if e := saveFeed(dir, f); e != nil {
		t.Fatal(e)
	}
	g, e := loadFeed(dir, "60s")
	if e != nil || g.Articles[0].Body != "新闻" {
		t.Fatal(g, e)
	}
	if _, e = loadFeed(dir, "ifanr"); e == nil {
		t.Fatal("accepted missing")
	}
	os.WriteFile(filepath.Join(dir, "60s.json"), []byte(`{"Version":1}`), 0644)
	if _, e = loadFeed(dir, "60s"); e == nil {
		t.Fatal("accepted corrupt cache")
	}
}
func TestHTTP(t *testing.T) {
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { http.Error(w, "limited", 429) }))
	defer srv.Close()
	if _, e := fetch(context.Background(), Source{"60s", "", srv.URL}); e == nil {
		t.Fatal("accepted HTTP 429")
	}
}
func TestWrap(t *testing.T) {
	for _, s := range []string{strings.Repeat("数独日报", 200), "中文 English 123\n第二行", strings.Repeat("a", 72)} {
		lines := wrap(s, 288)
		for _, l := range lines {
			w := 0
			for _, r := range l {
				w += glyphWidth(r)
			}
			if w > 288 {
				t.Fatal(w)
			}
		}
		if strings.Join(lines, "") != strings.ReplaceAll(s, "\n", "") {
			t.Fatal("lost text")
		}
	}
	if len(wrap(strings.Repeat("中", 18*6), 288)) != 6 {
		t.Fatal("extra blank page")
	}
}
func TestNavigation(t *testing.T) {
	u := UI{}
	for i := 0; i < 20; i++ {
		u.Feeds[0].Articles = append(u.Feeds[0].Articles, Article{Title: fmt.Sprint(i), Body: strings.Repeat("中", 300), Date: "2026-09-04"})
	}
	u.key(28)
	if u.View != 1 {
		t.Fatal(u)
	}
	u.key(106)
	if u.Selected != 2 {
		t.Fatal(u.Selected)
	}
	u.key(28)
	if u.View != 2 {
		t.Fatal(u.View)
	}
	u.key(106)
	if u.Page != 1 {
		t.Fatal(u.Page)
	}
	q, _ := u.key(102)
	if q || u.View != 2 {
		t.Fatal("HOME")
	}
	u.key(158)
	if u.View != 1 {
		t.Fatal("back article")
	}
	u.key(158)
	if u.View != 0 {
		t.Fatal("back list")
	}
	q, _ = u.key(158)
	if !q {
		t.Fatal("exit")
	}
}
func TestRender(t *testing.T) {
	u := UI{}
	a := u.render()
	b := u.render()
	if a != b {
		t.Fatal("unstable frame")
	}
	if len(font) != 65536*32 {
		t.Fatal("font size")
	}
	u.key(108)
	if a == u.render() {
		t.Fatal("selection unchanged")
	}
}

func TestSmallLayout(t *testing.T) {
	for _, s := range []string{strings.Repeat("中文段落。", 80), "这是一个 English word boundary 测试，不能丢字。", strings.Repeat("a", 100), "标题\n正文第一段\n第二段"} {
		lines := wrapSmall(s, 272)
		if strings.Join(lines, "") != strings.ReplaceAll(s, "\n", "") {
			t.Fatal("small wrap lost text")
		}
		for _, line := range lines {
			if measure(line) > 272 {
				t.Fatal("line overflow")
			}
		}
		pages := paginate(s)
		joined := ""
		for _, page := range pages {
			if len(page) == 0 {
				t.Fatal("blank page")
			}
			last := 0
			for _, line := range page {
				if line.Y < 32 || line.Y+16 > 123 || line.Y < last {
					t.Fatal("reader overlaps chrome")
				}
				last = line.Y + 16
				joined += line.Text
			}
		}
		if joined != strings.ReplaceAll(s, "\n", "") {
			t.Fatal("pagination lost content")
		}
	}
	p := paginate("首段\n次段")
	if len(p) != 1 || p[0][1].Y-p[0][0].Y != 25 {
		t.Fatal("paragraph spacing missing")
	}
	if len(font14) != 65536*32 || len(width14) != 65536 {
		t.Fatal("small font size")
	}
	for _, label := range []string{"确认 阅读", "Back 返回", "下页 →"} {
		if measure(label) > 83 {
			t.Fatal("button label overflow", label)
		}
	}
}
