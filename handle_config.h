#pragma once

// This is an example of assert macro for Windows. You should replace it with your own.
#define HDL_ASSERT(condition, ...) \
	if (!(condition)) { \
		HandleAssertionFailed(#condition, __FILE__, __LINE__, ##__VA_ARGS__); \
		__debugbreak(); \
	}

void HandleAssertionFailed(const char* _condition, const char* _file, int _line);
void HandleAssertionFailed(const char* _condition, const char* _file, int _line, const char* msg, ...);
