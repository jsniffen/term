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

	t->buffer = (struct slice *)malloc(sizeof(struct slice));
	slice_init(t->buffer, 256);

	DWORD thread_id;
	CreateThread(0, 0, handle_stdin, t, 0, &thread_id);

	return true;
}
