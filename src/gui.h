static struct color ColorBlue = {50, 104, 168};

void status_bar(struct terminal *t, uint8_t *b)
{
	struct cell c = {
		.bg = {234, 255, 255},
		.fg = {0, 0, 0},
		.c = ' ',
	};
	uint32_t x0 = 0;
	uint32_t x1 = t->width;


	uint32_t len = strlen(b);
	uint32_t y = 0;
	for (uint32_t x = x0; x < x1; ++x) {
		if (x < len) c.c = b[x];
		else c.c = ' ';
		set_cell(t, x, y, &c);
	}
}

void window(struct terminal *t, uint32_t count, uint32_t idx)
{
	struct cell c = {
		.bg = {255, 255, 234},
		.fg = {0, 0, 0},
		.c = ' ',
	};

	uint32_t x_diff = t->width / count;

	uint32_t x0, x1, y0, y1;
	x0 = idx*x_diff;
	x1 = x0 + x_diff;
	y0 = 1;
	y1 = t->height;

	for (uint32_t y = y0; y < y1; ++y) {
		for (uint32_t x = x0; x < x1; ++x) {
			if (x == x0 && y == y0) c.bg = (struct color) {136, 136, 204};
			else if (x == x0 && idx > 0) c.bg = ColorBlue;
			else if (y == y0) c.bg = (struct color) {234, 255, 255};
			else c.bg = (struct color) {255, 255, 234};
			set_cell(t, x, y, &c);
		}
	}
}

void modal(struct terminal *t, double padding)
{
	struct cell c = {
		.fg={0, 255, 0},
		.bg={0, 0, 0},
		.c='X',
	};

	uint32_t x_diff = (uint32_t)t->width*padding/2;
	uint32_t y_diff = (uint32_t)t->height*padding/2;

	uint32_t x0, x1, y0, y1;

	x0 = x_diff;
	x1 = t->width - x_diff;
	y0 = y_diff;
	y1 = t->height - y_diff;

	for (uint32_t y = y0; y < y1; ++y) {
		for (uint32_t x = x0; x < x1; ++x) {
			// top left
			if (x == x0 && y == y0) c.c = '┌';
			// top right
			else if (x == x1 - 1 && y == y0) c.c = '┐';
			// bottom left
			else if (x == x0 && y == y1 - 1) c.c = '└';
			// bottom right
			else if (x == x1 - 1 && y == y1 - 1) c.c = '┘';
			// top or bottom
			else if (y == y0 || y == y1 - 1) c.c = '─';
			// left or right
			else if (x == x0 || x == x1 - 1) c.c = '│';
			// else
			else c.c = ' ';

			set_cell(t, x, y, &c);
		}
	}
}
