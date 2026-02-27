module;
#include <cassert>

export module gl;

import std;
import glm;
import gl.core;

using std::println;
using std::format;
using std::string;
using std::string_view;
using std::span;
using std::array;
using std::runtime_error;
using std::size_t;
using std::is_same_v;
using std::to_underlying;

using glm::vec;
using glm::vec4;

namespace gl {
    #define X(F) \
        GLuint F() { GLuint name; F##s(1, &name); return name; }
    X(glCreateBuffer)
    X(glCreateVertexArray)
    X(glCreateFramebuffer)
    #undef X

    GLuint glCreateTexture(GLenum target) {
        GLuint texture;
        glCreateTextures(target, 1, &texture);
        return texture;
    }

    void glDeleteBuffer(GLuint name) { glDeleteBuffers(1, &name); }
    void glDeleteVertexArray(GLuint name) { glDeleteVertexArrays(1, &name); }
    void glDeleteTexture(GLuint name) { glDeleteTextures(1, &name); }
    void glDeleteFramebuffer(GLuint name) { glDeleteFramebuffers(1, &name); }
}

export namespace gl
{
    enum class DrawMode : GLenum {
        Points = GL_POINTS,
        LineStrip = GL_LINE_STRIP,
        LineStripAdjacency = GL_LINE_STRIP_ADJACENCY,
        LineLoop = GL_LINE_LOOP,
        Lines = GL_LINES,
        LinesAdjacency = GL_LINES_ADJACENCY,
        TriangleStrip = GL_TRIANGLE_STRIP,
        TriangleStripAdjacency = GL_TRIANGLE_STRIP_ADJACENCY,
        TriangleFan = GL_TRIANGLE_FAN,
        Triangles = GL_TRIANGLES,
        TrianglesAdjacency = GL_TRIANGLES_ADJACENCY,
        Patches = GL_PATCHES
    };

    struct error: runtime_error {
        error(GLenum, GLenum, GLuint, string_view message)
            : runtime_error(string(message)) {}
    };

    void debug_message_handler(
        GLenum source,
        GLenum type,
        GLuint id,
        GLenum severity,
        GLsizei length,
        const GLchar * message,
        const void *
    ) {
        println("gl: {}", message);
        assert(severity != GL_DEBUG_SEVERITY_HIGH);
    }

    void set_default_debug_message_handler() {
        glDebugMessageCallback(debug_message_handler, nullptr);
    }

    template<auto constructor, auto destructor>
    struct object
    {
        GLuint name;

        object(GLuint name)
           : name(name) {}

        object()
           : name(constructor()) {}

        /* delete copy constructor and move assignment */
        object(const object &) = delete;
        object & operator=(const object &) = delete;

        /* explicitly define move constructor... */
        object(object && other) : name(other.name) {
            other.name = 0;
        }

        /* ...and move assignment */
        object & operator=(object && other) {
            if (name != other.name) {
                destructor(name);
                name = other.name;
                other.name = 0;
            }

            return *this;
        }

        ~object() {
            if (name != 0)
                destructor(name);
        }


        explicit operator GLuint() {
            return name;
        }
    };

    #define object_t(T) object<glCreate##T, glDelete##T>

    using buffer_t = object_t(Buffer);
    using vertex_array_t = object_t(VertexArray);
    using texture_t = object_t(Texture);
    using framebuffer_t = object_t(Framebuffer);
    using shader_t = object_t(Shader);
    using program_t = object_t(Program);

    #undef object_t

    constexpr GLbitfield DEFAULT_BUFFER_ALLOC_FLAGS = \
        GL_MAP_PERSISTENT_BIT                    \
        | GL_MAP_COHERENT_BIT                    \
        | GL_MAP_READ_BIT                        \
        | GL_MAP_WRITE_BIT;
    constexpr GLbitfield DEFAULT_BUFFER_STORAGE_FLAGS = \
        GL_DYNAMIC_STORAGE_BIT;

    constexpr GLenum DEFAULT_MAP_ACCESS = \
        GL_READ_WRITE;

    /* === native objects === */

    /* --- buffer --- */
    struct buffer: buffer_t {
        using buffer_t::object;

        template<typename T>
        T * data(GLenum access = DEFAULT_MAP_ACCESS) {
            return static_cast<T *>(glMapNamedBuffer(name, access));
        }
    };

    buffer malloc(GLsizei size, GLbitfield flags = DEFAULT_BUFFER_ALLOC_FLAGS) {
        GLuint b = glCreateBuffer();
        glNamedBufferStorage(b, size, nullptr, flags);
        return buffer(b);
    }

    template<typename T>
    buffer malloc(GLbitfield flags = DEFAULT_BUFFER_ALLOC_FLAGS) {
        GLuint b = glCreateBuffer();
        glNamedBufferStorage(b, sizeof(T), nullptr, flags);
        return buffer(b);
    }

    buffer calloc(size_t n, GLsizei size, GLbitfield flags = DEFAULT_BUFFER_ALLOC_FLAGS) {
        GLuint b = glCreateBuffer();
        glNamedBufferStorage(b, n * size, nullptr, flags);
        glClearNamedBufferData(b, GL_R8, GL_R8, GL_UNSIGNED_BYTE, nullptr);
        return buffer(b);
    }

    template<typename T, size_t Extent>
    buffer calloc(GLbitfield flags = DEFAULT_BUFFER_ALLOC_FLAGS) {
        GLuint b = glCreateBuffer();
        glNamedBufferStorage(b, sizeof(T) * Extent, nullptr, flags);
        glClearNamedBufferData(b, GL_R8, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        return buffer(b);
    }

    template<typename T, size_t Extent>
    buffer store(span<T, Extent> data, GLbitfield flags = DEFAULT_BUFFER_STORAGE_FLAGS) {
        GLuint b = glCreateBuffer();
        glNamedBufferStorage(b, data.size_bytes(), data.data(), flags);
        return buffer(b);
    }
    /* --- */

    /* --- vertex array --- */
    struct vertex_array: vertex_array_t {
        void bind_vertex_buffer(GLuint index, buffer & buffer, GLintptr offset, GLsizei stride) {
            glVertexArrayVertexBuffer(name, index, buffer.name, offset, stride);
        }

        template<typename Vertex>
        void bind_vertex_buffer(GLuint index, buffer & buffer, GLintptr offset = 0) {
            glVertexArrayVertexBuffer(name, index, buffer.name, offset, sizeof(Vertex));
        }

        void bind_element_buffer(buffer & buffer) {
            glVertexArrayElementBuffer(name, buffer.name);
        }

        void enable_attribute(GLuint index) {
            glEnableVertexArrayAttrib(name, index);
        }

        void format_attribute(
            GLuint index,
            GLint size,
            GLenum type,
            GLboolean normalized,
            GLuint relativeoffset
        ) {
            glVertexArrayAttribFormat(name, index, size, type, normalized, relativeoffset);
        }

        void bind_attribute(GLuint attribute_index, GLuint binding_index) {
            glVertexArrayAttribBinding(name, attribute_index, binding_index);
        }
    };
    /* --- */

    struct texture: texture_t {};
    struct framebuffer: framebuffer_t {};

    /* --- shader --- */
    struct shader: shader_t {
        shader(GLenum type) : shader_t(glCreateShader(type)) {}

        void source(string_view src) {
            GLsizei size = static_cast<GLsizei>(src.size());
            const GLchar * data = src.data();
            glShaderSource(name, 1, &data, &size);
        }

        void compile() {
            glCompileShader(name);
        }
    };
    /* --- */

    /* --- program --- */
    struct program: program_t {
        void attach_shader(shader & shader) {
           glAttachShader(name, shader.name);
        }

        void link() {
            glLinkProgram(name);
        }

        void use() {
            glUseProgram(name);
        }

        GLuint get_uniform_location(const char * uniform_name) {
            return glGetUniformLocation(name, uniform_name);
        }
    };
    /* --- */

    /* === */

    /* === "derived" objects === */

    /* --- mesh --- */
    template<typename T> struct element_type;
    template<> struct element_type<GLubyte>
        { static constexpr GLenum value = GL_UNSIGNED_BYTE;  };
    template<> struct element_type<GLushort>
        { static constexpr GLenum value = GL_UNSIGNED_SHORT; };
    template<> struct element_type<GLuint>
        { static constexpr GLenum value = GL_UNSIGNED_INT;   };

    template<typename T>
    concept is_element_type = \
        is_same_v<T, GLubyte> || is_same_v<T, GLushort> || is_same_v<T, GLuint>;

    struct mesh {
        vertex_array va;
        buffer vb;
        buffer eb;
        size_t count;
        GLenum type;
        size_t offset;

        void draw(DrawMode mode) {
            glBindVertexArray(va.name);
            glDrawElements(to_underlying(mode), count, type, reinterpret_cast<const void *>(offset));
        }
    };

    /* "vertices" must contain normalized "vec" elements */
    template<int L, size_t VertexCount, typename ElementType, size_t ElementCount>
    requires is_element_type<ElementType>
    mesh make_mesh(
        span<vec<L, float>, VertexCount> vertices,
        span<ElementType, ElementCount> elements
    ) {
        buffer vb = store(vertices); int vb_bind_index = 0;
        buffer eb = store(elements);
        vertex_array va; int va_attr_index = 0;
        va.bind_vertex_buffer<vec<L, float>>(vb_bind_index, vb);
        va.bind_element_buffer(eb);

        va.enable_attribute(va_attr_index);
        va.format_attribute(va_attr_index, L, GL_FLOAT, GL_FALSE, 0);
        va.bind_attribute(va_attr_index, vb_bind_index);

        size_t count = elements.size();
        GLenum type = element_type<ElementType>::value;
        size_t offset = 0;

        return { std::move(va), std::move(vb), std::move(eb), count, type, offset };
    }
    /* --- */

    /* === */

    void bind_uniform_buffer(GLuint index, buffer & buffer) {
        glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.name);
    }

    void clear_color(vec4 color) {
        glClearColor(color.x, color.y, color.z, color.w);
    }
}
