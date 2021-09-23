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

bool finn_run(struct terminal *t)
{
	union event e;
	while (poll_event_terminal(t, &e)) {
		if (e.type == KeyboardEvent) {
			if (e.keyboard.alt && (e.keyboard.key == Keyq || e.keyboard.key == KeyQ)) {
				return false;
			}

			if (e.keyboard.key == Keyo) {
				render_square(t);
			}
		}
	}

	swap_buffers(t);
	render_terminal(t);
	reset_diff_buffer(t);

	return true;
}


