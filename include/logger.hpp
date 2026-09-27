#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

/**
 * @brief Minimal leveled logger with optional ANSI colors, analogous in spirit
 *        to Python's logging + rich combo: one centralized place that decides
 *        how messages are displayed, instead of scattered std::cout/std::cerr calls.
 */
class Logger {
public:
    static void log(LogLevel level, const std::string& message);
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

    // Overrides auto-detection (color is enabled by default only when stdout is a real terminal)
    static void set_color_enabled(bool enabled);

private:
    static bool color_enabled;
};

#endif