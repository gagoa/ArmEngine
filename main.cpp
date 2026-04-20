//
//  main.cpp
//  game_engine
//
//  Created by Armand Gago on 1/21/26.
//

#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "AudioDB.h"
#include "ComponentDB.hpp"
#include "Engine.hpp"
#include "EngineUtils.hpp"
#include "Helper.h"
#include "ImageDB.h"
#include "Renderer.h"
#if defined(__linux__)
#include <SDL2/SDL.h>
#else
#include "SDL2/SDL.h"
#endif
#include "SceneDB.hpp"
#include "TemplateDB.hpp"
#include "TextDB.h"
#include "rapidjson/document.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    (void) argc;
    (void) argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "error: SDL_Init failed: " << SDL_GetError();
        return 1;
    }

    SceneDB sceneDB;
    ImageDB imageDB;
    TextDB textDB;
    AudioDB audioDB;
    audioDB.initAudio();

    ComponentDB componentDB;
    componentDB.Initialize();
    componentDB.SetSceneDB(&sceneDB);
    componentDB.SetTextDB(&textDB);
    componentDB.SetAudioDB(&audioDB);
    componentDB.SetImageDB(&imageDB);
    componentDB.InitializeActorBindings();

    const fs::path resources { "resources" };
    const fs::path gameConfig { resources / "game.config" };
    const fs::path renderingConfig { resources / "rendering.config" };
    const fs::path scenes { resources / "scenes" };

    if (!sceneDB.pathExists(resources)) {
        std::cout << "error: resources/ missing";
        exit(0);
    }
    if (!sceneDB.pathExists(gameConfig)) {
        std::cout << "error: resources/game.config missing";
        exit(0);
    }

    rapidjson::Document gameMessages;
    EngineUtils::ReadJsonFile(gameConfig.string(), gameMessages);

    std::string initial_scene;
    if (gameMessages.HasMember("initial_scene") && gameMessages["initial_scene"].IsString()) {
        initial_scene = gameMessages["initial_scene"].GetString();
    } else {
        std::cout << "error: initial_scene unspecified";
        exit(0);
    }

    const char* game_title = "";
    if (gameMessages.HasMember("game_title")) {
        game_title = gameMessages["game_title"].GetString();
    }

    int windowX = 640;
    int windowY = 360;
    uint8_t clear_color_r = 255;
    uint8_t clear_color_g = 255;
    uint8_t clear_color_b = 255;

    if (sceneDB.pathExists(renderingConfig)) {
        rapidjson::Document renderings;
        EngineUtils::ReadJsonFile(renderingConfig.string(), renderings);
        if (renderings.HasMember("x_resolution") && renderings["x_resolution"].IsInt()) {
            windowX = renderings["x_resolution"].GetInt();
        }
        if (renderings.HasMember("y_resolution") && renderings["y_resolution"].IsInt()) {
            windowY = renderings["y_resolution"].GetInt();
        }
        if (renderings.HasMember("clear_color_r")) {
            clear_color_r = renderings["clear_color_r"].GetUint();
        }
        if (renderings.HasMember("clear_color_g")) {
            clear_color_g = renderings["clear_color_g"].GetUint();
        }
        if (renderings.HasMember("clear_color_b")) {
            clear_color_b = renderings["clear_color_b"].GetUint();
        }
    }

    Renderer rend(clear_color_r, clear_color_g, clear_color_b);
    rend.initializeWindow(game_title, windowX, windowY);
    rend.initializeRenderer();
    rend.setRenderClear();

    imageDB.setRenderer(rend);

    const fs::path initialScenePath = scenes / (initial_scene + ".scene");
    if (!sceneDB.pathExists(initialScenePath)) {
        std::cout << "error: scene " << initial_scene << " is missing";
        exit(0);
    }

    TemplateDB templateDB;
    templateDB.loadTemplates((resources / "actor_templates").string());

    sceneDB.setLoadContext(scenes.string(), &templateDB);
    sceneDB.setComponentDB(&componentDB);

    rapidjson::Document actorsDocument;
    EngineUtils::ReadJsonFile(initialScenePath.string(), actorsDocument);
    sceneDB.loadScene(actorsDocument, templateDB);
    sceneDB.current_scene_name = initial_scene;

    Engine engine(windowX, windowY, sceneDB, rend, imageDB, textDB, audioDB, componentDB);
    engine.game_loop();

    return 0;
}
