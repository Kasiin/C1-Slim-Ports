package main

import (
	"context"
	"fmt"
	"os"
	"strings"
	"sync/atomic"
	"testing"
)

func TestPublicArticle(t *testing.T) {
	body := strings.Repeat("这里是公开可阅读的文章正文。", 20)
	page := `<html><nav>不应出现的导航</nav><div class="article__main__wrapper"><div class="article__main__content"><p>` + body + `</p><p style="display:none">隐藏内容不应读取</p><script>隐藏数据</script></div></div><div>评论区</div></html>`
	got, e := publicBody([]byte(page))
	if e != nil || got != body {
		t.Fatal(got, e)
	}
	for _, bad := range []string{`<div>` + body + `</div>`, strings.Replace(page, body, "购买后阅读全文"+body, 1), strings.Replace(page, `class="article__main__wrapper"`, `class="article__main__wrapper paywall"`, 1)} {
		if _, e = publicBody([]byte(bad)); e == nil {
			t.Fatal("accepted restricted or unrecognized content")
		}
	}
}
func TestPostURL(t *testing.T) {
	for _, raw := range []string{"http://sspai.com/post/123", "https://evil.com/post/123", "https://sspai.com.evil.com/post/123", "https://sspai.com:443/post/123", "https://user@sspai.com/post/123", "https://sspai.com/api/article/123"} {
		if allowedPost(raw) {
			t.Fatal(raw)
		}
	}
	if !allowedPost("https://sspai.com/post/114164") {
		t.Fatal("public URL rejected")
	}
}
func TestEnrichFallback(t *testing.T) {
	articles := []Article{{Title: "已有正文", URL: "https://sspai.com/post/1", Body: "摘要", Summary: true}, {Title: "失败文章", URL: "https://sspai.com/post/2", Body: "保留摘要", Summary: true}, {Title: "新正文", URL: "https://sspai.com/post/3", Body: "摘要", Summary: true}}
	old := Feed{Articles: []Article{{Title: "已有正文", URL: articles[0].URL, Body: "上次缓存的全文", Summary: false}}}
	var calls atomic.Int32
	f := enrichSSPAI(context.Background(), Feed{Articles: articles}, old, func(_ context.Context, u string) (string, error) {
		calls.Add(1)
		if strings.HasSuffix(u, "/2") {
			return "", fmt.Errorf("offline")
		}
		return "本次成功获取的公开全文", nil
	})
	if calls.Load() != 2 || f.Articles[0].Summary || !f.Articles[1].Summary || f.Articles[1].Body != "保留摘要" || f.Articles[2].Summary {
		t.Fatal(f, calls.Load())
	}
}
func TestRealPublicPages(t *testing.T) {
	for _, path := range []string{"build/probes/sspai-article.html", "build/probes/sspai-normal.html"} {
		data, e := os.ReadFile(path)
		if os.IsNotExist(e) {
			continue
		}
		if e != nil {
			t.Fatal(e)
		}
		body, e := publicBody(data)
		if e != nil || len([]rune(body)) < 1000 {
			t.Fatal(path, len([]rune(body)), e)
		}
		t.Log(path, "public characters:", len([]rune(body)))
	}
}
