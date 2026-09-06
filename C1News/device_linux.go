package main

import (
	"context"
	"encoding/binary"
	"fmt"
	"io"
	"os"
	"os/signal"
	"syscall"
	"time"
)

type inputEvent struct {
	Code  uint16
	Value int32
}
type update struct {
	Index int
	Feed  Feed
	Err   error
}

func runDevice(dir string) error {
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()
	events := make(chan inputEvent, 128)
	errors := make(chan error, 2)
	files := []*os.File{}
	defer func() {
		for _, f := range files {
			f.Close()
		}
	}()
	for _, path := range []string{"/dev/input/event0", "/dev/input/event1"} {
		f, e := os.Open(path)
		if e != nil {
			return e
		}
		files = append(files, f)
		if _, _, errno := syscall.Syscall(syscall.SYS_IOCTL, f.Fd(), 0x80044590, 1); errno != 0 {
			return fmt.Errorf("input grab: %w", errno)
		}
		go func() {
			buf := make([]byte, 16)
			for {
				if _, e := io.ReadFull(f, buf); e != nil {
					select {
					case errors <- e:
					case <-ctx.Done():
					}
					return
				}
				if binary.LittleEndian.Uint16(buf[8:]) != 1 {
					continue
				}
				ev := inputEvent{binary.LittleEndian.Uint16(buf[10:]), int32(binary.LittleEndian.Uint32(buf[12:]))}
				select {
				case events <- ev:
				case <-ctx.Done():
					return
				}
			}
		}()
	}
	u := UI{}
	for i, s := range sources {
		u.Feeds[i], _ = loadFeed(dir, s.ID)
		u.Status[i] = cacheStatus(u.Feeds[i])
	}
	results := make(chan update, 3)
	pending := 0
	lastFetch := time.Time{}
	refresh := func() {
		if u.Busy || time.Since(lastFetch) < 30*time.Second {
			return
		}
		lastFetch = time.Now()
		u.Busy = true
		pending = 3
		for i, s := range sources {
			go func(i int, s Source) {
				f, e := refreshFeed(ctx, s, dir)
				if e == nil {
					e = saveFeed(dir, f)
				}
				select {
				case results <- update{i, f, e}:
				case <-ctx.Done():
				}
			}(i, s)
		}
	}
	// Revalidate at most once per hour on entry; never refresh the page mid-read.
	stale := false
	for _, f := range u.Feeds {
		t, e := time.Parse(time.RFC3339, f.Fetched)
		if e != nil || time.Since(t) > time.Hour {
			stale = true
		}
	}
	if stale {
		refresh()
	}
	sigs := make(chan os.Signal, 1)
	signal.Notify(sigs, syscall.SIGTERM, syscall.SIGHUP, syscall.SIGINT)
	defer signal.Stop(sigs)
	tick := time.NewTicker(30 * time.Millisecond)
	defer tick.Stop()
	start := time.Now()
	lastDraw := time.Time{}
	lastRepeat := time.Time{}
	dirty := true
	valid := false
	var previous Frame
	held := map[uint16]bool{}
	exiting := false
	quiet := time.Time{}
	// Completed downloads are applied only on the source screen, preserving the
	// selected article and its pagination during a background update.
	staged := map[int]update{}
	for {
		select {
		case <-sigs:
			return nil
		case e := <-errors:
			return e
		case ev := <-events:
			if ev.Value == 0 {
				delete(held, ev.Code)
			} else {
				held[ev.Code] = true
			}
			if exiting {
				quiet = time.Time{}
				continue
			}
			if time.Since(start) < 180*time.Millisecond {
				continue
			}
			arrow := ev.Code == 103 || ev.Code == 108 || ev.Code == 105 || ev.Code == 106
			if ev.Value == 1 || ev.Value == 2 && arrow && time.Since(lastRepeat) >= 150*time.Millisecond {
				lastRepeat = time.Now()
				quit, refreshNow := u.key(ev.Code)
				dirty = true
				if refreshNow {
					refresh()
				}
				if quit {
					exiting = true
					// Keep input readers alive until the exit key is released.
					// Deferred cancellation runs only after the release drain.
				}
			}
		case r := <-results:
			pending--
			if r.Err != nil {
				fmt.Printf("%s update: %v\n", sources[r.Index].ID, r.Err)
				u.Status[r.Index] = "更新失败，旧缓存可读"
				if len(u.Feeds[r.Index].Articles) == 0 {
					u.Status[r.Index] = "更新失败，请检查网络"
				}
			} else {
				staged[r.Index] = r
				u.Status[r.Index] = "已更新 " + r.Feed.Articles[0].Date
			}
			if pending == 0 {
				u.Busy = false
			}
			if u.View == 0 {
				dirty = true
			}
		case <-tick.C:
			if exiting {
				if len(held) > 0 {
					quiet = time.Time{}
				} else if quiet.IsZero() {
					quiet = time.Now()
				} else if time.Since(quiet) >= 150*time.Millisecond {
					return nil
				}
				continue
			}
			if u.View == 0 && len(staged) > 0 {
				for i, r := range staged {
					u.Feeds[i] = r.Feed
				}
				clear(staged)
				dirty = true
			}
			if dirty && time.Since(lastDraw) >= 100*time.Millisecond {
				f := u.render()
				if !valid || f != previous {
					d, e := os.OpenFile("/dev/epaper_lcd", os.O_WRONLY, 0)
					if e != nil {
						return e
					}
					n, e := d.Write(f[:])
					d.Close()
					if e != nil {
						return e
					}
					if n != len(f) {
						return io.ErrShortWrite
					}
					previous = f
					// Optional diagnostic snapshot; no extra display refresh.
					if path := os.Getenv("C1NEWS_FRAME"); path != "" {
						_ = os.WriteFile(path, f[:], 0600)
					}
					valid = true
					lastDraw = time.Now()
				}
				dirty = false
			}
		}
	}
}
