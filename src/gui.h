struct v2 {
	int x;
	int y;
};

struct v3 {
	int x;
	int y;
	int z;
};

struct buffer {
	uint8_t *bytes;
	uint32_t length;

	struct v2 position;
	struct v2 size;
};

bool create_buffer(struct buffer *b, char *fn, int x, int y, int w, int h)
{
	FILE *f = fopen(fn, "rw");
	fseek(f, 0, SEEK_END);
	b->length = ftell(f);
	fseek(f, 0, SEEK_SET);
	b->bytes = (uint8_t *)malloc(b->length);
	fread(b->bytes, 1, b->length, f);

	struct v2 position = {
		.x = x,
		.y = y,
	};

	struct v2 size = {
		.x = w,
		.y = h,
	};

	b->position = position;
	b->size = size;
}

void update_buffer(struct terminal *t, struct buffer *b, union event e)
{
	if (t->cursor_x < b->position.x || t->cursor_x >= b->position.x + b->size.x) {
		return;
	}

	if (t->cursor_y < b->position.y || t->cursor_y >= b->position.y + b->size.y) {
		return;
	}

	b->bytes[0] = 'X';
	return;
}

void render_buffer(struct terminal *t, struct buffer *b) 
{
	int x_0 = b->position.x;
	int y_0 = b->position.y;
	int width = b->size.x;
	int height = b->size.y;

	struct cell c = {
		.bg = {0, 0, 0},
		.fg = {255, 255, 255},
		.c = ' ',
	};

	int x = x_0;
	int y = y_0;

	for (int i = 0; i < b->length; ++i) {
		uint8_t byte = b->bytes[i];

		if (byte == '\n') {
			++y;
			x = x_0;
			continue;
		}

		if (byte == '\t') {
			x += 4;
			continue;
		}

		if (byte == ' ') {
			++x;
			continue;
		}

		if (x < 0 || x >= x_0 + width || y < 0 || y >= y_0 + height) continue;

		c.c = byte;
		set_cell(t, x, y, &c);
		++x;
	}
}
