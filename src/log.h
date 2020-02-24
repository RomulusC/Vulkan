#pragma  once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

class Logging
{
	
public:
	static Logging& instance()
	{
		static Logging* _instance = new Logging();
		return *_instance;
	}

	template<typename... T>
	void writeTrace(int severity, const char* _fmt, const T& ..._args)
	{
		switch (severity)
		{
		case 0:
		default:
		{
			trace->info(_fmt, _args...);
			break;
		}
		case 1:
		{
			trace->warn(_fmt, _args...);
			break;
		}
		case 2:
		{
			trace->error(_fmt, _args...);
			break;
		}
		case 3:
		{
			trace->critical(_fmt, _args...);
			break;
		}
		}
	}

	template<typename... T>
	void writeLog(int severity, const char* _fmt, const T& ..._args)
	{
		switch (severity)
		{
			case 0:
			default:
			{
				console->info(_fmt, _args...);
				break;
			}
			case 1:
			{
				console->warn(_fmt, _args...);
				break;
			}
			case 2:
			{
				console->error(_fmt, _args...);
				break;
			}
			case 3:
			{
				console->critical(_fmt, _args...);
				break;
			}			
		}
	}
	
private:
	Logging()
	{
		console = spdlog::stderr_color_mt("console");
		console->set_pattern("%^[%T] [%l]: %v %$");

		trace = spdlog::stderr_color_mt("trace");
		trace->set_pattern("%^%v %$");
	}
	
	std::shared_ptr<spdlog::logger> console;
	std::shared_ptr<spdlog::logger> trace;
};
