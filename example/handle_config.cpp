#include "handle_config.h"
#include <stdio.h>
#include <stdarg.h>

void HandleAssertionFailed(const char* _condition, const char* _file, int _line)
{
	fprintf(stderr,
		"Assertion failed: %s\n"
		"Source: %s, line %d\n",
		_condition, _file, _line
	);
}

void HandleAssertionFailed(const char* _condition, const char* _file, int _line, const char* _msg, ...)
{
	va_list args;
	va_start(args, _msg);

	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), _msg, args);
	buffer[sizeof(buffer) - 1] = 0;

	va_end(args);

	fprintf(stderr,
		"Assertion failed: %s\n"
		"Source: %s, line %d\n"
		"Message: %s\n",
		_condition, _file, _line, buffer
	);
}
