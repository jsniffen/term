package gui

import (
	"finn/term"
)

type Gui struct {
	t *term.Terminal
}

func CreateGui(t *term.Terminal) (*Gui, error) {
	return &Gui{t}, nil
}

var Pressed = false

func (g *Gui) Button(e *term.Event) bool {
	x0, y0 := 0, 0
	x1, y1 := 10, 2

	if g.t.CursorX >= x0 && g.t.CursorX < x1 && g.t.CursorY >= y0 && g.t.CursorY < y1 {
		if e.Key == term.KeyEnter {
			Pressed = !Pressed
		}
	}

	c := term.Cell{term.Blue, term.White, ' '}

	if Pressed {
		c.BG = term.Black
	}

	for y := y0; y < y1; y++ {
		for x := x0; x < x1; x++ {
			g.t.SetCell(x, y, c)
		}
	}

	return true
}
