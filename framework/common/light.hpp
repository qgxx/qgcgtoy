#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace cg {

class DirectLight {
public:
    glm::vec3 mPos;
    glm::vec3 mFront;
    glm::vec3 mUp;
    glm::vec3 mRight;
    float Yaw = -90.0f;
    float Pitch = 0.0f;

public:
    DirectLight(glm::vec3 pos, glm::vec3 tar) {
        mPos = pos;
        mFront = glm::normalize(tar - pos);
        mRight = glm::normalize(glm::cross(mFront, glm::vec3(0.0f, 1.0f, 0.0f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        mUp = glm::normalize(glm::cross(mRight, mFront));
    }

    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(mPos, mPos + mFront, mUp);
    }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime)
    {
        float velocity = 2.5f * deltaTime;
        if (direction == FORWARD)
            mPos += mFront * velocity;
        if (direction == BACKWARD)
            mPos -= mFront * velocity;
        if (direction == LEFT)
            mPos -= mRight * velocity;
        if (direction == RIGHT)
            mPos += mRight * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
    {
        xoffset *= 2.5f;
        yoffset *= 2.5f;

        Yaw   += xoffset;
        Pitch += yoffset;

        // make sure that when pitch is out of bounds, screen doesn't get flipped
        if (constrainPitch)
        {
            if (Pitch > 89.0f)
                Pitch = 89.0f;
            if (Pitch < -89.0f)
                Pitch = -89.0f;
        }

        // update Front, Right and Up Vectors using the updated Euler angles
        updateCameraVectors();
    }

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // calculate the new Front vector
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        mFront = glm::normalize(front);
        // also re-calculate the Right and Up vector
        mRight = glm::normalize(glm::cross(mFront, glm::vec3(0.0f, 1.0f, 0.0f)));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
        mUp    = glm::normalize(glm::cross(mRight, mFront));
    }

};

}