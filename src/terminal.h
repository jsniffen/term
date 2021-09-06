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

#elif _WIN32

#include <stdio.h>

#include <windows.h>

#endif


#define CSI "\x1b["

enum {
	KeyboardEvent,
	MouseEvent,
	WindowEvent,
};

struct keyboard_event {
	uint32_t type;

	char key;
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

	uint8_t *write_buffer;

	struct cell *front_buffer;
	struct cell *back_buffer;
	bool *diff_buffer;
};

bool create_terminal(struct terminal *t);
bool close_terminal(struct terminal *t);
bool write_terminal(struct terminal *t, char *buffer, int len);
DWORD read_terminal(struct terminal *t, char *buffer, int len);
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

DWORD read_terminal(struct terminal *t, char *buffer, int len)
{
	DWORD r;
	return ReadFile(t->console_stdin, buffer, len, &r, 0);
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

	t->write_buffer = (uint8_t *)malloc(sizeof(uint8_t)*32*t->width*t->height);
	t->front_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	t->back_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	t->diff_buffer = (bool *)malloc(sizeof(bool)*32*t->width*t->height);

	DWORD thread_id;
	CreateThread(0, 0, handle_stdin, t, 0, &thread_id);

	return true;
}
#endif

#ifdef __linux__
bool write_terminal(struct terminal *t, char *buffer, int len)
{
	return write(t->fd, buffer, len) != -1;
}

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
	tios.c_cc[VMIN] = 0;
	tios.c_cc[VTIME] = 0;
	tcsetattr(t->fd, TCSAFLUSH, &tios);

	t->write_buffer = (uint8_t *)malloc(sizeof(uint8_t)*32*t->width*t->height);
	t->front_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	t->back_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	t->diff_buffer = (bool *)malloc(sizeof(bool)*32*t->width*t->height);

	return true;
}

bool close_terminal(struct terminal *t)
{
	return tcsetattr(t->fd, TCSAFLUSH, &t->original_termios) == 0;
}
#endif

bool flush_terminal(struct terminal *t, int len)
{
	return write_terminal(t, t->write_buffer, len);
}

bool set_cell(struct terminal *t, int x, int y, struct cell c)
{
	int os = t->width*y + x;
	t->diff_buffer[os] = true;
	t->back_buffer[os] = c;
	return true;
}

bool clear_terminal(struct terminal *t)
{
	for (int y = 0; y < t->height; ++y) {
		for (int x = 0; x < t->width; ++x) {
			struct cell c;
			c.fg.r = 255;
			c.fg.b = 255;
			c.fg.g = 255;
			c.bg.r = 0;
			c.bg.b = 0;
			c.bg.g = 0;
			c.c = 'x';
			set_cell(t, x, y, c);
		}
	}
	return true;
}

void swap_buffers(struct terminal *t)
{
	struct cell *fb = t->front_buffer;
	struct cell *bb = t->back_buffer;
	bool *db = t->diff_buffer;

	for (int os = 0; os < t->width*t->height; ++os) {
		if (db[os]) fb[os] = bb[os];
	}
}

bool render_terminal(struct terminal *t)
{
	uint8_t *wb = t->write_buffer;
	struct cell *fb = t->front_buffer;
	bool *db = t->diff_buffer;

	for (int os = 0; os < t->width*t->height; ++os) {
		if (db[os]) {
			struct cell c = fb[os];
			wb += sprintf(wb, CSI "48;2;%d;%d;%dm", c.bg.r, c.bg.g, c.bg.b);
			wb += sprintf(wb, CSI "38;2;%d;%d;%dm", c.fg.r, c.fg.g, c.fg.b);
			*wb++ = c.c;
		}
	}

	return flush_terminal(t, wb - t->write_buffer);
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

bool hide_cursor_terminal(struct terminal *t)
{
	return write_terminal(t, CSI "?25l", 6);
}

bool show_cursor_terminal(struct terminal *t)
{
	return write_terminal(t, CSI "?25h", 6);
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
	if (len <= 0) return;

	if (len == 1) {
		union event e;
		e.type = KeyboardEvent;
		e.keyboard.key = buffer[0];

		// InterlockedIncrement(LONGevent_queue
		// int count = (int)InterlockedIncrement((LONG *)&event_count) - 1;
		// event_queue[count] = e;

		// WaitForSingleObject(mutex, INFINITE);
		event_queue[event_count++] = e;
		// ReleaseMutex(mutex);
	}

	if (len > 1) {
		printf("%d bytes: %s\n", len, buffer);
	}
}
