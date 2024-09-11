#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "model.hpp"
#include "camera.hpp"
#include "shader.hpp"

using namespace glm;

#define FLT_INF 114514.0f

std::vector<vec3> vertices;
int vertices_num = 0;
std::vector<GLuint> indices;
std::vector<vec3> lines; 

struct BVHNode {
    BVHNode* left = NULL;
    BVHNode* right = NULL;
    int n, index;              
    vec3 AA, BB; 
};

struct Triangle {
    vec3 p1, p2, p3; 
    vec3 center; 
    Triangle(vec3 a, vec3 b, vec3 c) {
        p1 = a, p2 = b, p3 = c;
        center = (p1 + p2 + p3) / vec3(3, 3, 3);
    }
};
std::vector<Triangle> triangles;

bool cmpx(const Triangle& t1, const Triangle& t2) {
    return t1.center.x < t2.center.x;
}
bool cmpy(const Triangle& t1, const Triangle& t2) {
    return t1.center.y < t2.center.y;
}
bool cmpz(const Triangle& t1, const Triangle& t2) {
    return t1.center.z < t2.center.z;
}


struct HitResult {
    Triangle* triangle = NULL;
    float distance = FLT_INF;
};

struct Ray {
    vec3 startPoint = vec3(0, 0, 0);
    vec3 direction = vec3(0, 0, 0); 
};

void addLine(vec3 p1, vec3 p2) {
    lines.push_back(p1);
    lines.push_back(p2);
}

void addBox(BVHNode* root) {
    float x1 = root->AA.x, y1 = root->AA.y, z1 = root->AA.z;
    float x2 = root->BB.x, y2 = root->BB.y, z2 = root->BB.z;
    lines.push_back(vec3(x1, y1, z1)), lines.push_back(vec3(x2, y1, z1));
    lines.push_back(vec3(x1, y1, z1)), lines.push_back(vec3(x1, y1, z2));
    lines.push_back(vec3(x1, y1, z1)), lines.push_back(vec3(x1, y2, z1));
    lines.push_back(vec3(x2, y1, z1)), lines.push_back(vec3(x2, y1, z2));
    lines.push_back(vec3(x2, y1, z1)), lines.push_back(vec3(x2, y2, z1));
    lines.push_back(vec3(x1, y2, z1)), lines.push_back(vec3(x2, y2, z1));
    lines.push_back(vec3(x1, y1, z2)), lines.push_back(vec3(x1, y2, z2));
    lines.push_back(vec3(x1, y2, z1)), lines.push_back(vec3(x1, y2, z2));
    lines.push_back(vec3(x1, y2, z2)), lines.push_back(vec3(x2, y2, z2));
    lines.push_back(vec3(x1, y1, z2)), lines.push_back(vec3(x2, y1, z2));
    lines.push_back(vec3(x2, y2, z1)), lines.push_back(vec3(x2, y2, z2));
    lines.push_back(vec3(x2, y1, z2)), lines.push_back(vec3(x2, y2, z2));
}

void addTriangle(Triangle* tri) {
    if (tri) {
        lines.push_back(tri->p1 - vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p2 - vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p2 - vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p3 - vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p3 - vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p1 - vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p1 + vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p2 + vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p2 + vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p3 + vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p3 + vec3(0.0005, 0.0005, 0.0005));
        lines.push_back(tri->p1 + vec3(0.0005, 0.0005, 0.0005));
    }
}

float hitTriangle(Triangle* triangle, Ray ray) {
    vec3 p1 = triangle->p1, p2 = triangle->p2, p3 = triangle->p3;
    vec3 S = ray.startPoint; 
    vec3 d = ray.direction; 
    vec3 N = normalize(cross(p2 - p1, p3 - p1)); 
    if (dot(N, d) > 0.0f) N = -N; 

    if (fabs(dot(N, d)) < 0.00001f) return FLT_INF;

    float t = (dot(N, p1) - dot(S, N)) / dot(d, N);
    if (t < 0.0005f) return FLT_INF; 

    vec3 P = S + d * t;

    vec3 c1 = cross(p2 - p1, P - p1);
    vec3 c2 = cross(p3 - p2, P - p2);
    vec3 c3 = cross(p1 - p3, P - p3);
    if (dot(c1, N) > 0 && dot(c2, N) > 0 && dot(c3, N) > 0) return t;
    if (dot(c1, N) < 0 && dot(c2, N) < 0 && dot(c3, N) < 0) return t;

    return FLT_INF;
}


BVHNode* buildBVH(std::vector<Triangle>& triangles, int l, int r, int n) {
    if (l > r) return 0;

    BVHNode* node = new BVHNode();
    node->AA = vec3(FLT_INF, FLT_INF, FLT_INF);
    node->BB = vec3(-FLT_INF, -FLT_INF, -FLT_INF);

    for (int i = l; i <= r; i++) {
        float minx = min(triangles[i].p1.x, min(triangles[i].p2.x, triangles[i].p3.x));
        float miny = min(triangles[i].p1.y, min(triangles[i].p2.y, triangles[i].p3.y));
        float minz = min(triangles[i].p1.z, min(triangles[i].p2.z, triangles[i].p3.z));
        node->AA.x = min(node->AA.x, minx);
        node->AA.y = min(node->AA.y, miny);
        node->AA.z = min(node->AA.z, minz);
        float maxx = max(triangles[i].p1.x, max(triangles[i].p2.x, triangles[i].p3.x));
        float maxy = max(triangles[i].p1.y, max(triangles[i].p2.y, triangles[i].p3.y));
        float maxz = max(triangles[i].p1.z, max(triangles[i].p2.z, triangles[i].p3.z));
        node->BB.x = max(node->BB.x, maxx);
        node->BB.y = max(node->BB.y, maxy);
        node->BB.z = max(node->BB.z, maxz);
    }

    if ((r - l + 1) <= n) {
        node->n = r - l + 1;
        node->index = l;
        return node;
    }

    float lenx = node->BB.x - node->AA.x;
    float leny = node->BB.y - node->AA.y;
    float lenz = node->BB.z - node->AA.z;
    if (lenx >= leny && lenx >= lenz)
        std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpx);
    if (leny >= lenx && leny >= lenz)
        std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpy);
    if (lenz >= lenx && lenz >= leny)
        std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpz);

    int mid = (l + r) / 2;
    node->left = buildBVH(triangles, l, mid, n);
    node->right = buildBVH(triangles, mid + 1, r, n);

    return node;
}

BVHNode* buildBVHwithSAH(std::vector<Triangle>& triangles, int l, int r, int n) {
    if (l > r) return 0;

    BVHNode* node = new BVHNode();
    node->AA = vec3(FLT_INF, FLT_INF, FLT_INF);
    node->BB = vec3(-FLT_INF, -FLT_INF, -FLT_INF);

    for (int i = l; i <= r; i++) {
        float minx = min(triangles[i].p1.x, min(triangles[i].p2.x, triangles[i].p3.x));
        float miny = min(triangles[i].p1.y, min(triangles[i].p2.y, triangles[i].p3.y));
        float minz = min(triangles[i].p1.z, min(triangles[i].p2.z, triangles[i].p3.z));
        node->AA.x = min(node->AA.x, minx);
        node->AA.y = min(node->AA.y, miny);
        node->AA.z = min(node->AA.z, minz);
        float maxx = max(triangles[i].p1.x, max(triangles[i].p2.x, triangles[i].p3.x));
        float maxy = max(triangles[i].p1.y, max(triangles[i].p2.y, triangles[i].p3.y));
        float maxz = max(triangles[i].p1.z, max(triangles[i].p2.z, triangles[i].p3.z));
        node->BB.x = max(node->BB.x, maxx);
        node->BB.y = max(node->BB.y, maxy);
        node->BB.z = max(node->BB.z, maxz);
    }

    if ((r - l + 1) <= n) {
        node->n = r - l + 1;
        node->index = l;
        return node;
    }

    float Cost = FLT_INF;
    int Axis = 0;
    int Split = (l + r) / 2;
    for (int axis = 0; axis < 3; axis++) {
        if (axis == 0) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpx);
        if (axis == 1) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpy);
        if (axis == 2) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpz);


        std::vector<vec3> leftMax(r - l + 1, vec3(-FLT_INF, -FLT_INF, -FLT_INF));
        std::vector<vec3> leftMin(r - l + 1, vec3(FLT_INF, FLT_INF, FLT_INF));

        // pre process max_xyz and min_xyz in [l, i]
        for (int i = l; i <= r; i++) {
            Triangle& t = triangles[i];
            int bias = (i == l) ? 0 : 1; 

            leftMax[i - l].x = max(leftMax[i - l - bias].x, max(t.p1.x, max(t.p2.x, t.p3.x)));
            leftMax[i - l].y = max(leftMax[i - l - bias].y, max(t.p1.y, max(t.p2.y, t.p3.y)));
            leftMax[i - l].z = max(leftMax[i - l - bias].z, max(t.p1.z, max(t.p2.z, t.p3.z)));

            leftMin[i - l].x = min(leftMin[i - l - bias].x, min(t.p1.x, min(t.p2.x, t.p3.x)));
            leftMin[i - l].y = min(leftMin[i - l - bias].y, min(t.p1.y, min(t.p2.y, t.p3.y)));
            leftMin[i - l].z = min(leftMin[i - l - bias].z, min(t.p1.z, min(t.p2.z, t.p3.z)));
        }
        // pre process max_xyz and min_xyz in [i, r]
        std::vector<vec3> rightMax(r - l + 1, vec3(-FLT_INF, -FLT_INF, -FLT_INF));
        std::vector<vec3> rightMin(r - l + 1, vec3(FLT_INF, FLT_INF, FLT_INF));
        for (int i = r; i >= l; i--) {
            Triangle& t = triangles[i];
            int bias = (i == r) ? 0 : 1; 

            rightMax[i - l].x = max(rightMax[i - l + bias].x, max(t.p1.x, max(t.p2.x, t.p3.x)));
            rightMax[i - l].y = max(rightMax[i - l + bias].y, max(t.p1.y, max(t.p2.y, t.p3.y)));
            rightMax[i - l].z = max(rightMax[i - l + bias].z, max(t.p1.z, max(t.p2.z, t.p3.z)));

            rightMin[i - l].x = min(rightMin[i - l + bias].x, min(t.p1.x, min(t.p2.x, t.p3.x)));
            rightMin[i - l].y = min(rightMin[i - l + bias].y, min(t.p1.y, min(t.p2.y, t.p3.y)));
            rightMin[i - l].z = min(rightMin[i - l + bias].z, min(t.p1.z, min(t.p2.z, t.p3.z)));
        }

        // find min split in this axis
        float cost = FLT_INF;
        int split = l;
        for (int i = l; i <= r - 1; i++) {
            float lenx, leny, lenz;
            vec3 leftAA = leftMin[i - l];
            vec3 leftBB = leftMax[i - l];
            lenx = leftBB.x - leftAA.x;
            leny = leftBB.y - leftAA.y;
            lenz = leftBB.z - leftAA.z;
            float leftS = 2.0 * ((lenx * leny) + (lenx * lenz) + (leny * lenz));
            float leftCost = leftS * (i - l + 1);

            vec3 rightAA = rightMin[i + 1 - l];
            vec3 rightBB = rightMax[i + 1 - l];
            lenx = rightBB.x - rightAA.x;
            leny = rightBB.y - rightAA.y;
            lenz = rightBB.z - rightAA.z;
            float rightS = 2.0 * ((lenx * leny) + (lenx * lenz) + (leny * lenz));
            float rightCost = rightS * (r - i);

            float totalCost = leftCost + rightCost;
            if (totalCost < cost) {
                cost = totalCost;
                split = i;
            }
        }
        if (cost < Cost) {
            Cost = cost;
            Axis = axis;
            Split = split;
        }
    }

    if (Axis == 0) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpx);
    if (Axis == 1) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpy);
    if (Axis == 2) std::sort(&triangles[0] + l, &triangles[0] + r + 1, cmpz);

    node->left = buildBVHwithSAH(triangles, l, Split, n);
    node->right = buildBVHwithSAH(triangles, Split + 1, r, n);

    return node;
}

void dfsNlevel(BVHNode* root, int depth, int targetDepth) {
    if (root == NULL) return;
    if (targetDepth == depth) {
        addBox(root);
        return;
    }
    dfsNlevel(root->left, depth + 1, targetDepth);
    dfsNlevel(root->right, depth + 1, targetDepth);
}

HitResult hitTriangleArray(Ray ray, std::vector<Triangle>& triangles, int l, int r) {
    HitResult res;
    for (int i = l; i <= r; i++) {
        float d = hitTriangle(&triangles[i], ray);
        if (d < FLT_INF && d < res.distance) {
            res.distance = d;
            res.triangle = &triangles[i];
        }
    }
    return res;
}

float hitAABB(Ray r, vec3 AA, vec3 BB) {
    // 1.0 / direction
    vec3 invdir = vec3(1.0 / r.direction.x, 1.0 / r.direction.y, 1.0 / r.direction.z);

    vec3 in = (BB - r.startPoint) * invdir;
    vec3 out = (AA - r.startPoint) * invdir;

    vec3 tmax = max(in, out);
    vec3 tmin = min(in, out);

    float t1 = min(tmax.x, min(tmax.y, tmax.z));
    float t0 = max(tmin.x, max(tmin.y, tmin.z));

    return (t1 >= t0) ? ((t0 > 0.0) ? (t0) : (t1)) : (-1);
}

HitResult hitBVH(Ray ray, std::vector<Triangle>& triangles, BVHNode* root) {
    if (root == NULL) return HitResult();

    if (root->n > 0) {
        return hitTriangleArray(ray, triangles, root->n, root->n + root->index - 1);
    }

    float d1 = FLT_INF, d2 = FLT_INF;
    if (root->left) d1 = hitAABB(ray, root->left->AA, root->left->BB);
    if (root->right) d2 = hitAABB(ray, root->right->AA, root->right->BB);

    HitResult r1, r2;
    if (d1 > 0) r1 = hitBVH(ray, triangles, root->left);
    if (d2 > 0) r2 = hitBVH(ray, triangles, root->right);

    return r1.distance < r2.distance ? r1 : r2;
}

vec3 rotateControl(0, 0, 0);
vec3 scaleControl(1, 1, 1);
double lastX = 0.0f, lastY = 0.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    scaleControl.x += 1 * yoffset * 0.1;
    scaleControl.y += 1 * yoffset * 0.1;
    scaleControl.z += 1 * yoffset * 0.1;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    rotateControl.y += -200 * (xposIn - lastX) / 512;
    rotateControl.x += -200 * (yposIn - lastY) / 512;
    lastX = xposIn, lastY = yposIn;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(512, 512, "KVH", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    cg::Model bunny("assets/models/bunny/bunny.obj");
    cg::Model floor("assets/models/floor/floor.obj");
    for (auto mesh : bunny.meshes) {
        for (auto vertex : mesh.vertices) {
            vertices.push_back(vertex.Position);
        }
        for (auto index : mesh.indices) {
            indices.push_back(index);
        }
    }
    int vertex_num = vertices.size();
    for (auto mesh : floor.meshes) {
        for (auto vertex : mesh.vertices) {
            vertices.push_back(vertex.Position * glm::vec3(0.1, 0.1, 0.1));
        }
        for (auto index : mesh.indices) {
            indices.push_back(index + vertex_num);
        }
    }
    for (int i = 0; i < indices.size(); i += 3) {
        triangles.push_back(Triangle(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]));
    }

    BVHNode* root = buildBVHwithSAH(triangles, 0, triangles.size() - 1, 8);
    dfsNlevel(root, 0, 5); // bvh aabb line

    Ray ray;
    ray.startPoint = vec3(0, 0, 1);
    ray.direction = normalize(vec3(0.1, -0.1, -0.7));
    HitResult res = hitBVH(ray, triangles, root);
    addTriangle(res.triangle);
    addLine(ray.startPoint, ray.startPoint + ray.direction * vec3(5, 5, 5));

    cg::Shader shader("assets/shaders/GLSL/bvh_vert.glsl", "assets/shaders/GLSL/bvh_frag.glsl");

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vec3) * (vertices.size() + lines.size()), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vec3) * vertices.size(), vertices.data());
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vec3) * vertices.size(), sizeof(vec3) * lines.size(), lines.data());

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);


    while (!glfwWindowShouldClose(window)) {
        processInput(window);

        glClearColor(0.0, 0.0, 0.0, 1.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.75f));
        model = glm::scale(model, scaleControl);
        model = glm::rotate(model, glm::radians(rotateControl.x), glm::vec3(1.0, 0.0, 0.0));
        model = glm::rotate(model, glm::radians(rotateControl.y), glm::vec3(0.0, 1.0, 0.0));
        model = glm::rotate(model, glm::radians(rotateControl.z), glm::vec3(0.0, 0.0, 1.0));
        shader.use();
        shader.setMat4("model", model);
        shader.setVec3("color", glm::vec3(1.0f, 0.0f, 0.0f));
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        shader.setVec3("color", glm::vec3(1.0f, 1.0f, 1.0f));
        glDrawArrays(GL_LINES, vertices.size(), lines.size());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}