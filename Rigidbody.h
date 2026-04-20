#ifndef Rigidbody_h
#define Rigidbody_h

#include <string>
#include <vector>

#include "box2d/box2d.h"
#include "glm/glm.hpp"

typedef void (*ContactCallback)(void* actorA, void* actorB, b2Vec2 point, b2Vec2 relative_velocity, b2Vec2 normal,
                                bool is_enter, bool is_trigger);

class ContactListener : public b2ContactListener {
public:
    inline static ContactCallback contact_callback = nullptr;

    void BeginContact(b2Contact* contact) override {
        b2Fixture* fixtureA = contact->GetFixtureA();
        b2Fixture* fixtureB = contact->GetFixtureB();

        void* actorA = reinterpret_cast<void*>(fixtureA->GetUserData().pointer);
        void* actorB = reinterpret_cast<void*>(fixtureB->GetUserData().pointer);
        if (!actorA || !actorB) return;

        bool sensorA = fixtureA->IsSensor();
        bool sensorB = fixtureB->IsSensor();

        if (sensorA != sensorB) return;

        b2Vec2 relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();
        bool is_trigger = sensorA;

        b2Vec2 point(-999.0f, -999.0f);
        b2Vec2 normal(-999.0f, -999.0f);

        if (!is_trigger) {
            b2WorldManifold world_manifold;
            contact->GetWorldManifold(&world_manifold);
            point = world_manifold.points[0];
            normal = world_manifold.normal;
        }

        if (contact_callback) {
            contact_callback(actorA, actorB, point, relative_velocity, normal, true, is_trigger);
        }
    }

    void EndContact(b2Contact* contact) override {
        b2Fixture* fixtureA = contact->GetFixtureA();
        b2Fixture* fixtureB = contact->GetFixtureB();

        void* actorA = reinterpret_cast<void*>(fixtureA->GetUserData().pointer);
        void* actorB = reinterpret_cast<void*>(fixtureB->GetUserData().pointer);
        if (!actorA || !actorB) return;

        bool sensorA = fixtureA->IsSensor();
        bool sensorB = fixtureB->IsSensor();

        if (sensorA != sensorB) return;

        b2Vec2 relative_velocity = fixtureA->GetBody()->GetLinearVelocity() - fixtureB->GetBody()->GetLinearVelocity();

        if (contact_callback) {
            contact_callback(actorA, actorB, b2Vec2(-999.0f, -999.0f), relative_velocity, b2Vec2(-999.0f, -999.0f),
                             false, sensorA);
        }
    }
};

class Rigidbody {
public:
    std::string key;
    bool enabled = true;
    void* actor = nullptr;

    float x = 0.0f;
    float y = 0.0f;
    std::string body_type = "dynamic";
    bool precise = true;
    float gravity_scale = 1.0f;
    float density = 1.0f;
    float angular_friction = 0.3f;
    float rotation = 0.0f;
    std::string collider_type = "box";
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    float friction = 0.3f;
    float bounciness = 0.3f;
    std::string trigger_type = "box";
    float trigger_width = 1.0f;
    float trigger_height = 1.0f;
    float trigger_radius = 0.5f;
    bool has_collider = true;
    bool has_trigger = true;

    b2Body* body = nullptr;

    inline static b2World* world = nullptr;
    inline static ContactListener* contact_listener = nullptr;

    b2Vec2 GetPosition() {
        if (body != nullptr) return body->GetPosition();
        return b2Vec2(x, y);
    }

    float GetRotation() {
        if (body != nullptr) return body->GetAngle() * (180.0f / b2_pi);
        return rotation;
    }

    b2Vec2 GetVelocity() {
        if (body != nullptr) return body->GetLinearVelocity();
        return b2Vec2(0.0f, 0.0f);
    }

    float GetAngularVelocity() {
        if (body != nullptr) return body->GetAngularVelocity() * (180.0f / b2_pi);
        return 0.0f;
    }

    float GetGravityScale() {
        if (body != nullptr) return body->GetGravityScale();
        return gravity_scale;
    }

    b2Vec2 GetUpDirection() {
        float angle = body ? body->GetAngle() : rotation * (b2_pi / 180.0f);
        b2Vec2 result = b2Vec2(glm::sin(angle), -glm::cos(angle));
        result.Normalize();
        return result;
    }

    b2Vec2 GetRightDirection() {
        float angle = body ? body->GetAngle() : rotation * (b2_pi / 180.0f);
        b2Vec2 result = b2Vec2(glm::cos(angle), glm::sin(angle));
        result.Normalize();
        return result;
    }

    void AddForce(b2Vec2 force) {
        if (!body) return;
        body->ApplyForceToCenter(force, true);
    }

    void SetVelocity(b2Vec2 velocity) {
        if (!body) return;
        body->SetLinearVelocity(velocity);
    }

    void SetPosition(b2Vec2 position) {
        if (body) {
            body->SetTransform(position, body->GetAngle());
        } else {
            x = position.x;
            y = position.y;
        }
    }

    void SetRotation(float degrees_clockwise) {
        if (body) {
            body->SetTransform(body->GetPosition(), degrees_clockwise * (b2_pi / 180.0f));
        } else {
            rotation = degrees_clockwise;
        }
    }

    void SetAngularVelocity(float degrees_clockwise) {
        if (!body) return;
        body->SetAngularVelocity(degrees_clockwise * (b2_pi / 180.0f));
    }

    void SetGravityScale(float scale) {
        if (body) {
            body->SetGravityScale(scale);
        } else {
            gravity_scale = scale;
        }
    }

    void SetUpDirection(b2Vec2 direction) {
        direction.Normalize();
        float new_angle_radians = glm::atan(direction.x, -direction.y);
        if (body) {
            body->SetTransform(body->GetPosition(), new_angle_radians);
        } else {
            rotation = new_angle_radians * (180.0f / b2_pi);
        }
    }

    void SetRightDirection(b2Vec2 direction) {
        direction.Normalize();
        float new_angle_radians = glm::atan(direction.x, -direction.y) - (b2_pi / 2.0f);
        if (body) {
            body->SetTransform(body->GetPosition(), new_angle_radians);
        } else {
            rotation = new_angle_radians * (180.0f / b2_pi);
        }
    }

    void OnDestroy() {
        if (body && world) {
            world->DestroyBody(body);
            body = nullptr;
        }
    }

    void OnStart() {
        if (body != nullptr) return;

        if (world == nullptr) {
            world = new b2World(b2Vec2(0.0f, 9.8f));
            contact_listener = new ContactListener();
            world->SetContactListener(contact_listener);
        }

        b2BodyDef bodyDef;
        if (body_type == "dynamic")
            bodyDef.type = b2_dynamicBody;
        else if (body_type == "static")
            bodyDef.type = b2_staticBody;
        else if (body_type == "kinematic")
            bodyDef.type = b2_kinematicBody;

        bodyDef.position.Set(x, y);
        bodyDef.angle = rotation * (b2_pi / 180.0f);
        bodyDef.bullet = precise;
        bodyDef.angularDamping = angular_friction;
        bodyDef.gravityScale = gravity_scale;

        body = world->CreateBody(&bodyDef);

        if (!has_collider && !has_trigger) {
            b2PolygonShape phantom_shape;
            phantom_shape.SetAsBox(width * 0.5f, height * 0.5f);

            b2FixtureDef phantom_fixture_def;
            phantom_fixture_def.shape = &phantom_shape;
            phantom_fixture_def.density = density;
            phantom_fixture_def.isSensor = true;
            phantom_fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
            phantom_fixture_def.filter.categoryBits = 0x0004;
            phantom_fixture_def.filter.maskBits = 0x0000;

            body->CreateFixture(&phantom_fixture_def);
        }

        if (has_collider) {
            b2FixtureDef fixture_def;
            fixture_def.isSensor = false;
            fixture_def.density = density;
            fixture_def.friction = friction;
            fixture_def.restitution = bounciness;
            fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
            fixture_def.filter.categoryBits = 0x0001;
            fixture_def.filter.maskBits = 0x0001;

            b2PolygonShape box_shape;
            b2CircleShape circle_shape;

            if (collider_type == "circle") {
                circle_shape.m_radius = radius;
                fixture_def.shape = &circle_shape;
            } else {
                box_shape.SetAsBox(width * 0.5f, height * 0.5f);
                fixture_def.shape = &box_shape;
            }

            body->CreateFixture(&fixture_def);
        }

        if (has_trigger) {
            b2FixtureDef trigger_fixture_def;
            trigger_fixture_def.isSensor = true;
            trigger_fixture_def.density = density;
            trigger_fixture_def.userData.pointer = reinterpret_cast<uintptr_t>(actor);
            trigger_fixture_def.filter.categoryBits = 0x0002;
            trigger_fixture_def.filter.maskBits = 0x0002;

            b2PolygonShape box_shape;
            b2CircleShape circle_shape;

            if (trigger_type == "circle") {
                circle_shape.m_radius = trigger_radius;
                trigger_fixture_def.shape = &circle_shape;
            } else {
                box_shape.SetAsBox(trigger_width * 0.5f, trigger_height * 0.5f);
                trigger_fixture_def.shape = &box_shape;
            }

            body->CreateFixture(&trigger_fixture_def);
        }
    }
};

#endif /* Rigidbody_h */
