#include "Logger.hpp"

#include <ctime>
#include <iomanip>
#include <print>
#include <sstream>

TLogLevel TLogger::_logLevel = TLogLevel::Info;

constexpr std::string_view LogLevelToString(TLogLevel level) {
    switch (level) {
        case TLogLevel::Verbose: return "\033[97;1mverbose\033[0m";
        case TLogLevel::Debug: return "\033[35;1mdebug\033[0m";
        case TLogLevel::Info:  return "\033[94;1minfo\033[0m";
        case TLogLevel::Warning:  return "\033[93;1mwarn\033[0m";
        case TLogLevel::Error: return "\033[91;1merror\033[0m";
        case TLogLevel::Fatal: return "\033[97;101mfatal\033[0m";
        default: return "unknown";
    }
}

auto TLogger::SetMinLogLevel(TLogLevel logLevel) -> void {
    _logLevel = logLevel;
}

auto TLogger::Verbose(const std::string_view message) -> void {
    Log(TLogLevel::Verbose, message);
}

auto TLogger::Debug(const std::string_view message) -> void {
    Log(TLogLevel::Debug, message);
}

auto TLogger::Info(const std::string_view message) -> void {
    Log(TLogLevel::Info, message);
}

auto TLogger::Warning(const std::string_view message) -> void {
    Log(TLogLevel::Warning, message);
}

auto TLogger::Error(const std::string_view message) -> void {
    Log(TLogLevel::Error, message);
}

auto TLogger::Fatal(const std::string_view message) -> void {
    Log(TLogLevel::Fatal, message);
}

auto TLogger::Log(TLogLevel logLevel, const std::string_view message) -> void {

    if (_logLevel > logLevel) {
        return;
    }

    std::time_t time = std::time(nullptr);
    std::tm localTime = *std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");

    std::print("[{0}] [{1}] {2}!\n", oss.str(), LogLevelToString(logLevel), message);
}
