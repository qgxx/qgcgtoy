#pragma once

#include <GLFW/glfw3.h>
#include "application.h"

namespace cg {

class WindonwsApplication : public IApplication {

public:
    int initialize() override;
    void finalize() = 0;
    void tick() = 0;

    void setCommandLineParameters(int argc, char** argv) final {
        m_nargc = argc;
        m_ppargv = argv;
    }
    int  getCommandLineArgumentsCount() const final {
        return m_nargc;
    }
    const char* getCommandLineArgument(int index) const final {
        return m_ppargv[index];
    }

protected:
    int m_nargc;
    char** m_ppargv;

    GLFWwindow* window;

};

int WindonwsApplication::initialize() {
    
}

}