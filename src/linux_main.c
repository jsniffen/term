#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>

#include "terminal.h"

static bool running = true;

int main(void)
{
	struct terminal t;
	if (!create_terminal(&t)) {
		printf("Error creating terminal\n");
		return -1;
	}

	union event e;
	int idx = 0;
	while (running) {
		while (poll_event_terminal(&t, &e)) {
			if (e.type == KeyboardEvent) {
				if (e.keyboard.alt && (e.keyboard.key == Keyq || e.keyboard.key == KeyQ)) {
					running = false;
					break;
				}
			}
		}

		clear_terminal(&t);

		struct cell c = {
			.bg = {0, 0, 0},
			.fg = {0, 0, 0},
			.c = ' ',
		};

		set_cell(&t, idx, idx, &c);
		idx = idx + 1 % 255;

		render_terminal(&t);
		usleep(100000);
	}

	reset_terminal(&t);
	close_terminal(&t);
	return 0;
}
