//go:build linux

package terminalui

import (
	"bytes"
	"fmt"
	"os"
	"syscall"
)

const (
	epaperDevice  = "/dev/epaper_lcd"
	epaperRefresh = "/sys/devices/platform/e0266a128/epaper/refresh"
)

type Display struct {
	cached []byte
}

func (display *Display) Write(frame []byte, fullRefresh bool) error {
	if len(frame) != FrameBytes {
		return fmt.Errorf("invalid frame size: %d", len(frame))
	}
	if !fullRefresh && bytes.Equal(display.cached, frame) {
		return nil
	}
	file, err := os.OpenFile(epaperDevice, os.O_WRONLY|syscall.O_NONBLOCK, 0)
	if err != nil {
		return err
	}
	written, writeErr := file.Write(frame)
	closeErr := file.Close()
	if writeErr != nil {
		return writeErr
	}
	if closeErr != nil {
		return closeErr
	}
	if written != len(frame) {
		return fmt.Errorf("short frame write: %d/%d", written, len(frame))
	}
	if fullRefresh {
		if err := os.WriteFile(epaperRefresh, []byte("1"), 0); err != nil {
			return err
		}
	}
	display.cached = append(display.cached[:0], frame...)
	return nil
}
