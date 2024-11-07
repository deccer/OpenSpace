#pragma once

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
