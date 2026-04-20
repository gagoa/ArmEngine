//
//  TextDB.h
//  game_engine
//
//  Created by Armand Gago on 2/3/26.
//

#ifndef TextDB_h
#define TextDB_h

#include <iostream>
#include <queue>
#include <string>
#include <unordered_map>

#include "Helper.h"
#if defined(__linux__)
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#else
#include "SDL2/SDL.h"
#include "SDL2_ttf/SDL_ttf.h"
#endif

struct TextDrawRequest {
    std::string content;
    int x;
    int y;
    std::string font_name;
    int font_size;
    int r;
    int g;
    int b;
    int a;
};

class TextDB {
public:
    TextDB() {
        if (TTF_Init() < 0) {
            std::cout << "error: TTF_Init failed";
            exit(0);
        }
    }

    ~TextDB() {
        for (auto& [name, sizeMap] : fontCache) {
            for (auto& [size, font] : sizeMap) {
                if (font) TTF_CloseFont(font);
            }
        }
    }

    void QueueDraw(const std::string& content, float x, float y, const std::string& font_name, float font_size, float r,
                   float g, float b, float a) {
        TextDrawRequest req;
        req.content = content;
        req.x = static_cast<int>(x);
        req.y = static_cast<int>(y);
        req.font_name = font_name;
        req.font_size = static_cast<int>(font_size);
        req.r = static_cast<int>(r);
        req.g = static_cast<int>(g);
        req.b = static_cast<int>(b);
        req.a = static_cast<int>(a);
        draw_requests.push(req);
    }

    void RenderAllText(SDL_Renderer* renderer) {
        while (!draw_requests.empty()) {
            const TextDrawRequest& req = draw_requests.front();

            TTF_Font* font = getFont(req.font_name, req.font_size);
            if (font) {
                SDL_Color color
                  = { static_cast<Uint8>(req.r), static_cast<Uint8>(req.g), static_cast<Uint8>(req.b), 255 };

                SDL_Surface* solid_surface = TTF_RenderText_Solid(font, req.content.c_str(), color);
                if (solid_surface) {
                    SDL_Surface* surface = SDL_ConvertSurfaceFormat(solid_surface, SDL_PIXELFORMAT_ARGB8888, 0);
                    SDL_FreeSurface(solid_surface);
                    if (surface) {
                        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                        if (texture) {
                            float tex_w = 0.0f;
                            float tex_h = 0.0f;
                            Helper::SDL_QueryTexture(texture, &tex_w, &tex_h);

                            SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
                            SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(req.a));

                            SDL_FRect dstrect = { static_cast<float>(req.x), static_cast<float>(req.y), tex_w, tex_h };

                            Helper::SDL_RenderCopyEx(0, "", renderer, texture, nullptr, &dstrect, 0, nullptr,
                                                     SDL_FLIP_NONE);

                            SDL_DestroyTexture(texture);
                        }
                        SDL_FreeSurface(surface);
                    }
                }
            }

            draw_requests.pop();
        }
    }

private:
    TTF_Font* getFont(const std::string& fontName, int fontSize) {
        auto& sizeMap = fontCache[fontName];
        auto it = sizeMap.find(fontSize);
        if (it != sizeMap.end()) return it->second;

        std::string path = "resources/fonts/" + fontName + ".ttf";
        TTF_Font* font = TTF_OpenFont(path.c_str(), fontSize);
        sizeMap[fontSize] = font;
        return font;
    }

    std::unordered_map<std::string, std::unordered_map<int, TTF_Font*>> fontCache;
    std::queue<TextDrawRequest> draw_requests;
};

#endif /* TextDB_h */
