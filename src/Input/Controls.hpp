#pragma once

#include "Key.hpp"
#include <glm/vec2.hpp>

struct TControlState {

    TKey Fast;
    TKey Faster;
    TKey Slow;

    TKey MoveForward;
    TKey MoveBackward;
    TKey MoveLeft;
    TKey MoveRight;
    TKey MoveUp;
    TKey MoveDown;

    TKey CursorMode;
    bool FreeLook;
    glm::vec2 CursorDelta;
    TKey ToggleMount;
};