/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#ifndef SHADER_CONTAINER_H
#define SHADER_CONTAINER_H

#include <vulkan/vulkan.h>

#include <string>
#include <unordered_map>
#include <vector>

struct ShaderSPIR_V {
    unsigned char type = 0; // ShaderContainer::StageType Undefined
    std::string entrypoint = {};
    std::vector<char> buffer = {};
    VkShaderModule module = {};
};

class ShaderContainer {
public:
    using StageType = unsigned char;
    constexpr static StageType Undefined = 0;
    constexpr static StageType Vertex = 1;
    constexpr static StageType Fragment = 2;

public:
    ShaderContainer() = default;
    explicit ShaderContainer(VkDevice* device) : m_device(device) {} ;
    ~ShaderContainer();

    inline void setDevice(VkDevice* device) { m_device = device; }
    inline VkDevice* device() { return m_device; }

    void addNewShader(const std::string& name, const std::string& filename, const std::string& entrypoint, StageType type);

    VkPipelineShaderStageCreateInfo generateCreateInfo(const std::string& name);

    std::vector<VkPipelineShaderStageCreateInfo> generateAllCreateInfos();

    void destroyAllShaderModules();

private:
    std::vector<char> readShaderFromBinary(const std::string& filename);

    VkShaderModule createShaderModule(const std::vector<char>& codes);

    VkShaderStageFlagBits convertStageType(StageType type);

private:
    VkDevice* m_device = nullptr;

    std::unordered_map<std::string, ShaderSPIR_V> m_shaders = {};
};

#endif // SHADER_CONTAINER_H
