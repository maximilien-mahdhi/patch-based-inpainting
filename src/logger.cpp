#include "logger.hpp"
#include <iostream>
#include <unistd.h>  // isatty
#include <cstdio>    // fileno

namespace {

const char* RESET  = "\033[0m";
const char* CYAN   = "\033[36m";
const char* GREEN  = "\033[32m";
const char* YELLOW = "\033[33m";
const char* RED    = "\033[31m";

const char* level_color(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return CYAN;
        case LogLevel::INFO:    return GREEN;
        case LogLevel::WARNING: return YELLOW;
        case LogLevel::ERROR:   return RED;
    }
    return RESET;
}

const char* level_name(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR:   return "ERROR";
    }
    return "";
}

} // anonymous namespace

// Auto-detect once at startup: no color if output is redirected to a file/pipe
bool Logger::color_enabled = isatty(fileno(stdout));

void Logger::set_color_enabled(bool enabled) {
    color_enabled = enabled;
}

void Logger::log(LogLevel level, const std::string& message) {
    // WARNING and ERROR go to stderr, like Python's logging default behavior
    std::ostream& out = (level == LogLevel::ERROR || level == LogLevel::WARNING) ? std::cerr : std::cout;

    if (color_enabled) {
        out << level_color(level) << "[" << level_name(level) << "]" << RESET << " " << message << std::endl;
    } else {
        out << "[" << level_name(level) << "] " << message << std::endl;
    }
}

void Logger::debug(const std::string& message)   { log(LogLevel::DEBUG, message); }
void Logger::info(const std::string& message)    { log(LogLevel::INFO, message); }
void Logger::warning(const std::string& message) { log(LogLevel::WARNING, message); }
void Logger::error(const std::string& message)   { log(LogLevel::ERROR, message); }