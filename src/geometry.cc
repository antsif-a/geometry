export module geometry;

import std;
import glm;

using std::size_t;
using std::array;
using std::vector;
using std::function;
using glm::vec3;

// a vertex expressed by index relative to a particular set of vertices
using point = unsigned int;

// an edge connecting two vertices expressed by indices relative to a particular set of vertices
using edge = array<point, 2>;

// a collection of indices pointing to a set of coplanar vectors being a subset of a particular set of vertices
template<size_t n>
using face = array<point, n>;

// O(1)
export array<vec3, 8> create_cube() {
    array<vec3, 8> vertices = {};
    for (int i = 0; i < 8; ++i) {
        float x = (i & 1) ? 1.f : -1.f;
        float y = (i & 2) ? 1.f : -1.f;
        float z = (i & 4) ? 1.f : -1.f;
        vertices[i] = vec3(x, y, z);
    }

    return vertices;
}

export array<point, 8> create_cube_points() {
    array<point, 8> P = {};
    for (int i = 0; i < 8; ++i)
        P[i] = i;
    return P;
}


export array<edge, 24> create_cube_lines() {
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

export array<face<4>, 6> create_cube_faces_static_old() {
    array<face<4>, 6> faces = {};
    for (int i = 0; i < 8; ++i) {
        // camera up is positive
        /* x:
         * -1: 6 2 0 4
         *  1: 3 7 5 1
         */
        /* y:
         * -1: 5 4 0 1
         *  1: 6 7 3 2
         */
        /* z:
         * -1: 2 3 1 0
         *  1: 7 6 4 5
         */
        bool x = i & 1;
        bool y = i & 2;
        bool z = i & 4;
        if (x)
            faces[x] = {3, 7, 5, 1};
        else 
            faces[x] = {6, 2, 0, 4};
        if (y)
            faces[y + 2] = {6, 7, 3, 2};
        else
            faces[y + 2] = {5, 4, 0, 1};
        if (z)
            faces[z + 4] = {7, 6, 4, 5};
        else
            faces[z + 4] = {2, 3, 1, 0};
    }
    return faces;
}

export array<face<4>, 6> create_cube_faces() {
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
export array<face<3>, 12> create_cube_triangles() {
    array<face<3>, 12> T;
    array<face<4>, 6> F = create_cube_faces();
    for (int i = 0; i < 6; ++i) {
        face<4> f = F[i];
        T[i * 2]     = {f[0], f[1], f[2]};
        T[i * 2 + 1] = {f[2], f[3], f[0]};
    }
    return T;
}

export vector<vec3> generate_surface(int U, int V, function<vec3 (float, float)> F) {
    vector<vec3> vertices;
    for (int i = 0; i < U; ++i)
        for (int j = 0; j < V; ++j)
            vertices.push_back(F((float) i / (U - 1), (float) j / (V - 1)));
    return vertices; 
}

export vector<int> generate_grid_indices(int W, int H) {
    vector<int> indices;
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
