#include "test_handle_config.h"
#include <stdio.h>
#include <stdarg.h>
#include "catch/catch.hpp"


std::string FormatAssertString(const char* _condition)
{
	return std::string() + "HDL_ASSERT(" + _condition + ") failed";
}

std::string FormatAssertString(const char* _condition, const char* _msg, ...)
{
	va_list args;
	va_start(args, _msg);

	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), _msg, args);
	buffer[sizeof(buffer) - 1] = 0;

	va_end(args);

	return std::string() + "HDL_ASSERT(" + _condition + ") failed; " + buffer;
}
