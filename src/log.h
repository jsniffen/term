#include <stdarg.h>

static FILE *log_file;
static char log_buffer[1024];

void log_open()
{
	log_file = fopen("finn.log", "a+");
}

#define logf(msg, ...) _logf(msg, ##__VA_ARGS__)
void _logf(char *msg, ...)
{
	if (!log_file) log_open();

	va_list ap;
	va_start(ap, msg);
	vsprintf(log_buffer, msg, ap);
	va_end(ap);

	fwrite(log_buffer, 1, strlen(msg), log_file);
	fflush(log_file);
}
