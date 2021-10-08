package term

import (
	"fmt"

	"golang.org/x/sys/windows"
)

type Platform struct {
	stdin  windows.Handle
	stdout windows.Handle
}

func (t *Terminal) platformSetup() error {
	var err error

	t.platform.stdout, err = windows.GetStdHandle(uint32(windows.STD_OUTPUT_HANDLE))
	if err != nil {
		return fmt.Errorf("Error getting stdout handle: %v\n", err)
	}

	t.platform.stdin, err = windows.GetStdHandle(uint32(windows.STD_INPUT_HANDLE))
	if err != nil {
		return fmt.Errorf("Error getting stdin handle: %v\n", err)
	}

	var mode uint32
	err = windows.GetConsoleMode(t.platform.stdout, &mode)
	if err != nil {
		return fmt.Errorf("Error getting stdout console mode: %v\n", err)
	}
	mode |= windows.ENABLE_VIRTUAL_TERMINAL_PROCESSING
	err = windows.SetConsoleMode(t.platform.stdout, mode)
	if err != nil {
		return fmt.Errorf("Error setting stdout console mode: %v\n", err)
	}

	err = windows.SetConsoleMode(t.platform.stdin, windows.ENABLE_VIRTUAL_TERMINAL_INPUT)
	if err != nil {
		return fmt.Errorf("Error setting stdin console mode: %v\n", err)
	}

	var info windows.ConsoleScreenBufferInfo
	err = windows.GetConsoleScreenBufferInfo(t.platform.stdout, &info)
	if err != nil {
		return fmt.Errorf("Error getting console screen buffer info: %v\n", err)
	}
	t.width = int(info.Window.Right - info.Window.Left + 1)
	t.height = int(info.Window.Bottom - info.Window.Top + 1)

	return nil
}

func (t *Terminal) Close() error {
	return t.Reset()
}

func (t *Terminal) write(b []byte) error {
	_, err := windows.Write(t.platform.stdout, b)
	return err
}

func (t *Terminal) readEvents() {
	b := make([]byte, 32)

	for true {
		n, err := windows.Read(t.platform.stdin, b)
		if err != nil {
			continue
		}
		t.parseInput(b, n)
	}
}
