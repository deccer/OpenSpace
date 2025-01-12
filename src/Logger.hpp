#pragma once

#include <string_view>

enum class TLogLevel {
    Verbose,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

class TLogger {
public:
    static auto Verbose(std::string_view message) -> void;
    static auto Debug(std::string_view message) -> void;
    static auto Info(std::string_view message) -> void;
    static auto Warning(std::string_view message) -> void;
    static auto Error(std::string_view message) -> void;
    static auto Fatal(std::string_view message) -> void;
private:
    static auto Log(TLogLevel logLevel, std::string_view message) -> void;
};