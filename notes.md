# ideas that i used but decided to replace them
## normals can be calculated on the gpu using partial derivatives
```c
vec3 normal = normalize(cross(dFdx(fragment_position), dFdy(fragment_position)));
```
## cube generation:
```cpp
array<vec3, 8> create_cube() {
    array<vec3, 8> vertices = {};
    for (int i = 0; i < 8; ++i) {
        float x = (i & 1) ? 1.f : -1.f;
        float y = (i & 2) ? 1.f : -1.f;
        float z = (i & 4) ? 1.f : -1.f;
        vertices[i] = vec3(x, y, z);
    }

    return vertices;
}
array<face<4>, 6> create_cube_faces() {
    array<face<4>, 6> faces = {};
    int k = 0;
    for (int axis = 0; axis < 3; ++axis) { // 0 -> x; 1 -> y; 2 -> z
        for (int sign = 0; sign < 2; ++sign) { // 0 -> -; 1 -> +
            int other1 = (axis + 1) % 3;
            int other2 = (axis + 2) % 3;
            array<edge, 4> face = {};
            if (sign == 0) {
                face = {{{0,0}, {0,1}, {1,1}, {1,0}}};
            } else {
                face = {{{0,0}, {1,0}, {1,1}, {0,1}}};
            }
            for (int i = 0; i < 4; ++i) {
                point p = sign << axis;
                p |= (face[i][0] << other1);
                p |= (face[i][1] << other2);
                faces[k][i] = p;
            }
            k++;
        }
    }
    return faces;
}

// O(1) triangulation
array<point, 36> create_cube_triangles() {
    array<face<3>, 12> T;
    array<face<4>, 6> F = create_cube_faces();
    for (int i = 0; i < 6; ++i) {
        face<4> f = F[i];
        T[i * 2]     = {f[0], f[1], f[2]};
        T[i * 2 + 1] = {f[2], f[3], f[0]};
    }
    return bit_cast<array<point, 6 * 2 * 3>>(T);
}

array<point, 8> create_cube_points() {
    array<point, 8> P = {};
    for (int i = 0; i < 8; ++i)
        P[i] = i;
    return P;
}


array<edge, 24> create_cube_lines() {
    array<edge, 24> A = {};
    int k = 0;
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 3; ++j) {
            auto p = i ^ (1 << j);
            if (p > i) // so we do not count same edge twice
                A[k++] = {(point) i, (point) p};
        }
    }
    return A;
}
```

## better cube generation (still using plain arrays)
```cpp

struct cube {
    array<vec3, 24> positions;
    array<vec3, 24> normals;
    array<unsigned char, 36> indices;
};

cube create_cube(array<unsigned int, 6> index_order, int normal_sign) {
    cube c = {};
    const mat3 id = mat3(1.0);
    int face_count = 0;
    for (int i = 0; i < 3; ++i) {
        for (float sign : {-1, 1}) {
            vec3 n = sign * id[i];
            vec3 u = vec3(n.y, n.z, n.x);
            vec3 v = cross(n, u);

            int v_idx = face_count * 4;
            int i_idx = face_count * 6;

            c.positions[v_idx + 0] = n - u - v;
            c.positions[v_idx + 1] = n + u - v;
            c.positions[v_idx + 2] = n + u + v;
            c.positions[v_idx + 3] = n - u + v;

            for (int k = 0; k < 4; ++k)
                c.normals[v_idx + k] = static_cast<float>(normal_sign) * n;
            for (int k = 0; k < 6; ++k)
                c.indices[i_idx + k] = v_idx + index_order[k];
            face_count++;
        }
    }

    return c;
}

cube create_cube_ccw() {
    return create_cube({0, 1, 2, 0, 2, 3}, 1);
}

cube create_cube_cw() {
    return create_cube({0, 3, 2, 0, 2, 1}, -1);
}
```
## uniform map generation
```cpp
struct string_hash
{
    using hash_type = std::hash<std::string_view>;
    using is_transparent = void;

    std::size_t operator()(const char* str) const        { return hash_type{}(str); }
    std::size_t operator()(std::string_view str) const   { return hash_type{}(str); }
    std::size_t operator()(const std::string& str) const { return hash_type{}(str); }
};

template<typename ValueType>
using unordered_str_map = unordered_map<string, ValueType, string_hash, std::equal_to<>>;

unordered_str_map<GLint> make_uniform_map(GLuint program) {
    GLint uniform_count, uniform_block_count;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &uniform_count);
    glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &uniform_block_count);

    if (uniform_count + uniform_block_count <= 0)
        return {};

    unordered_str_map<GLint> m(uniform_count + uniform_block_count);
    int len, maxlen;
    string name;

    if (uniform_count > 0) {
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxlen);
        name.resize(maxlen);
        for (int i = 0; i < uniform_count; ++i) {
            glGetActiveUniform(program, i, maxlen, &len, nullptr, nullptr, name.data());
            m[string(name.data(), len)] = glGetUniformLocation(program, name.c_str());
        }
    }

    if (uniform_block_count > 0) {
        glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH, &maxlen);
        if (maxlen > name.length())
            name.resize(maxlen);
        for(int i = 0; i < uniform_block_count; ++i) {
            glGetActiveUniformBlockName(program, i, maxlen, &len, name.data());
            m[string(name.data(), len)] = glGetUniformBlockIndex(program, name.c_str());
        }
    }
    return m;
}
struct program: program_t {
    DEBUG_CAPABILITIES(program);

    unordered_str_map<GLint> uniform_map;

    void attach_shader(shader & shader) {
       glAttachShader(name, shader.name);
    }

    void link() {
        glLinkProgram(name);
        uniform_map = make_uniform_map(name);
    }

    void use() {
        glUseProgram(name);
    }

    GLint get_uniform_location(string_view name) {
        if (auto it = uniform_map.find(name); it != uniform_map.end())
            return it->second;
        else
            return -1;
    }

    void uniform(string_view name, bool value) {
        glUniform1ui(get_uniform_location(name), value);
    }

    void uniform(string_view name, unsigned int value) {
        glUniform1ui(get_uniform_location(name), value);
    }

    void uniform(string_view name, int value) {
        glUniform1i(get_uniform_location(name), value);
    }

    void uniform(string_view name, float value) {
        glUniform1f(get_uniform_location(name), value);
    }

    void uniform(string_view name, vec3 value) {
        glUniform3fv(get_uniform_location(name), 1, value_ptr(value));
    }

    void uniform(string_view name, vec4 value) {
        glUniform4fv(get_uniform_location(name), 1, value_ptr(value));
    }

    void uniform(string_view name, mat3 value) {
        glUniformMatrix3fv(get_uniform_location(name), 1, GL_FALSE, value_ptr(value));
    }

    void uniform(string_view name, mat4 value) {
        glUniformMatrix4fv(get_uniform_location(name), 1, GL_FALSE, value_ptr(value));
    }
```
