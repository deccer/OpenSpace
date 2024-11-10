#pragma once

#include <cstdint>
#include <string_view>

struct TExtent2D
{
    uint32_t Width{};
    uint32_t Height{};

    bool operator==(const TExtent2D&) const noexcept = default;
    constexpr TExtent2D operator+(const TExtent2D& other) const { return { Width + other.Width, Height + other.Height }; }
    constexpr TExtent2D operator-(const TExtent2D& other) const { return { Width - other.Width, Height - other.Height }; }
    constexpr TExtent2D operator*(const TExtent2D& other) const { return { Width * other.Width, Height * other.Height }; }
    constexpr TExtent2D operator/(const TExtent2D& other) const { return { Width / other.Width, Height / other.Height }; }
    constexpr TExtent2D operator>>(const TExtent2D& other) const { return { Width >> other.Width, Height >> other.Height }; }
    constexpr TExtent2D operator<<(const TExtent2D& other) const { return { Width << other.Width, Height << other.Height }; }
    constexpr TExtent2D operator+(uint32_t value) const { return *this + TExtent2D{ value, value }; }
    constexpr TExtent2D operator-(uint32_t value) const { return *this - TExtent2D{ value, value }; }
    constexpr TExtent2D operator*(uint32_t value) const { return *this * TExtent2D{ value, value }; }
    constexpr TExtent2D operator/(uint32_t value) const { return *this / TExtent2D{ value, value }; }
    constexpr TExtent2D operator>>(uint32_t value) const { return *this >> TExtent2D{ value, value }; }
    constexpr TExtent2D operator<<(uint32_t value) const { return *this << TExtent2D{ value, value }; }
};

constexpr TExtent2D operator+(uint32_t value, TExtent2D extent) {
    return extent + value;
}

constexpr TExtent2D operator-(uint32_t value, TExtent2D extent) {
    return extent - value;
}

constexpr TExtent2D operator*(uint32_t value, TExtent2D extent) {
    return extent * value;
}

constexpr TExtent2D operator/(uint32_t value, TExtent2D extent) {
    return extent / value;
}

constexpr TExtent2D operator>>(uint32_t value, TExtent2D extent) {
    return extent >> value;
}

constexpr TExtent2D operator<<(uint32_t value, TExtent2D extent) {
    return extent << value;
}

struct TExtent3D
{
    uint32_t Width{};
    uint32_t Height{};
    uint32_t Depth{};

    operator TExtent2D() const { return { Width, Height }; }
    bool operator==(const TExtent3D&) const noexcept = default;
    constexpr TExtent3D operator+(const TExtent3D& other) const { return { Width + other.Width, Height + other.Height, Depth + other.Depth }; }
    constexpr TExtent3D operator-(const TExtent3D& other) const { return { Width - other.Width, Height - other.Height, Depth - other.Depth }; }
    constexpr TExtent3D operator*(const TExtent3D& other) const { return { Width * other.Width, Height * other.Height, Depth * other.Depth }; }
    constexpr TExtent3D operator/(const TExtent3D& other) const { return { Width / other.Width, Height / other.Height, Depth / other.Depth }; }
    constexpr TExtent3D operator>>(const TExtent3D& other) const { return { Width >> other.Width, Height >> other.Height, Depth >> other.Depth }; }
    constexpr TExtent3D operator<<(const TExtent3D& other) const { return { Width << other.Width, Height << other.Height, Depth << other.Depth }; }
    constexpr TExtent3D operator+(uint32_t value) const { return *this + TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator-(uint32_t value) const { return *this - TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator*(uint32_t value) const { return *this * TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator/(uint32_t value) const { return *this / TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator>>(uint32_t value) const { return *this >> TExtent3D{ value, value, value }; }
    constexpr TExtent3D operator<<(uint32_t value) const { return *this << TExtent3D{ value, value, value }; }
};

constexpr TExtent3D operator+(uint32_t value, TExtent3D extent) {
    return extent + value;
}

constexpr TExtent3D operator-(uint32_t value, TExtent3D extent) {
    return extent - value;
}

constexpr TExtent3D operator*(uint32_t value, TExtent3D extent) {
    return extent * value;
}

constexpr TExtent3D operator/(uint32_t value, TExtent3D extent) {
    return extent / value;
}

constexpr TExtent3D operator>>(uint32_t value, TExtent3D extent) {
    return extent >> value;
}

constexpr TExtent3D operator<<(uint32_t value, TExtent3D extent) {
    return extent << value;
}

struct TOffset2D {

    uint32_t X{};
    uint32_t Y{};

    bool operator==(const TOffset2D&) const noexcept = default;
    constexpr TOffset2D operator+(const TOffset2D & other) const { return { X + other.X, Y + other.Y }; }
    constexpr TOffset2D operator-(const TOffset2D & other) const { return { X - other.X, Y - other.Y }; }
    constexpr TOffset2D operator*(const TOffset2D & other) const { return { X * other.X, Y * other.Y }; }
    constexpr TOffset2D operator/(const TOffset2D & other) const { return { X / other.X, Y / other.Y }; }
    constexpr TOffset2D operator>>(const TOffset2D & other) const { return { X >> other.X, Y >> other.Y }; }
    constexpr TOffset2D operator<<(const TOffset2D & other) const { return { X << other.X, Y << other.Y }; }
    constexpr TOffset2D operator+(uint32_t value) const { return *this + TOffset2D{ value, value }; }
    constexpr TOffset2D operator-(uint32_t value) const { return *this - TOffset2D{ value, value }; }
    constexpr TOffset2D operator*(uint32_t value) const { return *this * TOffset2D{ value, value }; }
    constexpr TOffset2D operator/(uint32_t value) const { return *this / TOffset2D{ value, value }; }
    constexpr TOffset2D operator>>(uint32_t value) const { return *this >> TOffset2D{ value, value }; }
    constexpr TOffset2D operator<<(uint32_t value) const { return *this << TOffset2D{ value, value }; }
};

constexpr TOffset2D operator+(uint32_t value, TOffset2D offset) {
    return offset + value;
}

constexpr TOffset2D operator-(uint32_t value, TOffset2D offset) {
    return offset - value;
}

constexpr TOffset2D operator*(uint32_t value, TOffset2D offset) {
    return offset * value;
}

constexpr TOffset2D operator/(uint32_t value, TOffset2D offset) {
    return offset / value;
}

constexpr TOffset2D operator>>(uint32_t value, TOffset2D offset) {
    return offset >> value;
}

constexpr TOffset2D operator<<(uint32_t value, TOffset2D offset) {
    return offset << value;
}

struct TOffset3D {

    uint32_t X{};
    uint32_t Y{};
    uint32_t Z{};

    operator TOffset2D() const { return { X, Y }; }
    bool operator==(const TOffset3D&) const noexcept = default;
    constexpr TOffset3D operator+(const TOffset3D& other) const { return { X + other.X, Y + other.Y, Z + other.Z }; }
    constexpr TOffset3D operator-(const TOffset3D& other) const { return { X - other.X, Y - other.Y, Z - other.Z }; }
    constexpr TOffset3D operator*(const TOffset3D& other) const { return { X * other.X, Y * other.Y, Z * other.Z }; }
    constexpr TOffset3D operator/(const TOffset3D& other) const { return { X / other.X, Y / other.Y, Z / other.Z }; }
    constexpr TOffset3D operator>>(const TOffset3D& other) const { return { X >> other.X, Y >> other.Y, Z >> other.Z }; }
    constexpr TOffset3D operator<<(const TOffset3D& other) const { return { X << other.X, Y << other.Y, Z << other.Z }; }
    constexpr TOffset3D operator+(uint32_t value) const { return *this + TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator-(uint32_t value) const { return *this - TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator*(uint32_t value) const { return *this * TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator/(uint32_t value) const { return *this / TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator>>(uint32_t value) const { return *this >> TOffset3D{ value, value, value }; }
    constexpr TOffset3D operator<<(uint32_t value) const { return *this << TOffset3D{ value, value, value }; }
};

constexpr TOffset3D operator+(uint32_t value, TOffset3D offset) {
    return offset + value;
}

constexpr TOffset3D operator-(uint32_t value, TOffset3D offset) {
    return offset - value;
}

constexpr TOffset3D operator*(uint32_t value, TOffset3D offset) {
    return offset * value;
}

constexpr TOffset3D operator/(uint32_t value, TOffset3D offset) {
    return offset / value;
}

constexpr TOffset3D operator>>(uint32_t value, TOffset3D offset) {
    return offset >> value;
}

constexpr TOffset3D operator<<(uint32_t value, TOffset3D offset) {
    return offset << value;
}

// Stolen from Jaker who stole it from https://github.com/cdgiessen/vk-module/blob/076baa98cba35cd93a6ab56c3fd1b1ea2313f806/codegen_text.py#L53
// Thanks Jaker & Charles!
#define DECLARE_FLAG_TYPE(TFlagType, TFlagTypeBits, TFlagBaseType)                                \
                                                                                                  \
struct TFlagType                                                                                  \
{                                                                                                 \
    TFlagBaseType flags = static_cast<TFlagBaseType>(0);                                          \
    constexpr TFlagType() noexcept = default;                                                     \
    constexpr explicit TFlagType(TFlagBaseType in) noexcept : flags(in) {}                        \
    constexpr TFlagType(TFlagTypeBits in) noexcept : flags(static_cast<TFlagBaseType>(in)) {}     \
    constexpr bool operator==(TFlagType const& right) const                                       \
    {                                                                                             \
        return flags == right.flags;                                                              \
    }                                                                                             \
    constexpr bool operator!=(TFlagType const& right) const                                       \
    {                                                                                             \
        return flags != right.flags;                                                              \
    }                                                                                             \
    constexpr explicit operator TFlagBaseType() const                                             \
    {                                                                                             \
        return flags;                                                                             \
    }                                                                                             \
    constexpr explicit operator bool() const noexcept                                             \
    {                                                                                             \
        return flags != 0;                                                                        \
    }                                                                                             \
};                                                                                                \
                                                                                                  \
constexpr TFlagType operator|(TFlagType a, TFlagType b) noexcept                                  \
{                                                                                                 \
    return static_cast<TFlagType>(a.flags | b.flags);                                             \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator&(TFlagType a, TFlagType b) noexcept                                  \
{                                                                                                 \
    return static_cast<TFlagType>(a.flags & b.flags);                                             \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator^(TFlagType a, TFlagType b) noexcept                                  \
{                                                                                                 \
    return static_cast<TFlagType>(a.flags ^ b.flags);                                             \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator~(TFlagType a) noexcept                                               \
{                                                                                                 \
    return static_cast<TFlagType>(~a.flags);                                                      \
}                                                                                                 \
constexpr TFlagType& operator|=(TFlagType& a, TFlagType b) noexcept                               \
{                                                                                                 \
    a.flags = (a.flags | b.flags);                                                                \
    return a;                                                                                     \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType& operator&=(TFlagType& a, TFlagType b) noexcept                               \
{                                                                                                 \
    a.flags = (a.flags & b.flags);                                                                \
    return a;                                                                                     \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator^=(TFlagType& a, TFlagType b) noexcept                                \
{                                                                                                 \
    a.flags = (a.flags ^ b.flags);                                                                \
    return a;                                                                                     \
}                                                                                                 \
constexpr TFlagType operator|(TFlagTypeBits a, TFlagTypeBits b) noexcept                          \
{                                                                                                 \
    return static_cast<TFlagType>(static_cast<TFlagBaseType>(a) | static_cast<TFlagBaseType>(b)); \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator&(TFlagTypeBits a, TFlagTypeBits b) noexcept                          \
{                                                                                                 \
    return static_cast<TFlagType>(static_cast<TFlagBaseType>(a) & static_cast<TFlagBaseType>(b)); \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator~(TFlagTypeBits key) noexcept                                         \
{                                                                                                 \
    return static_cast<TFlagType>(~static_cast<TFlagBaseType>(key));                              \
}                                                                                                 \
                                                                                                  \
constexpr TFlagType operator^(TFlagTypeBits a, TFlagTypeBits b) noexcept                          \
{                                                                                                 \
    return static_cast<TFlagType>(static_cast<TFlagBaseType>(a) ^ static_cast<TFlagBaseType>(b)); \
}

template<class TTag>
struct TIdImpl {
    enum class Id : std::size_t { Invalid = SIZE_MAX };

    bool operator==(const TIdImpl&) const noexcept = default;
};

template<class TTag>
using TId = typename TIdImpl<TTag>::Id;

template<class TTag>
struct TIdGenerator {
public:
    auto Next() -> TId<TTag> {
        _counter += 1;
        return { _counter };
    }
private:
    size_t _counter = 0;
};

constexpr auto HashString(std::string_view str) -> uint32_t {
    auto hash = 2166136261u;
    for (auto ch: str) {
        hash ^= ch;
        hash *= 16777619u;
    }
    return hash;
}

constexpr auto operator"" _hash(const char* str, size_t) -> uint32_t {
    return HashString(str);
}