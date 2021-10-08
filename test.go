package main

import (
	"fmt"
	"log"
	"syscall"
	"unsafe"
)

type KEY_EVENT_RECORD struct {
	KeyDown         int32
	RepeatCount     uint16
	VirtualKeyCode  uint16
	VirtualScanCode uint16
	UnicodeChar     uint16
	ControlKeyState uint32
}

type INPUT_RECORD struct {
	EventType uint16
	KeyEvent  KEY_EVENT_RECORD
}

func main() {
	k32, err := syscall.LoadDLL("Kernel32.dll")
	if err != nil {
		log.Fatal(err)
	}

	procReadConsoleInput, err := k32.FindProc("ReadConsoleInputW")
	if err != nil {
		log.Fatal(err)
	}
	var count uint32

	buffer := make([]INPUT_RECORD, 100)
	for true {
		r1, r2, err := procReadConsoleInput.Call(uintptr(syscall.Stdin), uintptr(unsafe.Pointer(&buffer[0])), uintptr(len(buffer)), uintptr(unsafe.Pointer(&count)))
		fmt.Println(r1, r2, err, count)
		for i := 0; i < int(count); i++ {
			fmt.Println(buffer[i])
		}
	}
}
