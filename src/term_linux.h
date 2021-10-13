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

bool write_terminal(struct terminal *t, uint8_t *buffer, int len)
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

	t->buffer = (struct slice *)malloc(sizeof(struct slice));
	slice_init(t->buffer, 256);

	pthread_t pt;

	pthread_create(&pt, 0, handle_stdin, (void *)&t);

	t->cursor_x = 0;
	t->cursor_y = 0;

	return true;
}

bool close_terminal(struct terminal *t)
{
	return tcsetattr(t->fd, TCSAFLUSH, &t->original_termios) == 0;
}
