package term

import (
	"sync"

	"golang.org/x/sys/windows"
)

type Terminal struct {
	stdin  windows.Handle
	stdout windows.Handle

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

	t.stdout, err = windows.GetStdHandle(uint32(windows.STD_OUTPUT_HANDLE))
	if err != nil {
		return t, err
	}

	t.stdin, err = windows.GetStdHandle(uint32(windows.STD_INPUT_HANDLE))
	if err != nil {
		return t, err
	}

	var mode uint32
	err = windows.GetConsoleMode(t.stdout, &mode)
	if err != nil {
		return t, err
	}
	mode |= windows.ENABLE_VIRTUAL_TERMINAL_PROCESSING
	err = windows.SetConsoleMode(t.stdout, mode)
	if err != nil {
		return t, err
	}

	err = windows.SetConsoleMode(t.stdin, windows.ENABLE_VIRTUAL_TERMINAL_INPUT)
	if err != nil {
		return t, err
	}

	if err != nil {
		return t, err
	}

	var info windows.ConsoleScreenBufferInfo
	err = windows.GetConsoleScreenBufferInfo(t.stdout, &info)
	if err != nil {
		return t, err
	}
	t.width = int(info.Window.Right - info.Window.Left + 1)
	t.height = int(info.Window.Bottom - info.Window.Top + 1)

	t.frontBuffer = make([]Cell, 10000)
	t.backBuffer = make([]Cell, 10000)
	t.buffer = make([]byte, 10000)

	t.mutex = &sync.Mutex{}
	t.events = make([]Event, 0)

	go t.readEvents()

	return t, nil
}

func (t *Terminal) Close() error {
	return t.Reset()
}

func (t *Terminal) write(b []byte) error {
	_, err := windows.Write(t.stdout, b)
	return err
}

func (t *Terminal) readEvents() {
	b := make([]byte, 32)
	var read uint32
	var err error

	for true {
		err = windows.ReadFile(t.stdin, b, &read, nil)
		if err != nil {
			continue
		}

		if read == 1 {
			e := Event{b[0]}

			t.mutex.Lock()
			t.events = append(t.events, e)
			t.mutex.Unlock()
		}
	}
}
