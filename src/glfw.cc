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
using std::string;
using std::string_view;
using std::unordered_map;
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

    void init() {
        if (!glfwInit())
            throw runtime_error("Could not initialize window framework");
    }

    struct window {
        window(GLFWwindow * handle)
            : handle(handle) {}

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

        void on_key(void (*cb) (glfw::window, Key, int, Action, Modifier)) {
            glfwSetKeyCallback(handle, (GLFWkeyfun) cb);
        }

        void on_cursor(void (*cb) (window, double, double)) {
            glfwSetCursorPosCallback(handle, (GLFWcursorposfun) cb);
        }

        void on_framebuffer_resize(void (*cb) (window, int, int)) {
            glfwSetFramebufferSizeCallback(handle, (GLFWframebuffersizefun) cb);
        }

        void set_cursor_mode(CursorMode mode) {
            glfwSetInputMode(handle, GLFW_CURSOR, (int) mode);
        }

        void set_raw_mouse_motion(bool value) {
            glfwSetInputMode(handle, GLFW_RAW_MOUSE_MOTION, value);
        }

        operator GLFWwindow * () {
            return handle;
        }

        GLFWwindow * handle;
    };

    void set_current_context(window window) {
        glfwMakeContextCurrent(window);
    }

    void set_default_error_handler() {
        glfwSetErrorCallback(error_callback);
    }

    void window_hint(WindowHint hint, int value) {
        glfwWindowHint(to_underlying(hint), value);
    }

    void window_hint(WindowHint hint, const char * value) {
        glfwWindowHintString(to_underlying(hint), value);
    }

    window create_window(
        int w,
        int h,
        string_view title,
        GLFWmonitor * monitor = nullptr,
        GLFWwindow * share = nullptr
    ) {
        GLFWwindow * window = glfwCreateWindow(w, h, title.data(), monitor, share);
        if (!window)
            throw runtime_error("Could not create window");
        return window;
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
