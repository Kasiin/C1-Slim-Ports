package keyboard

import "testing"

func TestHomeAlwaysExitsWithoutShift(t *testing.T) {
	var mapper Mapper
	if !mapper.Handle(KeyHome, 1).Exit {
		t.Fatal("HOME did not exit")
	}
}

func TestShiftHomeIsSentToShell(t *testing.T) {
	var mapper Mapper
	mapper.Handle(KeyLeftShift, 1)
	result := mapper.Handle(KeyHome, 1)
	if result.Exit || string(result.Bytes) != "\x1b[H" {
		t.Fatalf("unexpected result: %#v", result)
	}
}

func TestShiftTapCyclesLayer(t *testing.T) {
	var mapper Mapper
	mapper.Handle(KeyLeftShift, 1)
	result := mapper.Handle(KeyLeftShift, 0)
	if !result.StateChange || mapper.Layer != LayerUpper {
		t.Fatalf("layer=%d result=%#v", mapper.Layer, result)
	}
}

func TestControlLetter(t *testing.T) {
	var mapper Mapper
	mapper.Handle(KeyOK, 1)
	result := mapper.Handle(KeyC, 1)
	if len(result.Bytes) != 1 || result.Bytes[0] != 3 {
		t.Fatalf("unexpected control byte: %v", result.Bytes)
	}
}
