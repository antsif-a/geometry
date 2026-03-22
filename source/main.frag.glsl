#version 460 core

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

layout (std430, binding = 2) buffer _2 {
    vec4 light_positions[];
};

layout (constant_id = 0) const uint    texture_count = 32U;
layout (binding = 3) uniform sampler2D textures[texture_count];

layout (location = 0) in vec3 fragment_position;
layout (location = 1) in vec3 fragment_normal;
layout (location = 2) in vec2 fragment_texcoords;
layout (location = 3) in flat vec4 instance_color;
layout (location = 4) in flat  int instance_texture_index;

layout (location = 0) out vec4 fragment_color;

vec4 get_fragment_color() {
    vec4 base_color;
    if (instance_texture_index < 0) {
        base_color = instance_color;
    } else {
        base_color = texture(textures[instance_texture_index], fragment_texcoords);
    }

    if (enable_light == 0) {
        return base_color;
    }
    vec4 color = vec4(0, 0, 0, base_color.a);
    vec3 normal = normalize(fragment_normal);
    vec3 view_dir = normalize(camera_position - fragment_position);
    for (int i = 0; i < light_positions.length(); ++i) {
        vec3 light_dir = normalize(light_positions[i].xyz - fragment_position);
        float diff = max(dot(normal, light_dir), 0.0f) * diffuse;
        vec3 reflect_dir = reflect(-light_dir, normal);
        float spec = pow(max(dot(view_dir, reflect_dir), 0.0f), specular_power) * specular;
        color.rgb += (ambient + diff + spec) * base_color.rgb;
    }
    return color;
}

void main() {
    fragment_color = get_fragment_color();
}
