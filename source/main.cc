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

extern const uint8_t _binary_shader_vert_glsl_spv_start[];
extern const uint8_t _binary_shader_vert_glsl_spv_end[];

extern const uint8_t _binary_shader_frag_glsl_spv_start[];
extern const uint8_t _binary_shader_frag_glsl_spv_end[];
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
    binding_instance_data_array,
    binding_light_position_array,
    binding_textures
};

/* binding_instance_data : std430 ssbo, array of */
struct alignas(vec4) instance_data {
    mat4 normal_matrix;
    mat4 model;
    vec4 color;
    int texture_index;
    uint32_t _[3];
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

/* --- components --- */
namespace object_type {
    struct main {};
    struct light {};
    struct room {};
};
struct normal_mat { mat4 value; };
struct model { mat4 value; };
struct color { vec4 value; };

void create_entities(entt::registry &registry) {
    /* --- entity creation --- */
    for (int i = 0; i < 1; ++i) {
        auto entity = registry.create();
        registry.emplace<object_type::main>(entity);
        registry.emplace<color>(entity, vec4(1, 0, 0, 1));
        registry.emplace<model>(entity, mat4(1.f));
        registry.patch<model>(entity, [] (auto &m) { m.value = translate(m.value, vec3(-1)); });
        registry.patch<model>(entity, [] (auto &m) { m.value = scale(m.value, vec3(1.5f)); });
    }
    vector<vec3> light_positions = {vec3(2)};
    for (auto & position : light_positions) {
        auto light = registry.create();
        registry.emplace<object_type::light>(light);
        registry.emplace<color>(light, vec4(1.f));
        registry.emplace<model>(light, mat4(1.f));
        registry.patch<model>(light, [&] (auto &m) { m.value = translate(m.value, position); });
        registry.patch<model>(light, [] (auto &m) { m.value = scale(m.value, vec3(0.2f)); });
    }

    auto room = registry.create();
    registry.emplace<object_type::room>(room);
    registry.emplace<color>(room, vec4(vec3(0.1), 1));
    registry.emplace<model>(room, mat4(1.f));
    registry.patch<model>(room, [] (auto &m) { m.value = scale(m.value, vec3(5)); });

    /* --- normal matrix generation --- */
    for (auto view = registry.view<model>(); auto entity: view)
        registry.emplace<normal_mat>(entity, transpose(inverse(view.get<model>(entity).value)));
}

template<typename T>
vector<instance_data> make_instances_of(entt::registry &registry) {
    vector<instance_data> instances = {};
    for (auto view = registry.view<normal_mat, model, color, T>(); auto entity: view) {
        auto [n, m, c] = view.get(entity);
        instances.push_back(instance_data(n.value, m.value, c.value, -1));
    }
    return instances;
}

vector<vec4> get_light_positions(entt::registry &registry) {
    vector<vec4> positions = {};
    for (auto view = registry.view<model, object_type::light>(); auto entity: view) {
        auto [m] = view.get(entity);
        vec3 position = m.value[3];
        positions.push_back(vec4(position.x, position.y, position.z, 0.0f));
    }
    return positions;
}

template<size_t N>
vector<gl::texture> load_textures(array<path, N> paths) {
    vector<gl::texture> textures;
    textures.reserve(N);
    uint8_t * pixels;
    int x, y, channels;
    stbi_set_flip_vertically_on_load(false);
    for (size_t i = 0; i < N; ++i) {
        logger::info("path = {}", paths[i].c_str());
        pixels = stbi_load(paths[i].c_str(), &x, &y, &channels, STBI_default);
        if (pixels == nullptr)
            logger::error("texture data = nullptr");
        textures.push_back(gl::make_texture(pixels, x, y, channels));
        stbi_image_free(pixels);
    }
    return textures;
}

/* --- */
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

    imgui gui(window.handle);

    gl::load(glfw::get_proc_address);
    gl::set_default_debug_message_handler();
    gl::enable(GL_DEBUG_OUTPUT);
    gl::enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    gl::enable(GL_DEPTH_TEST);
    gl::enable(GL_CULL_FACE);

    int m;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &m);
    logger::info("GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS = {}", m);

    /* --- shaders --- */
    gl::shader vs(GL_VERTEX_SHADER), fs(GL_FRAGMENT_SHADER);
    vs.binary(GL_SHADER_BINARY_FORMAT_SPIR_V, span(_binary_shader_vert_glsl_spv_start, _binary_shader_vert_glsl_spv_end));
    vs.specialize();
    fs.binary(GL_SHADER_BINARY_FORMAT_SPIR_V, span(_binary_shader_frag_glsl_spv_start, _binary_shader_frag_glsl_spv_end));
    fs.specialize();
    gl::program program;
    program.attach_shader(vs);
    program.attach_shader(fs);
    program.link();
    /* --- */

    entt::registry registry;
    create_entities(registry);
    vector<instance_data> main_instances = make_instances_of<object_type::main>(registry);
    vector<instance_data> light_instances = make_instances_of<object_type::light>(registry);
    vector<vec4> light_positions = get_light_positions(registry);
    vector<instance_data> room_instances = make_instances_of<object_type::room>(registry);

    /* --- textures --- */
    array<path, 2> texture_filenames = {"8k_stars.jpg", "8k_earth_daymap.jpg"};
    for (path &f : texture_filenames)
        f = "/home/andrew/Source/geometry++/assets" / f;
    vector<gl::texture> textures = load_textures(texture_filenames);
#if 0
    gl::make_texture_handle_resident(textures[0].get_handle());
    gl::make_texture_handle_resident(textures[1].get_handle());
#endif
    for (auto &data : room_instances)
        data.texture_index = 0;
    for (auto &data : main_instances)
        data.texture_index = 1;

    gl::bind_texture_units(binding_textures, span(textures));
    /* --- */

    gl::buffer main_instances_buffer  = gl::store(span(main_instances));
    gl::buffer light_instances_buffer = gl::store(span(light_instances));
    gl::buffer room_instances_buffer  = gl::store(span(room_instances));

    gl::buffer light_positions_buffer = gl::store(span(light_positions));
    gl::bind_shader_storage_buffer(binding_light_position_array, light_positions_buffer);

    /* --- uniform buffer --- */
    gl::buffer ubo_buffer = gl::malloc(sizeof(uniform_buffer));
    ub = ubo_buffer.data<uniform_buffer>();
    ub->view_matrix = glm::lookAt(vec3(0, 0, 5), vec3(0), vec3(0, 1, 0));
    ub->projection_matrix = glm::perspective(radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.1f, 100.f);
    ub->ambient = .1f;
    ub->diffuse = 1.f;
    ub->specular = .5f;
    ub->specular_power = 8;
    ub->enable_light = false;
    gl::bind_uniform_buffer(binding_uniform_buffer, ubo_buffer);
    /* --- */

    /* --- mesh generation --- */
    gl::mesh light_m = gl::make_mesh(
        generate_grid_indices(32, 32),
        {
            generate_surface(32, 32, sphere),
        }
    );

    int w = 64, h = 64;
    gl::mesh main_m = gl::make_mesh(
        generate_grid_indices(w, h),
        generate_surface(w, h, sphere),
        generate_normals(w, h, sphere),
        generate_texcoords(w, h)
    );

    auto cube = create_cube_cw();
    gl::mesh walls_m = gl::make_mesh(cube.indices, cube.positions, cube.normals, cube.texcoords);
    /* --- */

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
        ub->enable_light = true;
        gl::bind_shader_storage_buffer(binding_instance_data_array, main_instances_buffer);
        main_m.draw(gl::DrawMode::Triangles, main_instances.size());

        gl::bind_shader_storage_buffer(binding_instance_data_array, room_instances_buffer);
        walls_m.draw(gl::DrawMode::Triangles);

        gl::bind_shader_storage_buffer(binding_instance_data_array, light_instances_buffer);
        light_m.draw(gl::DrawMode::Triangles, light_instances.size());

        gui.new_frame(1 / dt, glm::value_ptr(screen_color), &ub->ambient, &ub->diffuse, &ub->specular, &ub->specular_power);
        gui.render();

        window.swap_buffers();
    }

#if 0
    for (auto &t : textures)
        gl::make_texture_handle_non_resident(t.get_handle());
#endif

    return 0;
}
