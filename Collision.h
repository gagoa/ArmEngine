#ifndef Collision_h
#define Collision_h

#include "SceneDB.hpp"
#include "box2d/box2d.h"

struct Collision {
    SceneDB::Actor* other = nullptr;
    b2Vec2 point = b2Vec2(-999.0f, -999.0f);
    b2Vec2 relative_velocity = b2Vec2(0.0f, 0.0f);
    b2Vec2 normal = b2Vec2(-999.0f, -999.0f);
};

struct HitResult {
    SceneDB::Actor* actor = nullptr;
    b2Vec2 point = b2Vec2(0.0f, 0.0f);
    b2Vec2 normal = b2Vec2(0.0f, 0.0f);
    bool is_trigger = false;
    float fraction = 0.0f;
};

#endif /* Collision_h */
