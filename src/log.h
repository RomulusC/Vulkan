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
	void writeLog(int severity, const char* _fmt, const T& ..._args)
	{
		switch (severity)
		{
			case 0:
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
			default:
			{
				console->info(_fmt, _args...);
				break;
			}
		}
	}
	
private:
	Logging()
	{
		console = spdlog::stderr_color_mt("console");
		spdlog::set_pattern("%^[%T] [%l]: %v %$");
	}
	
	std::shared_ptr<spdlog::logger> console;
};
