#include "ParticleSystem.hpp"

#include <algorithm>

#include "ImageDB.h"
#include "third_party/glm/glm.hpp"

namespace {
constexpr int kSeedEmitAngle = 298;
constexpr int kSeedEmitRadius = 404;
constexpr int kSeedRotation = 440;
constexpr int kSeedScale = 494;
constexpr int kSeedSpeed = 498;
constexpr int kSeedAngularVel = 305;
}   // namespace

ImageDB* ParticleSystem::s_image_db = nullptr;

void ParticleSystem::BindImageDB(ImageDB* db) {
    s_image_db = db;
}

void ParticleSystem::Stop() {
    emission_allowed_ = false;
}
void ParticleSystem::Play() {
    emission_allowed_ = true;
}

void ParticleSystem::Burst() {
    pending_burst_count_ += burst_quantity;
}

void ParticleSystem::ClearAllColumns() {
    while (!free_list_.empty()) free_list_.pop();

    particle_is_active_.clear();
    particle_start_frame_.clear();
    particle_pos_x_.clear();
    particle_pos_y_.clear();
    particle_vel_x_.clear();
    particle_vel_y_.clear();
    particle_rotation_.clear();
    particle_angular_vel_.clear();
    particle_scale_.clear();
    particle_r_.clear();
    particle_g_.clear();
    particle_b_.clear();
    particle_a_.clear();
}

void ParticleSystem::SpawnParticle() {
    const float angle_radians = glm::radians(rng_emit_angle_.Sample());
    const float radius = rng_emit_radius_.Sample();
    const float cos_angle = glm::cos(angle_radians);
    const float sin_angle = glm::sin(angle_radians);

    const float pos_x = x + cos_angle * radius;
    const float pos_y = y + sin_angle * radius;

    const float speed = rng_speed_.Sample();
    const float vel_x = cos_angle * speed;
    const float vel_y = sin_angle * speed;

    const float rotation = rng_rotation_.Sample();
    const float scale = rng_scale_.Sample();
    const float angular_vel = rng_angular_vel_.Sample();

    if (!free_list_.empty()) {
        const int slot = free_list_.front();
        free_list_.pop();

        particle_is_active_[slot] = 1;
        particle_start_frame_[slot] = local_frame_number_;
        particle_pos_x_[slot] = pos_x;
        particle_pos_y_[slot] = pos_y;
        particle_vel_x_[slot] = vel_x;
        particle_vel_y_[slot] = vel_y;
        particle_rotation_[slot] = rotation;
        particle_angular_vel_[slot] = angular_vel;
        particle_scale_[slot] = scale;
        particle_r_[slot] = start_color_r;
        particle_g_[slot] = start_color_g;
        particle_b_[slot] = start_color_b;
        particle_a_[slot] = start_color_a;
    } else {
        particle_is_active_.push_back(1);
        particle_start_frame_.push_back(local_frame_number_);
        particle_pos_x_.push_back(pos_x);
        particle_pos_y_.push_back(pos_y);
        particle_vel_x_.push_back(vel_x);
        particle_vel_y_.push_back(vel_y);
        particle_rotation_.push_back(rotation);
        particle_angular_vel_.push_back(angular_vel);
        particle_scale_.push_back(scale);
        particle_r_.push_back(start_color_r);
        particle_g_.push_back(start_color_g);
        particle_b_.push_back(start_color_b);
        particle_a_.push_back(start_color_a);
    }
}

void ParticleSystem::OnStart() {
    frames_between_bursts = std::max(1, frames_between_bursts);
    burst_quantity = std::max(1, burst_quantity);
    duration_frames = std::max(1, duration_frames);

    rng_emit_angle_.Configure(emit_angle_min, emit_angle_max, kSeedEmitAngle);
    rng_emit_radius_.Configure(emit_radius_min, emit_radius_max, kSeedEmitRadius);
    rng_rotation_.Configure(rotation_min, rotation_max, kSeedRotation);
    rng_scale_.Configure(start_scale_min, start_scale_max, kSeedScale);
    rng_speed_.Configure(start_speed_min, start_speed_max, kSeedSpeed);
    rng_angular_vel_.Configure(rotation_speed_min, rotation_speed_max, kSeedAngularVel);

    local_frame_number_ = 0;
    pending_burst_count_ = 0;
    ClearAllColumns();
}

void ParticleSystem::OnUpdate() {
    if (!s_image_db) return;

    if (local_frame_number_ % frames_between_bursts == 0 && emission_allowed_) {
        for (int i = 0; i < burst_quantity; ++i) SpawnParticle();
    }

    const int num_slots = static_cast<int>(particle_pos_x_.size());
    const float inv_duration = 1.0f / static_cast<float>(duration_frames);

    const bool has_end_scale = end_scale >= 0.0f;
    const bool has_end_r = end_color_r >= 0;
    const bool has_end_g = end_color_g >= 0;
    const bool has_end_b = end_color_b >= 0;
    const bool has_end_a = end_color_a >= 0;

    for (int i = 0; i < num_slots; ++i) {
        if (!particle_is_active_[i]) continue;

        const int frames_alive = local_frame_number_ - particle_start_frame_[i];
        if (frames_alive >= duration_frames) {
            particle_is_active_[i] = 0;
            free_list_.push(i);
            continue;
        }

        particle_vel_x_[i] += gravity_scale_x;
        particle_vel_y_[i] += gravity_scale_y;

        particle_vel_x_[i] *= drag_factor;
        particle_vel_y_[i] *= drag_factor;
        particle_angular_vel_[i] *= angular_drag_factor;

        particle_pos_x_[i] += particle_vel_x_[i];
        particle_pos_y_[i] += particle_vel_y_[i];
        particle_rotation_[i] += particle_angular_vel_[i];

        const float lifetime_progress = static_cast<float>(frames_alive) * inv_duration;

        float render_scale = particle_scale_[i];
        if (has_end_scale) render_scale = glm::mix(particle_scale_[i], end_scale, lifetime_progress);

        int render_r = particle_r_[i];
        int render_g = particle_g_[i];
        int render_b = particle_b_[i];
        int render_a = particle_a_[i];

        if (has_end_r)
            render_r = static_cast<int>(
              glm::mix(static_cast<float>(particle_r_[i]), static_cast<float>(end_color_r), lifetime_progress));
        if (has_end_g)
            render_g = static_cast<int>(
              glm::mix(static_cast<float>(particle_g_[i]), static_cast<float>(end_color_g), lifetime_progress));
        if (has_end_b)
            render_b = static_cast<int>(
              glm::mix(static_cast<float>(particle_b_[i]), static_cast<float>(end_color_b), lifetime_progress));
        if (has_end_a)
            render_a = static_cast<int>(
              glm::mix(static_cast<float>(particle_a_[i]), static_cast<float>(end_color_a), lifetime_progress));

        ImageDrawRequest req;
        req.image_name = image;
        req.x = particle_pos_x_[i];
        req.y = particle_pos_y_[i];
        req.rotation_degrees = static_cast<int>(particle_rotation_[i]);
        req.scale_x = render_scale;
        req.scale_y = render_scale;
        req.pivot_x = 0.5f;
        req.pivot_y = 0.5f;
        req.r = render_r;
        req.g = render_g;
        req.b = render_b;
        req.a = render_a;
        req.sorting_order = sorting_order;
        s_image_db->QueueSceneDraw(req);
    }

    ++local_frame_number_;

    if (pending_burst_count_ > 0) {
        for (int i = 0; i < pending_burst_count_; ++i) SpawnParticle();
        pending_burst_count_ = 0;
    }
}

void ParticleSystem::OnDestroy() {
    ClearAllColumns();
}

