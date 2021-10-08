package term

import (
	"strconv"
)

type Key int

const (
	CSI = "\x1b["
)

func (t *Terminal) setCell(x int, y int, c Cell) {
	if x < 0 || x >= t.width || y < 0 || y >= t.height {
		return
	}

	t.backBuffer[y*t.width+x] = c
}

func (t *Terminal) Clear(c Color) {
	for y := 0; y < t.height; y++ {
		for x := 0; x < t.width; x++ {
			c := Cell{
				fg: White,
				bg: c,
				r:  ' ',
			}
			t.setCell(x, y, c)
		}
	}
}

func (t *Terminal) setForegroundColor(c Color) {
	t.appendLiteral(CSI + "38;2;")
	t.appendNumber(int(c.r))
	t.appendLiteral(";")
	t.appendNumber(int(c.g))
	t.appendLiteral(";")
	t.appendNumber(int(c.b))
	t.appendLiteral("m")
}

func (t *Terminal) setBackgroundColor(c Color) {
	t.appendLiteral(CSI + "48;2;")
	t.appendNumber(int(c.r))
	t.appendLiteral(";")
	t.appendNumber(int(c.g))
	t.appendLiteral(";")
	t.appendNumber(int(c.b))
	t.appendLiteral("m")
}

func (t *Terminal) setCursor(x int, y int) {
	t.appendLiteral(CSI)
	t.appendNumber(y + 1)
	t.appendLiteral(";")
	t.appendNumber(x + 1)
	t.appendLiteral("H")
}

func (t *Terminal) appendLiteral(s string) {
	for _, c := range s {
		b := byte(c)
		t.buffer = append(t.buffer, b)
	}
}

func (t *Terminal) appendNumber(n int) {
	s := strconv.Itoa(n)
	t.appendLiteral(s)
}

func (t *Terminal) sendCode(x int, y int, c Cell) {
	if (t.lastX == 0 && t.lastY == 0) || x-1 != t.lastX || y != t.lastY {
		t.setCursor(x, y)
	}

	if t.lastCell.bg != c.bg {
		t.setBackgroundColor(c.bg)
	}

	if t.lastCell.fg != c.fg {
		t.setForegroundColor(c.fg)
	}

	t.buffer = append(t.buffer, c.r)

	t.lastX = x
	t.lastY = y
	t.lastCell = c
}

func (t *Terminal) Render() error {
	for y := 0; y < t.height; y++ {
		for x := 0; x < t.width; x++ {
			i := y*t.width + x
			if t.backBuffer[i] != t.frontBuffer[i] {
				t.frontBuffer[i] = t.backBuffer[i]

				t.sendCode(x, y, t.frontBuffer[i])
			}
		}
	}
	err := t.write(t.buffer)
	t.buffer = t.buffer[:0]
	return err
}

func (t *Terminal) PollEvents(e *Event) bool {
	if len(t.events) == 0 {
		return false
	}

	t.mutex.Lock()
	*e = t.events[0]
	t.events = t.events[1:]
	t.mutex.Unlock()

	return true
}

func (t *Terminal) Reset() error {
	return t.write([]byte(CSI + "0m"))
}

func (t *Terminal) parseInput(b []byte, n int) {
	var e Event

	if n <= 0 {
		return
	}

	if n == 1 {
		switch b[0] {
		case 0x7f:
			e.Key = KeyBackSpace
		default:
			e.Key = Key(b[0])
		}
	}

	t.mutex.Lock()
	t.events = append(t.events, e)
	t.mutex.Unlock()
}

func (t *Terminal) Modal(x_0, y_0, w, h int) {
	c := Cell{Blue, White, 'X'}
	for y := y_0; y < y_0+h; y++ {
		for x := x_0; x < x_0+w; x++ {
			t.setCell(x, y, c)
		}
	}
}
