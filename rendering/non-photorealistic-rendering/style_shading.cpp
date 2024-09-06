#include "application.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>

#include "model.hpp"
#include "shader.hpp"
#include "camera.hpp"

namespace cg {

class StyleShading : public IApplication {
public:
    virtual int initialize();
    virtual void finalize();
    virtual void tick();

    virtual void setCommandLineParameters(int argc, char** argv) final {
        m_nArgc = argc;
        m_ppArgv = argv;
    }
    virtual int  getCommandLineArgumentsCount() const final {
        return m_nArgc;
    }
    virtual const char* getCommandLineArgument(int index) const final {
        return m_ppArgv[index];
    }

    virtual bool isQuit() const final {
        return glfwWindowShouldClose(window);
    }

private:
    int m_nArgc;
    char** m_ppArgv;

private:
    GLFWwindow* window;

    Shader* toonShader;
    Shader* goochShader;
    Shader* sobelShader;
    Shader* hatchingShader;
    Shader* lightShader;

    Model* teapot;

    glm::vec3 lightPosition = glm::vec3(0.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

};


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void processInput(GLFWwindow *window);
void configureFBO(GLFWwindow* window, GLuint* FBO, std::vector<GLuint*>* textures, bool multisample, bool mipmap, bool depthOrStencil);
void renderSphere();
void renderCube();
void renderQuad();

// settings
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const char* SCR_TITLE = "Toon Shading";

// camera
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = 800.0f / 2.0;
float lastY = 600.0 / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

int m_useCursor = 0;

GLuint goochFBO, imageTextureMSAA, normalTextureMSAA, depthTextureMSAA;
std::vector<GLuint*> renderTargetsMSAA;

GLuint intermediateFBO, imageTexture, normalTexture, depthTexture;
std::vector<GLuint*> intermediateRenderTargets;

GLuint hatchingFBO, hatching0, hatching1, hatching2, hatching3, hatching4, hatching5;
std::vector<GLuint*> hatchingRenderTargets; // G buffers

GLuint GBuffers[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

const char* styles[] = { "Toon", "Gooch", "Gooch + Sobel", "Hatching" };
int style = 0;

int StyleShading::initialize() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, SCR_TITLE, NULL, NULL);
    glfwMakeContextCurrent(window);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext(nullptr);
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // configure global opengl state
	// -----------------------------
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    toonShader = new Shader("assets/shaders/GLSL/default_vert.glsl", "assets/shaders/GLSL/toonshading_frag.glsl");
    goochShader = new Shader("assets/shaders/GLSL/default_vert.glsl", "assets/shaders/GLSL/goochshading_frag.glsl");
    sobelShader = new Shader("assets/shaders/GLSL/render_quad_vert.glsl", "assets/shaders/GLSL/sobel_outline_frag.glsl");
    hatchingShader = new Shader("assets/shaders/GLSL/default_vert.glsl", "assets/shaders/GLSL/hatching_frag.glsl");
    lightShader = new Shader("assets/shaders/GLSL/light_vert.glsl", "assets/shaders/GLSL/light_frag.glsl");

    teapot = new Model("assets/models/teapot/teapot.obj");

	// configure frame buffer objects
	// ---------------------------------
	glGenTextures(1, &imageTextureMSAA);
	renderTargetsMSAA.push_back(&imageTextureMSAA);
	glGenTextures(1, &normalTextureMSAA);
	renderTargetsMSAA.push_back(&normalTextureMSAA);
	glGenTextures(1, &depthTextureMSAA);
	renderTargetsMSAA.push_back(&depthTextureMSAA);
	configureFBO(window, &goochFBO, &renderTargetsMSAA, true, false, true);

	glGenTextures(1, &imageTexture);
	intermediateRenderTargets.push_back(&imageTexture);
	glGenTextures(1, &normalTexture);
	intermediateRenderTargets.push_back(&normalTexture);
	glGenTextures(1, &depthTexture);
	intermediateRenderTargets.push_back(&depthTexture);
	configureFBO(window, &intermediateFBO, &intermediateRenderTargets, false, false, false);

	glGenTextures(1, &hatching0);
	hatchingRenderTargets.push_back(&hatching0);
	glGenTextures(1, &hatching1);
	hatchingRenderTargets.push_back(&hatching1);
	glGenTextures(1, &hatching2);
	hatchingRenderTargets.push_back(&hatching2);
	glGenTextures(1, &hatching3);
	hatchingRenderTargets.push_back(&hatching3);
	glGenTextures(1, &hatching4);
	hatchingRenderTargets.push_back(&hatching4);
	glGenTextures(1, &hatching5);
	hatchingRenderTargets.push_back(&hatching5);
	configureFBO(window, &hatchingFBO, &hatchingRenderTargets, false, true, false);

	// assign render targets for goochFBO and intermediateFBO
	// -----------------------------------
	glBindFramebuffer(GL_FRAMEBUFFER, goochFBO);
	glDrawBuffers(3, GBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, intermediateFBO);
	glDrawBuffers(3, GBuffers);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// shader configuration
	// ---------------------------------
	sobelShader->use();
	sobelShader->setInt("imageTexture", 0);
	sobelShader->setInt("normalTexture", 1);
	sobelShader->setInt("depthTexture", 2);

	hatchingShader->use();
	hatchingShader->setInt("hatching0", 0);
	hatchingShader->setInt("hatching1", 1);
	hatchingShader->setInt("hatching2", 2);
	hatchingShader->setInt("hatching3", 3);
	hatchingShader->setInt("hatching4", 4);
	hatchingShader->setInt("hatching5", 5);

    return 1;
}

void StyleShading::finalize() {
    glfwTerminate();
}

void StyleShading::tick() {
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
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);	// Set background color to black and opaque
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

    glm::vec3 new_lightPos = lightPosition + glm::vec3(sin(glfwGetTime()) * 10.0, 0.0, cos(glfwGetTime()) * 10.0);

    if (style == 0) {
        toonShader->use();
        toonShader->setVec3("lightPos", new_lightPos);
        toonShader->setVec3("lightColor", lightColor);
        toonShader->setVec3("objectColor", 1.0f, 0.5f, 0.5f);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, -5.0f));
        model = glm::scale(model, glm::vec3(0.05f));
        toonShader->setMat4("model", model);
        toonShader->setMat4("view", view);
        toonShader->setMat4("projection", projection);
        toonShader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        toonShader->setVec3("viewPos", camera.Position);
        teapot->Draw();
    }
    else if (style == 1) {
        // First pass: Render the model using Gooch shading, and render the camera-space normals and fragment depths to the other render targets
        goochShader->use();
        // set uniforms
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, -5.0f));
        model = glm::scale(model, glm::vec3(0.05f));
        goochShader->setMat4("model", model);
        goochShader->setMat4("view", view);
        goochShader->setMat4("projection", projection);
        goochShader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        goochShader->setVec3("lightPos", new_lightPos);
        goochShader->setVec3("viewPos", camera.Position);
        goochShader->setVec3("coolColor", 0.0f, 0.0f, 0.8f);
        goochShader->setVec3("warmColor", 0.4f, 0.4f, 0.0f);
        goochShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        goochShader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        goochShader->setFloat("specularStrength", 0.5f);
        goochShader->setFloat("alpha", 0.25f);
        goochShader->setFloat("beta", 0.5f);

        // render model
        teapot->Draw();
    }
    else if (style == 2) {
        glBindFramebuffer(GL_FRAMEBUFFER, goochFBO);
        glEnable(GL_DEPTH_TEST); // enable depth testing (is disabled for rendering screen-space quad)

        // Before rendering the first pass
        // clear the image's render target to white
        glDrawBuffer(GBuffers[0]);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // clear the normals' render target to a vector facing away from the camera
        glDrawBuffer(GBuffers[1]);
        glm::vec3 clearVec(0.0f, 0.0f, -1.0f);
        // from normalized vector to rgb color; from [-1,1] to [0,1]
        clearVec = (clearVec + glm::vec3(1.0f, 1.0f, 1.0f)) * 0.5f;
        glClearColor(clearVec.x, clearVec.y, clearVec.z, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // clear the depth's render target to black
        glDrawBuffer(GBuffers[2]);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // now enable all render targets for drawing
        glDrawBuffers(3, GBuffers);
        glClear(GL_DEPTH_BUFFER_BIT);


        // First pass: Render the model using Gooch shading, and render the camera-space normals and fragment depths to the other render targets
        goochShader->use();
        // set uniforms
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, -5.0f));
        model = glm::scale(model, glm::vec3(0.05f));
        goochShader->setMat4("model", model);
        goochShader->setMat4("view", view);
        goochShader->setMat4("projection", projection);
        goochShader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        goochShader->setVec3("lightPos", new_lightPos);
        goochShader->setVec3("viewPos", camera.Position);
        goochShader->setVec3("coolColor", 0.0f, 0.0f, 0.8f);
        goochShader->setVec3("warmColor", 0.4f, 0.4f, 0.0f);
        goochShader->setVec3("objectColor", 1.0f, 1.0f, 1.0f);
        goochShader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        goochShader->setFloat("specularStrength", 0.5f);
        goochShader->setFloat("alpha", 0.25f);
        goochShader->setFloat("beta", 0.5f);

        // render model
        teapot->Draw();

        // now blit multisampled buffer(s) to G intermediateFBO's G buffers
        glBindFramebuffer(GL_READ_FRAMEBUFFER, goochFBO);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBO);

        // must copy each color attachment one at a time
        // blitting a multisampled source texture into a singlesampled destination takes care of MSAA resolve
        // the resulting texture is anti-aliased
        glReadBuffer(GBuffers[0]); // image
        glDrawBuffer(GBuffers[0]);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glReadBuffer(GBuffers[1]); // normal
        glDrawBuffer(GBuffers[1]);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glReadBuffer(GBuffers[2]); // depth
        glDrawBuffer(GBuffers[2]);
        glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
            
        // Second pass: Do a full-screen edge detection filter over the normals from the first pass and draw feature edges
        // bind back to default framebuffer and draw a quad plane with the attached framebuffer color textures
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        //glDisable(GL_DEPTH_TEST); // disable depth test so screen-space quad isn't discarded due to depth test.
        // clear all relevant buffers
        // set background to white to be able to see rendered outline
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        sobelShader->use();

        // activate textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *intermediateRenderTargets[0]); // image
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, *intermediateRenderTargets[1]); // normal
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, *intermediateRenderTargets[2]); // depth
    
        // set uniforms
        sobelShader->setVec4("_OutlineColor", 0.0f, 0.0f, 0.0f, 1.0f);
        sobelShader->setFloat("depthThreshold", 0.04f);
        sobelShader->setFloat("normalThreshold", 0.085f);
        /*sobelShader.setFloat("depthThreshold", 0.075f);
        sobelShader.setFloat("normalThreshold", 0.075f);*/

        // finally render quad
        renderQuad();
    }
    else if (style == 3) {
        hatchingShader->use();

        // activate textures
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, *hatchingRenderTargets[0]); // light
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, *hatchingRenderTargets[1]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, *hatchingRenderTargets[2]);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, *hatchingRenderTargets[3]);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, *hatchingRenderTargets[4]);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, *hatchingRenderTargets[5]); // dark

        // set uniforms
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, -3.0f, -5.0f));
        model = glm::scale(model, glm::vec3(0.05f));
        hatchingShader->setMat4("model", model);
        hatchingShader->setMat4("view", view);
        hatchingShader->setMat4("projection", projection);
        hatchingShader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
        hatchingShader->setVec3("lightPos", new_lightPos);
        hatchingShader->setVec3("viewPos", camera.Position);
        hatchingShader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);

        // render model
        teapot->Draw();
    }

    lightShader->use();
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::mat4(1.0f);
    model = glm::translate(model, new_lightPos);
    model = glm::scale(model, glm::vec3(0.05f));
    lightShader->setMat4("model", model);
    lightShader->setMat4("view", view);
    lightShader->setMat4("projection", projection);
    lightShader->setVec3("lightColor", lightColor);
    renderCube();

    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Styles Select");
        ImGui::Combo("Styles: ", &style, styles, IM_ARRAYSIZE(styles));
        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

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
    if (m_useCursor) return;
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

// renderCube() renders a 1x1 3D cube in NDC.
// -------------------------------------------------
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// Bind textures and buffers to frame buffer object
// ----------------------------------------------------------------------
void configureFBO(GLFWwindow* window, GLuint* FBO, std::vector<GLuint*>* textures, bool multisample, bool mipmap, bool depthOrStencil) {

	glGenFramebuffers(1, FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, *FBO);

	// get default textures
	int width, height, nrChannels;

	// generate texture buffers
	for (int i = 0; i < (*textures).size(); i++)
	{
		if (multisample)
		{
			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, *(*textures)[i]);
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, SCR_WIDTH, SCR_HEIGHT, GL_TRUE);

			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D_MULTISAMPLE, *(*textures)[i], 0);

			glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		}
		else
		{
			glBindTexture(GL_TEXTURE_2D, *(*textures)[i]);

			if (mipmap) // create mipmaps for hatching textures
			{
				std::string filename = std::string("assets/textures/hatch/hatch_") + std::to_string(i) + std::string(".jpg");
				unsigned char* tex = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, tex);
				glGenerateMipmap(GL_TEXTURE_2D);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // trilinear filtering
				stbi_image_free(tex);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
			}
			
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, *(*textures)[i], 0);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	if (depthOrStencil)
	{
		// create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
		GLuint rbo;
		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);

		// use a single renderbuffer object for both a depth AND stencil buffer
		if (multisample)
		{
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
		}
		else
		{
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
		}
		
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
	}
	
	// now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

IApplication* g_pApp = static_cast<IApplication*>(new StyleShading);

} // namespace cg