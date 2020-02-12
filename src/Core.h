#pragma once
#include "log.h"

#define CLog(severity, ...) Logging::instance().writeLog(severity, __VA_ARGS__)
#define CRuntimeCrash(...) abortImpl(__FILE__, __LINE__, __VA_ARGS__); ::abort()
#define CBreakpoint(...) breakpointImpl(__VA_ARGS__); __debugbreak() 
#define CVerify(_bVerify,...) verifyBreakImpl(_bVerify, 1, __VA_ARGS__);
#define CVerifyCrash(_bVerify,...) verifyAbortImpl(__FILE__, __LINE__, _bVerify, 3, __VA_ARGS__);

template<typename... T>
void abortImpl(const char* _str, int _line, const char* _fmt, const T& ..._args)
{
	CLog(3, _fmt, _args...);
	CLog(3, "{:s}:{:d}", _str, _line);
	
}
template<typename... T>
void breakpointImpl(const char* _fmt, const T& ..._args)
{
	if (_fmt != nullptr)
		CLog(3, _fmt, _args...);
}
template<typename... T>
void verifyAbortImpl(const char* _file, int line, bool _verify, int _severity, const char* _fmt, const T& ..._args)
{
	if (!_verify)
	{
		CLog(_severity, "{} \n{:s}:{:d}",_fmt, _args..., _file, line);
		__debugbreak();
		abort();
	}
}
template<typename... T>
void verifyBreakImpl(bool _verify,int _severity, const char* _fmt, const T& ..._args)
{
	if (!_verify)
	{
		CLog(_severity, _fmt, _args...);
		__debugbreak();
	}
}