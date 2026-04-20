#ifndef ParticleSystem_hpp
#define ParticleSystem_hpp

#include <queue>
#include <string>
#include <vector>

#include "Helper.h"

class ImageDB;

class ParticleSystem {
public:
    std::string key;
    bool enabled = true;
    void* actor = nullptr;

    float x = 0.0f;
    float y = 0.0f;

    int frames_between_bursts = 1;
    int burst_quantity = 1;

    float emit_angle_min = 0.0f;
    float emit_angle_max = 360.0f;
    float emit_radius_min = 0.0f;
    float emit_radius_max = 0.5f;

    float start_scale_min = 1.0f;
    float start_scale_max = 1.0f;
    float rotation_min = 0.0f;
    float rotation_max = 0.0f;
    int start_color_r = 255;
    int start_color_g = 255;
    int start_color_b = 255;
    int start_color_a = 255;

    std::string image = "";
    int sorting_order = 9999;

    int duration_frames = 300;

    float start_speed_min = 0.0f;
    float start_speed_max = 0.0f;
    float rotation_speed_min = 0.0f;
    float rotation_speed_max = 0.0f;

    float gravity_scale_x = 0.0f;
    float gravity_scale_y = 0.0f;
    float drag_factor = 1.0f;
    float angular_drag_factor = 1.0f;

    float end_scale = -1.0f;
    int end_color_r = -1;
    int end_color_g = -1;
    int end_color_b = -1;
    int end_color_a = -1;

    void OnStart();
    void OnUpdate();
    void OnDestroy();

    void Stop();
    void Play();
    void Burst();

    static void BindImageDB(ImageDB* db);

private:
    static ImageDB* s_image_db;

    RandomEngine rng_emit_angle_;
    RandomEngine rng_emit_radius_;
    RandomEngine rng_rotation_;
    RandomEngine rng_scale_;
    RandomEngine rng_speed_;
    RandomEngine rng_angular_vel_;

    int local_frame_number_ = 0;
    bool emission_allowed_ = true;

    int pending_burst_count_ = 0;

    std::queue<int> free_list_;

    std::vector<uint8_t> particle_is_active_;
    std::vector<int> particle_start_frame_;

    std::vector<float> particle_pos_x_;
    std::vector<float> particle_pos_y_;
    std::vector<float> particle_vel_x_;
    std::vector<float> particle_vel_y_;

    std::vector<float> particle_rotation_;
    std::vector<float> particle_angular_vel_;

    std::vector<float> particle_scale_;
    std::vector<int> particle_r_;
    std::vector<int> particle_g_;
    std::vector<int> particle_b_;
    std::vector<int> particle_a_;

    void SpawnParticle();
    void ClearAllColumns();
};

#endif /* ParticleSystem_hpp */

