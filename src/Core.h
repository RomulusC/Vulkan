#pragma once
#include "log.h"

#define Log(severity, ...) Logging::instance().writeLog(severity, __VA_ARGS__)
#define RuntimeCrash(...) abortImpl(__FILE__, __LINE__, __VA_ARGS__);
#define Breakpoint(...) breakpointImpl(__VA_ARGS__)

template<typename... T>
void abortImpl(const char* _str, int _line, const char* _fmt, const T& ..._args)
{
	Log(3, _fmt, _args...);
	Log(3, "{:s}:{:d}", _str, _line);
	::abort();
}
template<typename... T>
void breakpointImpl(const char* _fmt, const T& ..._args)
{
	if (_fmt != nullptr)
		Log(3, _fmt, _args...);

	__debugbreak();
}
void verifyImpl()
{
}