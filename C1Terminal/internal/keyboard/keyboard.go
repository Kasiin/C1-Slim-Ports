// Package keyboard translates the MP-D261 Linux input keycodes into terminal bytes.
package keyboard

import "strings"

const (
	KeyQ          = 16
	KeyW          = 17
	KeyE          = 18
	KeyR          = 19
	KeyT          = 20
	KeyY          = 21
	KeyU          = 22
	KeyI          = 23
	KeyO          = 24
	KeyP          = 25
	KeyEnter      = 28
	KeyA          = 30
	KeyS          = 31
	KeyD          = 32
	KeyF          = 33
	KeyG          = 34
	KeyH          = 35
	KeyJ          = 36
	KeyK          = 37
	KeyL          = 38
	KeyLeftShift  = 42
	KeyZ          = 44
	KeyX          = 45
	KeyC          = 46
	KeyV          = 47
	KeyB          = 48
	KeyN          = 49
	KeyM          = 50
	KeySpace      = 57
	KeyHome       = 102
	KeyUp         = 103
	KeyLeft       = 105
	KeyRight      = 106
	KeyDown       = 108
	KeyDelete     = 111
	KeyVolumeDown = 114
	KeyVolumeUp   = 115
	KeyWakeup     = 143
	KeyBack       = 158
	KeyOK         = 352
)

const (
	LayerLower = iota
	LayerUpper
	LayerSymbols
)

type Result struct {
	Bytes       []byte
	Exit        bool
	StateChange bool
}

type Mapper struct {
	Layer        int
	ShiftPressed bool
	ShiftUsed    bool
	Control      bool
}

var keyLetters = map[uint16]byte{
	KeyQ: 'q', KeyW: 'w', KeyE: 'e', KeyR: 'r', KeyT: 't', KeyY: 'y',
	KeyU: 'u', KeyI: 'i', KeyO: 'o', KeyP: 'p', KeyA: 'a', KeyS: 's',
	KeyD: 'd', KeyF: 'f', KeyG: 'g', KeyH: 'h', KeyJ: 'j', KeyK: 'k',
	KeyL: 'l', KeyZ: 'z', KeyX: 'x', KeyC: 'c', KeyV: 'v', KeyB: 'b',
	KeyN: 'n', KeyM: 'm',
}

func (m *Mapper) Handle(code uint16, value int32) Result {
	if code == KeyLeftShift {
		if value == 1 {
			m.ShiftPressed = true
			m.ShiftUsed = false
			return Result{}
		}
		if value == 0 {
			stateChange := m.ShiftPressed && !m.ShiftUsed
			m.ShiftPressed = false
			m.ShiftUsed = false
			if stateChange {
				m.Layer = (m.Layer + 1) % 3
			}
			return Result{StateChange: stateChange}
		}
		return Result{}
	}
	if code == KeyOK {
		m.Control = value != 0
		return Result{StateChange: true}
	}
	if value != 1 && value != 2 {
		return Result{}
	}
	if m.ShiftPressed {
		m.ShiftUsed = true
	}
	if code == KeyHome && !m.ShiftPressed {
		return Result{Exit: true}
	}
	if letter, ok := keyLetters[code]; ok {
		if m.Control {
			return Result{Bytes: []byte{letter - 'a' + 1}}
		}
		return Result{Bytes: []byte{physicalCharacter(m.Layer, letter, m.ShiftPressed)}}
	}
	switch code {
	case KeySpace:
		return Result{Bytes: []byte{' '}}
	case KeyEnter:
		return Result{Bytes: []byte{'\r'}}
	case KeyDelete:
		if m.ShiftPressed {
			return Result{Bytes: []byte("\x1b[3~")}
		}
		return Result{Bytes: []byte{0x7f}}
	case KeyUp:
		return Result{Bytes: []byte("\x1b[A")}
	case KeyDown:
		return Result{Bytes: []byte("\x1b[B")}
	case KeyRight:
		return Result{Bytes: []byte("\x1b[C")}
	case KeyLeft:
		return Result{Bytes: []byte("\x1b[D")}
	case KeyHome:
		return Result{Bytes: []byte("\x1b[H")}
	case KeyVolumeDown:
		return Result{Bytes: []byte("\x1b[5~")}
	case KeyVolumeUp:
		return Result{Bytes: []byte("\x1b[6~")}
	case KeyWakeup, KeyBack:
		return Result{Bytes: []byte{0x1b}}
	default:
		return Result{}
	}
}

func Repeatable(code uint16) bool {
	switch code {
	case KeyUp, KeyDown, KeyLeft, KeyRight, KeyDelete, KeyVolumeDown, KeyVolumeUp:
		return true
	default:
		return false
	}
}

func LayerName(layer int) string {
	switch layer {
	case LayerUpper:
		return "ABC"
	case LayerSymbols:
		return "SYM"
	default:
		return "abc"
	}
}

func physicalCharacter(layer int, letter byte, shiftChord bool) byte {
	const letters = "qwertyuiopasdfghjklzxcvbnm"
	const symbols = "1234567890'@#$%&*().-/?:;,"
	position := strings.IndexByte(letters, letter)
	if position < 0 {
		return 0
	}
	if shiftChord || layer == LayerSymbols {
		return symbols[position]
	}
	if layer == LayerUpper {
		return letter - 'a' + 'A'
	}
	return letter
}
