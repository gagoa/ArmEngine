//
//  Engine.cpp
//  game_engine
//
//  Created by Armand Gago on 1/21/26 windows autograder
//

#include "Engine.hpp"

#include <algorithm>
#include <iostream>

#include "Collision.h"
#include "ComponentDB.hpp"
#include "Helper.h"
#include "ImageDB.h"
#include "Input.h"
#include "ParticleSystem.hpp"
#include "Renderer.h"
#include "Rigidbody.h"
#if defined(__linux__)
#include <SDL2/SDL.h>
#else
#include "SDL2/SDL.h"
#endif
#include "SceneDB.hpp"
#include "TextDB.h"

static void ReportError(const std::string& actor_name, const luabridge::LuaException& e) {
    std::string error_message = e.what();
    std::replace(error_message.begin(), error_message.end(), '\\', '/');
    std::cout << "\033[31m" << actor_name << " : " << error_message << "\033[0m" << std::endl;
}

static bool IsComponentEnabled(luabridge::LuaRef& component) {
    luabridge::LuaRef enabled = component["enabled"];
    if (enabled.isBool() && !enabled.cast<bool>()) return false;
    return true;
}

static void CallOnDestroy(SceneDB::Actor* actor, const std::string& key, luabridge::LuaRef& component) {
    if (component.isUserdata()) {
        const std::string& type = actor->component_types.count(key) ? actor->component_types.at(key) : "";
        if (type == "Rigidbody") {
            Rigidbody* rb = component.cast<Rigidbody*>();
            if (rb) rb->OnDestroy();
        } else if (type == "ParticleSystem") {
            ParticleSystem* ps = component.cast<ParticleSystem*>();
            if (ps) ps->OnDestroy();
        }
        return;
    }
    luabridge::LuaRef func = component["OnDestroy"];
    if (func.isFunction()) {
        try {
            func(component);
        } catch (luabridge::LuaException& e) {
            ReportError(actor->name, e);
        }
    }
}

static void CallCollisionFunction(SceneDB::Actor* actor, const char* functionName, const Collision& collision) {
    if (actor->destroyed) return;
    for (auto& [key, component] : actor->components) {
        if (actor->removed_components.count(key) > 0) continue;
        if (!IsComponentEnabled(*component)) continue;
        if (component->isUserdata()) continue;

        luabridge::LuaRef func = (*component)[functionName];
        if (func.isFunction()) {
            try {
                func(*component, collision);
            } catch (luabridge::LuaException& e) {
                ReportError(actor->name, e);
            }
        }
    }
}

static void DispatchContactEvent(void* rawA, void* rawB, b2Vec2 point, b2Vec2 relative_velocity, b2Vec2 normal,
                                 bool is_enter, bool is_trigger) {
    auto* actorA = static_cast<SceneDB::Actor*>(rawA);
    auto* actorB = static_cast<SceneDB::Actor*>(rawB);

    const char* funcName;
    if (is_trigger) {
        funcName = is_enter ? "OnTriggerEnter" : "OnTriggerExit";
    } else {
        funcName = is_enter ? "OnCollisionEnter" : "OnCollisionExit";
    }

    Collision collisionA;
    collisionA.other = actorB;
    collisionA.point = point;
    collisionA.relative_velocity = relative_velocity;
    collisionA.normal = normal;
    CallCollisionFunction(actorA, funcName, collisionA);

    Collision collisionB;
    collisionB.other = actorA;
    collisionB.point = point;
    collisionB.relative_velocity = relative_velocity;
    collisionB.normal = normal;
    CallCollisionFunction(actorB, funcName, collisionB);
}

static void CallLifecycle(SceneDB::Actor* actor, const std::string& key, luabridge::LuaRef& component,
                          const char* functionName) {
    if (actor->destroyed) return;
    if (actor->removed_components.count(key) > 0) return;
    if (!IsComponentEnabled(component)) return;

    if (component.isUserdata()) {
        const std::string fn(functionName);
        const std::string& type = actor->component_types.count(key) ? actor->component_types.at(key) : "";
        if (type == "Rigidbody") {
            Rigidbody* rb = component.cast<Rigidbody*>();
            if (rb && fn == "OnStart") rb->OnStart();
        } else if (type == "ParticleSystem") {
            ParticleSystem* ps = component.cast<ParticleSystem*>();
            if (ps) {
                if (fn == "OnStart")
                    ps->OnStart();
                else if (fn == "OnUpdate")
                    ps->OnUpdate();
            }
        }
        return;
    }

    luabridge::LuaRef func = component[functionName];
    if (func.isFunction()) {
        try {
            func(component);
        } catch (luabridge::LuaException& e) {
            ReportError(actor->name, e);
        }
    }
}

Engine::Engine(int windowX, int windowY, SceneDB& sceneDB, Renderer& renderer, ImageDB& imageDB, TextDB& textDB,
               AudioDB& audioDB, ComponentDB& componentDB)
    : sceneDB(sceneDB)
    , rend(renderer)
    , imageDB(imageDB)
    , textDB(textDB)
    , audioDB(audioDB)
    , componentDB(componentDB)
    , windowX(windowX)
    , windowY(windowY) {}

void Engine::game_loop() {
    Input::Init();
    ContactListener::contact_callback = &DispatchContactEvent;

    while (running) {
        SDL_Event next_event;
        SDL_memset(&next_event, 0, sizeof(next_event));
        while (Helper::SDL_PollEvent(&next_event)) {
            Input::ProcessEvent(next_event);
            if (next_event.type == SDL_QUIT) {
                running = false;
                break;
            }
        }

        if (sceneDB.scene_load_pending) {
            for (auto* actor : sceneDB.actors) {
                if (actor->dont_destroy) continue;
                for (auto& [key, component] : actor->components) {
                    CallOnDestroy(actor, key, *component);
                }
            }
        }

        sceneDB.ProcessPendingSceneLoad();

        size_t actor_count = sceneDB.actors.size();

        {
            std::vector<SceneDB::Actor*> starting = std::move(sceneDB.actors_to_start);
            sceneDB.actors_to_start.clear();
            for (auto* actor : starting) {
                for (auto& [key, component] : actor->components) {
                    CallLifecycle(actor, key, *component, "OnStart");
                }
            }
        }

        {
            std::vector<std::pair<SceneDB::Actor*, std::string>> comp_starting = std::move(sceneDB.components_to_start);
            sceneDB.components_to_start.clear();
            for (auto& [actor, key] : comp_starting) {
                auto it = actor->components.find(key);
                if (it != actor->components.end()) {
                    CallLifecycle(actor, key, *it->second, "OnStart");
                }
            }
        }

        for (size_t i = 0; i < actor_count; i++) {
            auto* actor = sceneDB.actors[i];
            for (auto& [key, component] : actor->components) {
                CallLifecycle(actor, key, *component, "OnUpdate");
            }
        }

        for (size_t i = 0; i < actor_count; i++) {
            auto* actor = sceneDB.actors[i];
            for (auto& [key, component] : actor->components) {
                CallLifecycle(actor, key, *component, "OnLateUpdate");
            }
        }

        for (auto* actor : sceneDB.actors) {
            for (const auto& key : actor->removed_components) {
                auto it = actor->components.find(key);
                if (it != actor->components.end()) {
                    CallOnDestroy(actor, key, *it->second);
                }
            }
        }

        for (auto* actor : sceneDB.actors_to_destroy) {
            if (!actor->destroyed) continue;
            for (auto& [key, component] : actor->components) {
                if (actor->removed_components.count(key) > 0) continue;
                CallOnDestroy(actor, key, *component);
            }
        }

        sceneDB.ProcessEndOfFrame();

        ComponentDB::ProcessEventBusPending();

        componentDB.PollHotReloadWatcher();

        if (Rigidbody::world != nullptr) {
            Rigidbody::world->Step(1.0f / 60.0f, 8, 3);
        }

        rend.setRenderClear();
        imageDB.RenderSceneImages();
        imageDB.RenderUIImages();
        textDB.RenderAllText(rend.renderer);
        imageDB.RenderPixels();
        Helper::SDL_RenderPresent(rend.renderer);
        Input::LateUpdate();
    }
}
