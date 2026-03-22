#include <entt/entity/registry.hpp>
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

import std;
import std.compat;

import gl;
import glfw;
import glm;

import imgui;
import imgui_impl_glfw;
import imgui_impl_opengl3;

import logger;
import camera;
import geometry;

using std::array;
using std::span;
using std::string;
using std::vector;
using std::filesystem::path;
using namespace glm;

extern const uint8_t _binary_main_vert_glsl_spv_start[];
extern const uint8_t _binary_main_vert_glsl_spv_end[];
extern const uint8_t _binary_main_frag_glsl_spv_start[];
extern const uint8_t _binary_main_frag_glsl_spv_end[];
/*
 * packed:
 * - implementation defined
 * shared (default):
 * + can be used across programs
 * + can be used by ssbo / ubo
 * - all variables are active, nothing is optimized
 * std140:
 * ! array stride is 16 bytes (vec4)
 * + can be used across programs
 * - not tightly packed - introduces unnecessary padding
 * std430:
 * + more tightly packed than std140 (except for vec3)
 * - can't be used by ubo
 */

/* === shader mappings === */
enum {
    binding_uniform_buffer,
    binding_instances_data,
    binding_light_positions,
    binding_textures
};

enum {
    constant_texture_count
};

/* binding_instance_data : std430 ssbo, array of */
struct alignas(vec4) instance_data {
    mat4 normal_matrix;
    mat4 model_matrix;
    vec4 color;
    int  texture_index;
    uint32_t _[27];
};

/* binding_view_projection : std140 ubo */
struct alignas(vec4) uniform_buffer {
    mat4 projection_matrix;
    mat4 view_matrix;
    vec3 camera_position;
    int   enable_light;
    float ambient;
    float diffuse;
    float specular;
    int   specular_power;
};
/* === */

struct imgui {
    bool vsync = 1;
    imgui(GLFWwindow * window) {
        ImGui::CheckVersion();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init();
    }

    ~imgui() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void new_frame(
        float fps,
        float *screen_color,
        float *ambient,
        float *diffuse,
        float *specular,
        int   *specular_power
    ) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ColorEdit4("Screen color", screen_color);
        ImGui::SliderFloat("Ambient", ambient, 0.f, 1.f);
        ImGui::SliderFloat("Diffuse", diffuse, 0.f, 1.f);
        ImGui::SliderFloat("Specular", specular, 0.f, 1.f);
        ImGui::SliderInt("Specular Power", specular_power, 1, 128);
        if (ImGui::Checkbox("Vsync", &vsync)) {
            glfw::swap_interval(vsync ? 1 : 0);
        }
        ImGui::Text("fps = %f", fps);
    }

    void render() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

const int WIDTH = 1400, HEIGHT = 1000;
uniform_buffer *ub; // somewhere in the gpu
lerp_camera camera;
vec4 screen_color = vec4(1);
bool camera_enabled = true;
dvec2 mouse_position = vec2(0, 0);
double camera_sensitivity = 0.1f;

void framebuffer_size_callback(glfw::window_view, int w, int h) {
    glViewport(0, 0, w, h);
    ub->projection_matrix = glm::perspective(glm::radians(45.0f), (float) w / (float) h, 0.1f, 100.f);
}

void cursor_callback(glfw::window_view, double xpos, double ypos) {
    if (camera_enabled) {
        double dx = mouse_position.x - xpos;
        double dy = mouse_position.y - ypos;
        camera.rotate(float(dx) * camera_sensitivity, float(dy) * camera_sensitivity);
        mouse_position.x = xpos;
        mouse_position.y = ypos;
    }
}

void key_callback(glfw::window_view window, glfw::Key key, int, glfw::Action action, glfw::Modifier) {
    using glfw::Key, glfw::Action, glfw::CursorMode;

    if (action == Action::Press || action == Action::Release) {
        bool to_move = action == Action::Press;
        switch (key) {
            case Key::A:
                camera.set_movement(lerp_camera::Left, to_move);
                break;
            case Key::D:
                camera.set_movement(lerp_camera::Right, to_move);
                break;
            case Key::W:
                camera.set_movement(lerp_camera::Front, to_move);
                break;
            case Key::S:
                camera.set_movement(lerp_camera::Back, to_move);
                break;
            case Key::Space:
                camera.set_movement(lerp_camera::Up, to_move);
                break;
            case Key::LeftShift:
                camera.set_movement(lerp_camera::Down, to_move);
                break;
            default:
                break;
        }
    }

    if (action == Action::Press && key == Key::C) {
        camera_enabled = !camera_enabled;
        if (camera_enabled) {
            mouse_position = window.get_cursor_position();
            window.set_cursor_mode(CursorMode::Disabled);
        } else {
            window.set_cursor_mode(CursorMode::Captured);
        }
    }

    if (action == Action::Press && key == Key::Escape) {
        window.close();
    }
}

enum {
    texture_index_stars,
    texture_index_earth_daymap
};

vector<gl::texture> make_textures() {
    constexpr size_t n = 2;
    array<path, n> filenames = {
        "/home/andrew/Source/geometry++/assets/8k_stars.jpg",
        "/home/andrew/Source/geometry++/assets/8k_earth_daymap.jpg"
    };
    vector<gl::texture> textures;
    textures.reserve(2);
    uint8_t * pixels;
    int x, y, channels;
    stbi_set_flip_vertically_on_load(false);
    for (size_t i = 0; i < n; ++i) {
        pixels = stbi_load(filenames[i].c_str(), &x, &y, &channels, STBI_default);
        if (pixels == nullptr)
            logger::error("texture data = nullptr (path = {})", filenames[i].c_str());
        textures.push_back(gl::make_texture(pixels, x, y, channels));
        stbi_image_free(pixels);
    }
    return textures;
}

enum {
    mesh_index_sphere_32x32,
    mesh_index_sphere_64x64,
    mesh_index_inner_cube
};

vector<gl::mesh> make_meshes() {
    vector<gl::mesh> meshes;
    meshes.reserve(3);
    meshes.push_back(gl::make_mesh(
        generate_grid_indices(32, 32),
        generate_surface(32, 32, sphere),
        generate_normals(32, 32, sphere),
        generate_texcoords(32, 32)
    ));
    meshes.push_back(gl::make_mesh(
        generate_grid_indices(64, 64),
        generate_surface(64, 64, sphere),
        generate_normals(64, 64, sphere),
        generate_texcoords(64, 64)
    ));
    auto cube = create_cube_cw();
    meshes.push_back(gl::make_mesh(
        cube.indices,
        cube.positions,
        cube.normals,
        cube.texcoords
    ));
    return meshes;
}

struct model_component {
    mat4 model_matrix;
    mat4 normal_matrix;
};

struct color_component {
    vec4 value;
};

struct texture_component {
    uint32_t index;
};

struct mesh_component {
    uint32_t index;
};

struct light_source_component {
    vec3 position;
};

vector<vec4> light_positions = {
    vec4(2),
};

void create_entities(entt::registry &reg) {
    {
        auto room = reg.create();
        const mat4 S = scale(mat4(1), vec3(5));
        const mat4 M = S;
        reg.emplace<model_component>(room, M, transpose(inverse(M)));
        reg.emplace<mesh_component> (room, mesh_index_inner_cube);
        reg.emplace<texture_component>(room, texture_index_stars);
    }

    {
        auto planet = reg.create();
        const mat4 T = translate(mat4(1), vec3(-1));
        const mat4 S = scale(mat4(1), vec3(1.5));
        const mat4 M = S * T;
        reg.emplace<model_component>  (planet, M, transpose(inverse(M)));
        reg.emplace<mesh_component>   (planet, mesh_index_sphere_64x64);
        reg.emplace<texture_component>(planet, texture_index_earth_daymap);
    }

    for (auto & position : light_positions) {
        auto e = reg.create();
        const mat4 T = translate(mat4(1), vec3(position));
        const mat4 S = scale(mat4(1), vec3(0.2));
        const mat4 M = S * T;
        reg.emplace<model_component>(e, M, transpose(inverse(M)));
        reg.emplace<mesh_component> (e, mesh_index_sphere_32x32);
        reg.emplace<color_component>(e, vec4(1));
        reg.emplace<light_source_component>(e, vec3(position));
    }
}

int main()
{
    glfw::set_default_error_handler();
    glfw::window window = glfw::create_window(WIDTH, HEIGHT, "glfw", {
        {glfw::WindowHint::ContextCreationApi, glfw::NativeContextApi},
        {glfw::WindowHint::ClientApi, glfw::OpenglApi},
        {glfw::WindowHint::ContextVersionMajor, 4},
        {glfw::WindowHint::ContextVersionMinor, 6},
        {glfw::WindowHint::OpenglProfile, glfw::OpenglCoreProfile},
        {glfw::WindowHint::OpenglForwardCompat, true},
        {glfw::WindowHint::ContextDebug, true}
    });

    glfw::set_current_context(window);
    glfw::swap_interval(1);
    if (glfw::is_raw_mouse_motion_supported())
        window.set_raw_mouse_motion(true);

    window.on_framebuffer_resize(framebuffer_size_callback);
    window.on_key(key_callback);
    window.on_cursor(cursor_callback);
    window.set_cursor_mode(glfw::CursorMode::Disabled);

    gl::load(glfw::get_proc_address);
    gl::set_default_debug_message_handler();
    gl::enable(GL_DEBUG_OUTPUT);
    gl::enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    gl::enable(GL_DEPTH_TEST);
    gl::enable(GL_CULL_FACE);

    imgui gui(window.handle);

    vector<gl::texture> textures = make_textures();
    vector<gl::mesh> meshes = make_meshes();

    /* --- entities --- */
    entt::registry registry;
    create_entities(registry);
    vector<vector<instance_data>> instance_groups(meshes.size());
    for (auto entity : registry.view<model_component, mesh_component>()) {
        auto &model = registry.get<model_component>(entity);
        instance_data data = {
            .normal_matrix = model.normal_matrix,
            .model_matrix = model.model_matrix
        };
        if (registry.all_of<texture_component>(entity)) {
            auto &texture = registry.get<texture_component>(entity);
            data.texture_index = texture.index;
        } else if (registry.all_of<color_component>(entity)) {
            auto &color = registry.get<color_component>(entity);
            data.color = color.value;
            data.texture_index = -1;
        } else {
            logger::warn("entity has neither color nor texture");
            data.color = vec4(1, 0, 0, 1);
        }

        auto &mesh = registry.get<mesh_component>(entity);
        instance_groups[mesh.index].push_back(data);
    }

    vector<instance_data> instances;
    vector<size_t>        instance_group_offsets;
    for (auto &instance_group : instance_groups) {
        instance_group_offsets.push_back(instances.size() * sizeof(instance_data));
        instances.insert(instances.end(), instance_group.begin(), instance_group.end());
    }

    gl::buffer instances_buffer = gl::store(span(instances));
    gl::buffer light_positions_buffer = gl::store(span(light_positions));

    /* --- shaders --- */
    gl::shader vs(GL_VERTEX_SHADER), fs(GL_FRAGMENT_SHADER);
    vs.binary(GL_SHADER_BINARY_FORMAT_SPIR_V, span(_binary_main_vert_glsl_spv_start, _binary_main_vert_glsl_spv_end));
    vs.specialize();
    fs.binary(GL_SHADER_BINARY_FORMAT_SPIR_V, span(_binary_main_frag_glsl_spv_start, _binary_main_frag_glsl_spv_end));
    fs.specialize("main", {
        {constant_texture_count, 2}
    });
    gl::program program;
    program.attach_shader(vs);
    program.attach_shader(fs);
    program.link();
    /* --- */

    /* --- uniforms --- */
    gl::buffer ubo_buffer = gl::malloc(sizeof(uniform_buffer));
    ub = ubo_buffer.data<uniform_buffer>();
    ub->view_matrix = glm::lookAt(vec3(0, 0, 5), vec3(0), vec3(0, 1, 0));
    ub->projection_matrix = glm::perspective(radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.1f, 100.f);
    ub->ambient = .1f;
    ub->diffuse = 1.f;
    ub->specular = .5f;
    ub->specular_power = 8;
    ub->enable_light = 1;
    /* --- */

    gl::bind_uniform_buffer(binding_uniform_buffer, ubo_buffer);
    gl::bind_shader_storage_buffer(binding_instances_data, instances_buffer);
    gl::bind_texture_units(binding_textures, span(textures));
    gl::bind_shader_storage_buffer(binding_light_positions, light_positions_buffer);

    double dt = 0;
    double last_frame_time = 0;
    glfw::set_time(0);
    while (!window.should_close()) {
        glfw::poll_events();
        double now = glfw::get_time();
        dt = now - last_frame_time;
        last_frame_time = now;
        if (camera_enabled) {
            camera.update(dt);
            ub->view_matrix = camera.compute_view_matrix();
            ub->camera_position = camera.position;
        }

        gl::clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::clear_color(screen_color);
        program.use();

        for (size_t i = 0; i < meshes.size(); ++i) {
            if (instance_groups[i].size() == 0)
                continue;
            static_assert(sizeof(instance_data) == 256);
            gl::bind_shader_storage_buffer(
                binding_instances_data,
                instances_buffer,
                instance_group_offsets[i],
                instance_groups[i].size() * sizeof(instance_data)
            );
            meshes[i].draw(gl::DrawMode::Triangles, instance_groups[i].size());
        }

        gui.new_frame(1 / dt, glm::value_ptr(screen_color), &ub->ambient, &ub->diffuse, &ub->specular, &ub->specular_power);
        gui.render();

        window.swap_buffers();
    }

    return 0;
}
