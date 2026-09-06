package main

import (
	_ "embed"
	"encoding/binary"
	"strings"
	"unicode"
)

//go:embed font14.bin
var font14 []byte

//go:embed width14.bin
var width14 []byte

//go:embed font15.bin
var font15 []byte

//go:embed width15.bin
var width15 []byte

func readerWidth(r rune) int {
	if r < 32 || r > 65535 {
		r = 0xfffd
	}
	return max(1, int(width15[r]))
}
func (f *Frame) reading(x, y int, s string) {
	for _, r := range s {
		w := readerWidth(r)
		if x+w > 296 {
			return
		}
		if r < 32 || r > 65535 {
			r = 0xfffd
		}
		for row := 0; row < 16; row++ {
			bits := binary.BigEndian.Uint16(font15[int(r)*32+row*2:])
			for col := 0; col < 16; col++ {
				if bits&(0x8000>>uint(col)) != 0 {
					f.pixel(x+col, y+row, true)
				}
			}
		}
		x += w
	}
}

func smallWidth(r rune) int {
	if r < 32 || r > 65535 {
		r = 0xfffd
	}
	return max(1, int(width14[r]))
}
func measure(s string) int {
	n := 0
	for _, r := range s {
		n += smallWidth(r)
	}
	return n
}
func (f *Frame) small(x, y int, s string, black bool) {
	for _, r := range s {
		w := smallWidth(r)
		if x+w > 296 {
			return
		}
		if r < 32 || r > 65535 {
			r = 0xfffd
		}
		for row := 0; row < 16; row++ {
			bits := binary.BigEndian.Uint16(font14[int(r)*32+row*2:])
			for col := 0; col < 16; col++ {
				if bits&(0x8000>>uint(col)) != 0 {
					f.pixel(x+col, y+row, black)
				}
			}
		}
		x += w
	}
}
func fitSmall(s string, width int) string {
	if measure(s) <= width {
		return s
	}
	r := []rune(s)
	for len(r) > 0 && measure(string(r))+measure("…") > width {
		r = r[:len(r)-1]
	}
	return string(r) + "…"
}

// Wrap at word boundaries where possible. Do not lose spaces or punctuation.
func wrapSmall(s string, width int) []string {
	return wrapWidth(s, width, smallWidth)
}
func wrapWidth(s string, width int, glyphWidth func(rune) int) []string {
	out := []string{}
	for _, p := range strings.Split(strings.ReplaceAll(s, "\r", ""), "\n") {
		runes := []rune(p)
		if len(runes) == 0 {
			out = append(out, "")
			continue
		}
		for len(runes) > 0 {
			n, w := 0, 0
			for n < len(runes) && w+glyphWidth(runes[n]) <= width {
				w += glyphWidth(runes[n])
				n++
			}
			if n == 0 {
				n = 1
			}
			if n < len(runes) {
				// Avoid splitting short English words, and avoid leading CJK punctuation.
				if runes[n] < 128 && unicode.IsLetter(runes[n]) {
					k := n
					for k > 0 && runes[k-1] < 128 && unicode.IsLetter(runes[k-1]) {
						k--
					}
					if k > 0 {
						n = k
					}
				}
				if strings.ContainsRune("，。！？；：、）》】」』", runes[n]) && n > 1 {
					n--
				}
			}
			out = append(out, string(runes[:n]))
			runes = runes[n:]
		}
	}
	return out
}

type ReadLine struct {
	Text string
	Y    int
}

func paginate(s string) [][]ReadLine {
	pages := [][]ReadLine{}
	page := []ReadLine{}
	y := 32
	for _, paragraph := range strings.Split(s, "\n") {
		if paragraph == "" {
			continue
		}
		for _, line := range wrapWidth(paragraph, 272, readerWidth) {
			if y+16 > 123 {
				pages = append(pages, page)
				page = nil
				y = 32
			}
			page = append(page, ReadLine{line, y})
			y += 19
		}
		y += 6
	}
	if len(page) > 0 {
		pages = append(pages, page)
	}
	if len(pages) == 0 {
		pages = append(pages, []ReadLine{{"暂无正文", 32}})
	}
	return pages
}
func (f *Frame) outline(x, y, w, h int) {
	f.rect(x, y, w, 1, true)
	f.rect(x, y+h-1, w, 1, true)
	f.rect(x, y, 1, h, true)
	f.rect(x+w-1, y, 1, h, true)
}
func (f *Frame) button(x, y, w int, label string, filled bool) {
	if filled {
		f.rect(x, y, w, 20, true)
	} else {
		f.outline(x, y, w, 20)
	}
	f.small(x+(w-measure(label))/2, y+2, label, !filled)
}
func (f *Frame) newspaper(x, y int) {
	f.outline(x, y, 20, 17)
	f.rect(x+3, y+3, 5, 5, true)
	for j := 0; j < 3; j++ {
		f.rect(x+11, y+3+j*3, 6, 1, true)
	}
	f.rect(x+3, y+11, 14, 1, true)
	f.rect(x+3, y+14, 14, 1, true)
}
func (f *Frame) chevron(x, y, dir int) {
	for i := 0; i < 4; i++ {
		f.pixel(x+dir*i, y+i, true)
		f.pixel(x+dir*i, y+6-i, true)
	}
}
