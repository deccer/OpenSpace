#pragma once

auto ReadBinaryFromFile(const std::filesystem::path& filePath) -> std::pair<std::unique_ptr<std::byte[]>, std::size_t>;
