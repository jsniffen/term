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
			if e.Key == 'q' {
				Running = false
			}
		}
		t.Clear()
		t.Modal(x, y, 10, 10)
		t.Render()

		time.Sleep(33 * time.Millisecond)
		x++
		y++
	}
}
