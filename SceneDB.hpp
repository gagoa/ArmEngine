//
//  SceneDB.hpp
//  game_engine
//
//  Created by Armand Gago on 1/26/26.
//


#ifndef SceneDB_hpp
#define SceneDB_hpp

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <stdio.h>

#include "EngineUtils.hpp"
#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"
#include "rapidjson/document.h"

class ComponentDB;
class TemplateDB;

class SceneDB {
public:
    struct Actor {
        int id;
        std::string name;
        std::map<std::string, luabridge::LuaRef*> components;
        std::map<std::string, std::string> component_types;
        std::set<std::string> removed_components;
        bool destroyed = false;
        bool dont_destroy = false;

        std::string GetName() const { return name; }
        int GetID() const { return id; }
        luabridge::LuaRef GetComponentByKey(const std::string& key);
        luabridge::LuaRef GetComponent(const std::string& type_name);
        luabridge::LuaRef GetComponents(const std::string& type_name);
        luabridge::LuaRef AddComponent(const std::string& type_name);
        void RemoveComponent(luabridge::LuaRef component_ref);

        void InjectConvenienceReferences();
    };

    struct PendingComponent {
        Actor* actor;
        std::string key;
        std::string type;
        luabridge::LuaRef* instance;
    };

    int nextActorId = 0;

    std::vector<Actor*> actors;
    std::vector<Actor*> actors_to_start;
    std::vector<std::pair<Actor*, std::string>> components_to_start;
    std::vector<PendingComponent> pending_add_components;
    std::vector<Actor*> actors_to_destroy;

    void loadScene(rapidjson::Document& actorsDoc, const TemplateDB& templateDB);
    void setLoadContext(const std::string& scenesPath, const TemplateDB* db);
    void setComponentDB(ComponentDB* cdb);
    void loadSceneFile(const std::string& sceneFilename);
    void ProcessPendingSceneLoad();
    void ProcessEndOfFrame();

    std::string current_scene_name;
    std::string pending_scene_name;
    bool scene_load_pending = false;
    bool pathExists(const std::filesystem::path& p, std::filesystem::file_status s = std::filesystem::file_status {});

    EngineUtils engineUtil;

    std::string scenesBasePath;
    const TemplateDB* templateDBRef = nullptr;
    ComponentDB* componentDB = nullptr;

private:
    Actor* makeActor(const rapidjson::Value* templateVal, const rapidjson::Value& actorVal);
    void InjectProperties(luabridge::LuaRef& instance, const rapidjson::Value& componentJson);
};

#endif /* SceneDB_hpp */
