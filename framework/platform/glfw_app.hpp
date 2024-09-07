#pragma once

#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace cg {

class GLFWApp {
public:
    struct Configure {
        unsigned int SCR_WIDTH;
        unsigned int SCR_HEIGHT;
        const char* SCR_TITLE;

        void(*framebuffer_size_callback)(GLFWwindow*, int, int);
        void(*mouse_callback)(GLFWwindow*, double, double);
        void(*scroll_callback)(GLFWwindow*, double, double);
        void(*key_callback)(GLFWwindow*, int, int, int, int);
    };

public:
    static GLFWwindow* instance(Configure config);
    static int config();

protected:
    static GLFWwindow* window;

    static Configure m_configure;

private:
    GLFWApp();

};

} // namespace cg