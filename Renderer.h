//
//  Renderer.h
//  game_engine
//
//  Created by Armand Gago on 2/2/26.
//

#ifndef Renderer_h
#define Renderer_h

#if defined(__linux__)
#include <SDL2/SDL.h>
#else
#include "SDL2/SDL.h"
#endif
#include "Helper.h"
#include "third_party/glm/glm.hpp"

class Renderer {
public:
    Helper helper;
    SDL_Window* window;
    SDL_Renderer* renderer;
    uint8_t clear_color_r;
    uint8_t clear_color_g;
    uint8_t clear_color_b;

    inline static glm::vec2 camera_position = glm::vec2(0.0f, 0.0f);
    inline static glm::ivec2 camera_dimensions = glm::ivec2(640, 360);
    inline static float camera_zoom = 1.0f;
    inline static SDL_Renderer* sdl_renderer = nullptr;

    static glm::vec2 GetCameraPosition() { return camera_position; }
    static glm::ivec2 GetCameraDimensions() { return camera_dimensions; }
    static float GetCameraZoomFactor() { return camera_zoom; }
    static SDL_Renderer* GetSDLRenderer() { return sdl_renderer; }

    Renderer(uint8_t r, uint8_t g, uint8_t b)
        : window(nullptr)
        , renderer(nullptr)
        , clear_color_r(r)
        , clear_color_g(g)
        , clear_color_b(b) {}

    void initializeWindow(const char* game_title, int windowX, int windowY) {
        window = helper.SDL_CreateWindow(game_title, 100, 100, windowX, windowY, SDL_WINDOW_SHOWN);
        camera_dimensions = glm::ivec2(windowX, windowY);
    }

    void initializeRenderer() {
        renderer = helper.SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
        sdl_renderer = renderer;
    }

    void setRenderClear() {
        SDL_SetRenderDrawColor(renderer, clear_color_r, clear_color_g, clear_color_b, 255);
        SDL_RenderClear(renderer);
    }
};

#endif /* Renderer_h */
