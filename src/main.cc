import std;
import std.compat;

import gl.core;
import gl;
import glfw;
import glm;

import imgui;
import imgui_impl_glfw;
import imgui_impl_opengl3;

import camera;
import geometry;

using std::array;
using std::memcpy;
using std::print;
using std::println;
using std::span;
using std::string;
using std::vector;
using namespace glm;

const string vs_src = R"glsl(#version 460 core

layout (location = 0) in vec3 position;
uniform mat4 model;
layout (std140, binding = 0) uniform camera {
    mat4 view;
    mat4 projection;
};
out vec3 fragment_position;

void main() {
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragment_position = position;
}

)glsl";

const string fs_src = R"glsl(#version 460 core

out vec4 color;
uniform vec4 color_u;
uniform vec3 light_position;
uniform vec3 camera_position;
in vec3 fragment_position;

void main() {
    vec3 dx = dFdx(fragment_position);
    vec3 dy = dFdy(fragment_position);
    vec3 normal = normalize(cross(dx, dy));
    if (!gl_FrontFacing) {
        normal = -normal;
    } 
    
    vec3 light_dir = normalize(light_position - fragment_position);
    vec3 view_dir = normalize(camera_position - fragment_position);

    float diff = max(dot(normal, light_dir), 0.0f) * 0.5;
    
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec_power = 32;
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0f), spec_power);

    float ambient = 0.2;
    color = vec4((ambient + diff + spec) * color_u.rgb, color_u.a);
}

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

mat4 *model, *view, *projection;
lerp_camera camera;
vec4 screen_color = {1, 1, 1, 1};
vec3 scale_v = vec3(0.5f);

const int WIDTH = 800, HEIGHT = 600;
void framebuffer_size_callback(glfw::window, int w, int h) {
    glViewport(0, 0, w, h);
    *projection = glm::perspective(glm::radians(45.0f), (float) w / (float) h, 0.1f, 100.f);
}

bool camera_enabled = true;
vec2 mouse_position = vec2(0, 0);
float camera_sensitivity = 0.1f;
void cursor_callback(glfw::window, double xpos, double ypos) {
    if (camera_enabled) {
        float dx = mouse_position.x - xpos;
        float dy = mouse_position.y - ypos;
        camera.rotate(dx * camera_sensitivity, dy * camera_sensitivity);
        mouse_position.x = (float) xpos;
        mouse_position.y = (float) ypos;
    }
}

void key_callback(glfw::window window, Key key, int, KeyAction action, int) {
    if (action == KeyAction::Press || action == KeyAction::Release) {
        bool to_move = action == KeyAction::Press;
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

    if (action == KeyAction::Press && key == Key::C) {
        camera_enabled = !camera_enabled;
        if (camera_enabled) {
            mouse_position = window.get_cursor_position();
            glfw::set_cursor_mode(window, CursorMode::Disabled);
        } else {
            glfw::set_cursor_mode(window, CursorMode::Captured);
        }
    }

    if (action == KeyAction::Press && key == Key::Escape)
        window.close();
}

vec3 l_pos = vec3(-2, 2, 2);

int main()
{
    glfw::set_default_error_handler();
    glfw::init();
    glfw::window_hint(WindowHint::WaylandAppId, "hello");
    auto window = glfw::create_window(WIDTH, HEIGHT, "glfw");
    glfw::set_current_context(window);
    glfw::swap_interval(1);
    glfw::set_framebuffer_size_cb(window, framebuffer_size_callback);
    if (glfw::is_raw_mouse_motion_supported())
        glfw::set_raw_mouse_motion(window, true);
    glfw::set_key_cb(window, key_callback);
    glfw::set_cursor_mode(window, CursorMode::Disabled);
    glfw::set_cursor_cb(window, cursor_callback);

    imgui gui(window);

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // shaders
    gl::set_default_debug_message_handler();
    gl::shader vs(GL_VERTEX_SHADER), gs(GL_GEOMETRY_SHADER), fs(GL_FRAGMENT_SHADER);
    vs.source(vs_src); glCompileShader(vs);
    fs.source(fs_src); glCompileShader(fs);
    gl::program p;
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    // --- uniform buffers ---
    // matrices
    gl::buffer mat = gl::calloc<mat4, 2>();
    view = mat.data<mat4>();
    projection = view + 1;
    *view = mat4(1.f);
    *view = lookAt(vec3(0, 0, 5), vec3(0), vec3(0, 1, 0));
    *projection = glm::perspective(glm::radians(45.0f), (float) WIDTH / (float) HEIGHT, 0.1f, 100.f);
    mat4 model = mat4(1.f);
    // --- uniforms ---
    vec4 color = vec4(1, 0, 0, 1);
    vec4 l_color = vec4(1);
    int model_uniform_loc = glGetUniformLocation(p, "model");
    int color_uniform_loc = glGetUniformLocation(p, "color_u");
    int light_pos_uniform_loc = glGetUniformLocation(p, "light_position");
    int camera_pos_uniform_loc = glGetUniformLocation(p, "camera_position");

    gl::bind_uniform_buffer(0, mat);
    // ---

    // --- vertex generation ---
    // main figure
    gl::vertex_array va;
    va.enable_attribute(0);
    va.format_attribute(0, 3, GL_FLOAT, GL_FALSE, 0);
    va.bind_attribute(0, 0);
    const vec2 grid = {100, 100};
    auto sphere = [] (float u, float v) {
        float theta = u * 2.0f * pi<float>();
        float phi = v * pi<float>();
        return vec3(
            cos(theta) * sin(phi),
            cos(phi),
            sin(theta) * sin(phi)
        );
    };

    auto torus = [] (float u, float v) {
        float theta = u * 2.0f * pi<float>();
        float phi = v * 2.0f * pi<float>();
        float R = 1.0f;
        float r = 0.5f;
        return vec3(
            (R + r * cos(phi)) * cos(theta),
            (R + r * cos(phi)) * sin(theta),
            r * sin(phi)
        );
    };

    auto helicoid = [] (float u, float v) -> vec3 {
        u *= 2 * pi<float>();
        v *= 2 * pi<float>();
        return {
            u * cos(v),
            u * sin(v),
            1 * v
        };
    };

    vector<vec3> vertices = generate_surface(grid.x, grid.y, sphere);
    vector<int> elements = generate_grid_indices(grid.x, grid.y);

    gl::buffer b = gl::store(span(vertices));
    gl::buffer e = gl::store(span(elements));
    va.bind_vertex_buffer<vec3>(0, b);
    va.bind_element_buffer(e);
    // light source
    gl::vertex_array l_va;
    l_va.enable_attribute(0);
    l_va.format_attribute(0, 3, GL_FLOAT, GL_FALSE, 0);
    l_va.bind_attribute(0, 0);
    auto l_vertices = create_cube();
    auto l_elements = create_cube_triangles();
    gl::buffer l_vertices_buf = gl::store(span(l_vertices));
    gl::buffer l_elements_buf = gl::store(span(l_elements));
    l_va.bind_vertex_buffer<vec3>(0, l_vertices_buf);
    l_va.bind_element_buffer(l_elements_buf);

    // ---
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
            *view = camera.compute_view_matrix();
        }

        gui.new_frame();
        ImGui::ColorEdit4("Screen color", (float*) &screen_color);
        ImGui::ColorEdit4("Cube color", (float*) &color);
        ImGui::SliderFloat("Scale (x)", glm::value_ptr(scale_v), 0.f, 5.f);
        ImGui::SliderFloat("Scale (y)", glm::value_ptr(scale_v) + 1, 0.f, 5.f);
        ImGui::SliderFloat("Scale (z)", glm::value_ptr(scale_v) + 2, 0.f, 5.f);
        ImGui::Text("dt = %f", dt);
        ImGui::Text("fps = %f", 1 / dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        gl::clear_color(screen_color);

        glBindVertexArray(va);

        glUseProgram(p);

        glUniform3fv(light_pos_uniform_loc, 1, glm::value_ptr(l_pos));
        glUniform3fv(camera_pos_uniform_loc, 1, glm::value_ptr(camera.position));

        model = glm::scale(mat4(1), scale_v);
        glUniformMatrix4fv(model_uniform_loc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform4fv(color_uniform_loc, 1, glm::value_ptr(color));
        glDrawElements(GL_TRIANGLES, elements.size(), GL_UNSIGNED_INT, 0);

        mat4 l_model = glm::scale(glm::translate(mat4(1.0f), l_pos), 0.1f * vec3(1));
        glBindVertexArray(l_va);
        glUniformMatrix4fv(model_uniform_loc, 1, GL_FALSE, glm::value_ptr(l_model));
        glUniform4fv(color_uniform_loc, 1, glm::value_ptr(l_color));
        glDrawElements(GL_TRIANGLES, l_elements.size() * 3, GL_UNSIGNED_INT, 0);

        gui.render();

        window.swap_buffers();
    }
}
