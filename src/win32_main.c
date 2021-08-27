#include <windows.h>
#include <synchapi.h>

#include <stdint.h>
#include <stdbool.h>

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

		Sleep(500);
	}

	return 0;
}
