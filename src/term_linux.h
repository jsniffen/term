static struct terminal *global_terminal;

bool update_window_size(struct terminal *t)
{
	struct winsize ws = {};
	if (ioctl(t->fd, TIOCGWINSZ, &ws) < 0) return false;

	t->width = ws.ws_col;
	t->height = ws.ws_row;

	logf("WINDOW: %d, %d\n", t->width, t->height);

	if (t->front_buffer) free(t->front_buffer);
	t->front_buffer = (struct cell *)malloc(sizeof(struct cell)*t->width*t->height);
	memset(t->front_buffer, 0, sizeof(struct cell)*t->width*t->height);

	if (t->back_buffer) free(t->front_buffer);
	t->back_buffer = (struct cell *)malloc(sizeof(struct cell)*t->width*t->height);
	memset(t->back_buffer, 0, sizeof(struct cell)*t->width*t->height);

	return true;
}

bool parse_ansi_event(union event *e, uint8_t *buf, uint32_t len)
{
	if (len <= 0) return false;

	if (len == 1) {
		e->type = KeyboardEvent;
		e->keyboard.key = (enum Key)buf[0];
		e->keyboard.alt = false;
		return true;
	}

	if (len == 2 && buf[0] == 0x1b) {
		e->type = KeyboardEvent;
		e->keyboard.key = (enum Key)buf[1];
		e->keyboard.alt = true;
		return true;
	}

	return false;

}

bool terminal_read_event(struct terminal *t, union event *e)
{
	uint8_t buf[32];
	int len = read(0, buf, 32);
	return parse_ansi_event(e, buf, len);
}

bool write_terminal(struct terminal *t, uint8_t *buffer, int len)
{
	return write(t->fd, buffer, len) != -1;
}

void sigwinch_handler(int _)
{
	update_window_size(global_terminal);
}

bool create_terminal(struct terminal *t)
{
	t->fd = open("/dev/tty", O_RDWR);
	if (t->fd < 0) return false;

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigwinch_handler;
	sa.sa_flags = 0;
	sigaction(SIGWINCH, &sa, 0);

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

	t->front_buffer = 0;
	t->back_buffer = 0;
	update_window_size(t);

	t->buffer = (struct slice *)malloc(sizeof(struct slice));
	slice_init(t->buffer, 256);

	t->cursor_x = 0;
	t->cursor_y = 0;

	global_terminal = t;
	return true;
}

bool close_terminal(struct terminal *t)
{
	return tcsetattr(t->fd, TCSAFLUSH, &t->original_termios) == 0;
}
