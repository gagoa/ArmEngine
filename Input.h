//
//  Input.h
//  game_engine
//
//  Created by Armand Gago on 2/9/26.
//

#ifndef Input_h
#define Input_h

#include <string>
#include <unordered_map>
#include <vector>

#if defined(__linux__)
#include <SDL2/SDL.h>
#else
#include "SDL2/SDL.h"
#endif
#include "third_party/glm/glm.hpp"

enum INPUT_STATE { INPUT_STATE_UP, INPUT_STATE_JUST_BECAME_UP, INPUT_STATE_DOWN, INPUT_STATE_JUST_BECAME_DOWN };

class Input {
public:
    static void Init() {
        for (int code = SDL_SCANCODE_UNKNOWN; code < SDL_NUM_SCANCODES; ++code) {
            keyboard_states[static_cast<SDL_Scancode>(code)] = INPUT_STATE_UP;
        }
        InitKeycodeMap();
    }

    static void ProcessEvent(const SDL_Event& e) {
        if (e.type == SDL_KEYDOWN) {
            keyboard_states[e.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_DOWN;
            just_became_down_scancodes.push_back(e.key.keysym.scancode);
        }
        if (e.type == SDL_KEYUP) {
            keyboard_states[e.key.keysym.scancode] = INPUT_STATE_JUST_BECAME_UP;
            just_became_up_scancodes.push_back(e.key.keysym.scancode);
        }
        if (e.type == SDL_MOUSEBUTTONDOWN) {
            int btn = e.button.button;
            mouse_button_states[btn] = INPUT_STATE_JUST_BECAME_DOWN;
            just_became_down_buttons.push_back(btn);
        }
        if (e.type == SDL_MOUSEBUTTONUP) {
            int btn = e.button.button;
            mouse_button_states[btn] = INPUT_STATE_JUST_BECAME_UP;
            just_became_up_buttons.push_back(btn);
        }
        if (e.type == SDL_MOUSEMOTION) {
            mouse_position.x = static_cast<float>(e.motion.x);
            mouse_position.y = static_cast<float>(e.motion.y);
        }
        if (e.type == SDL_MOUSEWHEEL) {
            mouse_scroll_delta += e.wheel.preciseY;
        }
    }

    static void LateUpdate() {
        for (const SDL_Scancode& code : just_became_down_scancodes) {
            keyboard_states[code] = INPUT_STATE_DOWN;
        }
        just_became_down_scancodes.clear();
        for (const SDL_Scancode& code : just_became_up_scancodes) {
            keyboard_states[code] = INPUT_STATE_UP;
        }
        just_became_up_scancodes.clear();

        for (int btn : just_became_down_buttons) {
            mouse_button_states[btn] = INPUT_STATE_DOWN;
        }
        just_became_down_buttons.clear();
        for (int btn : just_became_up_buttons) {
            mouse_button_states[btn] = INPUT_STATE_UP;
        }
        just_became_up_buttons.clear();

        mouse_scroll_delta = 0;
    }

    static bool GetKey(const std::string& keycode) {
        auto it = keycode_to_scancode.find(keycode);
        if (it == keycode_to_scancode.end()) return false;
        INPUT_STATE state = keyboard_states[it->second];
        return state == INPUT_STATE_DOWN || state == INPUT_STATE_JUST_BECAME_DOWN;
    }

    static bool GetKeyDown(const std::string& keycode) {
        auto it = keycode_to_scancode.find(keycode);
        if (it == keycode_to_scancode.end()) return false;
        return keyboard_states[it->second] == INPUT_STATE_JUST_BECAME_DOWN;
    }

    static bool GetKeyUp(const std::string& keycode) {
        auto it = keycode_to_scancode.find(keycode);
        if (it == keycode_to_scancode.end()) return false;
        return keyboard_states[it->second] == INPUT_STATE_JUST_BECAME_UP;
    }

    static glm::vec2 GetMousePosition() { return mouse_position; }

    static bool GetMouseButton(int button) {
        auto it = mouse_button_states.find(button);
        if (it == mouse_button_states.end()) return false;
        return it->second == INPUT_STATE_DOWN || it->second == INPUT_STATE_JUST_BECAME_DOWN;
    }

    static bool GetMouseButtonDown(int button) {
        auto it = mouse_button_states.find(button);
        if (it == mouse_button_states.end()) return false;
        return it->second == INPUT_STATE_JUST_BECAME_DOWN;
    }

    static bool GetMouseButtonUp(int button) {
        auto it = mouse_button_states.find(button);
        if (it == mouse_button_states.end()) return false;
        return it->second == INPUT_STATE_JUST_BECAME_UP;
    }

    static float GetMouseScrollDelta() { return mouse_scroll_delta; }

    static void HideCursor() { SDL_ShowCursor(SDL_DISABLE); }
    static void ShowCursor() { SDL_ShowCursor(SDL_ENABLE); }

private:
    static inline std::unordered_map<SDL_Scancode, INPUT_STATE> keyboard_states;
    static inline std::vector<SDL_Scancode> just_became_down_scancodes;
    static inline std::vector<SDL_Scancode> just_became_up_scancodes;

    static inline std::unordered_map<int, INPUT_STATE> mouse_button_states;
    static inline std::vector<int> just_became_down_buttons;
    static inline std::vector<int> just_became_up_buttons;

    static inline glm::vec2 mouse_position = { 0, 0 };
    static inline float mouse_scroll_delta = 0;

    static inline std::unordered_map<std::string, SDL_Scancode> keycode_to_scancode;

    static void InitKeycodeMap() {
        keycode_to_scancode["up"] = SDL_SCANCODE_UP;
        keycode_to_scancode["down"] = SDL_SCANCODE_DOWN;
        keycode_to_scancode["right"] = SDL_SCANCODE_RIGHT;
        keycode_to_scancode["left"] = SDL_SCANCODE_LEFT;
        keycode_to_scancode["escape"] = SDL_SCANCODE_ESCAPE;
        keycode_to_scancode["lshift"] = SDL_SCANCODE_LSHIFT;
        keycode_to_scancode["rshift"] = SDL_SCANCODE_RSHIFT;
        keycode_to_scancode["lctrl"] = SDL_SCANCODE_LCTRL;
        keycode_to_scancode["rctrl"] = SDL_SCANCODE_RCTRL;
        keycode_to_scancode["lalt"] = SDL_SCANCODE_LALT;
        keycode_to_scancode["ralt"] = SDL_SCANCODE_RALT;
        keycode_to_scancode["tab"] = SDL_SCANCODE_TAB;
        keycode_to_scancode["return"] = SDL_SCANCODE_RETURN;
        keycode_to_scancode["enter"] = SDL_SCANCODE_RETURN;
        keycode_to_scancode["backspace"] = SDL_SCANCODE_BACKSPACE;
        keycode_to_scancode["delete"] = SDL_SCANCODE_DELETE;
        keycode_to_scancode["insert"] = SDL_SCANCODE_INSERT;
        keycode_to_scancode["space"] = SDL_SCANCODE_SPACE;
        keycode_to_scancode["home"] = SDL_SCANCODE_HOME;
        keycode_to_scancode["end"] = SDL_SCANCODE_END;
        keycode_to_scancode["pageup"] = SDL_SCANCODE_PAGEUP;
        keycode_to_scancode["pagedown"] = SDL_SCANCODE_PAGEDOWN;
        keycode_to_scancode["capslock"] = SDL_SCANCODE_CAPSLOCK;

        keycode_to_scancode["a"] = SDL_SCANCODE_A;
        keycode_to_scancode["b"] = SDL_SCANCODE_B;
        keycode_to_scancode["c"] = SDL_SCANCODE_C;
        keycode_to_scancode["d"] = SDL_SCANCODE_D;
        keycode_to_scancode["e"] = SDL_SCANCODE_E;
        keycode_to_scancode["f"] = SDL_SCANCODE_F;
        keycode_to_scancode["g"] = SDL_SCANCODE_G;
        keycode_to_scancode["h"] = SDL_SCANCODE_H;
        keycode_to_scancode["i"] = SDL_SCANCODE_I;
        keycode_to_scancode["j"] = SDL_SCANCODE_J;
        keycode_to_scancode["k"] = SDL_SCANCODE_K;
        keycode_to_scancode["l"] = SDL_SCANCODE_L;
        keycode_to_scancode["m"] = SDL_SCANCODE_M;
        keycode_to_scancode["n"] = SDL_SCANCODE_N;
        keycode_to_scancode["o"] = SDL_SCANCODE_O;
        keycode_to_scancode["p"] = SDL_SCANCODE_P;
        keycode_to_scancode["q"] = SDL_SCANCODE_Q;
        keycode_to_scancode["r"] = SDL_SCANCODE_R;
        keycode_to_scancode["s"] = SDL_SCANCODE_S;
        keycode_to_scancode["t"] = SDL_SCANCODE_T;
        keycode_to_scancode["u"] = SDL_SCANCODE_U;
        keycode_to_scancode["v"] = SDL_SCANCODE_V;
        keycode_to_scancode["w"] = SDL_SCANCODE_W;
        keycode_to_scancode["x"] = SDL_SCANCODE_X;
        keycode_to_scancode["y"] = SDL_SCANCODE_Y;
        keycode_to_scancode["z"] = SDL_SCANCODE_Z;

        keycode_to_scancode["0"] = SDL_SCANCODE_0;
        keycode_to_scancode["1"] = SDL_SCANCODE_1;
        keycode_to_scancode["2"] = SDL_SCANCODE_2;
        keycode_to_scancode["3"] = SDL_SCANCODE_3;
        keycode_to_scancode["4"] = SDL_SCANCODE_4;
        keycode_to_scancode["5"] = SDL_SCANCODE_5;
        keycode_to_scancode["6"] = SDL_SCANCODE_6;
        keycode_to_scancode["7"] = SDL_SCANCODE_7;
        keycode_to_scancode["8"] = SDL_SCANCODE_8;
        keycode_to_scancode["9"] = SDL_SCANCODE_9;

        keycode_to_scancode["/"] = SDL_SCANCODE_SLASH;
        keycode_to_scancode[";"] = SDL_SCANCODE_SEMICOLON;
        keycode_to_scancode["="] = SDL_SCANCODE_EQUALS;
        keycode_to_scancode["-"] = SDL_SCANCODE_MINUS;
        keycode_to_scancode["."] = SDL_SCANCODE_PERIOD;
        keycode_to_scancode[","] = SDL_SCANCODE_COMMA;
        keycode_to_scancode["["] = SDL_SCANCODE_LEFTBRACKET;
        keycode_to_scancode["]"] = SDL_SCANCODE_RIGHTBRACKET;
        keycode_to_scancode["\\"] = SDL_SCANCODE_BACKSLASH;
        keycode_to_scancode["'"] = SDL_SCANCODE_APOSTROPHE;
        keycode_to_scancode["`"] = SDL_SCANCODE_GRAVE;

        keycode_to_scancode["f1"] = SDL_SCANCODE_F1;
        keycode_to_scancode["f2"] = SDL_SCANCODE_F2;
        keycode_to_scancode["f3"] = SDL_SCANCODE_F3;
        keycode_to_scancode["f4"] = SDL_SCANCODE_F4;
        keycode_to_scancode["f5"] = SDL_SCANCODE_F5;
        keycode_to_scancode["f6"] = SDL_SCANCODE_F6;
        keycode_to_scancode["f7"] = SDL_SCANCODE_F7;
        keycode_to_scancode["f8"] = SDL_SCANCODE_F8;
    }
};

#endif /* Input_h */
