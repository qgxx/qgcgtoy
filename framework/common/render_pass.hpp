#pragma once 

#include <glad/glad.h>

#include <glm/glm.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>

namespace cg {

class RenderPass {
public:
    GLuint FBO = 0;
    GLuint vao, vbo;
    std::vector<GLuint> colorAttachments;
    GLuint program;
    int width = 512;
    int height = 512;

    void bindData(bool finalPass = false) {
        if (!finalPass) glGenFramebuffers(1, &FBO);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        std::vector<glm::vec3> square = { glm::vec3(-1, -1, 0), glm::vec3(1, -1, 0), glm::vec3(-1, 1, 0), glm::vec3(1, 1, 0), glm::vec3(-1, 1, 0), glm::vec3(1, -1, 0) };
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * square.size(), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3) * square.size(), &square[0]);

        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glEnableVertexAttribArray(0);   // layout (location = 0) 
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
        
        if (!finalPass) {
            std::vector<GLuint> attachments;
            for (int i = 0; i < colorAttachments.size(); i++) {
                glBindTexture(GL_TEXTURE_2D, colorAttachments[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorAttachments[i], 0);
                attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
            }
            glDrawBuffers(attachments.size(), &attachments[0]);
        }
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void draw(std::vector<GLuint> texPassArray = {}) {
        glUseProgram(program);
        glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        glBindVertexArray(vao);
        
        // last frame's color-attachments
        for (int i = 0; i < texPassArray.size(); i++) {
            glActiveTexture(GL_TEXTURE0+i);
            glBindTexture(GL_TEXTURE_2D, texPassArray[i]);
            std::string uName = "texPass" + std::to_string(i);
            glUniform1i(glGetUniformLocation(program, uName.c_str()), i);
        }
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glUseProgram(0);
    }

};

} // namespace cg