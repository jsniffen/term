//TODO(Julian): 
// (1) Synchronize event threading
// (2) Handle UTF-8
// (3) Window Resize Events
// (4) Detect terminal type
//     - color support
//     - ANSI code support
// (5) Memory allocation

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

#define CSI "\x1b["

int parse_number(int num, char *buf)
{
	char *c = buf;
	int l = 0;
	for (int n = num; n > 0; n /= 10)
	{
		*c++ = '0' + n % 10;
		++l;
	}

	for (int i = 0; i < l/2; ++i) {
		char t = buf[i];
		buf[i] = buf[l - i - 1];
		buf[l - i - 1] = t;
	}

	return l;
}

#define append_literal(terminal, string) append_bytes(terminal, string, sizeof(string)-1)
#define append_number(terminal, number, buffer) append_bytes(terminal, buffer, parse_number(number, buffer))

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

union event {
	uint32_t type;

	struct keyboard_event keyboard;
};

struct color {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct cell {
	struct color fg;
	struct color bg;
	char c;
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
};

bool create_terminal(struct terminal *t);
bool close_terminal(struct terminal *t);
bool write_terminal(struct terminal *t, char *buffer, int len);
void parse_terminal_input(struct terminal *t, char *buffer, int len);

#ifdef _WIN32
HANDLE mutex;

DWORD handle_stdin(struct terminal *t)
{
	DWORD read;
	char buffer[32];

	HANDLE in = GetStdHandle(STD_INPUT_HANDLE);

	while (true) {
		ReadFile(in, buffer, 32, &read, 0);
		parse_terminal_input(t, buffer, read);
	}

	return 0;
}

bool write_terminal(struct terminal *t, char *buffer, int len)
{
	DWORD w;
	return WriteFile(t->console_stdout, buffer, len, &w, 0);
}

bool create_terminal(struct terminal *t)
{
	mutex = CreateMutex(0, 0, 0);

	t->console_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
	t->console_stdin = GetStdHandle(STD_INPUT_HANDLE);

	DWORD console_mode;
	GetConsoleMode(t->console_stdout, &console_mode);
	console_mode = console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(t->console_stdout, console_mode);

	SetConsoleMode(t->console_stdin, ENABLE_VIRTUAL_TERMINAL_INPUT);

	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(t->console_stdout, &info);

	t->width = info.srWindow.Right - info.srWindow.Left + 1;
	t->height = info.srWindow.Bottom - info.srWindow.Top + 1;

	t->front_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	memset(t->front_buffer, 0, sizeof(struct cell)*32*t->width*t->height);

	t->back_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	memset(t->back_buffer, 0, sizeof(struct cell)*32*t->width*t->height);

	t->code_buffer = (uint8_t *)malloc(sizeof(uint8_t)*32*t->width*t->height);
	memset(t->code_buffer, 0, sizeof(uint8_t)*32*t->width*t->height);

	t->code_buffer_index = 0;

	DWORD thread_id;
	CreateThread(0, 0, handle_stdin, t, 0, &thread_id);

	return true;
}
#endif

#ifdef __linux__

// linux
void *handle_stdin(void *ptr)
{
	struct terminal *t = (struct terminal *)ptr;
	char buffer[32];

	while (true) {
		int r = read(0, buffer, 32);
		parse_terminal_input(t, buffer, r);
	}

	return 0;
}

bool write_terminal(struct terminal *t, char *buffer, int len)
{
	return write(t->fd, buffer, len) != -1;
}

// linux
bool create_terminal(struct terminal *t)
{
	t->fd = open("/dev/tty", O_RDWR);
	if (t->fd < 0) return false;

	struct winsize ws = {};
	if (ioctl(t->fd, TIOCGWINSZ, &ws) < 0) return false;

	t->width = ws.ws_col;
	t->height = ws.ws_row;

	struct termios tios = {};
	tcgetattr(t->fd, &t->original_termios);
	memcpy(&tios, &t->original_termios, sizeof(struct termios));

	tios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tios.c_oflag &= ~OPOST;
	tios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tios.c_cflag &= ~(CSIZE | PARENB);
	tios.c_cflag |= CS8;
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 0;
	tcsetattr(t->fd, TCSAFLUSH, &tios);

	t->front_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	memset(t->front_buffer, 0, sizeof(struct cell)*32*t->width*t->height);

	t->back_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	memset(t->back_buffer, 0, sizeof(struct cell)*32*t->width*t->height);

	t->code_buffer = (uint8_t *)malloc(sizeof(uint8_t)*32*t->width*t->height);
	memset(t->code_buffer, 0, sizeof(uint8_t)*32*t->width*t->height);
	t->code_buffer_index = 0;

	pthread_t pt;

	pthread_create(&pt, 0, handle_stdin, (void *)&t);

	return true;
}

bool close_terminal(struct terminal *t)
{
	return tcsetattr(t->fd, TCSAFLUSH, &t->original_termios) == 0;
}
#endif

static int last_x, last_y;
static struct cell last_cell;

void append_bytes(struct terminal *t, uint8_t *b, int l)
{
	//TODO(Julian): Check memcpy out of bounds.
	uint8_t *cb = &t->code_buffer[t->code_buffer_index];
	memcpy(cb, b, l);
	t->code_buffer_index += l;
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
		sprintf(b, CSI "%d;%dH", y+1, x+1);
		append_code(t, b);
	}

	if (memcmp(&last_cell.bg, &c->bg, sizeof(struct color)) != 0) {
		sprintf(b, CSI "48;2;%d;%d;%dm", c->bg.r, c->bg.g, c->bg.b); 
		append_code(t, b);
	}

	if (memcmp(&last_cell.fg, &c->fg, sizeof(struct color)) != 0) {
		sprintf(b, CSI "38;2;%d;%d;%dm", c->fg.r, c->fg.g, c->fg.b);
		append_code(t, b);
	}

	t->code_buffer[t->code_buffer_index++] = c->c;

	last_x = x;
	last_y = y;
	memcpy(&last_cell, c, sizeof(struct cell));
}

bool flush_terminal(struct terminal *t)
{
	write_terminal(t, t->code_buffer, t->code_buffer_index);
	t->code_buffer_index = 0;
	return true;
}

bool set_cell(struct terminal *t, int x, int y, struct cell *c)
{
	memcpy(&t->back_buffer[t->width*y + x], c, sizeof(struct cell));
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

bool render_terminal(struct terminal *t)
{
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

	write_terminal(t, t->code_buffer, t->code_buffer_index);
	t->code_buffer_index = 0;
	return true;
}

bool reset_terminal(struct terminal *t)
{
	return write_terminal(t, CSI "0m", 4);
}

bool set_cursor_position_terminal(struct terminal *t, int x, int y)
{
	char b[32];
	sprintf(b, CSI "%d;%dH", y, x);
	return write_terminal(t, b, strlen(b));
}

void hide_cursor_terminal(struct terminal *t)
{
	char buf[32];
	append_literal(t, CSI "?");
	append_number(t, 25, buf);
	append_literal(t, "l");
	// append_code(t, CSI "?25l");
	//return write_terminal(t, CSI "?25l", 6);
}

void show_cursor_terminal(struct terminal *t)
{
	append_code(t, CSI "?25h");
	//return write_terminal(t, CSI "?25h", 6);
}

bool poll_event_terminal(struct terminal *t, union event *e)
{
	if (event_count <= 0) return false;

	// int count = (int)InterlockedDecrement((LONG *)&event_count);
	// *e = event_queue[count];

	// WaitForSingleObject(mutex, INFINITE);
	*e = event_queue[--event_count];
	// ReleaseMutex(mutex);

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
