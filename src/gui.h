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
			if (x == x0 && y == y0) c.c = 218;
			// top right
			else if (x == x1 - 1 && y == y0) c.c = 191;
			// bottom left
			else if (x == x0 && y == y1 - 1) c.c = 192;
			// bottom right
			else if (x == x1 - 1 && y == y1 - 1) c.c = 217;
			// top or bottom
			else if (y == y0 || y == y1 - 1) c.c = 196;
			// left or right
			else if (x == x0 || x == x1 - 1) c.c = 179;
			// else
			else c.c = ' ';

			set_cell(t, x, y, &c);
		}
	}
}
