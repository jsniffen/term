#include <windows.h>
#include <synchapi.h>

#include "terminal.h"

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

	for (int i = 1; i < 255; ++i) {
		set_cursor_terminal(&t, i, i);
		write_terminal(&t, "J", 1);
		Sleep(500);
	}

	return 0;
}
