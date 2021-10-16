#include <windows.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "log.h"
#include "term.h"
#include "gui.h"
#include "finn.h"

int main(void)
{
	struct terminal t;
	struct finn f;

	if (!create_terminal(&t)) {
		printf("Error creating terminal\n");
		return -1;
	}

	finn_init(&f);

	int y, x = 0;
	union event e;
	bool running = true;
	while (f.running) {
		if (read_terminal(&t, &e)) {
			clear_terminal(&t);

			finn_update_and_render(&f, &t, &e);

			render_terminal(&t);
		}
	}
	return 0;
}
