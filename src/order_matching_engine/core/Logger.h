#ifndef LOGGER_H
#define LOGGER_H

#pragma once

#include <memory>
#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger> logger;
extern std::shared_ptr<spdlog::logger> netLogger;

#endif // LOGGER_H