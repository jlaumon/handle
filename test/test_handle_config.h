#pragma once

#include "catch/catch.hpp"

#define HDL_ASSERT(condition, ...) \
	if (!(condition)) { \
		auto str = FormatAssertString(#condition, ##__VA_ARGS__); \
		FAIL(str); \
	}

std::string FormatAssertString(const char* _condition);
std::string FormatAssertString(const char* _condition, const char* _msg, ...);
