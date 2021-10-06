/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#ifndef GRAPHICS_RESOURCES_H
#define GRAPHICS_RESOURCES_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include <array>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription generateBindingDescription() {
        VkVertexInputBindingDescription bindDesc = {};

        bindDesc.binding = 0;
        bindDesc.stride = sizeof(Vertex);
        bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindDesc;
    }

    static std::array<VkVertexInputAttributeDescription, 2> generateAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attrDescs = {};

        attrDescs[0].binding = 0;
        attrDescs[0].location = 0;
        attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrDescs[0].offset = offsetof(Vertex, pos);

        attrDescs[1].binding = 0;
        attrDescs[1].location = 1;
        attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrDescs[1].offset = offsetof(Vertex, color);

        return attrDescs;
    }
};

struct UniformBufferObject {
    glm::mat4 modelMat;
    glm::mat4 viewMat;
    glm::mat4 projMat;
};

#endif // GRAPHICS_RESOURCES_H