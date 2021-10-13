#include <windows.h>
#include <synchapi.h>

#include <stdint.h>
#include <stdbool.h>

#include "term.h"

static bool running = true;

void modal(struct terminal *t, int x_0, int y_0, int w, int h)
{
	struct cell c = {
		.bg = {0, 0, 255},
		.fg = {0, 0, 0},
		.c = ' ',
	};
	for (int y = y_0; y < y_0 + h; ++y) {
		for (int x = x_0; x < x_0 + w; ++x) {
			set_cell(t, x, y, &c);
		}
	}
}

int main(void)
{
	struct terminal t;
	if (!create_terminal(&t)) {
		printf("Error creating terminal\n");
		return -1;
	}

	int y, x = 0;
	union event e;
	while (running) {
		if (read_terminal(&t, &e)) {
			clear_terminal(&t);

			if (e.keyboard.alt && (e.keyboard.key == Keyq || e.keyboard.key == KeyQ)) {
				running = false;
			}
			if (e.keyboard.key == Keyh) {
				--x;
			}
			if (e.keyboard.key == Keyj) {
				++y;
			}
			if (e.keyboard.key == Keyk) {
				--y;
			}
			if (e.keyboard.key == Keyl) {
				++x;
			}
			if (e.keyboard.key == Keys) {
				show_cursor_terminal(&t);
			}
			if (e.keyboard.key == Keye) {
				hide_cursor_terminal(&t);
			}

			modal(&t, x, y, 10, 10);

			render_terminal(&t);
		}
	}
	return 0;
}
