/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#include <fstream>
#include <ios>
#include <unistd.h>

#include "Platforms/ExecuteCommand.h"
#include "ShaderContainer.h"

ShaderContainer::~ShaderContainer() {
    destroyAllShaderModules(); // In case someone forgets destroy created shader modules.
}

void ShaderContainer::addGlslShader(const std::string& name, const std::string& filename, const std::string& binaryStorePath,
                                    const std::string& entrypoint, ShaderContainer::StageType type) {
    std::string glslcShaderStage;
    compileGlslShader(filename, binaryStorePath);
    addCompiledShader(name, binaryStorePath, entrypoint, type);
}

void ShaderContainer::compileGlslShader(const std::string& filename, const std::string& binaryStorePath) {
    const char* glslcArgs[] = { "-o", binaryStorePath.c_str(), filename.c_str() };
    if (!executeCommand("/Users/fort.w/VulkanSDK/1.2.189.0/macOS/bin/glslc", glslcArgs, 3)) {
        throw std::runtime_error("Failed to execute GLSL compilation command.");
    }
}

void ShaderContainer::addCompiledShader(
        const std::string& name,
        const std::string& binaryName,
        const std::string& entrypoint,
        ShaderContainer::StageType type)
{
    if (m_device == nullptr) return;

    ShaderSPIR_V shader = {};

    shader.type = type;
    shader.entrypoint = entrypoint;
    shader.buffer = readShaderFromBinary(binaryName);
    shader.module = createShaderModule(shader.buffer);

    m_shaders[name] = shader;
}

VkPipelineShaderStageCreateInfo ShaderContainer::generateCreateInfo(const std::string& name) {
    const auto& spirv = m_shaders[name];
    VkPipelineShaderStageCreateInfo info = {};

    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    convertStageType(spirv.type, info.stage);
    info.module = spirv.module;
    info.pName = spirv.entrypoint.c_str();
    info.pSpecializationInfo = nullptr;

    return info;
}

std::vector<VkPipelineShaderStageCreateInfo> ShaderContainer::generateAllCreateInfos() {
    std::vector<VkPipelineShaderStageCreateInfo> infos(m_shaders.size());

    size_t i = 0;
    for (const auto& shader : m_shaders) {
        infos[i++] = generateCreateInfo(shader.first);
    }

    return infos;
}

void ShaderContainer::destroyAllShaderModules() {
    for (auto& shader : m_shaders) {
        vkDestroyShaderModule(*m_device, shader.second.module, nullptr);
    }
    m_shaders.clear();
}

std::vector<char> ShaderContainer::readShaderFromBinary(const std::string& filename) {

    // ate: Start reading at the end  of the file. The advantage is that we can
    // use the read position to determine the size of the file and allocate a buffer.
    std::ifstream fin(filename, std::ios::ate | std::ios::binary);

    if (!fin.is_open()) {
        throw std::runtime_error("Failed to open SPIR-V file " + filename + ".");
    }

    size_t fileSize = fin.tellg();
    std::vector<char> buffer(fileSize);

    fin.seekg(0);
    fin.read(buffer.data(), fileSize);

    fin.close();

    return buffer;
}

VkShaderModule ShaderContainer::createShaderModule(const std::vector<char>& codes) {
    VkShaderModuleCreateInfo shaderModuleInfo = {};
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleInfo.codeSize = codes.size();
    shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(codes.data());

    VkShaderModule shaderModule = {};
    if (vkCreateShaderModule(*m_device, &shaderModuleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module.");
    }

    return shaderModule;
}

void ShaderContainer::convertStageType(ShaderContainer::StageType type, VkShaderStageFlagBits& target) {
    switch (type) {
        case Undefined:
            throw std::runtime_error("Undefined shader stage type.");
        case Vertex:
            target = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case Fragment:
            target = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        default:
            throw std::runtime_error("Unknown shader stage type.");
    }
}
