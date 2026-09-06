package main

import (
	_ "embed"
	"encoding/binary"
	"fmt"
	"strings"
	"time"
)

//go:embed font.bin
var font []byte

type Frame [5624]byte

func (f *Frame) pixel(x, y int, black bool) {
	if x < 0 || x >= 296 || y < 0 || y >= 152 {
		return
	}
	mask := byte(0x80 >> uint(y%8))
	i := y/8*296 + x
	if black {
		f[i] |= mask
	} else {
		f[i] &= ^mask
	}
}
func (f *Frame) rect(x, y, w, h int, black bool) {
	for j := y; j < y+h; j++ {
		for i := x; i < x+w; i++ {
			f.pixel(i, j, black)
		}
	}
}
func glyphWidth(r rune) int {
	if r >= 32 && r < 127 {
		return 8
	}
	return 16
}
func (f *Frame) text(x, y int, s string, black bool) {
	for _, r := range s {
		w := glyphWidth(r)
		if x+w > 296 {
			return
		}
		if r < 32 || r > 65535 {
			r = 0xfffd
		}
		off := int(r) * 32
		for row := 0; row < 16; row++ {
			v := binary.BigEndian.Uint16(font[off+row*2:])
			for col := 0; col < w; col++ {
				if v&(0x8000>>uint(col)) != 0 {
					f.pixel(x+col, y+row, black)
				}
			}
		}
		x += w
	}
}
func fit(s string, width int) string {
	n := 0
	var b strings.Builder
	for _, r := range s {
		w := glyphWidth(r)
		if n+w > width {
			break
		}
		b.WriteRune(r)
		n += w
	}
	return b.String()
}
func wrap(s string, width int) []string {
	lines := []string{}
	var b strings.Builder
	n := 0
	for _, r := range s {
		if r == '\r' {
			continue
		}
		if r == '\n' {
			lines = append(lines, b.String())
			b.Reset()
			n = 0
			continue
		}
		w := glyphWidth(r)
		if n+w > width {
			lines = append(lines, b.String())
			b.Reset()
			n = 0
		}
		b.WriteRune(r)
		n += w
	}
	if b.Len() > 0 {
		lines = append(lines, b.String())
	}
	if len(lines) == 0 {
		lines = append(lines, "")
	}
	return lines
}

type UI struct {
	Feeds                        [3]Feed
	Status                       [3]string
	Source, Selected, Page, View int
	Lines                        []string
	Pages                        [][]ReadLine
	Busy                         bool
}

func (u *UI) openArticle() {
	f := u.Feeds[u.Source]
	if u.Selected >= len(f.Articles) {
		return
	}
	a := f.Articles[u.Selected]
	body := a.Title + "\n" + a.Body
	if u.Source == 0 && a.Title != "每日微语" {
		body = a.Body
	}
	if a.Summary {
		body += "\n[订阅源摘要，非全文]"
	}
	u.Lines = wrap(body, 288)
	u.Pages = paginate(body)
	u.Page = 0
	u.View = 2
}
func (u *UI) pageCount() int { return len(u.Pages) }

// Returns exit, refresh. HOME never exits or leaks to the stock launcher.
func (u *UI) key(k uint16) (bool, bool) {
	if k == 102 {
		return false, false
	}
	if k == 19 {
		return false, !u.Busy
	} // R refresh, once per press
	if k == 158 || k == 1 {
		if u.View == 0 {
			return true, false
		}
		u.View--
		return false, false
	}
	if k == 49 {
		u.View = 0
		return false, false
	} // N sources
	if u.View == 0 {
		switch k {
		case 103:
			u.Source = (u.Source + 2) % 3
		case 108:
			u.Source = (u.Source + 1) % 3
		case 16, 17, 18:
			u.Source = int(k - 16)
		case 28, 352:
			u.View = 1
			u.Selected = 0
		}
	} else if u.View == 1 {
		n := len(u.Feeds[u.Source].Articles)
		switch k {
		case 103:
			if u.Selected > 0 {
				u.Selected--
			}
		case 108:
			if u.Selected+1 < n {
				u.Selected++
			}
		case 105:
			u.Selected -= 2
			if u.Selected < 0 {
				u.Selected = 0
			}
		case 106:
			u.Selected += 2
			if u.Selected >= n {
				u.Selected = max(0, n-1)
			}
		case 28, 352:
			if n > 0 {
				u.openArticle()
			}
		}
	} else {
		switch k {
		case 108, 106, 28, 352:
			if u.Page+1 < u.pageCount() {
				u.Page++
			}
		case 103, 105:
			if u.Page > 0 {
				u.Page--
			}
		}
	}
	return false, false
}
func (u *UI) render() Frame {
	var f Frame
	if u.View == 0 {
		f.newspaper(8, 3)
		f.text(36, 3, "每日简报", true)
		f.small(207, 5, "DAILY / 03", true)
		f.rect(8, 25, 280, 1, true)
		for i, s := range sources {
			y := 32 + i*25
			selected := i == u.Source
			f.outline(8, y, 280, 22)
			if selected {
				f.rect(8, y, 4, 22, true)
			}
			f.button(17, y+1, 22, string("QWE"[i]), selected)
			f.small(48, y+4, s.Name, true)
			f.small(211, y+4, fmt.Sprintf("%02d 条", len(u.Feeds[i].Articles)), true)
			if selected {
				f.chevron(273, y+8, 1)
			}
		}
		status := u.Status[u.Source]
		if status == "" {
			status = "离线可读"
		}
		if u.Busy {
			status = "正在更新 · 可继续阅读"
		}
		f.small(10, 109, fitSmall(status, 276), true)
		f.button(8, 131, 99, "确认  阅读", true)
		f.button(113, 131, 76, "R 更新", false)
		f.button(195, 131, 93, "Back 退出", false)
		return f
	}
	feed := u.Feeds[u.Source]
	f.small(8, 4, sources[u.Source].Name, true)
	if u.View == 1 {
		label := fmt.Sprintf("%02d / %02d", min(u.Selected+1, len(feed.Articles)), len(feed.Articles))
		f.small(288-measure(label), 4, label, true)
		f.rect(8, 24, 280, 1, true)
		if len(feed.Articles) == 0 {
			f.newspaper(138, 43)
			f.small(38, 73, "暂无缓存，按 R 联网更新", true)
		}
		start := u.Selected / 2 * 2
		for i := start; i < min(start+2, len(feed.Articles)); i++ {
			y := 30 + (i-start)*49
			sel := i == u.Selected
			if sel {
				f.outline(8, y, 280, 42)
				f.rect(8, y, 3, 42, true)
			}
			f.small(17, y+13, fmt.Sprintf("%02d", i+1), true)
			title := feed.Articles[i].Title
			if u.Source == 0 && len(title) > 3 && title[2] == ' ' {
				title = title[3:]
			}
			lines := wrapSmall(title, 231)
			if len(lines) == 1 {
				f.small(43, y+13, lines[0], true)
			} else {
				f.small(43, y+4, lines[0], true)
				line := lines[1]
				if len(lines) > 2 {
					line = fitSmall(line+"…", 231)
				}
				f.small(43, y+22, line, true)
			}
		}
		f.button(8, 131, 88, "↑↓ 选择", false)
		f.button(102, 131, 87, "确认 阅读", true)
		f.button(195, 131, 93, "Back 返回", false)
	} else {
		a := feed.Articles[u.Selected]
		meta := a.Date
		if u.Source == 0 {
			meta += " 短讯"
		} else if a.Summary {
			meta += " 摘要"
		}
		f.small(288-measure(meta), 4, meta, true)
		f.rect(8, 24, 280, 1, true)
		if u.Page >= 0 && u.Page < len(u.Pages) {
			for _, line := range u.Pages[u.Page] {
				f.reading(12, line.Y, line.Text)
			}
		}
		f.button(8, 131, 70, "← 上页", false)
		label := fmt.Sprintf("%d/%d", u.Page+1, u.pageCount())
		f.small(85+(36-measure(label))/2, 134, label, true)
		next := "下页 →"
		if u.Page+1 == u.pageCount() {
			next = "末页"
		}
		f.button(125, 131, 70, next, false)
		f.button(201, 131, 87, "Back 返回", true)
	}
	return f
}
func cacheStatus(f Feed) string {
	if len(f.Articles) == 0 {
		return "暂无缓存 R联网更新"
	}
	t, e := time.Parse(time.RFC3339, f.Fetched)
	if e != nil {
		return "已有缓存 R更新"
	}
	return "缓存 " + t.In(time.FixedZone("CST", 8*3600)).Format("01-02 15:04") + " R更新"
}
