#pragma once

class TRenderer {
public:
    TRenderer();
    virtual ~TRenderer();

    auto Load() -> bool;
    auto Render() -> void;
    auto Unload() -> void;
};
