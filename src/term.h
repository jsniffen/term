//TODO(Julian): 
// (1) Synchronize event threading
// (2) Handle UTF-8
// (3) Window Resize Events
// (4) Detect terminal type
//     - color support
//     - ANSI code support
// (5) Memory allocation
// (6) Logging

/*
 High Level Terminal Interface:

bool create_terminal(struct terminal *t);
bool set_cell(struct terminal *t, int x, int y, struct cell *c)

bool poll_event_terminal(struct terminal *t, union event *e)
bool clear_terminal(struct terminal *t)
bool render_terminal(struct terminal *t)
*/

#include <stdbool.h>
#include <stdint.h>

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#elif _WIN32


#include <stdio.h>

#include <windows.h>

#endif

#include "slice.h"

#define CSI "\x1b["

int parse_number(int num, char *buf)
{
	char *c = buf;
	int l = 0;

	for (int n = num; n > 0; n /= 10) {
		*c++ = '0' + n % 10;
		++l;
	}

	for (int i = 0; i < l/2; ++i) {
		char t = buf[i];
		buf[i] = buf[l-i-1];
		buf[l-i-1] = t;
	}

	return l;
}


enum Key {
	KeyBackSpace = 8, KeyTab,
	KeyEnter = 13,

	KeySpace = 32, KeyExclamation, KeyQuote, KeyNumber, KeyDollar, KeyPercent, KeyAmpersand, KeyApostrophe,
	KeyLParen, KeyRParen, KeyAsterisk, KeyPlus, KeyComma, KeyMinus, KeyPeriod, KeyForwardslash,

	Key0, Key1, Key2, Key3, Key4, Key5, Key6, Key7, Key8, Key9,

	KeyColon, KeySemiColon, KeyLess, KeyEquals, KeyGreater, KeyQuestion, KeyAt,

	KeyA, KeyB, KeyC, KeyD, KeyE, KeyF, KeyG, KeyH, KeyI, KeyJ, KeyK, KeyL, KeyM, KeyN, KeyO, KeyP, KeyQ, KeyR, KeyS, KeyT, KeyU, KeyV, KeyW, KeyX, KeyY, KeyZ,
	KeyLBracket, KeyBackslash, KeyRBracket, KeyCaret, KeyUnderscore, KeyBacktick,
	Keya, Keyb, Keyc, Keyd, Keye, Keyf, Keyg, Keyh, Keyi, Keyj, Keyk, Keyl, Keym, Keyn, Keyo, Keyp, Keyq, Keyr, Keys, Keyt, Keyu, Keyv, Keyw, Keyx, Keyy, Keyz,
	KeyLCurlyBracket, KeyBar, KeyRCurlyBracket, KeyTilde, KeyDelete,
};

enum EventType {
	KeyboardEvent,
	WindowEvent,
};

struct keyboard_event {
	uint32_t type;
	enum Key key;
	bool alt;
};

struct window_event {
	uint32_t type;
	uint32_t width;
	uint32_t height;
};

union event {
	uint32_t type;

	struct keyboard_event keyboard;
	struct window_event window;
};

struct color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct cell {
	struct color fg;
	struct color bg;
	uint32_t c;
};

static int event_count;
static union event event_queue[256];

struct terminal {
#ifdef __linux__
	struct termios original_termios;
	uint32_t fd;
#elif _WIN32
	HANDLE console_stdin;
	HANDLE console_stdout;
#endif

	uint32_t width;
	uint32_t height;

	uint8_t *code_buffer;
	uint64_t code_buffer_index;

	struct cell *front_buffer;
	struct cell *back_buffer;

	struct slice *buffer;

	int cursor_x;
	int cursor_y;
};

bool create_terminal(struct terminal *t);
bool close_terminal(struct terminal *t);
bool write_terminal(struct terminal *t, uint8_t *buffer, int len);
void parse_terminal_input(struct terminal *t, char *buffer, int len);

#ifdef __linux__
#include "term_linux.h"
#endif

#ifdef _WIN32
#include "term_windows.h"
#endif

static int last_x, last_y;
static struct cell last_cell;
static bool initialized = false;

#define append_literal(terminal, string) append_bytes(terminal, string, sizeof(string)-1)
#define append_number(terminal, number, buffer) append_bytes(terminal, buffer, parse_number(number, buffer))
void append_bytes(struct terminal *t, uint8_t *b, int l)
{
	slice_append(t->buffer, b, l);
}

void append_utf8(struct terminal *t, uint32_t rune)
{
	uint8_t buf[4];
	buf[0] = (rune & 0xFF000000) >> 24;
	buf[1] = (rune & 0x00FF0000) >> 16;
	buf[2] = (rune & 0x0000FF00) >> 8;
	buf[3] = (rune & 0x000000FF) >> 0;
	append_bytes(t, buf, 4);
}

void append_code(struct terminal *t, char *code)
{
	int len = strlen(code);
	if (len == 0) return;

	//TODO(Julian): Check memcpy out of bounds.
	uint8_t *code_buffer = &t->code_buffer[t->code_buffer_index];
	memcpy(code_buffer, code, len);

	t->code_buffer_index += len;
}

void send_code(struct terminal *t, int x, int y, struct cell *c)
{
	char b[32];

	if ((last_x == 0 && last_y == 0) || x - 1 != last_x || y != last_y) {
		int l = sprintf(b, CSI "%d;%dH", y+1, x+1);
		append_bytes(t, b, l);
	}

	if (memcmp(&last_cell.bg, &c->bg, sizeof(struct color)) != 0 || !initialized) {
		int l = sprintf(b, CSI "48;2;%d;%d;%dm", c->bg.r, c->bg.g, c->bg.b); 
		append_bytes(t, b, l);
	}

	if (memcmp(&last_cell.fg, &c->fg, sizeof(struct color)) != 0 || !initialized) {
		int l = sprintf(b, CSI "38;2;%d;%d;%dm", c->fg.r, c->fg.g, c->fg.b);
		append_bytes(t, b, l);
	}

	append_utf8(t, c->c);

	initialized = true;
	last_x = x;
	last_y = y;
	memcpy(&last_cell, c, sizeof(struct cell));
}

bool set_cell(struct terminal *t, int x, int y, struct cell *c)
{
	if (x < 0 || x >= t->width || y < 0 || y >= t->height) return false;

	memcpy(&t->back_buffer[t->width*y + x], c, sizeof(struct cell));
	return true;
}

bool clear_terminal(struct terminal *t)
{
	for (int y = 0; y < t->height; ++y) {
		for (int x = 0; x < t->width; ++x) {
			struct cell c = {
				.bg = {0, 0, 0},
				.fg = {255, 255, 255},
				.c = ' ',
			};
			set_cell(t, x, y, &c);
		}
	}
	return true;
}

void set_cursor_terminal(struct terminal *t, int x, int y)
{
	char buf[32];
	append_literal(t, CSI);
	append_number(t, ++y, buf);
	append_literal(t, ";");
	append_number(t, ++x, buf);
	append_literal(t, "H");
}

void hide_cursor_terminal(struct terminal *t)
{
	char buf[32];
	append_literal(t, CSI "?");
	append_number(t, 25, buf);
	append_literal(t, "l");
}

void show_cursor_terminal(struct terminal *t)
{
	char buf[32];
	append_literal(t, CSI "?");
	append_number(t, 25, buf);
	append_literal(t, "h");
}

bool render_terminal(struct terminal *t)
{
	hide_cursor_terminal(t);
	struct cell *back = t->back_buffer;
	struct cell *front = t->front_buffer;

	for (int y = 0; y < t->height; ++y) {
		for (int x = 0; x < t->width; ++x) {
			if (memcmp(back, front, sizeof(struct cell)) != 0) {
				memcpy(front, back, sizeof(struct cell));

				send_code(t, x, y, front);
			}

			++back;
			++front;
		}
	}

	set_cursor_terminal(t, t->cursor_x, t->cursor_y);
	show_cursor_terminal(t);
	write_terminal(t, t->buffer->buf, t->buffer->len);
	t->buffer->len = 0;
	return true;
}

bool reset_terminal(struct terminal *t)
{
	return write_terminal(t, CSI "0m", 4);
}


//TODO(Julian): Thread synchronization...
bool poll_event_terminal(struct terminal *t, union event *e)
{
	if (event_count <= 0) return false;

	*e = event_queue[--event_count];
	return true;
}

// If the buffer contains valid events, parse them and 
// add them into the event queue.
void parse_terminal_input(struct terminal *t, char *buffer, int len)
{
	// printf("total: %d\t1: %x\t2: %c\n", len, buffer[0], buffer[1]);
	// printf("%d bytes: %x\n", len, buffer);
	if (len <= 0) return;

	if (len == 1) {
		switch (buffer[0]) {
			case 0x7f:
				printf("backspace\n");
				return;
			case 0x1a:
				printf("pause\n");
				return;
			case 0x1b:
				printf("escape\n");
				return;
		}

		union event e;
		e.type = KeyboardEvent;
		e.keyboard.key = (enum Key)buffer[0];
		e.keyboard.alt = false;

		// InterlockedIncrement(LONGevent_queue
		// int count = (int)InterlockedIncrement((LONG *)&event_count) - 1;
		// event_queue[count] = e;

		// WaitForSingleObject(mutex, INFINITE);
		event_queue[event_count++] = e;
		// ReleaseMutex(mutex);
		return;
	}

	if (len == 2 && buffer[0] == 0x1b) {
		union event e;
		e.type = KeyboardEvent;
		e.keyboard.key = (enum Key)buffer[1];
		e.keyboard.alt = true;

		// InterlockedIncrement(LONGevent_queue
		// int count = (int)InterlockedIncrement((LONG *)&event_count) - 1;
		// event_queue[count] = e;

		// WaitForSingleObject(mutex, INFINITE);
		event_queue[event_count++] = e;
		// ReleaseMutex(mutex);
		return;
	}

	if (buffer[0] == 0x1b && buffer[1] == 'O') {
		switch (buffer[2]) {
			case 'P':
				printf("F1\n");
				return;
			case 'Q':
				printf("F2\n");
				return;
			case 'R':
				printf("F3\n");
				return;
			case 'S':
				printf("F4\n");
				return;
		}
	}

	if (buffer[0] == 0x1b && buffer[1] == '[') {
		if (len == 6) {
			if (buffer[2] == '1' && buffer[3] == ';' && buffer[4] == '5') {
				switch (buffer[5]) {
					case 'A':
						printf("Ctrl + Up\n");
						return;
					case 'B':
						printf("Ctrl + Down\n");
						return;
					case 'C':
						printf("Ctrl + Right\n");
						return;
					case 'D':
						printf("Ctrl + Left\n");
						return;
				}
			}
		}
		
		if (len == 5) {
			if (buffer[4] == '~') {
				if (buffer[2] == '1') {
					switch (buffer[3]) {
						case '5':
							printf("F5\n");
							return;
						case '7':
							printf("F6\n");
							return;
						case '8':
							printf("F7\n");
							return;
						case '9':
							printf("F8\n");
							return;
					}
				}
				if (buffer[2] == '2') {
					switch (buffer[3]) {
						case '0':
							printf("F9\n");
							return;
						case '1':
							printf("F10\n");
							return;
						case '3':
							printf("F11\n");
							return;
						case '4':
							printf("F12\n");
							return;
					}
				}
			}
		}

		if (len == 4) {
			if (buffer[3] == '~') {
				switch (buffer[2]) {
					case '2':
						printf("Insert\n");
						return;
					case '3':
						printf("Delete\n");
						return;
					case '5':
						printf("Page Up\n");
						return;
					case '6':
						printf("Page Down\n");
						return;
				}
			}
		}

		if (len == 3) {
			switch (buffer[2]) {
				case 'A':
					printf("Up\n");
					return;
				case 'B':
					printf("Down\n");
					return;
				case 'C':
					printf("Right\n");
					return;
				case 'D':
					printf("Left\n");
					return;
				case 'H':
					printf("Home\n");
					return;
				case 'F':
					printf("End\n");
					return;
			}
		}
	}
}
