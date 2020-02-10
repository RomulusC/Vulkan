#pragma  once
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <sstream>



class Logging
{
	
public:
	static Logging& instance()
	{
		static Logging* _instance = new Logging();
		return *_instance;
	}
	template<typename... T>
	std::string getFMT(const char* file, int line, const char* fmt, const T& ..._args)
	{
		writeLog(3, fmt, _args...);		
		std::string str = file;
		str += " : Line:";
		str += std::to_string(line);
		return str;
	}

	template<typename... T>
	void writeLog(int severity, const char* fmt, const T& ..._args) const
	{
		switch (severity)
		{
			case 0:
			{
				console->info(fmt, _args...);
				break;
			}
			case 1:
			{
				console->warn(fmt, _args...);
				break;
			}
			case 2:
			{
				console->error(fmt, _args...);
				break;
			}
			case 3:
			{
				console->critical(fmt, _args...);
				break;
			}
			default:
			{
				console->info(fmt, _args...);
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

#define FLog(severity, ...) Logging::instance().writeLog(severity, __VA_ARGS__)
#define FRuntimeErr(...) std::runtime_error(Logging::instance().getFMT(__FILE__, __LINE__, __VA_ARGS__));