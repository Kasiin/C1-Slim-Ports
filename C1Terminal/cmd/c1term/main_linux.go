//go:build linux

package main

import (
	"encoding/binary"
	"errors"
	"fmt"
	"io"
	"os"
	"os/exec"
	"os/signal"
	"syscall"
	"time"

	"c1terminal/internal/keyboard"
	"c1terminal/internal/terminalui"
	"github.com/creack/pty"
	"github.com/hinshun/vt10x"
)

const version = "0.2.1"

// _IOW('E', 0x90, int) on Linux/MIPS. MIPS uses 13 size bits and _IOC_WRITE=4,
// so this differs from the common asm-generic value. Grabbing both input
// devices prevents the suspended launcher from receiving queued keystrokes.
const eviocgrab = uintptr(0x80044590)

type inputEvent struct {
	typeCode uint16
	keyCode  uint16
	value    int32
}

func main() {
	if len(os.Args) == 2 && os.Args[1] == "--version" {
		fmt.Printf("C1Terminal %s standalone\n", version)
		return
	}
	if len(os.Args) != 1 {
		fmt.Fprintf(os.Stderr, "usage: %s [--version]\n", os.Args[0])
		os.Exit(2)
	}
	if err := run(); err != nil {
		fmt.Fprintf(os.Stderr, "c1term: %v\n", err)
		os.Exit(1)
	}
}

func run() error {
	inputs, events, inputErrors, err := openInputs()
	if err != nil {
		return err
	}
	defer func() {
		for _, input := range inputs {
			_ = input.Close()
		}
	}()

	command := exec.Command("/bin/bash", "--noprofile", "--norc", "-i")
	command.Dir = "/usr/data"
	command.Env = append(os.Environ(),
		"TERM=xterm-256color",
		"PATH=/usr/data/c1term/bin:/sbin:/usr/sbin:/bin:/usr/bin",
		"SHELL=/bin/bash",
		"HOME=/root",
		"USER=root",
		"LOGNAME=root",
		"C1TERM=1",
		"PS1=c1slim# ",
		"PROMPT_COMMAND=printf 'C1-Slim standalone terminal\\nBash 5.2; SHIFT=layer; HOME=exit\\n\\n'; unset PROMPT_COMMAND",
	)
	command.SysProcAttr = &syscall.SysProcAttr{Pdeathsig: syscall.SIGHUP}
	master, err := pty.StartWithSize(command, &pty.Winsize{
		Rows: terminalui.Rows,
		Cols: terminalui.Columns,
	})
	if err != nil {
		return fmt.Errorf("start bash PTY: %w", err)
	}

	childDone := false
	waitResult := make(chan error, 1)
	go func() { waitResult <- command.Wait() }()
	defer func() {
		if !childDone && command.Process != nil {
			_ = syscall.Kill(-command.Process.Pid, syscall.SIGHUP)
			select {
			case <-waitResult:
				childDone = true
			case <-time.After(400 * time.Millisecond):
				_ = syscall.Kill(-command.Process.Pid, syscall.SIGKILL)
				<-waitResult
				childDone = true
			}
		}
		_ = master.Close()
	}()

	virtualTerminal := vt10x.New(
		vt10x.WithSize(terminalui.Columns, terminalui.Rows),
		vt10x.WithWriter(master),
	)
	display := &terminalui.Display{}
	mapper := &keyboard.Mapper{}
	if err := display.Write(terminalui.Render(virtualTerminal, keyboard.LayerName(mapper.Layer), mapper.Control), true); err != nil {
		return fmt.Errorf("initial display: %w", err)
	}

	output := make(chan []byte, 16)
	outputErrors := make(chan error, 1)
	go readPTY(master, output, outputErrors)

	signals := make(chan os.Signal, 1)
	signal.Notify(signals, syscall.SIGINT, syscall.SIGTERM, syscall.SIGHUP)
	defer signal.Stop(signals)

	ticker := time.NewTicker(25 * time.Millisecond)
	defer ticker.Stop()
	dirty := false
	lastChange := time.Time{}
	renderCount := 0
	var repeatCode uint16
	repeatAt := time.Time{}
	pressed := make(map[uint16]bool)
	exitPending := false
	releaseSince := time.Time{}

	beginExit := func() {
		exitPending = true
		repeatCode = 0
		repeatAt = time.Time{}
		releaseSince = time.Time{}
		if len(pressed) == 0 {
			releaseSince = time.Now()
		}
	}

	writeKey := func(code uint16, value int32) (bool, error) {
		result := mapper.Handle(code, value)
		if result.Exit {
			return true, nil
		}
		if len(result.Bytes) > 0 {
			if _, err := master.Write(result.Bytes); err != nil {
				return false, fmt.Errorf("write terminal input: %w", err)
			}
		}
		if result.StateChange {
			dirty = true
			lastChange = time.Now()
		}
		return false, nil
	}

	for {
		select {
		case event := <-events:
			if event.typeCode != 1 {
				continue
			}
			if event.value == 0 {
				delete(pressed, event.keyCode)
			} else if event.value == 1 || event.value == 2 {
				pressed[event.keyCode] = true
			}
			if exitPending {
				if len(pressed) == 0 {
					if releaseSince.IsZero() {
						releaseSince = time.Now()
					}
				} else {
					releaseSince = time.Time{}
				}
				continue
			}
			if keyboard.Repeatable(event.keyCode) {
				if event.value == 1 {
					repeatCode = event.keyCode
					repeatAt = time.Now().Add(450 * time.Millisecond)
				} else if event.value == 0 && repeatCode == event.keyCode {
					repeatCode = 0
					repeatAt = time.Time{}
				}
			}
			exit, err := writeKey(event.keyCode, event.value)
			if err != nil {
				return err
			}
			if exit {
				beginExit()
			}
		case bytes := <-output:
			if len(bytes) > 0 {
				if _, err := virtualTerminal.Write(bytes); err != nil {
					return fmt.Errorf("parse terminal output: %w", err)
				}
				dirty = true
				lastChange = time.Now()
			}
		case err := <-outputErrors:
			if err != nil && !errors.Is(err, os.ErrClosed) {
				fmt.Fprintf(os.Stderr, "PTY read stopped: %v\n", err)
			}
			outputErrors = nil
		case err := <-inputErrors:
			if err != nil && !errors.Is(err, os.ErrClosed) {
				return err
			}
		case err := <-waitResult:
			childDone = true
			waitResult = nil
			if err != nil {
				fmt.Fprintf(os.Stderr, "bash exited: %v\n", err)
			}
			beginExit()
		case <-signals:
			return nil
		case now := <-ticker.C:
			if exitPending {
				if !releaseSince.IsZero() && now.Sub(releaseSince) >= 120*time.Millisecond {
					return nil
				}
				continue
			}
			if repeatCode != 0 && !repeatAt.IsZero() && !now.Before(repeatAt) {
				exit, err := writeKey(repeatCode, 2)
				if err != nil {
					return err
				}
				if exit {
					return nil
				}
				repeatAt = now.Add(90 * time.Millisecond)
			}
			if dirty && now.Sub(lastChange) >= 100*time.Millisecond {
				renderCount++
				fullRefresh := renderCount%30 == 0
				frame := terminalui.Render(virtualTerminal, keyboard.LayerName(mapper.Layer), mapper.Control)
				if err := display.Write(frame, fullRefresh); err != nil {
					return fmt.Errorf("display update: %w", err)
				}
				dirty = false
			}
		}
	}
}

func openInputs() ([]*os.File, <-chan inputEvent, <-chan error, error) {
	paths := []string{"/dev/input/event0", "/dev/input/event1"}
	files := make([]*os.File, 0, len(paths))
	events := make(chan inputEvent, 64)
	errors := make(chan error, len(paths))
	for _, path := range paths {
		file, err := os.Open(path)
		if err != nil {
			for _, opened := range files {
				_ = opened.Close()
			}
			return nil, nil, nil, fmt.Errorf("open %s: %w", path, err)
		}
		if _, _, errno := syscall.Syscall(syscall.SYS_IOCTL, file.Fd(), eviocgrab, 1); errno != 0 {
			_ = file.Close()
			for _, opened := range files {
				_ = opened.Close()
			}
			return nil, nil, nil, fmt.Errorf("grab %s: %w", path, errno)
		}
		files = append(files, file)
		go readInput(file, events, errors)
	}
	return files, events, errors, nil
}

func readInput(file *os.File, events chan<- inputEvent, errors chan<- error) {
	buffer := make([]byte, 16)
	for {
		if _, err := io.ReadFull(file, buffer); err != nil {
			errors <- fmt.Errorf("read %s: %w", file.Name(), err)
			return
		}
		events <- inputEvent{
			typeCode: binary.LittleEndian.Uint16(buffer[8:10]),
			keyCode:  binary.LittleEndian.Uint16(buffer[10:12]),
			value:    int32(binary.LittleEndian.Uint32(buffer[12:16])),
		}
	}
}

func readPTY(master *os.File, output chan<- []byte, errors chan<- error) {
	buffer := make([]byte, 4096)
	for {
		count, err := master.Read(buffer)
		if count > 0 {
			chunk := append([]byte(nil), buffer[:count]...)
			output <- chunk
		}
		if err != nil {
			errors <- err
			return
		}
	}
}
