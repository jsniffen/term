package main

import (
	"fmt"
	"log"
	"time"

	"golang.org/x/sys/unix"
)

const (
	CSI = "\x1b["
)

var (
	Black = Color{0, 0, 0}
	White = Color{255, 255, 255}
	Red   = Color{255, 0, 0}
	Green = Color{0, 255, 0}
	Blue  = Color{0, 0, 255}
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

type Terminal struct {
	termios *unix.Termios
	fd      int

	width  int
	height int

	backBuffer  []Cell
	frontBuffer []Cell
	buffer      []byte

	lastCell Cell
	lastX    int
	lastY    int
}

func create_terminal() (*Terminal, error) {
	var err error
	t := &Terminal{}

	t.fd, err = unix.Open("/dev/tty", unix.O_RDWR, 0666)
	if err != nil {
		return t, err
	}

	ws, err := unix.IoctlGetWinsize(t.fd, unix.TIOCGWINSZ)
	if err != nil {
		return t, err
	}
	t.width = int(ws.Col)
	t.height = int(ws.Row)

	t.termios, err = unix.IoctlGetTermios(t.fd, unix.TCGETS)
	if err != nil {
		return t, err
	}

	termios := *t.termios
	termios.Iflag &= ^((uint32)(unix.IGNBRK) | (uint32)(unix.BRKINT) | (uint32)(unix.PARMRK) | (uint32)(unix.ISTRIP) | (uint32)(unix.INLCR) | (uint32)(unix.IGNCR) | (uint32)(unix.ICRNL) | (uint32)(unix.IXON))
	termios.Oflag &= ^(uint32)(unix.OPOST)
	termios.Lflag &= ^((uint32)(unix.ECHO) | (uint32)(unix.ECHONL) | (uint32)(unix.ICANON) | (uint32)(unix.ISIG) | (uint32)(unix.IEXTEN))
	termios.Cflag &= ^((uint32)(unix.CSIZE) | (uint32)(unix.PARENB))
	termios.Cflag |= (uint32)(unix.CS8)
	termios.Cc[unix.VMIN] = 1
	termios.Cc[unix.VTIME] = 0
	err = unix.IoctlSetTermios(t.fd, unix.TCSETS, &termios)
	if err != nil {
		return t, err
	}

	t.frontBuffer = make([]Cell, 4096)
	t.backBuffer = make([]Cell, 4096)
	t.buffer = make([]byte, 4096)

	return t, nil
}

func (t *Terminal) Close() error {
	return unix.IoctlSetTermios(t.fd, unix.TCSETS, t.termios)
}

func (t *Terminal) Clear() {
	for y := 0; y < t.height; y++ {
		for x := 0; x < t.width; x++ {
			c := Cell{
				fg: White,
				bg: Red,
				r:  ' ',
			}
			t.setCell(x, y, c)
		}
	}
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
	_, err := unix.Write(t.fd, t.buffer)
	t.buffer = t.buffer[:0]
	return err
}

func (t *Terminal) sendCode(x int, y int, c Cell) {
	var buf []byte

	if (t.lastX == 0 && t.lastY == 0) || x-1 != t.lastX || y != t.lastY {
		buf = []byte(fmt.Sprintf("%s%d:%dH", CSI, y+1, x+1))
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

func (t *Terminal) setCell(x int, y int, c Cell) {
	if x < 0 || x >= t.width || y < 0 || y >= t.height {
		return
	}
	t.backBuffer[y*t.width+x] = c
}

func (t *Terminal) setCursor(x int, y int) error {
	buf := fmt.Sprintf("%s%d:%dH", CSI, y+1, x+1)
	_, err := unix.Write(t.fd, []byte(buf))
	return err
}

func (t *Terminal) modal(x_0, y_0, w, h int) {
	c := Cell{Blue, White, 'X'}
	for y := y_0; y < y_0+h; y++ {
		for x := x_0; x < x_0+w; x++ {
			t.setCell(x, y, c)
		}
	}
}

func main() {
	t, err := create_terminal()
	if err != nil {
		log.Fatal(err)
	}
	defer t.Close()

	x, y := 0, 0
	for true {
		t.Clear()
		t.modal(x, y, 10, 10)
		t.Render()

		time.Sleep(1 * time.Second)
		x++
		y++
	}
}
