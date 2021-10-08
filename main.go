package main

import (
	"log"

	"finn/gui"
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

	g, err := gui.CreateGui(t)
	if err != nil {
		log.Fatal(err)
	}

	t.Clear(term.Red)
	t.Render()

	var e term.Event
	for Running {
		for t.PollEvents(&e) {
			switch e.Key {
			case term.Keyq:
				Running = false
			case term.Keyh:
				t.CursorX--
			case term.Keyj:
				t.CursorY++
			case term.Keyk:
				t.CursorY--
			case term.Keyl:
				t.CursorX++
			}

			t.Clear(term.Red)

			g.Button(&e)

			t.Render()
		}
	}
}
