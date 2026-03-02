#include <cstdlib>
#include <entt/entity/registry.hpp>

import std;
import std.compat;

import gl.core;
import gl;
import glfw;
import glfw.core;
import glm;

import imgui;
import imgui_impl_glfw;
import imgui_impl_opengl3;

import camera;
import geometry;

using std::array;
using std::span;
using std::srand, std::rand, std::time;
using std::string;
using std::vector;
using namespace glm;

float random(float from, float to) {
    return (float(rand()) / RAND_MAX) * (to - from) + from;
}

const string vs_src = R"glsl(#version 460 core
/* --- */
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

layout (std140, binding = 0) uniform Globals {
    mat4 view;
    mat4 projection;
};

struct instance_data {
    mat4 normal_matrix;
    mat4 model;
    vec4 color;
};

layout (std430, binding = 1) buffer InstanceData {
    instance_data instances[];
};

out vec3 fragment_position;
out vec3 fragment_normal;
out flat vec4 fragment_color;

void main() {
    instance_data data = instances[gl_InstanceID];
    vec4 world_position = data.model * vec4(position, 1.0);
    fragment_position = world_position.xyz;
    fragment_normal = mat3(data.normal_matrix) * normal;
    fragment_color = data.color;
    gl_Position = projection * view * world_position;
}
/* --- */
)glsl";

const string fs_src = R"glsl(#version 460 core
/* --- */
out vec4 color;
in vec3 fragment_position;
in vec3 fragment_normal;
in flat vec4 fragment_color;

uniform vec3 light_position;
uniform vec3 camera_position;
uniform bool calculate_normals;
uniform bool calculate_light;
uniform float specular;
uniform int specular_pow;
uniform float ambient;
uniform float diffuse;

void main() {
    vec3 normal;
    if (calculate_normals) {
        normal = normalize(cross(dFdx(fragment_position), dFdy(fragment_position)));
    } else {
        normal = normalize(fragment_normal);
    }

    if (calculate_light) {
        vec3 light_dir = normalize(light_position - fragment_position);
        vec3 view_dir = normalize(camera_position - fragment_position);
        float diff = max(dot(normal, light_dir), 0.0f) * diffuse;

        vec3 reflect_dir = reflect(-light_dir, normal);
        float spec = pow(max(dot(view_dir, reflect_dir), 0.0f), specular_pow) * specular;

        color = vec4((ambient + diff + spec) * fragment_color.rgb, fragment_color.a);
    } else {
        color = fragment_color;
    }
}
/* --- */
)glsl";

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

    void new_frame() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void render() {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

mat4 *view_matrix, *projection_matrix;
lerp_camera camera;
vec4 screen_color = vec4(0);
vec3 scale_v = vec3(0.5f);

const int WIDTH = 800, HEIGHT = 600;
void framebuffer_size_callback(glfw::window_view, int w, int h) {
    glViewport(0, 0, w, h);
    *projection_matrix = glm::perspective(glm::radians(45.0f), (float) w / (float) h, 0.1f, 100.f);
}

bool camera_enabled = true;
vec2 mouse_position = vec2(0, 0);
float camera_sensitivity = 0.1f;

void cursor_callback(glfw::window_view, double xpos, double ypos) {
    if (camera_enabled) {
        float dx = mouse_position.x - xpos;
        float dy = mouse_position.y - ypos;
        camera.rotate(dx * camera_sensitivity, dy * camera_sensitivity);
        mouse_position.x = (float) xpos;
        mouse_position.y = (float) ypos;
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

struct instance_data {
    mat4 normal_matrix;
    mat4 model;
    vec4 color;
};

auto light_position = vec3(-1.2f, 1.1f, 1.3f) * 2;
void create_entities(entt::registry & registry) {
    srand(time(0));
    /* --- entity creation --- */
    for (float x = -10; x < 10; ++x) {
        for (float y = -10; y < 10; ++y) {
            for (float z = -10; z < 10; ++z) {
                auto entity = registry.create();
                registry.emplace<object_type::main>(entity);
                registry.emplace<color>(entity, vec4(1, 0, 0, 1));
                registry.emplace<model>(entity, mat4(1.f));
                registry.patch<model>(entity, [&] (auto &m) { m.value = translate(m.value, vec3(x, y, z)); });
                registry.patch<model>(entity, [&] (auto &m) { m.value = scale(m.value, vec3(0.5f)); });
            }
        }
    }
    auto light = registry.create();
    registry.emplace<object_type::light>(light);
    registry.emplace<color>(light, vec4(1.f));
    registry.emplace<model>(light, mat4(1.f));
    registry.patch<model>(light, [] (auto &m) { m.value = translate(m.value, light_position); });
    registry.patch<model>(light, [] (auto &m) { m.value = scale(m.value, vec3(0.2f)); });

    auto room = registry.create();
    registry.emplace<object_type::room>(room);
    registry.emplace<color>(room, vec4(vec3(0.3), 1));
    registry.emplace<model>(room, mat4(1.f));
    registry.patch<model>(room, [] (auto &m) { m.value = scale(m.value, vec3(3)); });

    /* --- normal matrix generation --- */
    for (auto view = registry.view<model>(); auto entity: view)
        registry.emplace<normal_mat>(entity, transpose(inverse(view.get<model>(entity).value)));
}

template<typename T>
vector<instance_data> make_instances_of(entt::registry & registry) {
    vector<instance_data> instances = {};
    for (auto view = registry.view<normal_mat, model, color, T>(); auto entity: view) {
        auto [n, m, c] = view.get(entity);
        instances.push_back(instance_data(n.value, m.value, c.value));
    }
    return instances;
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

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    /* --- shaders --- */
    gl::set_default_debug_message_handler();
    gl::shader vs(GL_VERTEX_SHADER), fs(GL_FRAGMENT_SHADER);
    vs.source(vs_src); vs.compile();
    fs.source(fs_src); fs.compile();
    gl::program p;
    p.attach_shader(vs);
    p.attach_shader(fs);
    p.link();
    /* --- */

    /* --- uniform buffers --- */
    gl::buffer mat = gl::calloc<mat4, 2>();
    view_matrix = mat.data<mat4>();
    projection_matrix = view_matrix + 1;
    *view_matrix = lookAt(vec3(0, 0, 5), vec3(0), vec3(0, 1, 0));
    *projection_matrix = glm::perspective(glm::radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.1f, 100.f);
    mat4 model_matrix = mat4(1.f);
    mat3 normal_matrix = mat3(1.f);
    gl::bind_uniform_buffer(0, mat);
    /* --- */

    /* --- uniforms --- */
    GLfloat ambient = 0.1f;
    GLfloat diffuse = 0.5f;
    GLfloat specular = 0.4f;
    GLint specular_pow = 32;
    /* --- */

    /* --- mesh generation --- */
    gl::mesh light_m;
    {
        const vec2 grid = {32, 32};
        auto vertices = generate_surface(grid.x, grid.y, sphere);
        auto elements = generate_grid_indices(grid.x, grid.y);
        auto mesh = gl::make_mesh(span(vertices), span(elements));
        light_m = std::move(mesh);
    }

    gl::mesh_with_normals main_m;
    {
        const vec2 grid = {32, 32};
        auto surface = generate_surface(grid.x, grid.y, sphere);
        auto normals = generate_normals(grid.x, grid.y, sphere);
        auto elements = generate_grid_indices(grid.x, grid.y);
        auto mesh = gl::make_mesh_with_normals(span(surface), span(normals), span(elements));
        main_m = std::move(mesh);
    }

    gl::mesh_with_normals walls_m;
    {
        auto cube = create_cube_cw();
        auto mesh = gl::make_mesh_with_normals(span(cube.positions), span(cube.normals), span(cube.indices));
        walls_m = std::move(mesh);
    }
    /* --- */

    entt::registry registry;
    create_entities(registry);
    vector<instance_data> main_instances = make_instances_of<object_type::main>(registry);
    vector<instance_data> light_instances = make_instances_of<object_type::light>(registry);
    vector<instance_data> room_instances = make_instances_of<object_type::room>(registry);
    gl::buffer mains_buf = gl::store(span(main_instances));
    gl::buffer lights_buf = gl::store(span(light_instances));
    gl::buffer rooms_buf = gl::store(span(room_instances));

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
            *view_matrix = camera.compute_view_matrix();
        }

        gui.new_frame();
        ImGui::ColorEdit4("Screen color", glm::value_ptr(screen_color));
        // ImGui::ColorEdit4("Cube color", glm::value_ptr(color));
        // ImGui::SliderFloat("Scale (x)", glm::value_ptr(scale_v), 0.f, 5.f);
        // ImGui::SliderFloat("Scale (y)", glm::value_ptr(scale_v) + 1, 0.f, 5.f);
        // ImGui::SliderFloat("Scale (z)", glm::value_ptr(scale_v) + 2, 0.f, 5.f);
        ImGui::SliderFloat("Ambient", &ambient, 0.f, 1.f);
        ImGui::SliderFloat("Diffuse", &diffuse, 0.f, 1.f);
        ImGui::SliderFloat("Specular", &specular, 0.f, 1.f);
        ImGui::SliderInt("Specular Power", &specular_pow, 1, 128);
        ImGui::Text("fps = %f", 1 / dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::clear_color(screen_color);

        p.use();
        p.uniform("light_position", camera.position);
        p.uniform("camera_position", camera.position);
        p.uniform("ambient", ambient);
        p.uniform("diffuse", diffuse);
        p.uniform("specular", specular);
        p.uniform("specular_pow", specular_pow);

        p.uniform("calculate_light", true);
        p.uniform("calculate_normals", false);
        gl::bind_shader_storage_buffer(1, mains_buf);
        main_m.draw(gl::DrawMode::Triangles, main_instances.size());

        p.uniform("calculate_light", false);
        p.uniform("calculate_normals", true);
        gl::bind_shader_storage_buffer(1, lights_buf);
        light_m.draw(gl::DrawMode::Triangles);

        p.uniform("calculate_light", true);
        p.uniform("calculate_normals", false);
        gl::bind_shader_storage_buffer(1, rooms_buf);
        walls_m.draw(gl::DrawMode::Triangles);


        gui.render();

        window.swap_buffers();
    }

    return 0;
}
