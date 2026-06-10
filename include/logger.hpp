// logger.hpp
#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>

namespace Logger {

inline void info(const std::string &msg) {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::cout << "[INFO] " << std::put_time(std::localtime(&time), "%F %T")
            << " - " << msg << '\n';
}

inline void error(const std::string &msg) {
  auto now = std::chrono::system_clock::now();
  auto time = std::chrono::system_clock::to_time_t(now);

  std::cerr << "[ERROR] " << std::put_time(std::localtime(&time), "%F %T")
            << " - " << msg << '\n';
}

} // namespace Logger
