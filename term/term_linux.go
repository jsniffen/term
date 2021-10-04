package term

import (
	"sync"

	"golang.org/x/sys/unix"
)

type Terminal struct {
	termios *unix.Termios
	fd      int

	width  int
	height int

	backBuffer  []Cell
	frontBuffer []Cell
	buffer      []byte

	mutex  *sync.Mutex
	events []Event

	lastCell Cell
	lastX    int
	lastY    int
}

func CreateTerminal() (*Terminal, error) {
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

	t.mutex = &sync.Mutex{}
	t.events = make([]Event, 0)

	go t.readEvents()

	return t, nil
}

func (t *Terminal) Close() error {
	return unix.IoctlSetTermios(t.fd, unix.TCSETS, t.termios)
}

func (t *Terminal) write(b []byte) error {
	_, err := unix.Write(t.fd, b)
	return err
}

func (t *Terminal) readEvents() {
	b := make([]byte, 32)

	for true {
		n, err := unix.Read(t.fd, b)
		if err != nil || n == 0 {
			continue
		}

		if n == 1 {
			e := Event{b[0]}

			t.mutex.Lock()
			t.events = append(t.events, e)
			t.mutex.Unlock()
		}
	}
}
