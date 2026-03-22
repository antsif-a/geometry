module;
#include <cassert>
#define GL_GLEXT_PROTOTYPES
#include <GL/glcorearb.h>

export module gl;

import std;
import glm;
import logger;

using std::println;
using std::to_underlying;
using std::find_if;
using std::isspace;
using std::array;
using std::convertible_to;
using std::vector;
using std::format;
using std::is_same_v;
using std::runtime_error;
using std::size_t;
using std::span;
using std::string;
using std::string_view;
using std::map;
using std::unordered_map;

using namespace glm;

/* === opengl function loader === */
/* TODO load all functions */
#define FOR_EACH_FUNCTION(X) \
    X(PFNGLGETTEXTUREHANDLEARBPROC, glGetTextureHandleARB); \
    X(PFNGLMAKETEXTUREHANDLERESIDENTARBPROC, glMakeTextureHandleResidentARB); \
    X(PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC, glMakeTextureHandleNonResidentARB); \
    X(PFNGLSPECIALIZESHADERPROC, glSpecializeShader);

export namespace gl {
    #define X(T, name) T name;
    FOR_EACH_FUNCTION(X);
    #undef X

    template<typename F>
    concept GLFunctionLoader = requires(F f, const char * name) {
        { f(name) } -> convertible_to<void (*) ()>;
    };

    template<typename T> requires GLFunctionLoader<T>
    void load(T get_proc_address) {
        #define X(T, name) \
            name = reinterpret_cast<T>(get_proc_address(#name));
        FOR_EACH_FUNCTION(X);
        #undef X
    }
};

#undef FOR_EACH_FUNCTION
/* === */

#define FOR_EACH_CREATE(X) \
    X(glCreateBuffer); \
    X(glCreateVertexArray); \
    X(glCreateFramebuffer);

#define FOR_EACH_DELETE(X) \
    X(glDeleteBuffer); \
    X(glDeleteVertexArray); \
    X(glDeleteTexture); \
    X(glDeleteFramebuffer);

export namespace gl {
    #define X(F) \
        GLuint F() { GLuint name; F##s(1, &name); return name; }
    FOR_EACH_CREATE(X);
    #undef X

    #define X(F) \
        void F(GLuint name) { F##s(1, &name); }
    FOR_EACH_DELETE(X);
    #undef X

    /* special case with a parameter */
    GLuint glCreateTexture(GLenum target) {
        GLuint texture;
        glCreateTextures(target, 1, &texture);
        return texture;
    }
}

#undef FOR_EACH_CREATE
#undef FOR_EACH_DELETE

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
        switch (severity) {
            case GL_DEBUG_SEVERITY_HIGH:
                logger::error("{}", message);
                assert(false);
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                logger::warn("{}", message);
                break;
            case GL_DEBUG_SEVERITY_LOW:
                logger::notice("{}", message);
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                logger::debug("{}", message);
            default:
                return;
        };
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

        object(const object &) = delete;
        object(object && other) : name(other.name) {
            other.name = 0;
        }

        object & operator=(const object &) = delete;
        object & operator=(object &&other) {
            if (name != other.name) {
                if (name != 0)
                    destructor(name);
                name = other.name;
                other.name = 0;
            }
            return * this;
        }

        ~object() {
            if (name != 0)
                destructor(name);
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

    #define DEBUG_CAPABILITIES(T) using T##_t::object; \
        T() : T##_t() { logger::debug(#T "({}) new", name);}\
        T(GLuint name) : T##_t(name) {logger::debug(#T "({})", name);}\
        ~T() {logger::debug(#T "({}) delete", name);}\
        T(T&&) noexcept = default;\
        T& operator=(T&&) noexcept = default;

    constexpr GLbitfield DEFAULT_BUFFER_ALLOC_FLAGS = \
        GL_MAP_PERSISTENT_BIT                    \
        | GL_MAP_COHERENT_BIT                    \
        | GL_MAP_READ_BIT                        \
        | GL_MAP_WRITE_BIT;
    constexpr GLbitfield DEFAULT_BUFFER_STORAGE_FLAGS = \
        GL_DYNAMIC_STORAGE_BIT;

    constexpr GLenum DEFAULT_MAP_ACCESS = \
        GL_READ_WRITE;

    /* === "native" objects === */

    /* --- buffer --- */
    struct buffer: buffer_t {
        /* TODO reconsider method name and add unmap functionality */
        template<typename T>
        T * data(GLenum access = DEFAULT_MAP_ACCESS) {
            return static_cast<T *>(glMapNamedBuffer(name, access));
        }

        /* useful when buffer is already created being a member of a struct
         * and we don't want to create another buffer */
        template<typename T, size_t Extent>
        void store(span<T, Extent> data, GLbitfield flags = DEFAULT_BUFFER_STORAGE_FLAGS) {
            glNamedBufferStorage(name, data.size_bytes(), data.data(), flags);
        }

        template<typename T>
        void store(T * data, size_t size, GLbitfield flags = DEFAULT_BUFFER_STORAGE_FLAGS) {
            glNamedBufferStorage(name, size, data, flags);
        }
    };

    /* these functions always create new buffer */
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

    template<typename T>
    buffer store(T * data, size_t size, GLbitfield flags = DEFAULT_BUFFER_STORAGE_FLAGS) {
        GLuint b = glCreateBuffer();
        glNamedBufferStorage(b, size, data, flags);
        return buffer(b);
    }
    /* --- */

    /* --- vertex array --- */
    struct vertex_array: vertex_array_t {
        void bind_vertex_buffer(GLuint index, buffer &buffer, GLintptr offset, GLsizei stride) {
            glVertexArrayVertexBuffer(name, index, buffer.name, offset, stride);
        }

        template<typename Vertex>
        void bind_vertex_buffer(GLuint index, buffer &buffer, GLintptr offset = 0) {
            glVertexArrayVertexBuffer(name, index, buffer.name, offset, sizeof(Vertex));
        }

        void bind_element_buffer(buffer &buffer) {
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

    /* --- texture --- */
    struct texture: texture_t {
        texture(GLenum target) : texture_t(glCreateTexture(target)) {}

        GLuint64 get_handle() {
            return glGetTextureHandleARB(name);
        }
    };

    GLenum texture_format(unsigned int channels) {
        switch (channels) {
            case 1:
                return GL_RED;
            case 2:
                return GL_RG;
            case 3:
                return GL_RGB;
            case 4:
                return GL_RGBA;
            default:
                logger::error("no avaliable format for {} channels", channels);
                return GL_NONE;
        }
    }

    GLenum texture_internalformat(unsigned int channels) {
        switch (channels) {
            case 1:
                return GL_R8;
            case 2:
                return GL_RG8;
            case 3:
                return GL_RGB8;
            case 4:
                return GL_RGBA8;
            default:
                logger::error("no avaliable internal format for {} channels", channels);
                return GL_NONE;
        }
    }

    texture make_texture(uint8_t * pixels, int x, int y, int channels) {
        texture t(GL_TEXTURE_2D);
        glTextureStorage2D (t.name, 1, texture_internalformat(channels), x, y);
        glTextureSubImage2D(t.name, 0, 0, 0, x, y, texture_format(channels), GL_UNSIGNED_BYTE, pixels);
        glTextureParameteri(t.name, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(t.name, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(t.name, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(t.name, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glGenerateTextureMipmap(t.name);
        return t;
    }
    /* --- */

    /* --- framebuffer --- */
    struct framebuffer: framebuffer_t {};
    /* --- */

    /* --- shader --- */
    struct shader: shader_t {
        shader(GLenum type) : shader_t(glCreateShader(type)) {}

        void source(string_view src) {
            GLsizei size = static_cast<GLsizei>(src.size());
            const GLchar * data = src.data();
            glShaderSource(name, 1, &data, &size);
        }

        void binary(GLenum binary_format, span<const uint8_t> binary) {
            // TODO bulk shader initialization?
            glShaderBinary(1, &name, binary_format, binary.data(), binary.size());
        }

        void compile() {
            glCompileShader(name);
        }

        void specialize(string_view entry_point = "main", map<GLuint, GLuint> constants = {}) {
            // TODO specialization constants
            glSpecializeShader(name, entry_point.data(), 0, nullptr, nullptr);
        }
    };
    /* --- */

    /* --- program --- */
    struct program: program_t {
        DEBUG_CAPABILITIES(program);

        void attach_shader(shader &shader) {
           glAttachShader(name, shader.name);
        }

        void link() {
            glLinkProgram(name);
            GLint status;
            glGetProgramiv(name, GL_LINK_STATUS, &status);
            if (status == GL_FALSE) {
                GLint length;
                glGetProgramiv(name, GL_INFO_LOG_LENGTH, &length);
                string log = string(length, '\0');
                glGetProgramInfoLog(name, length, nullptr, log.data());
                logger::error("program(3) link {}", log);
                assert(status == GL_TRUE);
            }
        }

        void use() {
            glUseProgram(name);
        }

        void uniform(GLuint location, bool value) {
            glUniform1ui(location, value);
        }

        void uniform(GLuint location, unsigned int value) {
            glUniform1ui(location, value);
        }

        void uniform(GLuint location, int value) {
            glUniform1i(location, value);
        }

        void uniform(GLuint location, float value) {
            glUniform1f(location, value);
        }

        void uniform(GLuint location, vec3 value) {
            glUniform3fv(location, 1, value_ptr(value));
        }

        void uniform(GLuint location, vec4 value) {
            glUniform4fv(location, 1, value_ptr(value));
        }

        void uniform(GLuint location, mat3 value) {
            glUniformMatrix3fv(location, 1, GL_FALSE, value_ptr(value));
        }

        void uniform(GLuint location, mat4 value) {
            glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(value));
        }
    };
    /* --- */

    /* === */

    /* === "derived" objects === */

    /* --- mesh --- */
    template<typename T> struct element_type;
    template<> struct element_type<GLubyte> {
        static constexpr GLenum value = GL_UNSIGNED_BYTE;  };
    template<> struct element_type<GLushort> {
        static constexpr GLenum value = GL_UNSIGNED_SHORT; };
    template<> struct element_type<GLuint> {
        static constexpr GLenum value = GL_UNSIGNED_INT;   };

    template<typename T>
    concept is_element_type_v = \
        is_same_v<T, GLubyte> || is_same_v<T, GLushort> || is_same_v<T, GLuint>;

    struct mesh {
        vertex_array va;
        vector<buffer> buffers;
        buffer element_buffer;
        GLsizei count;
        GLenum type;
        size_t offset;

        void draw(DrawMode mode) {
            glBindVertexArray(va.name);
            glDrawElements(to_underlying(mode), count, type, reinterpret_cast<const void *>(offset));
        }

        void draw(DrawMode mode, GLsizei instance_count) {
            glBindVertexArray(va.name);
            glDrawElementsInstanced(
                to_underlying(mode),
                count,
                type,
                reinterpret_cast<const void *>(offset),
                instance_count
            );
        }
    };

    // TODO accept any type of vectors for attributes
    template<is_element_type_v ElementType>
    mesh make_mesh(
        vector<ElementType> elements,
        vector<vector<vec3>> vertices
    ) {
        mesh m;
        m.element_buffer.store(span(elements));
        m.va.bind_element_buffer(m.element_buffer);

        m.buffers.reserve(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i) {
            buffer& vb = m.buffers.emplace_back();
            vb.store(span(vertices[i]));
            m.va.enable_attribute(i);
            m.va.format_attribute(i, 3, GL_FLOAT, GL_FALSE, 0);
            m.va.bind_vertex_buffer<vec3>(i, vb);
            m.va.bind_attribute(i, i);
        }

        m.count = elements.size();
        m.type = element_type<ElementType>::value;
        m.offset = 0;
        return m;
    }

    template<is_element_type_v ElementType>
    mesh make_mesh(
        vector<ElementType> elements,
        vector<vec3> positions,
        vector<vec3> normals,
        vector<vec2> texcoords
    ) {
        mesh m;
        m.element_buffer.store(span(elements));
        m.va.bind_element_buffer(m.element_buffer);

        m.buffers.reserve(3);
        buffer &pb = m.buffers.emplace_back();
        pb.store(span(positions));
        buffer &nb = m.buffers.emplace_back();
        nb.store(span(normals));
        buffer &tb = m.buffers.emplace_back();
        tb.store(span(texcoords));

        m.va.enable_attribute(0);
        m.va.format_attribute(0, 3, GL_FLOAT, GL_FALSE, 0);

        m.va.bind_vertex_buffer<vec3>(0, pb);
        m.va.bind_attribute(0, 0);
        m.va.enable_attribute(1);
        m.va.format_attribute(1, 3, GL_FLOAT, GL_FALSE, 0);
        m.va.bind_vertex_buffer<vec3>(1, nb);
        m.va.bind_attribute(1, 1);

        m.va.enable_attribute(2);
        m.va.format_attribute(2, 2, GL_FLOAT, GL_FALSE, 0);
        m.va.bind_vertex_buffer<vec2>(2, tb);
        m.va.bind_attribute(2, 2);

        m.count = elements.size();
        m.type = element_type<ElementType>::value;
        m.offset = 0;
        return m;
    }
    /* --- */

    /* === */
    void enable(GLenum capability) {
        glEnable(capability);
    }

    void clear(GLenum mode) {
        glClear(mode);
    }

    void clear_color(vec4 color) {
        glClearColor(color.x, color.y, color.z, color.w);
    }

    void bind_uniform_buffer(GLuint index, buffer &b) {
        glBindBufferBase(GL_UNIFORM_BUFFER, index, b.name);
    }

    void bind_shader_storage_buffer(GLuint index, buffer &b) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, b.name);
    }

    void bind_texture_units(GLuint index, span<texture> textures) {
        static_assert(sizeof(texture) == sizeof(GLuint));
        glBindTextures(index, textures.size(), reinterpret_cast<GLuint *>(textures.data()));
    }

    void make_texture_handle_resident(GLuint64 handle) {
        glMakeTextureHandleResidentARB(handle);
    }

    void make_texture_handle_non_resident(GLuint64 handle) {
        glMakeTextureHandleNonResidentARB(handle);
    }
};
