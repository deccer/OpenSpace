#pragma once

#include <cstdint>
#include <memory>

using int8_t = std::int8_t;
using int16_t = std::int16_t;
using int32_t = std::int32_t;
using int64_t = std::int64_t;
using uint8_t = std::uint8_t;
using uint16_t = std::uint16_t;
using uint32_t = std::uint32_t;
using uint64_t = std::uint64_t;
using float32_t = float;
using float64_t = double;

template<typename T>
using TReference = std::shared_ptr<T>;

template<typename T>
using TScoped = std::unique_ptr<T>;

template<typename T>
using TWeak = std::weak_ptr<T>;

template<typename T, typename ... Args>
constexpr TReference<T> CreateReference(Args&& ... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T, typename ... Args>
constexpr TScoped<T> CreateScoped(Args&& ... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}
