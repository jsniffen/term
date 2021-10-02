#include <stdio.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdint.h>

#include "terminal.h"
#include "finn.c"

static bool running = true;


int main(void)
{
	struct terminal t;
	if (!create_terminal(&t)) {
		printf("Error creating terminal\n");
		return -1;
	}

	struct finn f;
	if (!create_finn(&f)) {
		printf("Error creating finn\n");
		return -1;
	}



	union event e;
	while (finn_running(&f)) {
		clear_terminal(&t);

		while (poll_event_terminal(&t, &e)) {
			finn_update(&f, &t, &e);
		}

		render_terminal(&t);

		usleep(16000);
	}

	reset_terminal(&t);
	close_terminal(&t);

	return 0;
}
