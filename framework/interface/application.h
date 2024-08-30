#pragma once

namespace cg {

class IApplication {
public:
    virtual int initialize() = 0;
    virtual void finalize() = 0;
    virtual void tick() = 0;

    virtual void setCommandLineParameters(int argc, char** argv) = 0;
    virtual int  getCommandLineArgumentsCount() const = 0;
    virtual const char* getCommandLineArgument(int index) const = 0;

    virtual bool isQuit() const = 0;

};

extern IApplication* g_pApp;

}