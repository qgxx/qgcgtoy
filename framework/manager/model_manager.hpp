#pragma once

#include <map>

#include <glm/glm.hpp>
#include <imgui/imgui.h>

#include "model.hpp"

namespace cg {

class ModelManager {

public:
    struct ModelMatrix {
        glm::mat4 translate = glm::mat4(0.0f);
        glm::mat4 scale = glm::mat4(1.0f);
        glm::mat4 rotate = glm::mat4(1.0f);
    };

public:
    void add(Model* model) {
        ModelMatrix matrix;
        manager[model] = matrix;
    }
    void editor() {
        static Model* selected;
        ImGui::Begin("Model Editor");
        ImGui::End;
    }
    ModelMatrix get(Model* model) {
        return manager[model];
    }
    
private:
    std::map<Model*, ModelMatrix> manager;

};

};