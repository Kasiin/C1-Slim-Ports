package terminalui

import "testing"

func TestPixelLayout(t *testing.T) {
	frame := make([]byte, FrameBytes)
	if !setPixel(frame, 7, 9, true) {
		t.Fatal("setPixel rejected valid coordinate")
	}
	offset := Width + 7
	if frame[offset] != 0x40 {
		t.Fatalf("frame[%d]=0x%02x, want 0x40", offset, frame[offset])
	}
	if setPixel(frame, Width, 0, true) {
		t.Fatal("setPixel accepted out-of-range coordinate")
	}
}

func TestASCIIAndBoxGlyphs(t *testing.T) {
	if glyph('A') == ([5]byte{}) {
		t.Fatal("ASCII glyph missing")
	}
	if glyph('─') != glyph('-') || glyph('│') != glyph('|') || glyph('┼') != glyph('+') {
		t.Fatal("box-drawing fallback mismatch")
	}
}
