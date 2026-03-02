module;
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

export module glfw;

import std;
import glm;

import glfw.core;

using std::println;
using std::to_underlying;
using std::format;
using std::is_same_v;
using std::decay_t;
using std::string;
using std::string_view;
using std::unordered_map;
using std::variant;
using std::function;
using std::runtime_error;
using namespace glm;

export namespace glfw
{
    struct error: runtime_error {
        error(const char * description)
            : runtime_error(description) {}
    };

    void error_callback(int, const char * description) {
        throw error(description);
    }

    struct window_view {
        GLFWwindow * handle;

        bool should_close() {
            return glfwWindowShouldClose(handle);
        }

        void close() {
            glfwSetWindowShouldClose(handle, GLFW_TRUE);
        }

        void swap_buffers() {
            glfwSwapBuffers(handle);
        }

        vec2 get_cursor_position() {
            double xpos, ypos;
            glfwGetCursorPos(handle, &xpos, &ypos);
            return vec2((float) xpos, (float) ypos);
        }

        void on_key(void (*cb) (window_view, Key, int, Action, Modifier)) {
            glfwSetKeyCallback(handle, (GLFWkeyfun) cb);
        }

        void on_cursor(void (*cb) (window_view, double, double)) {
            glfwSetCursorPosCallback(handle, (GLFWcursorposfun) cb);
        }

        void on_framebuffer_resize(void (*cb) (window_view, int, int)) {
            glfwSetFramebufferSizeCallback(handle, (GLFWframebuffersizefun) cb);
        }

        void set_cursor_mode(CursorMode mode) {
            glfwSetInputMode(handle, GLFW_CURSOR, (int) mode);
        }

        void set_raw_mouse_motion(bool value) {
            glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, value);
        }

    };

    struct window: public window_view {
        window(GLFWwindow * handle)
            : window_view(handle) {}

        ~window() {
            glfwDestroyWindow(handle);
            glfwTerminate();
        }

        window(const window &) = delete;
    };

    void set_current_context(window & window) {
        glfwMakeContextCurrent(window.handle);
    }

    void set_default_error_handler() {
        glfwSetErrorCallback(error_callback);
    }

    window create_window(
        int width,
        int height,
        string_view title,
        unordered_map<WindowHint, variant<int, string>> hints = {}
    ) {
        if (!glfwInit())
            throw runtime_error("Could not initialize window framework");
        for (const auto & [key, value] : hints) {
            value.visit(
                [&] <typename T> (T && arg) {
                    if constexpr (is_same_v<decay_t<T>, int>) {
                        glfwWindowHint(to_underlying(key), arg);
                    } else if constexpr (is_same_v<decay_t<T>, string &&>) {
                        glfwWindowHintString(to_underlying(key), arg.c_str());
                    }
                }
            );

        }

        GLFWwindow * window = glfwCreateWindow(width, height, title.data(), glfwGetPrimaryMonitor(), nullptr);
        if (window == nullptr)
            throw runtime_error("Could not create window");
        return glfw::window(window);
    }

    void set_time(double time) {
        glfwSetTime(time);
    }

    double get_time() {
        return glfwGetTime();
    }

    void poll_events() {
        glfwPollEvents();
    }

    void swap_interval(int interval) {
        glfwSwapInterval(interval);
    }

    bool is_raw_mouse_motion_supported() {
        return glfwRawMouseMotionSupported();
    }
};
