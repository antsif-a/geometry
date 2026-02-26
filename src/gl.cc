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
using glm::vec4;

//namespace {
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
//}

export namespace gl
{
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
        if (severity == GL_DEBUG_SEVERITY_HIGH)
            throw error(source, type, id, string_view(message, length));
        println("gl: {}", message);
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

        // delete copy constructor and move assignment
        object(const object &) = delete;
        object & operator=(const object &) = delete;

        // explicitly define move constructor... 
        object(object && other) : name(other.name) {
            other.name = 0;
        }

        // ...and move assignment
        object & operator=(object && other) {
            if (this != other) {
                destructor(name);
                name = other.name;
                other.name = 0;
            }

            return *this;
        }

        ~object() {
            destructor(name);
        }

        operator GLuint() {
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

    struct buffer: buffer_t {
        using buffer_t::object;

        template<typename T>
        T * data(GLenum access = DEFAULT_MAP_ACCESS) {
            return static_cast<T *>(glMapNamedBuffer(name, access));
        }
    };
    struct vertex_array: vertex_array_t {
        void bind_vertex_buffer(GLuint index, buffer & buffer, GLintptr offset, GLsizei stride) {
            glVertexArrayVertexBuffer(name, index, buffer, offset, stride);
        }

        template<typename Vertex>
        void bind_vertex_buffer(GLuint index, buffer & buffer, GLintptr offset = 0) {
            glVertexArrayVertexBuffer(name, index, buffer, offset, sizeof(Vertex));
        }

        void bind_element_buffer(buffer & buffer) {
            glVertexArrayElementBuffer(name, buffer);
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
    struct texture: texture_t {};
    struct framebuffer: framebuffer_t {};
    struct shader: shader_t {
        shader(GLenum type) : shader_t(glCreateShader(type)) {}

        void source(string_view src) {
            GLsizei size = src.size();
            const GLchar * data = src.data();
            glShaderSource(name, 1, &data, &size);
        }
    };

    struct program: program_t {};

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
        return buffer(b);
    }

    template<typename T, size_t Extent>
    buffer calloc(GLbitfield flags = DEFAULT_BUFFER_ALLOC_FLAGS) {
        GLuint b = glCreateBuffer();
        glNamedBufferStorage(b, sizeof(T) * Extent, nullptr, flags);
        return buffer(b);
    }
    
    template<typename T, size_t Extent>
    buffer store(span<T, Extent> data, GLbitfield flags = DEFAULT_BUFFER_STORAGE_FLAGS) {
        GLuint b = glCreateBuffer();
        glNamedBufferStorage(b, data.size_bytes(), data.data(), flags);
        return buffer(b);
    }

    void bind_uniform_buffer(GLuint index, buffer & buffer) {
        glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer);
    }

    void clear_color(vec4 color) {
        glClearColor(color.x, color.y, color.z, color.w);
    }
}
