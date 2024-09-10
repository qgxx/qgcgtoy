#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <random>

#include "shader.hpp"
#include "model.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "geo_render.hpp"
#include "umath.hpp"
#include "application.h"

namespace cg {

class RSM : public IApplication {

public:
    int initialize() override;
    void finalize() override;
    void tick() override;

    void setCommandLineParameters(int argc, char** argv) {
        m_nargc = argc;
        m_ppargv = argv;
    }
    int getCommandLineArgumentsCount() const override {
        return m_nargc;
    }
    const char* getCommandLineArgument(int index) const override {
        return m_ppargv[index];
    }

    bool isQuit() const override {
        return glfwWindowShouldClose(window);
    }

private:
    int m_nargc;
    char** m_ppargv;

private:
    GLFWwindow* window;

    Shader* sceneFlux;
    Shader* rsm;
    Shader* debug;
    Shader* lighting;

    Model* box;
    Model* marry;

    unsigned int gBuffer;
    unsigned int gNormal, gPosition, gFlux;
    unsigned int gDepth;
    unsigned int ubo;

    std::vector<glm::vec4> sampleCoordsandWeights;

    const int SAMPLE_NUMBER = 64;

};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
int hizPass(Shader* shader, unsigned int depthTexture, unsigned int p, unsigned int n, unsigned int c);
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1024;
const unsigned int SCR_HEIGHT = 1024;
const char* SCR_TITLE = "Reflective Shadow Maps";

// camera
Camera camera(glm::vec3(5.0f, 5.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// light
DirectLight light(glm::vec3(3.0f, 3.0f, 3.0f), glm::vec3(-3.0f, -3.0f, -3.0f));

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int RSM::initialize() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, SCR_TITLE, NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);

    // build and compile shaders
    // -------------------------
    sceneFlux = new Shader("assets/shaders/GLSL/scene_flux_vert.glsl", "assets/shaders/GLSL/scene_flux_frag.glsl");
    rsm = new Shader("assets/shaders/GLSL/rsm_vert.glsl", "assets/shaders/GLSL/rsm_frag.glsl");
    debug = new Shader("assets/shaders/GLSL/debug_vert.glsl", "assets/shaders/GLSL/debug_frag.glsl");
    lighting = new Shader("assets/shaders/GLSL/light_vert.glsl", "assets/shaders/GLSL/light_frag.glsl");

    // load models
    // -----------
    marry = new Model("assets/models/mary/Marry.obj");
    box = new Model("assets/models/CornellBox/CornellBox-Empty-RGB.obj");

    // configure g-buffer framebuffer
    // ------------------------------
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gFlux);
    glBindTexture(GL_TEXTURE_2D, gFlux);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gFlux, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    glGenTextures(1, &gDepth);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //随机采样map
    std::default_random_engine e;
    std::uniform_real_distribution<float> u(0, 1);
    for (int i = 0; i < SAMPLE_NUMBER; i++) {
        float xi1 = u(e);
        float xi2 = u(e);
        sampleCoordsandWeights.push_back({ xi1 * sin(2 * PI * xi2), xi1 * cos(2 * PI * xi2), xi1 * xi1, 0 });
    }
    unsigned int uniformBlockIndex = glGetUniformBlockIndex(rsm->ID, "randomMap");
    glUniformBlockBinding(rsm->ID, uniformBlockIndex, 0);

    glGenBuffers(1, &ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER, 4 * sizeof(GL_FLOAT) * sampleCoordsandWeights.size(), sampleCoordsandWeights.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, ubo, 0, 4 * sizeof(GL_FLOAT) * sampleCoordsandWeights.size());

    sceneFlux->use();
    sceneFlux->setVec3("lightPos", light.mPos);
    sceneFlux->setVec3("viewPos", camera.Position);

    rsm->use();
    rsm->setInt("normalTexture", 0);
    rsm->setInt("positionTexture", 1);
    rsm->setInt("fluxTexture", 2);
    rsm->setInt("depthTexture", 3);
    rsm->setFloat("filterRange", 25);

    debug->use();
    debug->setInt("normalTexture", 0);
    debug->setInt("positionTexture", 1);
    debug->setInt("fluxTexture", 2);
    debug->setInt("depthTexture", 3);

    return 1;
}

void RSM::finalize() {
    glfwTerminate();
}

void RSM::tick() {
    // per-frame time logic
    // --------------------
    float currentFrame = static_cast<float>(glfwGetTime());
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    // input
    // -----
    processInput(window);

    // render
    // ------
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glm::mat4 lightView = light.GetViewMatrix();   
    glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1e-2f, 20.0f);
    glm::mat4 lightSpace = lightProjection * lightView;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model,glm::vec3(3.0, 3.0, 3.0));
    sceneFlux->use();
    sceneFlux->setMat4("model", model);
    sceneFlux->setMat4("lightSpace", lightSpace);
    sceneFlux->setVec3("lightPos", light.mPos);
    sceneFlux->setVec3("viewPos", camera.Position);
    box->Draw(*sceneFlux);
    model = glm::mat4(1.0f);
    sceneFlux->setMat4("model", model);
    marry->Draw(*sceneFlux);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // debug->use();
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, gNormal);
    // glActiveTexture(GL_TEXTURE1);
    // glBindTexture(GL_TEXTURE_2D, gPosition);
    // glActiveTexture(GL_TEXTURE2);
    // glBindTexture(GL_TEXTURE_2D, gFlux);
    // glActiveTexture(GL_TEXTURE3);
    // glBindTexture(GL_TEXTURE_2D, gDepth);
    // renderQuad();

    rsm->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, gFlux);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, gDepth);

    glm::mat4 view(1.0);
    view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 50.0f);
    rsm->setMat4("view", view);
    rsm->setMat4("projection", projection);
    rsm->setMat4("lightSpace", lightSpace);
    rsm->setVec3("lightPos", light.mPos);
    rsm->setVec3("viewPos", camera.Position);
    model = glm::mat4(1.0f);
    model = glm::scale(model,glm::vec3(3.0, 3.0, 3.0));
    rsm->setMat4("model", model);
    box->Draw(*rsm);
    model = glm::mat4(1.0f);
    rsm->setMat4("model", model);
    marry->Draw(*rsm);

    lighting->use();
    lighting->setMat4("projection", projection);
    lighting->setMat4("view", view);
    model = glm::mat4(1.0f);
    model = glm::translate(model, light.mPos);
    model = glm::scale(model, glm::vec3(0.1f));
    lighting->setMat4("model", model);
    lighting->setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    renderSphere();

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(window);
    glfwPollEvents();
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

IApplication* g_pApp = static_cast<IApplication*>(new RSM);

}