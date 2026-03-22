export module geometry;

import std;
import glm;

using std::array;
using std::vector;
using std::convertible_to;
using std::numeric_limits;
using namespace glm;

export struct cube {
    vector<unsigned char> indices;
    vector<vec3> positions;
    vector<vec3> normals;
    vector<vec2> texcoords;
};

cube create_cube(array<unsigned int, 6> index_order, float normal_sign) {
    cube c = {};
    c.positions.reserve(24);
    c.normals.reserve(24);
    c.indices.reserve(36);
    const mat3 id = mat3(1.0);
    for (int i = 0; i < 3; ++i) {
        for (float sign : {-1, 1}) {
            vec3 n = sign * id[i];
            vec3 u = vec3(n.y, n.z, n.x);
            vec3 v = cross(n, u);
            for (int k = 0; k < 6; ++k)
                c.indices.push_back(c.positions.size() + index_order[k]);
            c.positions.push_back(n - u - v);
            c.positions.push_back(n + u - v);
            c.positions.push_back(n + u + v);
            c.positions.push_back(n - u + v);
            for (int k = 0; k < 4; ++k)
                c.normals.push_back(normal_sign * n);
            c.texcoords.push_back(vec2(0.0f, 0.0f));
            c.texcoords.push_back(vec2(1.0f, 0.0f));
            c.texcoords.push_back(vec2(1.0f, 1.0f));
            c.texcoords.push_back(vec2(0.0f, 1.0f));
        }
    }

    return c;
}

export cube create_cube_ccw() {
    return create_cube({0, 1, 2, 0, 2, 3}, 1.f);
}

export cube create_cube_cw() {
    return create_cube({0, 3, 2, 0, 2, 1}, -1.f);
}

template<typename F>
concept can_make_surface = requires(F f, float u, float v) {
    { f(u, v) } -> convertible_to<vec3>;
};

export vector<vec3> generate_surface(int W, int H, can_make_surface auto f) {
    vector<vec3> vertices;
    for (int i = 0; i < W; ++i) {
        float u = float(i) / (W - 1);
        for (int j = 0; j < H; ++j) {
            float v = float(j) / (H - 1);
            vertices.push_back(f(u, v));
        }
    }
    return vertices;
}

export vector<vec3> generate_normals(int W, int H, can_make_surface auto f) {
    vector<vec3> normals;
    for (int i = 0; i < W; ++i) {
        float u = float(i) / (W - 1);
        for (int j = 0; j < H; ++j) {
            float v = float(j) / (H - 1);
            float eps = 1e-4f;
            float u_next = u + eps;
            float u_prev = u - eps;
            float v_next = min(v + eps, 1.0f);
            float v_prev = max(v - eps, 0.0f);

            vec3 dFdu = (f(mod(u_next, 1.f), v) - f(mod(u_prev, 1.f), v)) / 2.0f * eps;
            vec3 dFdv = (f(u, v_next) - f(u, v_prev)) / v_next - v_prev;
            vec3 n = cross(dFdu, dFdv);
            if (length(n) < 1e-6f) {
                n = normalize(f(u, v));
            } else {
                n = normalize(n);
            }

            normals.push_back(n);
        }
    }
    return normals;
}

export vector<unsigned int> generate_grid_indices(int W, int H) {
    vector<unsigned int> indices;
    for (int i = 0; i < H - 1; ++i) {
        for (int j = 0; j < W - 1; ++j) {
            int LT = i * W + j;
            int RT = LT + 1;
            int LB = LT + W;
            int RB = LB + 1;

            indices.push_back(LT);
            indices.push_back(LB);
            indices.push_back(RT);

            indices.push_back(RT);
            indices.push_back(LB);
            indices.push_back(RB);
        }
    }
    return indices;
}

export vector<vec2> generate_texcoords(int W, int H) {
    vector<vec2> texcoords;
    texcoords.reserve(W * H);
    for (int i = 0; i < W; ++i) {
        float u = float(W - i - 1) / (W - 1);
        for (int j = 0; j < H; ++j) {
            float v = float(j) / (H - 1);
            texcoords.push_back(vec2(u, v));
        }
    }
    return texcoords;
}

export auto sphere = [] (float u, float v) {
    float theta = u * 2.0f * pi<float>();
    float phi = v * pi<float>();
    return vec3(
        cos(theta) * sin(phi),
        cos(phi),
        sin(theta) * sin(phi)
    );
};

export auto torus = [] (float u, float v) {
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

export auto helicoid = [] (float u, float v) -> vec3 {
    u *= 2 * pi<float>();
    v *= 2 * pi<float>();
    return {
        u * cos(v),
        u * sin(v),
        1 * v
    };
};

