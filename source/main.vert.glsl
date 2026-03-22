#version 460 core

struct instance_data {
    mat4 normal_matrix;
    mat4 model;
    vec4 color;
    int  texture_index;
    int  _[27];
};

layout (std140, binding = 0) uniform _0 {
    mat4 projection_matrix;
    mat4 view_matrix;
    vec3 camera_position;
    int   enable_light;
    float ambient;
    float diffuse;
    float specular;
    int   specular_power;
};

layout (std430, binding = 1) buffer _1 {
    instance_data instances[];
};

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoords;

layout (location = 0) out vec3 fragment_position;
layout (location = 1) out vec3 fragment_normal;
layout (location = 2) out vec2 fragment_texcoords;
layout (location = 3) out flat vec4 instance_color;
layout (location = 4) out flat  int instance_texture_index;

vec4 get_position() {
    instance_data data = instances[gl_InstanceID];
    vec4 world_position = data.model * vec4(position, 1.0);

    fragment_position = world_position.xyz;
    fragment_normal = mat3(data.normal_matrix) * normal;
    fragment_texcoords = texcoords;

    instance_color = data.color;
    instance_texture_index = data.texture_index;

    return projection_matrix * view_matrix * world_position;
}

void main() {
    gl_Position = get_position();
}
