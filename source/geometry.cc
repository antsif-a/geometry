export module geometry;

import std;
import glm;

using std::array;
using std::vector;
using std::convertible_to;
using std::numeric_limits;
using namespace glm;

export struct cube {
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

export cube create_cube_ccw() {
    return create_cube({0, 1, 2, 0, 2, 3}, 1);
}

export cube create_cube_cw() {
    return create_cube({0, 3, 2, 0, 2, 1}, -1);
}

template<typename F>
concept can_make_surface = requires(F f, float u, float v) {
    { f(u, v) } -> convertible_to<vec3>;
};

export struct surface {
    vector<vec3> positions;
    vector<vec3> normals;
    vector<unsigned int> indices;
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
            vec3 f0 = f(u, v);
            float eps = 1e-4f;

            vec3 dFdu = (f(mod(u + eps, 1.f), v) - f0) / eps;
            vec3 dFdv = (f(u, mod(v + eps, 1.f)) - f0) / eps;
            vec3 n = cross(dFdu, dFdv);
            if (length(n) < 1e-6f)
                n = normalize(f0);
            else
                n = normalize(n);

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

