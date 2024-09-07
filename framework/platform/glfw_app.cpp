#include "glfw_app.hpp"

#include <iostream>

#include "camera.hpp"

namespace cg {

extern Camera main_camera;

int m_useCursor;
int firstMouse;
float lastX, lastY;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

static GLFWwindow* window = nullptr;

static GLFWApp::Configure m_configure = {1280, 720, "GLFW", framebuffer_size_callback, mouse_callback, 
    scroll_callback, key_callback};

GLFWwindow* GLFWApp::instance(Configure config) {
    m_configure = config;

    if (window == nullptr) {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(m_configure.SCR_WIDTH, m_configure.SCR_HEIGHT, m_configure.SCR_TITLE, NULL, NULL);
    }
    return window;
}

int GLFWApp::config() {
    glfwMakeContextCurrent(window);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    if (m_configure.framebuffer_size_callback != nullptr) 
        glfwSetFramebufferSizeCallback(window, m_configure.framebuffer_size_callback);
    if (m_configure.mouse_callback != nullptr)  
        glfwSetCursorPosCallback(window, m_configure.mouse_callback);
    if (m_configure.scroll_callback != nullptr) 
        glfwSetScrollCallback(window, m_configure.scroll_callback);
    if (m_configure.key_callback != nullptr) 
        glfwSetKeyCallback(window, m_configure.key_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

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

    main_camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    main_camera.ProcessMouseScroll(static_cast<float>(yoffset));
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

} // namespace cg