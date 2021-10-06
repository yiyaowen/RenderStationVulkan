/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#ifndef CAMERA_H
#define CAMERA_H

#include "glm/glm.hpp"

class Camera {
public:
    Camera(int viewWidth, int viewHeight, float verticalFov) // View size in pixel unit.
        : m_viewWidth(viewWidth), m_viewHeight(viewHeight), m_verticalFov(verticalFov) {}

    void updateViewMatrix(glm::mat4& viewMat);

    void updateProjMatrix(glm::mat4& projMat);

    inline void setFrustumDepth(float nearZ, float farZ) { m_nearZ = nearZ; m_farZ = farZ; }

    inline glm::vec3 eye() const { return m_eye; }
    inline void setEye(const glm::vec3& value) { m_eye = value; }

    inline glm::vec3 right() const { return m_right; }
    inline void setRight(const glm::vec3& value) { m_right = value; }

    inline glm::vec3 up() const { return m_up; }
    inline void setUp(const glm::vec3& value) { m_up = value; }

    inline glm::vec3 lookAt() const { return m_lookAt; }
    inline void setLookAt(const glm::vec3& value) { m_lookAt = value; }

private:
    int m_viewWidth = 0, m_viewHeight = 0;
    float m_verticalFov = 0.0f;
    float m_nearZ = 0.1f, m_farZ = 100.0f;

    glm::vec3 m_eye = {};

    glm::vec3 m_right = {}, m_up = {}, m_lookAt = {};

public:
    void translate(float dx, float dy, float dz);

    void rotate(float pitch, float yaw, float roll = 0.0f);

    void zoom(float delta);
};

#endif //CAMERA_H
