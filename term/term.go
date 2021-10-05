package term

import "fmt"

const (
	CSI = "\x1b["
)

var (
	Black = Color{0, 0, 0}
	White = Color{255, 255, 255}
	Red   = Color{255, 0, 0}
	Green = Color{0, 255, 0}
	Blue  = Color{0, 0, 255}

	Running = true
)

type Color struct {
	r uint8
	g uint8
	b uint8
}

type Cell struct {
	fg Color
	bg Color
	r  byte
}

type Event struct {
	Key byte
}

func (t *Terminal) setCell(x int, y int, c Cell) {
	if x < 0 || x >= t.width || y < 0 || y >= t.height {
		return
	}
	t.backBuffer[y*t.width+x] = c
}

func (t *Terminal) Clear() {
	for y := 0; y < t.height; y++ {
		for x := 0; x < t.width; x++ {
			c := Cell{
				fg: White,
				bg: Black,
				r:  ' ',
			}
			t.setCell(x, y, c)
		}
	}
}

func (t *Terminal) setCursor(x int, y int) error {
	buf := fmt.Sprintf("%s%d:%dH", CSI, y+1, x+1)
	return t.write([]byte(buf))
}

func (t *Terminal) sendCode(x int, y int, c Cell) {
	var buf []byte

	if (t.lastX == 0 && t.lastY == 0) || x-1 != t.lastX || y != t.lastY {
		buf = []byte(fmt.Sprintf("%s%d;%dH", CSI, y+1, x+1))
		t.buffer = append(t.buffer, buf...)
	}

	if t.lastCell.bg != c.bg {
		buf = []byte(fmt.Sprintf("%s48;2;%d;%d;%dm", CSI, c.bg.r, c.bg.g, c.bg.b))
		t.buffer = append(t.buffer, buf...)
	}

	if t.lastCell.fg != c.fg {
		buf = []byte(fmt.Sprintf("%s38;2;%d;%d;%dm", CSI, c.fg.r, c.fg.g, c.fg.b))
		t.buffer = append(t.buffer, buf...)
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
		e.Key = b[0]

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
