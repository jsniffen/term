enum state {
	Welcome, Quit,
};

struct finn {
	enum state state;
};

void render_welcome_screen(struct terminal *t)
{
	struct cell cell = {
		.bg = {0, 0, 0},
		.fg = {255, 255, 255},
		.c = 'X',
	};
	set_cell(t, 1, 1, &cell);
}

bool finn_running(struct finn *f)
{
	return f->state != Quit;
}

bool create_finn(struct finn *f)
{
	f->state = Welcome;
	return true;
}

void finn_update(struct finn *f, struct terminal *t, union event *e)
{
	if (e->type == KeyboardEvent) {
		if (e->keyboard.alt && (e->keyboard.key == Keyq || e->keyboard.key == KeyQ)) {
			f->state = Quit;
			return;
		}
	}

	if (f->state == Welcome) render_welcome_screen(t);
}

#if 0
void render_square(struct terminal *t)
{
	struct cell border = {
		.fg = {255, 255, 255},
		.bg = {0, 0, 0},
		.c = '=',
	};
	struct cell space = {
		.fg = {255, 255, 255},
		.bg = {0, 0, 0},
		.c = ' ',
	};
	set_cell(t, 0, 0, border);
	set_cell(t, 1, 0, space);
}
#endif
