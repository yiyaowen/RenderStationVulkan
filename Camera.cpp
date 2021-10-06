/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

void Camera::updateViewMatrix(glm::mat4& viewMat) {
    m_right.y = 0.0f;
    m_right = glm::normalize(m_right);
    m_lookAt = glm::normalize(glm::cross(m_right, m_up));
    m_up = glm::cross(m_lookAt, m_right);

    viewMat = glm::lookAtLH(m_eye, m_eye + m_lookAt, m_up);
}

void Camera::updateProjMatrix(glm::mat4& projMat) {
    projMat = glm::perspectiveFovLH(glm::radians(m_verticalFov),
                                    static_cast<float>(m_viewWidth),
                                    static_cast<float>(m_viewHeight),
                                    m_nearZ, m_farZ);
}

void Camera::translate(float dx, float dy, float dz) {
    m_eye += glm::vec3(dx, dy, dz);
}

void Camera::rotate(float pitch, float yaw, float roll) {
    auto doPitch = glm::rotate(glm::mat4(1.0f), pitch, m_right);
    auto doPitchYaw = glm::rotate(doPitch, yaw, m_up);
//    auto doRoll = glm::rotate(doPitchYaw, roll, m_lookAt);

    auto right4 = glm::vec4(m_right, 0.0f);
    auto up4 = glm::vec4(m_up, 0.0f);
    auto lookAt4 = glm::vec4(m_lookAt, 0.0f);

    right4 = right4 * doPitchYaw;
    up4 = up4 * doPitchYaw;
    lookAt4 = lookAt4 * doPitchYaw;

    m_right = glm::vec3(right4);
    m_up = glm::vec3(up4);
    m_lookAt = glm::vec3(lookAt4);
}

void Camera::zoom(float delta) {
    glm::vec3 T = delta * m_lookAt;
    translate(T.x, T.y, T.z);
}
