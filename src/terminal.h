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

#include <windows.h>

#endif

#define CSI "\x1b["

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

struct terminal {
#ifdef _WIN32
	HANDLE console;
#endif

	uint32_t fd;
	uint32_t width;
	uint32_t height;

	uint8_t *write_buffer;

	struct cell *front_buffer;
	struct cell *back_buffer;
	bool *diff_buffer;
};

bool create_terminal(struct terminal *t);
bool write_terminal(struct terminal *t);

#ifdef _WIN32
bool write_terminal(struct terminal *t, int len)
{
	DWORD w;
	return WriteFile(t->console, t->write_buffer, len, &w, 0);
}

bool create_terminal(struct terminal *t)
{
	t->console = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD console_mode;
	GetConsoleMode(t->console, &console_mode);
	SetConsoleMode(t->console, console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

	CONSOLE_SCREEN_BUFFER_INFO info;
	GetConsoleScreenBufferInfo(t->console, &info);

	t->width = info.srWindow.Right - info.srWindow.Left + 1;
	t->height = info.srWindow.Bottom - info.srWindow.Top + 1;

	t->write_buffer = (uint8_t *)malloc(sizeof(uint8_t)*32*t->width*t->height);
	t->front_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	t->back_buffer = (struct cell *)malloc(sizeof(struct cell)*32*t->width*t->height);
	t->diff_buffer = (bool *)malloc(sizeof(bool)*32*t->width*t->height);
}
#endif

#ifdef __linux__
bool write_terminal(struct terminal *t, int len)
{
	return write(t->fd, t->write_buffer, wb - t->write_buffer) != -1;
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
	tcgetattr(t->fd, &tios);

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
#endif

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

	return write_terminal(t, wb - t->write_buffer);
}

