#include "Logger.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

// Single definition, one-time initialization
std::shared_ptr<spdlog::logger> logger =
	spdlog::basic_logger_mt<spdlog::async_factory>("order_logger", "logs/app.log", true);
