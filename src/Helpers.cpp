#include "Helpers.hpp"

#include <format>

auto GetSafeResourceName(
    const char* const baseName,
    const char* const text,
    const char* const resourceType,
    const std::size_t resourceIndex) -> std::string {

    return (text == nullptr) || strlen(text) == 0
           ? std::format("{}-{}-{}", baseName, resourceType, resourceIndex)
           : std::format("{}-{}", baseName, text);
}
