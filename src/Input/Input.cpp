#include "Input.hpp"

#include <SDL2/SDL_keycode.h>

#include <utility>

auto KeyCodeToInputKey(const int32_t keyCode) -> int32_t {
    switch (keyCode) {
        case SDLK_ESCAPE: return INPUT_KEY_ESCAPE;
        case SDLK_F1: return INPUT_KEY_F1;
        case SDLK_F2: return INPUT_KEY_F2;
        case SDLK_F3: return INPUT_KEY_F3;
        case SDLK_F4: return INPUT_KEY_F4;
        case SDLK_F5: return INPUT_KEY_F5;
        case SDLK_F6: return INPUT_KEY_F6;
        case SDLK_F7: return INPUT_KEY_F7;
        case SDLK_F8: return INPUT_KEY_F8;
        case SDLK_F9: return INPUT_KEY_F9;
        case SDLK_F10: return INPUT_KEY_F10;
        case SDLK_F11: return INPUT_KEY_F11;
        case SDLK_F12: return INPUT_KEY_F12;
        case SDLK_PRINTSCREEN: return INPUT_KEY_PRINT_SCREEN;
        case SDLK_SCROLLLOCK: return INPUT_KEY_SCROLL_LOCK;
        case SDLK_PAUSE: return INPUT_KEY_PAUSE;
        case SDLK_INSERT: return INPUT_KEY_INSERT;
        case SDLK_DELETE: return INPUT_KEY_DELETE;
        case SDLK_HOME: return INPUT_KEY_HOME;
        case SDLK_END: return INPUT_KEY_END;
        case SDLK_PAGEUP: return INPUT_KEY_PAGE_UP;
        case SDLK_PAGEDOWN: return INPUT_KEY_PAGE_DOWN;
        case SDLK_LEFT: return INPUT_KEY_LEFT;
        case SDLK_RIGHT: return INPUT_KEY_RIGHT;
        case SDLK_UP: return INPUT_KEY_UP;
        case SDLK_DOWN: return INPUT_KEY_DOWN;

        case SDLK_BACKQUOTE: return INPUT_KEY_GRAVE_ACCENT;
        case SDLK_1: return INPUT_KEY_1;
        case SDLK_2: return INPUT_KEY_2;
        case SDLK_3: return INPUT_KEY_3;
        case SDLK_4: return INPUT_KEY_4;
        case SDLK_5: return INPUT_KEY_5;
        case SDLK_6: return INPUT_KEY_6;
        case SDLK_7: return INPUT_KEY_7;
        case SDLK_8: return INPUT_KEY_8;
        case SDLK_9: return INPUT_KEY_9;
        case SDLK_0: return INPUT_KEY_0;
        case SDLK_MINUS: return INPUT_KEY_MINUS;
        case SDLK_EQUALS: return INPUT_KEY_EQUAL;
        case SDLK_BACKSPACE: return INPUT_KEY_BACKSPACE;
        case SDLK_TAB: return INPUT_KEY_TAB;
        case SDLK_q: return INPUT_KEY_Q;
        case SDLK_w: return INPUT_KEY_W;
        case SDLK_e: return INPUT_KEY_E;
        case SDLK_r: return INPUT_KEY_R;
        case SDLK_t: return INPUT_KEY_T;
        case SDLK_y: return INPUT_KEY_Y;
        case SDLK_u: return INPUT_KEY_U;
        case SDLK_i: return INPUT_KEY_I;
        case SDLK_o: return INPUT_KEY_O;
        case SDLK_p: return INPUT_KEY_P;
        case SDLK_LEFTBRACKET: return INPUT_KEY_LEFT_BRACKET;
        case SDLK_RIGHTBRACKET: return INPUT_KEY_RIGHT_BRACKET;
        case SDLK_BACKSLASH: return INPUT_KEY_BACKSLASH;
        case SDLK_CAPSLOCK: return INPUT_KEY_CAPS_LOCK;
        case SDLK_a: return INPUT_KEY_A;
        case SDLK_s: return INPUT_KEY_S;
        case SDLK_d: return INPUT_KEY_D;
        case SDLK_f: return INPUT_KEY_F;
        case SDLK_g: return INPUT_KEY_G;
        case SDLK_h: return INPUT_KEY_H;
        case SDLK_j: return INPUT_KEY_J;
        case SDLK_k: return INPUT_KEY_K;
        case SDLK_l: return INPUT_KEY_L;
        case SDLK_SEMICOLON: return INPUT_KEY_SEMICOLON;
        case SDLK_QUOTE: return INPUT_KEY_APOSTROPHE;
        case SDLK_RETURN: return INPUT_KEY_ENTER;
        case SDLK_LSHIFT: return INPUT_KEY_LEFT_SHIFT;
        case SDLK_z: return INPUT_KEY_Z;
        case SDLK_x: return INPUT_KEY_X;
        case SDLK_c: return INPUT_KEY_C;
        case SDLK_v: return INPUT_KEY_V;
        case SDLK_b: return INPUT_KEY_B;
        case SDLK_n: return INPUT_KEY_N;
        case SDLK_m: return INPUT_KEY_M;
        case SDLK_COMMA: return INPUT_KEY_COMMA;
        case SDLK_PERIOD: return INPUT_KEY_PERIOD;
        case SDLK_SLASH: return INPUT_KEY_SLASH;
        case SDLK_RSHIFT: return INPUT_KEY_RIGHT_SHIFT;
        case SDLK_LCTRL: return INPUT_KEY_LEFT_CONTROL;
        case SDLK_LGUI: return INPUT_KEY_LEFT_SUPER;
        case SDLK_LALT: return INPUT_KEY_LEFT_ALT;
        case SDLK_SPACE: return INPUT_KEY_SPACE;
        case SDLK_RALT: return INPUT_KEY_RIGHT_ALT;
        case SDLK_RGUI: return INPUT_KEY_RIGHT_SUPER;
        case SDLK_MENU: return INPUT_KEY_MENU;
        case SDLK_RCTRL: return INPUT_KEY_RIGHT_CONTROL;

        case SDLK_NUMLOCKCLEAR: return INPUT_KEY_NUM_LOCK;
        case SDLK_KP_DIVIDE: return INPUT_KEY_KP_DIVIDE;
        case SDLK_KP_MULTIPLY: return INPUT_KEY_KP_MULTIPLY;
        case SDLK_KP_MINUS: return INPUT_KEY_KP_SUBTRACT;
        case SDLK_KP_PLUS: return INPUT_KEY_KP_ADD;
        case SDLK_KP_ENTER: return INPUT_KEY_KP_ENTER;
        case SDLK_KP_1: return INPUT_KEY_KP_1;
        case SDLK_KP_2: return INPUT_KEY_KP_2;
        case SDLK_KP_3: return INPUT_KEY_KP_3;
        case SDLK_KP_4: return INPUT_KEY_KP_4;
        case SDLK_KP_5: return INPUT_KEY_KP_5;
        case SDLK_KP_6: return INPUT_KEY_KP_6;
        case SDLK_KP_7: return INPUT_KEY_KP_7;
        case SDLK_KP_8: return INPUT_KEY_KP_8;
        case SDLK_KP_9: return INPUT_KEY_KP_9;
        case SDLK_KP_0: return INPUT_KEY_KP_0;
        case SDLK_KP_DECIMAL: return INPUT_KEY_KP_DECIMAL;

        default: std::unreachable();
    }
}
