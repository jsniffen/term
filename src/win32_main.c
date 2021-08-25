#include <windows.h>

#include <stdio.h>

#include "terminal.h"

int main(void)
{
	struct terminal t;
	create_terminal(&t);

	clear_terminal(&t);

	swap_buffers(&t);
	render_terminal(&t);

	return 0;
}
