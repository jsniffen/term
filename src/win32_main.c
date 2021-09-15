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
		while (poll_event_terminal(&t, &e)) {
			if (e.type == KeyboardEvent) {
				printf("handling event: %c\n", e.keyboard.key);

				if (e.keyboard.key == KeyA) {
					printf("A\n");
				}

				if (e.keyboard.key == Key0) {
					printf("0\n");
				}

				if (e.keyboard.key == Keyq || e.keyboard.key == KeyQ) {
					running = false;
					break;
				}
			}
		}

		Sleep(100);
	}

	return 0;
}
