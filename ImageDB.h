//
//  ImageDB.h
//  game_engine
//
//  Created by Armand Gago on 2/3/26.
//

#ifndef ImageDB_h
#define ImageDB_h

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Renderer.h"
#if defined(__linux__)
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#else
#include "SDL2/SDL.h"
#include "SDL_image/SDL_image.h"
#endif
#include "Helper.h"
#include "third_party/glm/glm.hpp"

struct ImageDrawRequest {
    std::string image_name;
    float x;
    float y;
    int rotation_degrees;
    float scale_x;
    float scale_y;
    float pivot_x;
    float pivot_y;
    int r;
    int g;
    int b;
    int a;
    int sorting_order;
};

struct PixelDrawRequest {
    int x;
    int y;
    int r;
    int g;
    int b;
    int a;
};

static bool CompareImageRequests(const ImageDrawRequest& a, const ImageDrawRequest& b) {
    return a.sorting_order < b.sorting_order;
}

class ImageDB {
public:
    ImageDB() {
        if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
            std::cout << "error: IMG_Init failed: " << IMG_GetError() << "\n";
            exit(0);
        }
    }

    void setRenderer(Renderer& renderer) { rend = &renderer; }

    // Homework 9: procedural 8x8 white RGBA default particle (no resources/images PNG).
    void CreateDefaultParticleTextureWithName(const std::string& name) {
        if (texture_bank.find(name) != texture_bank.end()) return;

        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 8, 8, 32, SDL_PIXELFORMAT_RGBA8888);
        if (!surface) return;

        Uint32 white_color = SDL_MapRGBA(surface->format, 255, 255, 255, 255);
        SDL_FillRect(surface, nullptr, white_color);

        SDL_Renderer* sdl_renderer = Renderer::GetSDLRenderer();
        if (!sdl_renderer) {
            SDL_FreeSurface(surface);
            return;
        }

        SDL_Texture* texture = SDL_CreateTextureFromSurface(sdl_renderer, surface);
        SDL_FreeSurface(surface);

        if (texture) {
            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
            texture_bank[name] = texture;
        }
    }

    SDL_Texture* getOrLoadTexture(const std::string& imageName) {
        if (imageName.empty()) {
            CreateDefaultParticleTextureWithName("");
            auto it = texture_bank.find("");
            if (it != texture_bank.end()) return it->second;
            return nullptr;
        }
        auto it = texture_bank.find(imageName);
        if (it != texture_bank.end()) return it->second;
        if (!rend || !rend->renderer) return nullptr;
        std::string path = "resources/images/" + imageName + ".png";
        SDL_Texture* tex = IMG_LoadTexture(rend->renderer, path.c_str());
        if (tex) texture_bank[imageName] = tex;
        return tex;
    }

    void QueueSceneDraw(const ImageDrawRequest& req) { scene_image_queue.push_back(req); }

    void QueueUIDraw(const ImageDrawRequest& req) { ui_image_queue.push_back(req); }

    void QueuePixelDraw(const PixelDrawRequest& req) { pixel_queue.push_back(req); }

    void RenderSceneImages() {
        std::stable_sort(scene_image_queue.begin(), scene_image_queue.end(), CompareImageRequests);

        SDL_Renderer* sdl_renderer = Renderer::GetSDLRenderer();
        float zoom_factor = Renderer::GetCameraZoomFactor();
        SDL_RenderSetScale(sdl_renderer, zoom_factor, zoom_factor);

        for (auto& request : scene_image_queue) {
            const int pixels_per_meter = 100;
            glm::vec2 final_rendering_position = glm::vec2(request.x, request.y) - Renderer::GetCameraPosition();

            SDL_Texture* tex = getOrLoadTexture(request.image_name);
            if (!tex) continue;

            float tex_w = 0.0f;
            float tex_h = 0.0f;
            Helper::SDL_QueryTexture(tex, &tex_w, &tex_h);

            int flip_mode = SDL_FLIP_NONE;
            if (request.scale_x < 0) flip_mode |= SDL_FLIP_HORIZONTAL;
            if (request.scale_y < 0) flip_mode |= SDL_FLIP_VERTICAL;

            float x_scale = glm::abs(request.scale_x);
            float y_scale = glm::abs(request.scale_y);

            tex_w *= x_scale;
            tex_h *= y_scale;

            SDL_FPoint pivot_point = { request.pivot_x * tex_w, request.pivot_y * tex_h };

            glm::ivec2 cam_dimensions = Renderer::GetCameraDimensions();

            SDL_FRect tex_rect;
            tex_rect.w = tex_w;
            tex_rect.h = tex_h;
            tex_rect.x = final_rendering_position.x * pixels_per_meter + cam_dimensions.x * 0.5f * (1.0f / zoom_factor)
                       - pivot_point.x;
            tex_rect.y = final_rendering_position.y * pixels_per_meter + cam_dimensions.y * 0.5f * (1.0f / zoom_factor)
                       - pivot_point.y;

            SDL_SetTextureColorMod(tex, request.r, request.g, request.b);
            SDL_SetTextureAlphaMod(tex, request.a);

            Helper::SDL_RenderCopyEx(0, "", sdl_renderer, tex, nullptr, &tex_rect, request.rotation_degrees,
                                     &pivot_point, static_cast<SDL_RendererFlip>(flip_mode));

            SDL_RenderSetScale(sdl_renderer, zoom_factor, zoom_factor);

            SDL_SetTextureColorMod(tex, 255, 255, 255);
            SDL_SetTextureAlphaMod(tex, 255);
        }

        SDL_RenderSetScale(sdl_renderer, 1, 1);
        scene_image_queue.clear();
    }

    void RenderUIImages() {
        std::stable_sort(ui_image_queue.begin(), ui_image_queue.end(), CompareImageRequests);

        SDL_Renderer* sdl_renderer = Renderer::GetSDLRenderer();

        for (auto& request : ui_image_queue) {
            SDL_Texture* tex = getOrLoadTexture(request.image_name);
            if (!tex) continue;

            float tex_w = 0.0f;
            float tex_h = 0.0f;
            Helper::SDL_QueryTexture(tex, &tex_w, &tex_h);

            float x_scale = glm::abs(request.scale_x);
            float y_scale = glm::abs(request.scale_y);
            tex_w *= x_scale;
            tex_h *= y_scale;

            SDL_FRect tex_rect = { request.x, request.y, tex_w, tex_h };

            SDL_SetTextureColorMod(tex, request.r, request.g, request.b);
            SDL_SetTextureAlphaMod(tex, request.a);

            Helper::SDL_RenderCopy(sdl_renderer, tex, nullptr, &tex_rect);

            SDL_SetTextureColorMod(tex, 255, 255, 255);
            SDL_SetTextureAlphaMod(tex, 255);
        }

        ui_image_queue.clear();
    }

    void RenderPixels() {
        if (pixel_queue.empty()) return;

        SDL_Renderer* sdl_renderer = Renderer::GetSDLRenderer();
        SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_BLEND);

        for (auto& pixel : pixel_queue) {
            SDL_SetRenderDrawColor(sdl_renderer, pixel.r, pixel.g, pixel.b, pixel.a);
            SDL_RenderDrawPoint(sdl_renderer, pixel.x, pixel.y);
        }

        SDL_SetRenderDrawBlendMode(sdl_renderer, SDL_BLENDMODE_NONE);
        pixel_queue.clear();
    }

    Renderer* rend = nullptr;

private:
    std::unordered_map<std::string, SDL_Texture*> texture_bank;
    std::vector<ImageDrawRequest> scene_image_queue;
    std::vector<ImageDrawRequest> ui_image_queue;
    std::vector<PixelDrawRequest> pixel_queue;
};

#endif /* ImageDB_h */
