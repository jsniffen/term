package main

import (
	"log"
	"time"

	"finn/term"
)

var (
	Running = true
)

func main() {
	t, err := term.CreateTerminal()
	if err != nil {
		log.Fatal(err)
	}
	defer t.Close()

	x, y := 1, 1
	var e term.Event
	for Running {
		for t.PollEvents(&e) {
			switch e.Key {
			case term.Keyq:
				Running = false
			case term.Keyh:
				x--
			case term.Keyj:
				y++
			case term.Keyk:
				y--
			case term.Keyl:
				x++
			}

		}
		t.Clear(term.Red)
		t.Modal(x, y, 10, 10)
		t.Render()

		time.Sleep(33 * time.Millisecond)
	}
}
