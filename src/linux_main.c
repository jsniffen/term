#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>

#include "terminal.h"

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

	union event e;
	int idx = 0;

	int y, x = 0;
	while (running) {
		while (poll_event_terminal(&t, &e)) {
			if (e.type == KeyboardEvent) {
				if (e.keyboard.alt && (e.keyboard.key == Keyq || e.keyboard.key == KeyQ)) {
					running = false;
					break;
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
			}
		}

		clear_terminal(&t);


		modal(&t, x, y, 10, 10);

		render_terminal(&t);
		usleep(16000);
	}

	reset_terminal(&t);
	close_terminal(&t);
	return 0;
}
