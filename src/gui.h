static int x0 = 0;
static int y0 = 0;
static int x1 = 10;
static int y1 = 2;

bool button(struct terminal *t) {
	int w = x1 - x0;
	int h = y1 - y0;

	struct cell c = {
		.bg = {255, 255, 255},
		.fg = {0, 0, 0},
		.c = ' ',
	};

	if (t->cursor_x < x1 && t->cursor_x >= x0 && t->cursor_y < y1 && t->cursor_y >= y0) {
		c.bg.b=0;
		c.bg.r=0;
	}


	for (int y = y0; y < y0 + h; ++y) {
		for (int x = x0; x < x0 + w; ++x) {
			set_cell(t, x, y, &c);
		}
	}

	return true;
}
