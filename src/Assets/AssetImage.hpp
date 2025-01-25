#pragma once

#include <string>

enum class TAssetImageType {
    Uncompressed,
    CompressedDds,
    CompressedKtx2,
};

struct TAssetImage {
    std::string Name;
    std::vector<uint8_t> Data;
    TAssetImageType Type;
};
