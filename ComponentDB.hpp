//
//  ComponentDB.h
//  game_engine
//
//  Created by Armand Gago
//

#ifndef ComponentDB_h
#define ComponentDB_h

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
 
#include "lua.hpp"
#include "LuaBridge/LuaBridge.h"

class AudioDB;
class ImageDB;
class SceneDB;
class TextDB;

class ComponentDB {
public:
    lua_State* lua_state = nullptr;

    static lua_State* GetLuaState();

    void Initialize();
    void InitializeActorBindings();
    void SetSceneDB(SceneDB* sdb);
    void SetTextDB(TextDB* tdb);
    void SetAudioDB(AudioDB* adb);
    void SetImageDB(ImageDB* idb);

    static void ProcessEventBusPending();
    void EstablishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table);
    void LoadComponentType(const std::string& typeName);
    luabridge::LuaRef* CreateComponentInstance(const std::string& typeName, const std::string& key);

    bool ReloadComponentType(const std::string& typeName);

    void PollHotReloadWatcher();

private:
    std::unordered_map<std::string, luabridge::LuaRef*> component_tables;
    std::unordered_map<std::string, std::filesystem::file_time_type> component_file_mtimes;
    int hot_reload_poll_counter = 0;
};

#endif /* ComponentDB_h */
