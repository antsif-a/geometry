export module camera;

import std;
import glm;

using std::bitset;
using namespace glm;

export struct lerp_camera {
    enum direction {
        Left, Right, Up, Down, Front, Back
    };

    const float mouse_sensitivity;
    const float key_sensitivity;
    const vec3 up;
    
    bitset<6> movement_bits;
    vec3 position;
    vec3 front;
    vec3 velocity;

    lerp_camera(
        float mouse_sensitivity = 5 * 0.1f,
        float key_sensitivity = 5.0f,
        vec3 init_position = {0, 0, 5},
        vec3 init_front = {0, 0, -1},
        vec3 up = {0, 1, 0}
    ) : mouse_sensitivity(mouse_sensitivity)
      , key_sensitivity(key_sensitivity)
      , up(up)
      , position(init_position)
      , front(init_front) {}

    void rotate(float yaw, float pitch) {
        yaw *= mouse_sensitivity;
        pitch *= mouse_sensitivity;
        vec3 right = cross(front, up);
        vec3 pitch_rotation = glm::rotate(front, radians(pitch), right);
        if (abs(dot(pitch_rotation, up)) < 0.99f)
            front = pitch_rotation;
        front = glm::rotate(front, radians(yaw), up);
        front = normalize(front);
    }

    void set_movement(int dir, bool on) {
        movement_bits.set(dir, on);
    }

    void update(float dt) {
        vec3 target_velocity = {0, 0, 0};
        target_velocity += (float) (movement_bits[Right] - movement_bits[Left]) * cross(front, up);
        target_velocity += (float) (movement_bits[Up]    - movement_bits[Down]) * up;
        target_velocity += (float) (movement_bits[Front] - movement_bits[Back]) * front;

        velocity = mix(velocity, target_velocity * dt * key_sensitivity, 0.5);
        position += velocity;
    }

    mat4 compute_view_matrix() {
        return lookAt(position, position + front, up);
    }   
};
