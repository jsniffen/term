HANDLE mutex;

bool update_window_size(struct terminal *t) {
	CONSOLE_SCREEN_BUFFER_INFO info;
	if (!GetConsoleScreenBufferInfo(t->console_stdout, &info)) return false;

	t->width = info.srWindow.Right - info.srWindow.Left + 1;
	t->height = info.srWindow.Bottom - info.srWindow.Top + 1;

	if (t->front_buffer) free(t->front_buffer);
	t->front_buffer = (struct cell *)malloc(sizeof(struct cell)*t->width*t->height);
	memset(t->front_buffer, 0, sizeof(struct cell)*t->width*t->height);

	if (t->back_buffer) free(t->back_buffer);
	t->back_buffer = (struct cell *)malloc(sizeof(struct cell)*t->width*t->height);
	memset(t->back_buffer, 0, sizeof(struct cell)*t->width*t->height);

	return true;
}

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

bool parse_key_event(KEY_EVENT_RECORD r, union event *e)
{
	if (!r.bKeyDown) return false;

	switch (r.wVirtualKeyCode) {
		case 0x51:
			e->keyboard.key = Keyq;
			break;
		default:
			break;
	}

	e->keyboard.alt = (r.dwControlKeyState & LEFT_ALT_PRESSED) ||
			  (r.dwControlKeyState & RIGHT_ALT_PRESSED);
	return true;
}

bool read_terminal(struct terminal *t, union event *e)
{
	HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
	INPUT_RECORD input[1];
	DWORD length = 1;
	DWORD read;
	if (!ReadConsoleInput(in, input, length, &read)) {
		//TODO(Julian): Handle error.
		return false;
	}

	if (length) {
		switch(input[0].EventType) {
			case KEY_EVENT:
				e->type = KeyboardEvent;
				return parse_key_event(input[0].Event.KeyEvent, e);
			case WINDOW_BUFFER_SIZE_EVENT:
				e->type = WindowEvent;
				return update_window_size(t);
			default:
				break;
		}
	}

	return true;
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

	SetConsoleMode(t->console_stdin, ENABLE_WINDOW_INPUT);

	t->front_buffer = 0;
	t->back_buffer = 0;
	update_window_size(t);

#if 0
#endif

	t->buffer = (struct slice *)malloc(sizeof(struct slice));
	slice_init(t->buffer, 256);

	// DWORD thread_id;
	// CreateThread(0, 0, handle_stdin, t, 0, &thread_id);

	return true;
}
