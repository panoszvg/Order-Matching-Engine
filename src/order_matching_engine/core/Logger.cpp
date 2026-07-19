#include "Logger.h"
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

// Single definition, one-time initialization
std::shared_ptr<spdlog::logger> logger =
	spdlog::basic_logger_mt<spdlog::async_factory>("order_logger", "logs/app.log", true);

std::shared_ptr<spdlog::logger> netLogger = [] {
	auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/network.log", true);
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	auto lg = std::make_shared<spdlog::logger>("net_logger", spdlog::sinks_init_list{file_sink, console_sink});
	spdlog::register_logger(lg);
	return lg;
}();
