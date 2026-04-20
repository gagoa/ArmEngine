//
//  SceneDB.cpp
//  game_engine
//
//  Created by Armand Gago on 1/26/26.
//

#include "SceneDB.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

#include "ComponentDB.hpp"
#include "ParticleSystem.hpp"
#include "Rigidbody.h"
#include "TemplateDB.hpp"
#include "rapidjson/document.h"

static ComponentDB* s_componentDB = nullptr;
static SceneDB* s_sceneDB_instance = nullptr;
static int s_add_component_counter = 0;

bool SceneDB::pathExists(const fs::path& p, fs::file_status s) {
    if (fs::status_known(s) ? fs::exists(s) : fs::exists(p))
        return true;
    else
        return false;
}

luabridge::LuaRef SceneDB::Actor::GetComponentByKey(const std::string& key) {
    if (removed_components.count(key) > 0) return luabridge::LuaRef(ComponentDB::GetLuaState());
    auto it = components.find(key);
    if (it != components.end()) {
        return *it->second;
    }
    return luabridge::LuaRef(ComponentDB::GetLuaState());
}

luabridge::LuaRef SceneDB::Actor::GetComponent(const std::string& type_name) {
    for (auto& [key, type] : component_types) {
        if (type == type_name && removed_components.count(key) == 0) {
            return *components[key];
        }
    }
    return luabridge::LuaRef(ComponentDB::GetLuaState());
}

luabridge::LuaRef SceneDB::Actor::GetComponents(const std::string& type_name) {
    lua_State* L = ComponentDB::GetLuaState();
    luabridge::LuaRef table = luabridge::newTable(L);
    int index = 1;
    for (auto& [key, type] : component_types) {
        if (type == type_name && removed_components.count(key) == 0) {
            table[index++] = *components[key];
        }
    }
    return table;
}

luabridge::LuaRef SceneDB::Actor::AddComponent(const std::string& type_name) {
    std::string key = "r" + std::to_string(s_add_component_counter);
    s_add_component_counter++;

    luabridge::LuaRef* instance = s_componentDB->CreateComponentInstance(type_name, key);
    if (instance->isTable()) {
        (*instance)["actor"] = this;
    } else if (instance->isUserdata()) {
        if (type_name == "Rigidbody") {
            Rigidbody* rb = instance->cast<Rigidbody*>();
            if (rb) rb->actor = this;
        } else if (type_name == "ParticleSystem") {
            ParticleSystem* ps = instance->cast<ParticleSystem*>();
            if (ps) ps->actor = this;
        }
    }

    s_sceneDB_instance->pending_add_components.push_back({ this, key, type_name, instance });

    return *instance;
}

void SceneDB::Actor::RemoveComponent(luabridge::LuaRef component_ref) {
    luabridge::LuaRef key_ref = component_ref["key"];
    if (key_ref.isString()) {
        std::string key = key_ref.cast<std::string>();
        component_ref["enabled"] = false;
        removed_components.insert(key);
    }
}

void SceneDB::Actor::InjectConvenienceReferences() {
    for (auto& [key, component] : components) {
        if (component->isTable()) {
            (*component)["actor"] = this;
        } else if (component->isUserdata()) {
            const std::string& type = component_types.count(key) ? component_types.at(key) : "";
            if (type == "Rigidbody") {
                Rigidbody* rb = component->cast<Rigidbody*>();
                if (rb) rb->actor = this;
            } else if (type == "ParticleSystem") {
                ParticleSystem* ps = component->cast<ParticleSystem*>();
                if (ps) ps->actor = this;
            }
        }
    }
}

void SceneDB::setComponentDB(ComponentDB* cdb) {
    componentDB = cdb;
    s_componentDB = cdb;
    s_sceneDB_instance = this;
}

void SceneDB::InjectProperties(luabridge::LuaRef& instance, const rapidjson::Value& componentJson) {
    for (auto it = componentJson.MemberBegin(); it != componentJson.MemberEnd(); ++it) {
        std::string propName = it->name.GetString();
        if (propName == "type") continue;

        if (it->value.IsString()) {
            instance[propName] = std::string(it->value.GetString());
        } else if (it->value.IsInt()) {
            instance[propName] = it->value.GetInt();
        } else if (it->value.IsDouble()) {
            instance[propName] = it->value.GetDouble();
        } else if (it->value.IsBool()) {
            instance[propName] = it->value.GetBool();
        }
    }
}

SceneDB::Actor* SceneDB::makeActor(const rapidjson::Value* templateVal, const rapidjson::Value& actorVal) {
    auto getStr = [&](const char* key, const char* def) -> std::string {
        if (actorVal.HasMember(key) && actorVal[key].IsString()) return actorVal[key].GetString();
        if (templateVal && templateVal->HasMember(key) && (*templateVal)[key].IsString())
            return (*templateVal)[key].GetString();
        return def;
    };

    Actor* actor = new Actor();
    actor->id = nextActorId++;
    actor->name = getStr("name", "");

    if (templateVal && templateVal->HasMember("components") && (*templateVal)["components"].IsObject()) {
        const auto& comps = (*templateVal)["components"].GetObject();
        for (auto it = comps.MemberBegin(); it != comps.MemberEnd(); ++it) {
            std::string key = it->name.GetString();
            if (!it->value.HasMember("type") || !it->value["type"].IsString()) continue;
            std::string type = it->value["type"].GetString();

            luabridge::LuaRef* instance = componentDB->CreateComponentInstance(type, key);
            InjectProperties(*instance, it->value);
            actor->components[key] = instance;
            actor->component_types[key] = type;
        }
    }

    if (actorVal.HasMember("components") && actorVal["components"].IsObject()) {
        const auto& comps = actorVal["components"].GetObject();
        for (auto it = comps.MemberBegin(); it != comps.MemberEnd(); ++it) {
            std::string key = it->name.GetString();

            if (actor->components.count(key) > 0) {
                InjectProperties(*actor->components[key], it->value);
            } else {
                if (!it->value.HasMember("type") || !it->value["type"].IsString()) continue;
                std::string type = it->value["type"].GetString();

                luabridge::LuaRef* instance = componentDB->CreateComponentInstance(type, key);
                InjectProperties(*instance, it->value);
                actor->components[key] = instance;
                actor->component_types[key] = type;
            }
        }
    }

    actor->InjectConvenienceReferences();

    return actor;
}

void SceneDB::setLoadContext(const std::string& scenesPath, const TemplateDB* db) {
    scenesBasePath = scenesPath;
    templateDBRef = db;
}

void SceneDB::loadScene(rapidjson::Document& actorsDoc, const TemplateDB& templateDB) {
    if (actorsDoc.HasMember("actors") && actorsDoc["actors"].IsArray()) {
        const auto& actorsJson = actorsDoc["actors"].GetArray();

        for (const rapidjson::Value& a : actorsJson) {
            const rapidjson::Value* templateVal = nullptr;
            if (a.HasMember("template") && a["template"].IsString()) {
                std::string templateName = a["template"].GetString();
                templateVal = templateDB.getTemplate(templateName);
                if (!templateVal) {
                    std::cout << "error: template " << templateName << " is missing";
                    exit(0);
                }
            }
            Actor* actor = makeActor(templateVal, a);
            actors.push_back(actor);
            if (!actor->components.empty()) {
                actors_to_start.push_back(actor);
            }
        }
    }
}

void SceneDB::ProcessEndOfFrame() {
    for (auto& pending : pending_add_components) {
        pending.actor->components[pending.key] = pending.instance;
        pending.actor->component_types[pending.key] = pending.type;
        components_to_start.push_back({ pending.actor, pending.key });
    }
    pending_add_components.clear();

    for (auto* actor : actors) {
        for (const auto& key : actor->removed_components) {
            auto it = actor->components.find(key);
            if (it != actor->components.end()) {
                delete it->second;
                actor->components.erase(it);
            }
            actor->component_types.erase(key);
        }
        actor->removed_components.clear();
    }

    actors_to_start.erase(
      std::remove_if(actors_to_start.begin(), actors_to_start.end(), [](Actor* a) { return a->destroyed; }),
      actors_to_start.end());

    components_to_start.erase(
      std::remove_if(components_to_start.begin(), components_to_start.end(),
                     [](const std::pair<Actor*, std::string>& p) { return p.first->destroyed; }),
      components_to_start.end());

    actors.erase(std::remove_if(actors.begin(), actors.end(),
                                [](Actor* a) {
                                    if (a->destroyed) {
                                        delete a;
                                        return true;
                                    }
                                    return false;
                                }),
                 actors.end());
    actors_to_destroy.clear();
}

void SceneDB::ProcessPendingSceneLoad() {
    if (!scene_load_pending) return;
    scene_load_pending = false;

    current_scene_name = pending_scene_name;

    std::vector<Actor*> kept_actors;
    for (auto* actor : actors) {
        if (actor->dont_destroy) {
            kept_actors.push_back(actor);
        } else {
            delete actor;
        }
    }

    actors.clear();
    actors_to_start.clear();
    components_to_start.clear();
    pending_add_components.clear();
    actors_to_destroy.clear();

    actors = kept_actors;

    const fs::path scenePath = scenesBasePath + "/" + pending_scene_name + ".scene";
    if (!pathExists(scenePath)) {
        std::cout << "error: scene " << pending_scene_name << " is missing";
        exit(0);
    }

    rapidjson::Document doc;
    engineUtil.ReadJsonFile(scenePath.string(), doc);
    loadScene(doc, *templateDBRef);
}

void SceneDB::loadSceneFile(const std::string& sceneFilename) {
    for (auto* a : actors) delete a;
    actors.clear();
    actors_to_start.clear();
    components_to_start.clear();
    pending_add_components.clear();
    actors_to_destroy.clear();
    nextActorId = 0;

    const fs::path scenePath = scenesBasePath + "/" + sceneFilename + ".scene";

    if (!pathExists(scenePath)) {
        std::cout << "error: scene " << sceneFilename << " is missing";
        exit(0);
    }

    rapidjson::Document doc;
    engineUtil.ReadJsonFile(scenePath.string(), doc);
    loadScene(doc, *templateDBRef);
}
