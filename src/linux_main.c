#include <stdio.h>

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

	clear_terminal(&t);
	swap_buffers(&t);
	render_terminal(&t);

	union event e;
	while (running) {
		while (poll_event_terminal(&e)) {
			if (e.type == KeyboardEvent) {
				if (e.keyboard.key == 'q') {
					running = false;
					break;
				}
			}
		}

		sleep(1);
	}

	close_terminal(&t);

	return 0;
}
