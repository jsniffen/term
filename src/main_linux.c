#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>

#include "term.h"
#include "gui.h"

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
		if (terminal_read_event(&t, &e)) {
			clear_terminal(&t);

			if (e.type == KeyboardEvent) {
				if (e.keyboard.alt && (e.keyboard.key == Keyq || e.keyboard.key == KeyQ)) {
					running = false;
					break;
				}
				if (e.keyboard.key == Keyh) {
					if (t.cursor_x > 0) --t.cursor_x;
				}
				if (e.keyboard.key == Keyj) {
					if (t.cursor_y < t.height - 1) ++t.cursor_y;
				}
				if (e.keyboard.key == Keyk) {
					if (t.cursor_y > 0) --t.cursor_y;
				}
				if (e.keyboard.key == Keyl) {
					if (t.cursor_x < t.width - 1) ++t.cursor_x;
				}
				if (e.keyboard.key == Keys) {
					show_cursor_terminal(&t);
				}
				if (e.keyboard.key == Keye) {
					hide_cursor_terminal(&t);
				}
			}

			button(&t);

			render_terminal(&t);
		}
	}

	reset_terminal(&t);
	close_terminal(&t);
	return 0;
}
