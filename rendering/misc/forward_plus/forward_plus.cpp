#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "geo_render.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>

using namespace cg;
using namespace std;

// Renderer constants
const glm::ivec2 SCREEN_SIZE(800, 600);
const unsigned int NUM_LIGHTS = 1024;
const float LIGHT_RADIUS = 30.0f;
const float NEAR_PLANE = 0.1f;
const float FAR_PLANE = 1500.0f;

// Defines exposure level for HDR lighting
const float exposure = 1.0f;

// Constants for light animations
const glm::vec3 LIGHT_MIN_BOUNDS = glm::vec3(-135.0f, -20.0f, -60.0f);
const glm::vec3 LIGHT_MAX_BOUNDS = glm::vec3(135.0f, 170.0f, 60.0f);
const float LIGHT_DELTA_TIME = -0.6f;

// Mouse and keyboard variables
bool firstMouse = true;
GLfloat lastX = 400.0f, lastY = 300.0f;
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// Used for storage buffer objects to hold light data and visible light indicies data
GLuint lightBuffer = 0;
GLuint visibleLightIndicesBuffer = 0;

// structures defining the data of both buffers
struct PointLight {
	glm::vec4 color;
	glm::vec4 position;
	glm::vec4 paddingAndRadius;
};

struct VisibleIndex {
	int index;
};

// X and Y work group dimension variables for compute shader
GLuint workGroupsX = 0;
GLuint workGroupsY = 0;

// Camera object
Camera camera(glm::vec3(0.0f, -1.0f, 0.0f));

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);

glm::vec3 RandomPosition(uniform_real_distribution<> dis, mt19937 gen) {
	glm::vec3 position = glm::vec3(0.0);
	for (int i = 0; i < 3; i++) {
		float min = LIGHT_MIN_BOUNDS[i];
		float max = LIGHT_MAX_BOUNDS[i];
		position[i] = (GLfloat)dis(gen) * (max - min) + min;
	}

	return position;
}

void SetupLights() {
	if (lightBuffer == 0) {
		return;
	}
	
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<> dis(0, 1);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	PointLight *pointLights = (PointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	for (int i = 0; i < NUM_LIGHTS; i++) {
		PointLight &light = pointLights[i];
		light.position = glm::vec4(RandomPosition(dis, gen), 1.0f);
		light.color = glm::vec4(1.0f + dis(gen), 1.0f + dis(gen), 1.0f + dis(gen), 1.0f);
		light.paddingAndRadius = glm::vec4(glm::vec3(0.0f), LIGHT_RADIUS);
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void UpdateLights() {
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	PointLight *pointLights = (PointLight*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_WRITE);

	for (int i = 0; i < NUM_LIGHTS; i++) {
		PointLight &light = pointLights[i];
		float min = LIGHT_MIN_BOUNDS[1];
		float max = LIGHT_MAX_BOUNDS[1];

		light.position.y = fmod((light.position.y + (-4.5f * deltaTime) - min + max), max) + min;
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCREEN_SIZE.x, SCREEN_SIZE.y, "Forward Plus", NULL, NULL);
    if (window == NULL)
    {
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
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Enable any OpenGL features we want to use
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);

    camera.MovementSpeed = 20.0f;


    Shader depthShader("assets/shaders/GLSL/depth_vert.glsl", "assets/shaders/GLSL/depth_frag.glsl");
    Shader lightCullingShader("assets/shaders/GLSL/light_culling_comp.glsl");
    Shader lightAccumulationShader("assets/shaders/GLSL/light_accumulation_vert.glsl", "assets/shaders/GLSL/light_accumulation_frag.glsl");
	Shader hdrShader("assets/shaders/GLSL/hdr_vert.glsl", "assets/shaders/GLSL/hdr_frag.glsl");

    GLuint depthMapFBO;
	glGenFramebuffers(1, &depthMapFBO);

	GLuint depthMap;
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCREEN_SIZE.x, SCREEN_SIZE.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Create a floating point HDR frame buffer and a floating point color buffer (as a texture)
	GLuint hdrFBO;
	glGenFramebuffers(1, &hdrFBO);

	GLuint colorBuffer;
	glGenTextures(1, &colorBuffer);
	glBindTexture(GL_TEXTURE_2D, colorBuffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCREEN_SIZE.x, SCREEN_SIZE.y, 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// It will also need a depth component as a render buffer, attached to the hdrFBO
	GLuint rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCREEN_SIZE.x, SCREEN_SIZE.y);

	glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBuffer, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Model sponzaModel("assets/models/crytek-sponza/sponza.obj");
    
    // Define work group sizes in x and y direction based off screen size and tile size (in pixels)
	workGroupsX = (SCREEN_SIZE.x + (SCREEN_SIZE.x % 16)) / 16;
	workGroupsY = (SCREEN_SIZE.y + (SCREEN_SIZE.y % 16)) / 16;
	size_t numberOfTiles = workGroupsX * workGroupsY;

	// Generate our shader storage buffers
	glGenBuffers(1, &lightBuffer);
	glGenBuffers(1, &visibleLightIndicesBuffer);

	// Bind light buffer
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, NUM_LIGHTS * sizeof(PointLight), 0, GL_DYNAMIC_DRAW);

	// Bind visible light indices buffer
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, visibleLightIndicesBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, numberOfTiles * sizeof(VisibleIndex) * 1024, 0, GL_STATIC_DRAW);

	// Set the default values for the light buffer
	SetupLights();

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);


    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
    // For each shader, bind the uniforms that will during the program's execution
	depthShader.use();
	glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));

	lightCullingShader.use();
	glUniform1i(glGetUniformLocation(lightCullingShader.ID, "lightCount"), NUM_LIGHTS);
	glUniform2iv(glGetUniformLocation(lightCullingShader.ID, "screenSize"), 1, &SCREEN_SIZE[0]);

	lightAccumulationShader.use();
	glUniformMatrix4fv(glGetUniformLocation(lightAccumulationShader.ID, "model"), 1, GL_FALSE, glm::value_ptr(model));
	glUniform1i(glGetUniformLocation(lightAccumulationShader.ID, "numberOfTilesX"), workGroupsX);

    // Set viewport dimensions and background color
	glViewport(0, 0, SCREEN_SIZE.x, SCREEN_SIZE.y);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    while (!glfwWindowShouldClose(window)) {
        // per-frame time logic
        // --------------------
        auto currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // input
        // -----
        processInput(window);

        UpdateLights();

		// Calculate new projection and view matrices and save current cameraPosition
		glm::mat4 projection = glm::perspective(camera.Zoom, (float)SCREEN_SIZE.x / (float)SCREEN_SIZE.y, NEAR_PLANE, FAR_PLANE);
		glm::mat4 view = camera.GetViewMatrix();
		glm::vec3 cameraPosition = camera.Position;

		// Step 1: Render the depth of the scene to a depth map
		depthShader.use();
		glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(depthShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		
		// Bind the depth map's frame buffer and draw the depth map to it
		glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
		sponzaModel.Draw(depthShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		// Step 2: Perform light culling on point lights in the scene
		lightCullingShader.use();
		glUniformMatrix4fv(glGetUniformLocation(lightCullingShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(lightCullingShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));

		// Bind depth map texture to texture location 4 (which will not be used by any model texture)
		glActiveTexture(GL_TEXTURE4);
		glUniform1i(glGetUniformLocation(lightCullingShader.ID, "depthMap"), 4);
		glBindTexture(GL_TEXTURE_2D, depthMap);
		
		// Bind shader storage buffer objects for the light and indice buffers
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightBuffer);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, visibleLightIndicesBuffer);


		// Dispatch the compute shader, using the workgroup values calculated earlier
		glDispatchCompute(workGroupsX, workGroupsY, 1);

		// Unbind the depth map
		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Step 3: Accumulate the remaining lights after culling and render (or execute one of the debug views of a flag is enabled
		// (Or run one of the debug shaders)
		// We render the scene into the floating point HDR frame buffer
		glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		lightAccumulationShader.use();
		glUniformMatrix4fv(glGetUniformLocation(lightAccumulationShader.ID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(glGetUniformLocation(lightAccumulationShader.ID, "view"), 1, GL_FALSE, glm::value_ptr(view));
		glUniform3fv(glGetUniformLocation(lightAccumulationShader.ID, "viewPosition"), 1, &cameraPosition[0]);

		// Blending might be needed if we add additional passes, but may not work correctly with objects with transparency masks
		// Disabled for now
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA);

		sponzaModel.Draw(lightAccumulationShader);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		//glDisable(GL_BLEND);
		
		// Tonemap the HDR colors to the default framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Weirdly, moving this call drops performance into the floor
		hdrShader.use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, colorBuffer);
		glUniform1f(glGetUniformLocation(hdrShader.ID, "exposure"), exposure);
		renderQuad();
		
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
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