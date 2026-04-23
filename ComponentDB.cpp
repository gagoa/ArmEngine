//
//  ComponentDB.cpp
//  game_engine
//
//  Created by Armand Gago
//

#include "ComponentDB.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <thread>

#include "AudioDB.h"
#include "Collision.h"
#include "Helper.h"
#include "ImageDB.h"
#include "Input.h"
#include "ParticleSystem.hpp"
#include "Rigidbody.h"
#include "SceneDB.hpp"
#include "TemplateDB.hpp"
#include "TextDB.h"

static lua_State* s_lua_state = nullptr;
static SceneDB* s_scene_db = nullptr;
static TextDB* s_text_db = nullptr;
static AudioDB* s_audio_db = nullptr;
static ImageDB* s_image_db = nullptr;
static ComponentDB* s_component_db_self = nullptr;

static std::map<std::string, std::vector<std::pair<luabridge::LuaRef, luabridge::LuaRef>>> s_event_subscriptions;

struct PendingEventOp {
    std::string event_type;
    luabridge::LuaRef component;
    luabridge::LuaRef function;
    PendingEventOp(const std::string& et, const luabridge::LuaRef& c, const luabridge::LuaRef& f)
        : event_type(et)
        , component(c)
        , function(f) {}
};

static std::vector<PendingEventOp> s_pending_subscribes;
static std::vector<PendingEventOp> s_pending_unsubscribes;

lua_State* ComponentDB::GetLuaState() {
    return s_lua_state;
}

void ComponentDB::SetSceneDB(SceneDB* sdb) {
    s_scene_db = sdb;
}

void ComponentDB::SetTextDB(TextDB* tdb) {
    s_text_db = tdb;
}

void ComponentDB::SetAudioDB(AudioDB* adb) {
    s_audio_db = adb;
}

void ComponentDB::SetImageDB(ImageDB* idb) {
    s_image_db = idb;
    ParticleSystem::BindImageDB(idb);
}

static void CppTextDraw(const std::string& content, float x, float y, const std::string& font_name, float font_size,
                        float r, float g, float b, float a) {
    if (s_text_db != nullptr) {
        s_text_db->QueueDraw(content, x, y, font_name, font_size, r, g, b, a);
    }
}

static void CppSceneLoad(const std::string& scene_name) {
    if (s_scene_db) {
        s_scene_db->pending_scene_name = scene_name;
        s_scene_db->scene_load_pending = true;
    }
}

static std::string CppSceneGetCurrent() {
    if (s_scene_db) {
        return s_scene_db->current_scene_name;
    }
    return "";
}

static void CppSceneDontDestroy(SceneDB::Actor* actor) {
    if (actor) {
        actor->dont_destroy = true;
    }
}

static void CppCameraSetPosition(float x, float y) {
    Renderer::camera_position = glm::vec2(x, y);
}

static float CppCameraGetPositionX() {
    return Renderer::camera_position.x;
}

static float CppCameraGetPositionY() {
    return Renderer::camera_position.y;
}

static void CppCameraSetZoom(float zoom_factor) {
    Renderer::camera_zoom = zoom_factor;
}

static float CppCameraGetZoom() {
    return Renderer::camera_zoom;
}

static void CppImageDraw(const std::string& image_name, float x, float y) {
    if (!s_image_db) return;
    ImageDrawRequest req;
    req.image_name = image_name;
    req.x = x;
    req.y = y;
    req.rotation_degrees = 0;
    req.scale_x = 1.0f;
    req.scale_y = 1.0f;
    req.pivot_x = 0.5f;
    req.pivot_y = 0.5f;
    req.r = 255;
    req.g = 255;
    req.b = 255;
    req.a = 255;
    req.sorting_order = 0;
    s_image_db->QueueSceneDraw(req);
}

static void CppImageDrawEx(const std::string& image_name, float x, float y, float rotation_degrees, float scale_x,
                           float scale_y, float pivot_x, float pivot_y, float r, float g, float b, float a,
                           float sorting_order) {
    if (!s_image_db) return;
    ImageDrawRequest req;
    req.image_name = image_name;
    req.x = x;
    req.y = y;
    req.rotation_degrees = static_cast<int>(rotation_degrees);
    req.scale_x = scale_x;
    req.scale_y = scale_y;
    req.pivot_x = pivot_x;
    req.pivot_y = pivot_y;
    req.r = static_cast<int>(r);
    req.g = static_cast<int>(g);
    req.b = static_cast<int>(b);
    req.a = static_cast<int>(a);
    req.sorting_order = static_cast<int>(sorting_order);
    s_image_db->QueueSceneDraw(req);
}

static void CppImageDrawUI(const std::string& image_name, float x, float y) {
    if (!s_image_db) return;
    ImageDrawRequest req;
    req.image_name = image_name;
    req.x = static_cast<int>(x);
    req.y = static_cast<int>(y);
    req.rotation_degrees = 0;
    req.scale_x = 1.0f;
    req.scale_y = 1.0f;
    req.pivot_x = 0.5f;
    req.pivot_y = 0.5f;
    req.r = 255;
    req.g = 255;
    req.b = 255;
    req.a = 255;
    req.sorting_order = 0;
    s_image_db->QueueUIDraw(req);
}

static void CppImageDrawUIEx(const std::string& image_name, float x, float y, float r, float g, float b, float a,
                             float sorting_order) {
    if (!s_image_db) return;
    ImageDrawRequest req;
    req.image_name = image_name;
    req.x = static_cast<int>(x);
    req.y = static_cast<int>(y);
    req.rotation_degrees = 0;
    req.scale_x = 1.0f;
    req.scale_y = 1.0f;
    req.pivot_x = 0.5f;
    req.pivot_y = 0.5f;
    req.r = static_cast<int>(r);
    req.g = static_cast<int>(g);
    req.b = static_cast<int>(b);
    req.a = static_cast<int>(a);
    req.sorting_order = static_cast<int>(sorting_order);
    s_image_db->QueueUIDraw(req);
}

static void CppImageDrawUIScaled(const std::string& image_name, float x, float y,
                                 float scale_x, float scale_y,
                                 float r, float g, float b, float a,
                                 float sorting_order) {
    if (!s_image_db) return;
    ImageDrawRequest req;
    req.image_name = image_name;
    req.x = x;
    req.y = y;
    req.rotation_degrees = 0;
    req.scale_x = scale_x;
    req.scale_y = scale_y;
    req.pivot_x = 0.5f;
    req.pivot_y = 0.5f;
    req.r = static_cast<int>(r);
    req.g = static_cast<int>(g);
    req.b = static_cast<int>(b);
    req.a = static_cast<int>(a);
    req.sorting_order = static_cast<int>(sorting_order);
    s_image_db->QueueUIDraw(req);
}

static void CppImageDrawPixel(float x, float y, float r, float g, float b, float a) {
    if (!s_image_db) return;
    PixelDrawRequest req;
    req.x = static_cast<int>(x);
    req.y = static_cast<int>(y);
    req.r = static_cast<int>(r);
    req.g = static_cast<int>(g);
    req.b = static_cast<int>(b);
    req.a = static_cast<int>(a);
    s_image_db->QueuePixelDraw(req);
}

static void CppAudioPlay(int channel, const std::string& clip_name, bool does_loop) {
    if (s_audio_db) {
        Mix_Chunk* chunk = s_audio_db->loadAudio(clip_name);
        if (chunk) {
            AudioHelper::Mix_PlayChannel(channel, chunk, does_loop ? -1 : 0);
        }
    }
}

static void CppAudioHalt(int channel) {
    AudioHelper::Mix_HaltChannel(channel);
}

static void CppAudioSetVolume(int channel, float volume) {
    AudioHelper::Mix_Volume(channel, static_cast<int>(volume));
}

static void CppDebugLog(const std::string& message) {
    std::cout << message << std::endl;
}

static void CppAppQuit() {
    exit(0);
}

static void CppAppSleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

static int CppAppGetFrame() {
    return Helper::GetFrameNumber();
}

static void CppAppOpenURL(const std::string& url) {
#if defined(_WIN32)
    std::system(("start " + url).c_str());
#elif defined(__APPLE__)
    std::system(("open " + url).c_str());
#else
    std::system(("xdg-open " + url).c_str());
#endif
}

static luabridge::LuaRef FindActor(const std::string& name) {
    for (auto* actor : s_scene_db->actors) {
        if (!actor->destroyed && actor->name == name) {
            return luabridge::LuaRef(s_lua_state, actor);
        }
    }
    return luabridge::LuaRef(s_lua_state);
}

static luabridge::LuaRef FindAllActors(const std::string& name) {
    luabridge::LuaRef table = luabridge::newTable(s_lua_state);
    int index = 1;
    for (auto* actor : s_scene_db->actors) {
        if (!actor->destroyed && actor->name == name) {
            table[index++] = actor;
        }
    }
    return table;
}

static luabridge::LuaRef InstantiateActor(const std::string& template_name) {
    const rapidjson::Value* templateVal = s_scene_db->templateDBRef->getTemplate(template_name);
    if (!templateVal) {
        std::cout << "error: template " << template_name << " is missing";
        exit(0);
    }

    rapidjson::Document emptyActor;
    emptyActor.SetObject();

    SceneDB::Actor* actor = new SceneDB::Actor();
    actor->id = s_scene_db->nextActorId++;

    if (templateVal->HasMember("name") && (*templateVal)["name"].IsString()) {
        actor->name = (*templateVal)["name"].GetString();
    }

    if (templateVal->HasMember("components") && (*templateVal)["components"].IsObject()) {
        const auto& comps = (*templateVal)["components"].GetObject();
        for (auto it = comps.MemberBegin(); it != comps.MemberEnd(); ++it) {
            std::string key = it->name.GetString();
            if (!it->value.HasMember("type") || !it->value["type"].IsString()) continue;
            std::string type = it->value["type"].GetString();

            luabridge::LuaRef* instance = s_scene_db->componentDB->CreateComponentInstance(type, key);

            for (auto prop = it->value.MemberBegin(); prop != it->value.MemberEnd(); ++prop) {
                std::string propName = prop->name.GetString();
                if (propName == "type") continue;
                if (prop->value.IsString())
                    (*instance)[propName] = std::string(prop->value.GetString());
                else if (prop->value.IsInt())
                    (*instance)[propName] = prop->value.GetInt();
                else if (prop->value.IsDouble())
                    (*instance)[propName] = prop->value.GetDouble();
                else if (prop->value.IsBool())
                    (*instance)[propName] = prop->value.GetBool();
            }

            actor->components[key] = instance;
            actor->component_types[key] = type;
        }
    }

    actor->InjectConvenienceReferences();
    s_scene_db->actors.push_back(actor);
    s_scene_db->actors_to_start.push_back(actor);

    return luabridge::LuaRef(s_lua_state, actor);
}

static void DestroyActor(SceneDB::Actor* actor) {
    if (actor->destroyed) return;
    actor->destroyed = true;
    for (auto& [key, component] : actor->components) {
        (*component)["enabled"] = false;
    }
    s_scene_db->actors_to_destroy.push_back(actor);
}

class RaycastCallbackClosest : public b2RayCastCallback {
public:
    HitResult result;
    bool has_hit = false;

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        void* actor_ptr = reinterpret_cast<void*>(fixture->GetUserData().pointer);
        if (!actor_ptr) return -1;
        if (fixture->GetFilterData().categoryBits == 0x0004) return -1;

        result.actor = static_cast<SceneDB::Actor*>(actor_ptr);
        result.point = point;
        result.normal = normal;
        result.is_trigger = fixture->IsSensor();
        result.fraction = fraction;
        has_hit = true;
        return fraction;
    }
};

class RaycastCallbackAll : public b2RayCastCallback {
public:
    std::vector<HitResult> hits;

    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        void* actor_ptr = reinterpret_cast<void*>(fixture->GetUserData().pointer);
        if (!actor_ptr) return -1;
        if (fixture->GetFilterData().categoryBits == 0x0004) return -1;

        HitResult hit;
        hit.actor = static_cast<SceneDB::Actor*>(actor_ptr);
        hit.point = point;
        hit.normal = normal;
        hit.is_trigger = fixture->IsSensor();
        hit.fraction = fraction;
        hits.push_back(hit);
        return 1;
    }
};

static luabridge::LuaRef CppPhysicsRaycast(b2Vec2 pos, b2Vec2 dir, float dist) {
    if (!Rigidbody::world || dist <= 0.0f) return luabridge::LuaRef(s_lua_state);

    dir.Normalize();
    b2Vec2 endPoint(pos.x + dir.x * dist, pos.y + dir.y * dist);

    RaycastCallbackClosest callback;
    Rigidbody::world->RayCast(&callback, pos, endPoint);

    if (!callback.has_hit) return luabridge::LuaRef(s_lua_state);

    return luabridge::LuaRef(s_lua_state, callback.result);
}

static luabridge::LuaRef CppPhysicsRaycastAll(b2Vec2 pos, b2Vec2 dir, float dist) {
    luabridge::LuaRef table = luabridge::newTable(s_lua_state);

    if (!Rigidbody::world || dist <= 0.0f) return table;

    dir.Normalize();
    b2Vec2 endPoint(pos.x + dir.x * dist, pos.y + dir.y * dist);

    RaycastCallbackAll callback;
    Rigidbody::world->RayCast(&callback, pos, endPoint);

    std::sort(callback.hits.begin(), callback.hits.end(),
              [](const HitResult& a, const HitResult& b) { return a.fraction < b.fraction; });

    int index = 1;
    for (auto& hit : callback.hits) {
        table[index++] = hit;
    }
    return table;
}

static void CppEventPublish(const std::string& event_type, luabridge::LuaRef event_object) {
    auto it = s_event_subscriptions.find(event_type);
    if (it == s_event_subscriptions.end()) return;
    for (auto& [component, func] : it->second) {
        try {
            func(component, event_object);
        } catch (luabridge::LuaException& e) {
            std::string error_message = e.what();
            std::replace(error_message.begin(), error_message.end(), '\\', '/');
            std::cout << "\033[31m" << error_message << "\033[0m" << std::endl;
        }
    }
}

static void CppEventSubscribe(const std::string& event_type, luabridge::LuaRef component, luabridge::LuaRef function) {
    s_pending_subscribes.emplace_back(event_type, component, function);
}

static void CppEventUnsubscribe(const std::string& event_type, luabridge::LuaRef component,
                                luabridge::LuaRef function) {
    s_pending_unsubscribes.emplace_back(event_type, component, function);
}

static bool CppHotReloadLuaComponent(const std::string& typeName) {
    if (!s_component_db_self) {
        std::cout << "HotReload: ComponentDB not initialized.\n";
        return false;
    }
    return s_component_db_self->ReloadComponentType(typeName);
}

void ComponentDB::ProcessEventBusPending() {
    for (auto& op : s_pending_subscribes) {
        s_event_subscriptions[op.event_type].emplace_back(op.component, op.function);
    }
    s_pending_subscribes.clear();

    for (auto& op : s_pending_unsubscribes) {
        auto it = s_event_subscriptions.find(op.event_type);
        if (it != s_event_subscriptions.end()) {
            auto& subs = it->second;
            subs.erase(std::remove_if(subs.begin(), subs.end(),
                                      [&](const std::pair<luabridge::LuaRef, luabridge::LuaRef>& sub) {
                                          return sub.first == op.component && sub.second == op.function;
                                      }),
                       subs.end());
        }
    }
    s_pending_unsubscribes.clear();
}

void ComponentDB::Initialize() {
    lua_state = luaL_newstate();
    s_lua_state = lua_state;
    luaL_openlibs(lua_state);

    luabridge::getGlobalNamespace(lua_state).beginNamespace("Debug").addFunction("Log", CppDebugLog).endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Application")
      .addFunction("Quit", CppAppQuit)
      .addFunction("Sleep", CppAppSleep)
      .addFunction("GetFrame", CppAppGetFrame)
      .addFunction("OpenURL", CppAppOpenURL)
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginClass<glm::vec2>("vec2")
      .addProperty("x", &glm::vec2::x)
      .addProperty("y", &glm::vec2::y)
      .endClass();

    luabridge::getGlobalNamespace(lua_state)
      .beginClass<b2Vec2>("Vector2")
      .addConstructor<void (*)(float, float)>()
      .addProperty("x", &b2Vec2::x)
      .addProperty("y", &b2Vec2::y)
      .addFunction("Normalize", &b2Vec2::Normalize)
      .addFunction("Length", &b2Vec2::Length)
      .addFunction("__add", &b2Vec2::operator_add)
      .addFunction("__sub", &b2Vec2::operator_sub)
      .addFunction("__mul", &b2Vec2::operator_mul)
      .endClass();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Vector2")
      .addFunction("Distance", b2Distance)
      .addFunction("Dot", static_cast<float (*)(const b2Vec2&, const b2Vec2&)>(&b2Dot))
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginClass<Rigidbody>("Rigidbody")
      .addConstructor<void (*)()>()
      .addProperty("key", &Rigidbody::key)
      .addProperty("enabled", &Rigidbody::enabled)
      .addProperty("x", &Rigidbody::x)
      .addProperty("y", &Rigidbody::y)
      .addProperty("body_type", &Rigidbody::body_type)
      .addProperty("precise", &Rigidbody::precise)
      .addProperty("gravity_scale", &Rigidbody::gravity_scale)
      .addProperty("density", &Rigidbody::density)
      .addProperty("angular_friction", &Rigidbody::angular_friction)
      .addProperty("rotation", &Rigidbody::rotation)
      .addProperty("collider_type", &Rigidbody::collider_type)
      .addProperty("width", &Rigidbody::width)
      .addProperty("height", &Rigidbody::height)
      .addProperty("radius", &Rigidbody::radius)
      .addProperty("friction", &Rigidbody::friction)
      .addProperty("bounciness", &Rigidbody::bounciness)
      .addProperty("trigger_type", &Rigidbody::trigger_type)
      .addProperty("trigger_width", &Rigidbody::trigger_width)
      .addProperty("trigger_height", &Rigidbody::trigger_height)
      .addProperty("trigger_radius", &Rigidbody::trigger_radius)
      .addProperty("has_collider", &Rigidbody::has_collider)
      .addProperty("has_trigger", &Rigidbody::has_trigger)
      .addFunction("GetPosition", &Rigidbody::GetPosition)
      .addFunction("GetRotation", &Rigidbody::GetRotation)
      .addFunction("GetVelocity", &Rigidbody::GetVelocity)
      .addFunction("GetAngularVelocity", &Rigidbody::GetAngularVelocity)
      .addFunction("GetGravityScale", &Rigidbody::GetGravityScale)
      .addFunction("GetUpDirection", &Rigidbody::GetUpDirection)
      .addFunction("GetRightDirection", &Rigidbody::GetRightDirection)
      .addFunction("AddForce", &Rigidbody::AddForce)
      .addFunction("SetVelocity", &Rigidbody::SetVelocity)
      .addFunction("SetPosition", &Rigidbody::SetPosition)
      .addFunction("SetRotation", &Rigidbody::SetRotation)
      .addFunction("SetAngularVelocity", &Rigidbody::SetAngularVelocity)
      .addFunction("SetGravityScale", &Rigidbody::SetGravityScale)
      .addFunction("SetUpDirection", &Rigidbody::SetUpDirection)
      .addFunction("SetRightDirection", &Rigidbody::SetRightDirection)
      .endClass();

    luabridge::getGlobalNamespace(lua_state)
      .beginClass<ParticleSystem>("ParticleSystem")
      .addConstructor<void (*)()>()
      .addProperty("key", &ParticleSystem::key)
      .addProperty("enabled", &ParticleSystem::enabled)

      .addProperty("x", &ParticleSystem::x)
      .addProperty("y", &ParticleSystem::y)

      .addProperty("frames_between_bursts", &ParticleSystem::frames_between_bursts)
      .addProperty("burst_quantity", &ParticleSystem::burst_quantity)

      .addProperty("emit_angle_min", &ParticleSystem::emit_angle_min)
      .addProperty("emit_angle_max", &ParticleSystem::emit_angle_max)
      .addProperty("emit_radius_min", &ParticleSystem::emit_radius_min)
      .addProperty("emit_radius_max", &ParticleSystem::emit_radius_max)

      .addProperty("start_scale_min", &ParticleSystem::start_scale_min)
      .addProperty("start_scale_max", &ParticleSystem::start_scale_max)
      .addProperty("rotation_min", &ParticleSystem::rotation_min)
      .addProperty("rotation_max", &ParticleSystem::rotation_max)
      .addProperty("start_color_r", &ParticleSystem::start_color_r)
      .addProperty("start_color_g", &ParticleSystem::start_color_g)
      .addProperty("start_color_b", &ParticleSystem::start_color_b)
      .addProperty("start_color_a", &ParticleSystem::start_color_a)

      .addProperty("image", &ParticleSystem::image)
      .addProperty("sorting_order", &ParticleSystem::sorting_order)

      .addProperty("duration_frames", &ParticleSystem::duration_frames)

      .addProperty("start_speed_min", &ParticleSystem::start_speed_min)
      .addProperty("start_speed_max", &ParticleSystem::start_speed_max)
      .addProperty("rotation_speed_min", &ParticleSystem::rotation_speed_min)
      .addProperty("rotation_speed_max", &ParticleSystem::rotation_speed_max)

      .addProperty("gravity_scale_x", &ParticleSystem::gravity_scale_x)
      .addProperty("gravity_scale_y", &ParticleSystem::gravity_scale_y)
      .addProperty("drag_factor", &ParticleSystem::drag_factor)
      .addProperty("angular_drag_factor", &ParticleSystem::angular_drag_factor)

      .addProperty("end_scale", &ParticleSystem::end_scale)
      .addProperty("end_color_r", &ParticleSystem::end_color_r)
      .addProperty("end_color_g", &ParticleSystem::end_color_g)
      .addProperty("end_color_b", &ParticleSystem::end_color_b)
      .addProperty("end_color_a", &ParticleSystem::end_color_a)

      .addFunction("Stop", &ParticleSystem::Stop)
      .addFunction("Play", &ParticleSystem::Play)
      .addFunction("Burst", &ParticleSystem::Burst)
      .endClass();

    luabridge::getGlobalNamespace(lua_state)
      .beginClass<Collision>("Collision")
      .addData("other", &Collision::other)
      .addData("point", &Collision::point)
      .addData("relative_velocity", &Collision::relative_velocity)
      .addData("normal", &Collision::normal)
      .endClass();

    luabridge::getGlobalNamespace(lua_state)
      .beginClass<HitResult>("HitResult")
      .addData("actor", &HitResult::actor)
      .addData("point", &HitResult::point)
      .addData("normal", &HitResult::normal)
      .addData("is_trigger", &HitResult::is_trigger)
      .endClass();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Physics")
      .addFunction("Raycast", CppPhysicsRaycast)
      .addFunction("RaycastAll", CppPhysicsRaycastAll)
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Input")
      .addFunction("GetKey", &Input::GetKey)
      .addFunction("GetKeyDown", &Input::GetKeyDown)
      .addFunction("GetKeyUp", &Input::GetKeyUp)
      .addFunction("GetMousePosition", &Input::GetMousePosition)
      .addFunction("GetMouseButton", &Input::GetMouseButton)
      .addFunction("GetMouseButtonDown", &Input::GetMouseButtonDown)
      .addFunction("GetMouseButtonUp", &Input::GetMouseButtonUp)
      .addFunction("GetMouseScrollDelta", &Input::GetMouseScrollDelta)
      .addFunction("HideCursor", &Input::HideCursor)
      .addFunction("ShowCursor", &Input::ShowCursor)
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state).beginNamespace("Text").addFunction("Draw", CppTextDraw).endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Audio")
      .addFunction("Play", CppAudioPlay)
      .addFunction("Halt", CppAudioHalt)
      .addFunction("SetVolume", CppAudioSetVolume)
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Image")
      .addFunction("DrawUI", CppImageDrawUI)
      .addFunction("DrawUIEx", CppImageDrawUIEx)
      .addFunction("DrawUIScaled", CppImageDrawUIScaled)
      .addFunction("Draw", CppImageDraw)
      .addFunction("DrawEx", CppImageDrawEx)
      .addFunction("DrawPixel", CppImageDrawPixel)
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Camera")
      .addFunction("SetPosition", CppCameraSetPosition)
      .addFunction("GetPositionX", CppCameraGetPositionX)
      .addFunction("GetPositionY", CppCameraGetPositionY)
      .addFunction("SetZoom", CppCameraSetZoom)
      .addFunction("GetZoom", CppCameraGetZoom)
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Scene")
      .addFunction("Load", CppSceneLoad)
      .addFunction("GetCurrent", CppSceneGetCurrent)
      .addFunction("DontDestroy", CppSceneDontDestroy)
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Event")
      .addFunction("Publish", CppEventPublish)
      .addFunction("Subscribe", CppEventSubscribe)
      .addFunction("Unsubscribe", CppEventUnsubscribe)
      .endNamespace();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("HotReload")
      .addFunction("ReloadLuaComponent", CppHotReloadLuaComponent)
      .endNamespace();

    s_component_db_self = this;
}

void ComponentDB::InitializeActorBindings() {
    luabridge::getGlobalNamespace(lua_state)
      .beginClass<SceneDB::Actor>("Actor")
      .addFunction("GetName", &SceneDB::Actor::GetName)
      .addFunction("GetID", &SceneDB::Actor::GetID)
      .addFunction("GetComponentByKey", &SceneDB::Actor::GetComponentByKey)
      .addFunction("GetComponent", &SceneDB::Actor::GetComponent)
      .addFunction("GetComponents", &SceneDB::Actor::GetComponents)
      .addFunction("AddComponent", &SceneDB::Actor::AddComponent)
      .addFunction("RemoveComponent", &SceneDB::Actor::RemoveComponent)
      .endClass();

    luabridge::getGlobalNamespace(lua_state)
      .beginNamespace("Actor")
      .addFunction("Find", FindActor)
      .addFunction("FindAll", FindAllActors)
      .addFunction("Instantiate", InstantiateActor)
      .addFunction("Destroy", DestroyActor)
      .endNamespace();
}

void ComponentDB::EstablishInheritance(luabridge::LuaRef& instance_table, luabridge::LuaRef& parent_table) {
    luabridge::LuaRef new_metatable = luabridge::newTable(lua_state);
    new_metatable["__index"] = parent_table;

    instance_table.push(lua_state);
    new_metatable.push(lua_state);
    lua_setmetatable(lua_state, -2);
    lua_pop(lua_state, 1);
}

void ComponentDB::LoadComponentType(const std::string& typeName) {
    if (component_tables.count(typeName) > 0) return;

    std::string path = "resources/component_types/" + typeName + ".lua";

    if (!std::filesystem::exists(path)) {
        std::cout << "error: failed to locate component " << typeName;
        exit(0);
    }

    if (luaL_dofile(lua_state, path.c_str()) != LUA_OK) {
        std::cout << "problem with lua file " << typeName;
        exit(0);
    }

    luabridge::LuaRef* base_table = new luabridge::LuaRef(luabridge::getGlobal(lua_state, typeName.c_str()));
    component_tables[typeName] = base_table;

    // Saving each files first last modified time here
    std::error_code mec;
    auto mtime = std::filesystem::last_write_time(path, mec);
    if (!mec) component_file_mtimes[typeName] = mtime;
}

luabridge::LuaRef* ComponentDB::CreateComponentInstance(const std::string& typeName, const std::string& key) {
    if (typeName == "Rigidbody") {
        Rigidbody* rb = new Rigidbody();
        luabridge::LuaRef* instance = new luabridge::LuaRef(luabridge::LuaRef(lua_state, rb));
        (*instance)["key"] = key;
        (*instance)["enabled"] = true;
        return instance;
    }
    if (typeName == "ParticleSystem") {
        ParticleSystem* ps = new ParticleSystem();
        luabridge::LuaRef* instance = new luabridge::LuaRef(luabridge::LuaRef(lua_state, ps));
        (*instance)["key"] = key;
        (*instance)["enabled"] = true;
        return instance;
    }

    LoadComponentType(typeName);

    luabridge::LuaRef* parent = component_tables[typeName];
    luabridge::LuaRef* instance = new luabridge::LuaRef(luabridge::newTable(lua_state));

    EstablishInheritance(*instance, *parent);
    (*instance)["key"] = key;
    (*instance)["enabled"] = true;

    return instance;
}

// This function is called once per frame and uses polling so that we can avoid OS specific functions that would make it so
// it would not build on OSX, Linux, and Windows. However this does cause some latency but you can make the latency as low as you
// want at the cost of some performance by changing the kPollInterval value
void ComponentDB::PollHotReloadWatcher() {
    
    constexpr int kPollInterval = 15;
    if (++hot_reload_poll_counter < kPollInterval) return;
    hot_reload_poll_counter = 0;

    // leave function if the folder is missing instead of crashing
    const std::filesystem::path dir = "resources/component_types";
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec) || !std::filesystem::is_directory(dir, ec)) return;

    for (auto it = std::filesystem::directory_iterator(dir, ec);
         !ec && it != std::filesystem::directory_iterator(); it.increment(ec)) {
        const auto& entry = *it;
        if (!entry.is_regular_file()) continue;
        const auto& path = entry.path();
        if (path.extension() != ".lua") continue;

        const std::string typeName = path.stem().string();

        std::error_code mec;
        auto mtime = std::filesystem::last_write_time(path, mec);
        if (mec) continue;

        auto cached = component_file_mtimes.find(typeName);
        if (cached == component_file_mtimes.end()) {
            // First time we've ever seen this file so we record its mtime
            component_file_mtimes[typeName] = mtime;
            continue;
        }
        if (cached->second == mtime) continue;

        // Update the cache before reloading
        cached->second = mtime;

        // Only reload types we've already loaded into component_tables. If a
        // lua file exists on disk but no actor uses it yet then we dont need to do anything
        if (component_tables.count(typeName) == 0) continue;

        std::cout << "HotReload: detected change in " << path.string() << "\n";
        ReloadComponentType(typeName);
    }
}

bool ComponentDB::ReloadComponentType(const std::string& typeName) {
    if (typeName == "Rigidbody" || typeName == "ParticleSystem") {
        std::cout << "HotReload: \"" << typeName << "\" is not lua\n";
        return false;
    }
    if (!s_scene_db) {
        std::cout << "HotReload: SceneDB not set.\n";
        return false;
    }

    const std::string path = "resources/component_types/" + typeName + ".lua";
    if (!std::filesystem::exists(path)) {
        std::cout << "HotReload: file not found: " << path << "\n";
        return false;
    }

    if (luaL_loadfile(lua_state, path.c_str()) != LUA_OK) {
        const char* err = lua_tostring(lua_state, -1);
        std::cout << "HotReload: " << typeName << " load error: " << (err ? err : "?") << "\n";
        lua_pop(lua_state, 1);
        return false;
    }
    if (lua_pcall(lua_state, 0, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(lua_state, -1);
        std::cout << "HotReload: " << typeName << " run error: " << (err ? err : "?") << "\n";
        lua_pop(lua_state, 1);
        return false;
    }

    luabridge::LuaRef proto = luabridge::getGlobal(lua_state, typeName.c_str());
    if (!proto.isTable()) {
        std::cout << "HotReload: global \"" << typeName << "\" must be a table after loading " << path << "\n";
        return false;
    }

    luabridge::LuaRef* new_parent = new luabridge::LuaRef(luabridge::getGlobal(lua_state, typeName.c_str()));
    auto it = component_tables.find(typeName);
    if (it != component_tables.end()) {
        delete it->second;
        it->second = new_parent;
    } else {
        component_tables[typeName] = new_parent;
    }

    luabridge::LuaRef* parent = component_tables[typeName];
    for (SceneDB::Actor* actor : s_scene_db->actors) {
        if (!actor || actor->destroyed) continue;
        for (auto& [key, compPtr] : actor->components) {
            if (actor->removed_components.count(key) > 0) continue;
            auto tit = actor->component_types.find(key);
            if (tit == actor->component_types.end() || tit->second != typeName) continue;
            luabridge::LuaRef& comp = *compPtr;
            if (!comp.isTable()) continue;
            EstablishInheritance(comp, *parent);
        }
    }

    std::cout << "HotReload succesful\n";
    return true;
}
