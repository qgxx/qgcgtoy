#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <ctime>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "stb_image.h"
#include "render_pass.hpp"
#include "shader.hpp"
#include "model.hpp"
#include "colors.hpp"

using namespace glm;

#define FLT_INF 114514.0f

struct Material {
    vec3 emissive = vec3(0, 0, 0);  // light source color
    vec3 baseColor = vec3(1, 1, 1);

    // disney principles
    float subsurface = 0.0;
    float metallic = 0.0;
    float specular = 0.0;
    float specularTint = 0.0;
    float roughness = 0.0;
    float anisotropic = 0.0;
    float sheen = 0.0;
    float sheenTint = 0.0;
    float clearcoat = 0.0;
    float clearcoatGloss = 0.0;
    float IOR = 1.0;
    float transmission = 0.0;
};

struct Triangle {
    vec3 p1, p2, p3;  // pos
    vec3 n1, n2, n3;  // normal
    Material material;
};

struct BVHNode {
    int left, right; 
    int n, index;     // include node from index to index + n - 1               
    vec3 AA, BB;      // AABB
};


// encode data to glsl
struct Triangle_encoded {
    vec3 p1, p2, p3;    
    vec3 n1, n2, n3;    

    // material
    vec3 emissive;      
    vec3 baseColor;     
    vec3 param1;        // (subsurface, metallic, specular)
    vec3 param2;        // (specularTint, roughness, anisotropic)
    vec3 param3;        // (sheen, sheenTint, clearcoat)
    vec3 param4;        // (clearcoatGloss, IOR, transmission)
};

struct BVHNode_encoded {
    vec3 childs;        // (left, right, _)
    vec3 leafInfo;      // (n, index, _)
    vec3 AA, BB;        
};


GLuint trianglesTextureBuffer;
GLuint nodesTextureBuffer;
GLuint lastFrame;
GLuint hdrMap;

cg::RenderPass pass1;
cg::RenderPass pass2;
cg::RenderPass pass3;

bool cmpx(const Triangle& t1, const Triangle& t2) {
    vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / vec3(3, 3, 3);
    vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / vec3(3, 3, 3);
    return center1.x < center2.x;
}
bool cmpy(const Triangle& t1, const Triangle& t2) {
    vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / vec3(3, 3, 3);
    vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / vec3(3, 3, 3);
    return center1.y < center2.y;
}
bool cmpz(const Triangle& t1, const Triangle& t2) {
    vec3 center1 = (t1.p1 + t1.p2 + t1.p3) / vec3(3, 3, 3);
    vec3 center2 = (t2.p1 + t2.p2 + t2.p3) / vec3(3, 3, 3);
    return center1.z < center2.z;
}

GLuint getTextureRGB32F(int width, int height) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return tex;
}

void addModel(cg::Model& model, std::vector<Triangle>& triangles, Material material, mat4 trans, bool smoothNormal) {
    std::vector<vec3> vertices;
    std::vector<GLuint> indices;

    float maxx = -FLT_INF;
    float maxy = -FLT_INF;
    float maxz = -FLT_INF;
    float minx = FLT_INF;
    float miny = FLT_INF;
    float minz = FLT_INF;

    for (auto mesh : model.meshes) {
        for (auto vertex : mesh.vertices) {
            glm::vec3 pos = vertex.Position;
            vertices.push_back(pos);
            maxx = max(maxx, pos.x); maxy = max(maxx, pos.y); maxz = max(maxx, pos.z);
            minx = min(minx, pos.x); miny = min(minx, pos.y); minz = min(minx, pos.z);
        }
        for (auto index : mesh.indices) {
            indices.push_back(index);
        }
    }

    // model size normalization
    float lenx = maxx - minx;
    float leny = maxy - miny;
    float lenz = maxz - minz;
    float maxaxis = max(lenx, max(leny, lenz));
    for (auto& v : vertices) {
        v.x /= maxaxis;
        v.y /= maxaxis;
        v.z /= maxaxis;
    }

    for (auto& v : vertices) {
        vec4 vv = vec4(v.x, v.y, v.z, 1);
        vv = trans * vv;
        v = vec3(vv.x, vv.y, vv.z);
    }

    // generate normal
    std::vector<vec3> normals(vertices.size(), vec3(0, 0, 0));
    for (int i = 0; i < indices.size(); i += 3) {
        vec3 p1 = vertices[indices[i]];
        vec3 p2 = vertices[indices[i + 1]];
        vec3 p3 = vertices[indices[i + 2]];
        vec3 n = normalize(cross(p2 - p1, p3 - p1));
        normals[indices[i]] += n;
        normals[indices[i + 1]] += n;
        normals[indices[i + 2]] += n;
    }


    int offset = triangles.size();
    triangles.resize(offset + indices.size() / 3);
    for (int i = 0; i < indices.size(); i += 3) {
        Triangle& t = triangles[offset + i / 3];
        t.p1 = vertices[indices[i]];
        t.p2 = vertices[indices[i + 1]];
        t.p3 = vertices[indices[i + 2]];
        if (!smoothNormal) {
            vec3 n = normalize(cross(t.p2 - t.p1, t.p3 - t.p1));
            t.n1 = n; t.n2 = n; t.n3 = n;
        }
        else {
            t.n1 = normalize(normals[indices[i]]);
            t.n2 = normalize(normals[indices[i + 1]]);
            t.n3 = normalize(normals[indices[i + 2]]);
        }

        t.material = material;
    }
}


// use index instead if point 
int buildBVH(std::vector<Triangle>& triangles, std::vector<BVHNode>& nodes, int l, int r, int n) {
    if (l > r) return 0;

    nodes.push_back(BVHNode());
    int id = nodes.size() - 1; 
    nodes[id].left = nodes[id].right = nodes[id].n = nodes[id].index = 0;
    nodes[id].AA = vec3(FLT_INF, FLT_INF, FLT_INF);
    nodes[id].BB = vec3(-FLT_INF, -FLT_INF, -FLT_INF);  // don't use -FLT_INF(exist some error on opengl)


    for (int i = l; i <= r; i++) {
        float minx = min(triangles[i].p1.x, min(triangles[i].p2.x, triangles[i].p3.x));
        float miny = min(triangles[i].p1.y, min(triangles[i].p2.y, triangles[i].p3.y));
        float minz = min(triangles[i].p1.z, min(triangles[i].p2.z, triangles[i].p3.z));
        nodes[id].AA.x = min(nodes[id].AA.x, minx);
        nodes[id].AA.y = min(nodes[id].AA.y, miny);
        nodes[id].AA.z = min(nodes[id].AA.z, minz);
        float maxx = max(triangles[i].p1.x, max(triangles[i].p2.x, triangles[i].p3.x));
        float maxy = max(triangles[i].p1.y, max(triangles[i].p2.y, triangles[i].p3.y));
        float maxz = max(triangles[i].p1.z, max(triangles[i].p2.z, triangles[i].p3.z));
        nodes[id].BB.x = max(nodes[id].BB.x, maxx);
        nodes[id].BB.y = max(nodes[id].BB.y, maxy);
        nodes[id].BB.z = max(nodes[id].BB.z, maxz);
    }

    if ((r - l + 1) <= n) {
        nodes[id].n = r - l + 1;
        nodes[id].index = l;
        return id;
    }

    float lenx = nodes[id].BB.x - nodes[id].AA.x;
    float leny = nodes[id].BB.y - nodes[id].AA.y;
    float lenz = nodes[id].BB.z - nodes[id].AA.z;
    if (lenx >= leny && lenx >= lenz)
        std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpx);
    if (leny >= lenx && leny >= lenz)
        std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpy);
    if (lenz >= lenx && lenz >= leny)
        std::sort(triangles.begin() + l, triangles.begin() + r + 1, cmpz);

    int mid = (l + r) / 2;
    int left = buildBVH(triangles, nodes, l, mid, n);
    int right = buildBVH(triangles, nodes, mid + 1, r, n);

    nodes[id].left = left;
    nodes[id].right = right;

    return id;
}


int buildBVHwithSAH(std::vector<Triangle>& triangles, std::vector<BVHNode>& nodes, int l, int r, int n) {
    if (l > r) return 0;

    nodes.push_back(BVHNode());
    int id = nodes.size() - 1;   
    nodes[id].left = nodes[id].right = nodes[id].n = nodes[id].index = 0;
    nodes[id].AA = vec3(FLT_INF, FLT_INF, FLT_INF);
    nodes[id].BB = vec3(-FLT_INF, -FLT_INF, -FLT_INF);

    for (int i = l; i <= r; i++) {
        float minx = min(triangles[i].p1.x, min(triangles[i].p2.x, triangles[i].p3.x));
        float miny = min(triangles[i].p1.y, min(triangles[i].p2.y, triangles[i].p3.y));
        float minz = min(triangles[i].p1.z, min(triangles[i].p2.z, triangles[i].p3.z));
        nodes[id].AA.x = min(nodes[id].AA.x, minx);
        nodes[id].AA.y = min(nodes[id].AA.y, miny);
        nodes[id].AA.z = min(nodes[id].AA.z, minz);
        float maxx = max(triangles[i].p1.x, max(triangles[i].p2.x, triangles[i].p3.x));
        float maxy = max(triangles[i].p1.y, max(triangles[i].p2.y, triangles[i].p3.y));
        float maxz = max(triangles[i].p1.z, max(triangles[i].p2.z, triangles[i].p3.z));
        nodes[id].BB.x = max(nodes[id].BB.x, maxx);
        nodes[id].BB.y = max(nodes[id].BB.y, maxy);
        nodes[id].BB.z = max(nodes[id].BB.z, maxz);
    }

    if ((r - l + 1) <= n) {
        nodes[id].n = r - l + 1;
        nodes[id].index = l;
        return id;
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

    int left  = buildBVHwithSAH(triangles, nodes, l, Split, n);
    int right = buildBVHwithSAH(triangles, nodes, Split + 1, r, n);

    nodes[id].left = left;
    nodes[id].right = right;

    return id;
}

mat4 getTransformMatrix(vec3 rotateCtrl, vec3 translateCtrl, vec3 scaleCtrl) {
    glm::mat4 unit( 
        glm::vec4(1, 0, 0, 0),
        glm::vec4(0, 1, 0, 0),
        glm::vec4(0, 0, 1, 0),
        glm::vec4(0, 0, 0, 1)
    );
    mat4 scale = glm::scale(unit, scaleCtrl);
    mat4 translate = glm::translate(unit, translateCtrl);
    mat4 rotate = unit;
    rotate = glm::rotate(rotate, glm::radians(rotateCtrl.x), glm::vec3(1, 0, 0));
    rotate = glm::rotate(rotate, glm::radians(rotateCtrl.y), glm::vec3(0, 1, 0));
    rotate = glm::rotate(rotate, glm::radians(rotateCtrl.z), glm::vec3(0, 0, 1));

    mat4 model = translate * rotate * scale;
    return model;
}

int frameCounter = 0;

// camera settings
float upAngle = 0.0;
float rotatAngle = 0.0;
float r = 4.0;
double lastX = 0.0f, lastY = 0.0f;
bool m_useCursor = false;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (m_useCursor) return;

    frameCounter = 0;
    r += -yoffset * 0.5;
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (m_useCursor) return;

    frameCounter = 0;
    rotatAngle += 150 * (xposIn - lastX) / 512;
    upAngle += 150 * (yposIn - lastY) / 512;
    upAngle = min(upAngle, 89.0f);
    upAngle = max(upAngle, -89.0f);
    lastX = xposIn, lastY = yposIn;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        m_useCursor = !m_useCursor;
        if (m_useCursor) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else {
            glfwSetCursorPos(window, lastX, lastY);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(512, 512, "Path Tracing GPU(Disney Principle)", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);


    // load model
    std::vector<Triangle> triangles;

    Material m;
    m.baseColor = cg::Color::SkyBlue;
    m.roughness = 0.1;
    m.metallic = 0.0;
    m.clearcoat = 1.0;
    m.subsurface = 1.0;
    cg::Model bunny("assets/models/bunny/bunny-simple.obj");
    addModel(bunny, triangles, m, getTransformMatrix(vec3(0, 0, 0), vec3(-2.6, -1.5, 0), vec3(2.5, 2.5, 2.5)), true);

    m.baseColor = cg::Color::Gold;
    m.roughness = 0.15;
    m.metallic = 1.0;
    m.clearcoat = 1.0;
    m.subsurface = 1.0;
    cg::Model teapot("assets/models/teapot/teapot.obj");
    addModel(teapot, triangles, m, getTransformMatrix(vec3(0, 0, 0), vec3(0, -0.4, 0), vec3(1.75, 1.75, 1.75)), true);

    int nTriangles = triangles.size();
    std::cout << "Completed load all models, triangle nums: " << nTriangles << std::endl;

    // build BVH
    BVHNode testNode;
    testNode.left = 255;
    testNode.right = 128;
    testNode.n = 30;
    testNode.AA = vec3(1, 1, 0);
    testNode.BB = vec3(0, 1, 0);
    std::vector<BVHNode> nodes{ testNode };
    buildBVHwithSAH(triangles, nodes, 0, triangles.size() - 1, 8);
    int nNodes = nodes.size();
    std::cout << "Completed build BVH, node nums:  " << nNodes << std::endl;

    // encode triangle
    std::vector<Triangle_encoded> triangles_encoded(nTriangles);
    for (int i = 0; i < nTriangles; i++) {
        Triangle& t = triangles[i];
        Material& m = t.material;
        triangles_encoded[i].p1 = t.p1;
        triangles_encoded[i].p2 = t.p2;
        triangles_encoded[i].p3 = t.p3;
        triangles_encoded[i].n1 = t.n1;
        triangles_encoded[i].n2 = t.n2;
        triangles_encoded[i].n3 = t.n3;
        triangles_encoded[i].emissive = m.emissive;
        triangles_encoded[i].baseColor = m.baseColor;
        triangles_encoded[i].param1 = vec3(m.subsurface, m.metallic, m.specular);
        triangles_encoded[i].param2 = vec3(m.specularTint, m.roughness, m.anisotropic);
        triangles_encoded[i].param3 = vec3(m.sheen, m.sheenTint, m.clearcoat);
        triangles_encoded[i].param4 = vec3(m.clearcoatGloss, m.IOR, m.transmission);
    }

    // encode BVHNode
    std::vector<BVHNode_encoded> nodes_encoded(nNodes);
    for (int i = 0; i < nNodes; i++) {
        nodes_encoded[i].childs = vec3(nodes[i].left, nodes[i].right, 0);
        nodes_encoded[i].leafInfo = vec3(nodes[i].n, nodes[i].index, 0);
        nodes_encoded[i].AA = nodes[i].AA;
        nodes_encoded[i].BB = nodes[i].BB;
    }


    // setup texture
    GLuint tbo0;
    glGenBuffers(1, &tbo0);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo0);
    glBufferData(GL_TEXTURE_BUFFER, triangles_encoded.size() * sizeof(Triangle_encoded), &triangles_encoded[0], GL_STATIC_DRAW);
    glGenTextures(1, &trianglesTextureBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, trianglesTextureBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo0);

    GLuint tbo1;
    glGenBuffers(1, &tbo1);
    glBindBuffer(GL_TEXTURE_BUFFER, tbo1);
    glBufferData(GL_TEXTURE_BUFFER, nodes_encoded.size() * sizeof(BVHNode_encoded), &nodes_encoded[0], GL_STATIC_DRAW);
    glGenTextures(1, &nodesTextureBuffer);
    glBindTexture(GL_TEXTURE_BUFFER, nodesTextureBuffer);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, tbo1);

    int width, height, nrComponents;
    float *data = stbi_loadf("assets/textures/hdr/newport_loft.hdr", &width, &height, &nrComponents, 0);
    if (data) {
        hdrMap = getTextureRGB32F(width, height);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, data);

        stbi_image_free(data);
    }
    else {
        std::cout << "Failed to load HDR image." << std::endl;
    }


    // Pipeline
    cg::Shader pass1Shader("assets/shaders/GLSL/pt_vert.glsl", "assets/shaders/GLSL/disney_frag.glsl");
    pass1.program = pass1Shader.ID;
    pass1.colorAttachments.push_back(getTextureRGB32F(pass1.width, pass1.height));
    pass1.colorAttachments.push_back(getTextureRGB32F(pass1.width, pass1.height));
    pass1.colorAttachments.push_back(getTextureRGB32F(pass1.width, pass1.height));
    pass1.bindData();
   
    glUseProgram(pass1.program);
    glUniform1i(glGetUniformLocation(pass1.program, "nTriangles"), triangles.size());
    glUniform1i(glGetUniformLocation(pass1.program, "nNodes"), nodes.size());
    glUniform1i(glGetUniformLocation(pass1.program, "width"), pass1.width);
    glUniform1i(glGetUniformLocation(pass1.program, "height"), pass1.height);
    glUseProgram(0);

    cg::Shader pass2Shader("assets/shaders/GLSL/pt_vert.glsl", "assets/shaders/GLSL/pt_pass2_frag.glsl");
    pass2.program = pass2Shader.ID;
    lastFrame = getTextureRGB32F(pass2.width, pass2.height);
    pass2.colorAttachments.push_back(lastFrame);
    pass2.bindData();

    cg::Shader pass3Shader("assets/shaders/GLSL/pt_vert.glsl", "assets/shaders/GLSL/pt_pass3_frag.glsl");
    pass3.program = pass3Shader.ID;
    pass3.bindData(true);

    while (!glfwWindowShouldClose(window)) {
        // render
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // camera settings
        vec3 eye = vec3(-sin(radians(rotatAngle)) * cos(radians(upAngle)), sin(radians(upAngle)), cos(radians(rotatAngle)) * cos(radians(upAngle)));
        eye.x *= r; eye.y *= r; eye.z *= r;
        mat4 cameraRotate = lookAt(eye, vec3(0, 0, 0), vec3(0, 1, 0));
        cameraRotate = inverse(cameraRotate);
        glUseProgram(pass1.program);
        glUniform3fv(glGetUniformLocation(pass1.program, "eye"), 1, value_ptr(eye));
        glUniformMatrix4fv(glGetUniformLocation(pass1.program, "cameraRotate"), 1, GL_FALSE, value_ptr(cameraRotate));
        glUniform1i(glGetUniformLocation(pass1.program, "frameCounter"), frameCounter++);  // random seed

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, trianglesTextureBuffer);
        glUniform1i(glGetUniformLocation(pass1.program, "triangles"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, nodesTextureBuffer);
        glUniform1i(glGetUniformLocation(pass1.program, "nodes"), 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, lastFrame);
        glUniform1i(glGetUniformLocation(pass1.program, "lastFrame"), 2);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, hdrMap);
        glUniform1i(glGetUniformLocation(pass1.program, "hdrMap"), 3);

        pass1.draw();
        pass2.draw(pass1.colorAttachments);
        pass3.draw(pass2.colorAttachments);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}