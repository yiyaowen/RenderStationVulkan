/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#ifndef VULKAN_ENGINE_H
#define VULKAN_ENGINE_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_metal.h>

#include <QDebug>

#include "Camera.h"
#include "GraphicsResource.h"
#include "ShaderContainer.h"

#define FUNC_PARAM_UNUSED(x) ((void)(x))

namespace VulkanEngineStructs {
    struct SurfaceInfo {
        void* handle = nullptr;

        int DPR = 1;

        uint32_t screenCoordWidth = 0;
        uint32_t screenCoordHeight = 0;

        uint32_t pixelWidth = 0;
        uint32_t pixelHeight = 0;
    };

    struct CreateInfo {
        SurfaceInfo surface = {};
    };

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics = {};
        std::optional<uint32_t> present = {};

        bool isFullySupported() const { return graphics.has_value() && present.has_value(); };
    };

    struct SwapchainDetails {
        VkSurfaceCapabilitiesKHR capabilities = {};
        std::vector<VkSurfaceFormatKHR> formats = {};
        std::vector<VkPresentModeKHR> presentModes = {};
    };

    struct PhysicalDeviceInfo {
        // Core infos.
        std::vector<const char*> supportedExtensions = {};
        QueueFamilyIndices queueFamilyIndices = {};
        SwapchainDetails swapchainDetails = {};

        // Buffer infos.
        VkPhysicalDeviceMemoryProperties memoryProperties = {};
    };
}

class VulkanEngine {
public:
#ifdef QT_DEBUG
    constexpr static bool EnableValidationLayers = true;
#else
    constexpr static bool EnableValidationLayers = false;
#endif

    std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

public:
    ~VulkanEngine();

    inline bool isInited() { return m_isInited; }

    void init(const VulkanEngineStructs::CreateInfo& info);

    inline VulkanEngineStructs::CreateInfo originInfo() { return m_originInfo; }

    void renderFrame();

    inline bool renderEnable() { return m_renderEnable; }
    inline void setRenderEnable(bool value) { m_renderEnable = value; }

    void resize(uint32_t width, uint32_t height);

private:
    bool m_isInited = false;

    void initCore();

    void destroyCore();

    VulkanEngineStructs::CreateInfo m_originInfo = {};

    void mRenderFrame();

    size_t m_currFrameIndex = 0;

    bool m_renderEnable = true;

    void recreateSwapchain();

    void destroyOldSwapchain();

private:
    VkInstance m_instance = {};

    void createInstance();

    VkDebugUtilsMessengerEXT m_debugMessenger = {};

    void bindDebugCallbackFunc();

    VkSurfaceKHR m_surface = {};

    VulkanEngineStructs::SurfaceInfo m_surfaceInfo = {};

    void createSurface();

    VkPhysicalDevice m_physicalDevice = {};

    VulkanEngineStructs::PhysicalDeviceInfo m_physicalDeviceInfo = {};

    void selectPhysicalDevice();

    VkDevice m_device = {};

    VkQueue m_graphicsQueue = {};
    VkQueue m_presentQueue = {};

    void createLogicalDevice();

    VkSwapchainKHR m_swapchain = {};

    std::vector<VkImage> m_swapchainImages = {};

    VkFormat m_swapchainImageFormat = {};

    VkExtent2D m_swapchainExtent2D = {};

    void createSwapchain();

    std::vector<VkImageView> m_swapchainImageViews = {};

    void createImageViews();

    std::unordered_map<std::string, VkRenderPass> m_renderPasses = {};

    void createRenderPasses();

    std::vector<VkFramebuffer> m_swapchainFramebuffers = {};

    void createFramebuffers();

    std::unordered_map<std::string, VkPipeline> m_graphicsPipelines = {};

    ShaderContainer m_shaderContainer = {};

    std::unordered_map<std::string, VkDescriptorSetLayout> m_descSetLayouts = {};
    std::unordered_map<std::string, VkPipelineLayout> m_pipelineLayouts = {};

    void createGraphicsPipelines();

    VkCommandPool m_commandPool = {};

    void createCommandPool();

    std::vector<VkCommandBuffer> m_commandBuffers = {};

    void createCommandBuffers();

    constexpr static size_t MAX_FRAMES_IN_FLIGHT = 2;

    std::unordered_map<std::string, std::vector<VkFence>> m_fences = {};
    std::unordered_map<std::string, std::vector<VkSemaphore>> m_semaphores = {};

    // Used for referring other fences; NOT created by vkCreateFence.
    std::unordered_map<std::string, std::vector<VkFence>> m_fenceRefs = {};

    void createFencesAndSemaphores();

private:
    std::vector<VkLayerProperties> m_supportedLayers = {};

    void enumerateSupportedLayers();

    bool isLayerSupported(const char* layerName);

    bool isLayersSupported(const std::vector<const char*>& layerNames);

    std::vector<VkExtensionProperties> m_supportedExtensions = {};

    void enumerateSupportedExtensions();

    bool isExtensionSupported(const char* extensionName);

    bool isExtensionsSupported(const std::vector<const char*>& extensionNames);

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& info);

    VulkanEngineStructs::PhysicalDeviceInfo queryPhysicalDeviceInfo(const VkPhysicalDevice& device);

    std::vector<const char*> queryDeviceSupportedExtensions(const VkPhysicalDevice& device);

    VulkanEngineStructs::QueueFamilyIndices queryDeviceQueueFamilyIndices(const VkPhysicalDevice& device);

    VulkanEngineStructs::SwapchainDetails queryDeviceSwapchainDetails(const VkPhysicalDevice& device);

    std::vector<const char*> deviceMinimumRequiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    bool isDeviceAdequate(const VulkanEngineStructs::PhysicalDeviceInfo& info);

    bool isPropertyInSupportedProperties(const char* name, const std::vector<const char*>& names);

    bool isPropertiesAllInSupportedProperties(const std::vector<const char*>& subset,
                                              const std::vector<const char*>& superset);

    VkSurfaceFormatKHR selectSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& candidateFormats);

    VkPresentModeKHR selectSwapchainPresentMode(const std::vector<VkPresentModeKHR>& candidateModes);

    VkExtent2D selectSwapchainExtent2D(const VkSurfaceCapabilitiesKHR& capabilities);

public:
    inline std::string currBindVertexBufferLabel() { return m_currBindVertexBufferLabel; }
    inline void setCurrBindVertexBufferLabel(const std::string& value) { m_currBindVertexBufferLabel = value; }

    inline std::string currBindIndexBufferLabel() { return m_currBindIndexBufferLabel; }
    inline void setCurrBindIndexBufferLabel(const std::string& value) { m_currBindIndexBufferLabel = value; }

    void declareVertices(const std::string& bufferLabel, bool enableServerBuffer, const std::vector<Vertex>& vertices);

    void declareIndices(const std::string& bufferLabel, const std::vector<uint32_t>& indices);

private:
    struct BufferResource {
        VkBuffer  buffer = {};
        VkDeviceMemory memory = {};
        VkMemoryRequirements requirements = {};
    };

    struct VertexBuffer {
        std::vector<Vertex> data = {};

        BufferResource clientResource = {};

        // Note this field should be decided when declaring, i.e. before creating the actual buffers.
        bool isServerResourceEnabled = false;
        // Optional server resource (Unused if client resource is host visible and coherent)
        BufferResource serverResource = {};

        VertexBuffer() = default;
        VertexBuffer(bool enableServerBuffer, const std::vector<Vertex>& vertices) {
            isServerResourceEnabled = enableServerBuffer;
            data = vertices;
        }
    };

    std::unordered_map<std::string, VertexBuffer> m_vertexBuffers = {};

    std::string m_currBindVertexBufferLabel = {};

    void createCoherentVertexBuffer(const std::string& label, size_t vertexCount);

    void createIsolatedVertexBuffer(const std::string& label, size_t vertexCount);

    uint32_t findAdequateMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createAllDeclaredVertexBuffers();

    VkMemoryRequirements createExclusiveBuffer(VkBufferUsageFlags usage, VkDeviceSize size, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory);

    void copyBufferData(VkBuffer src, VkBuffer dst, VkDeviceSize size);

    struct IndexBuffer {
        std::vector<uint32_t> data = {};

        BufferResource clientResource = {};
        BufferResource serverResource = {};

        IndexBuffer() = default;
        explicit IndexBuffer(const std::vector<uint32_t>& indices) { data = indices; }
    };

    std::unordered_map<std::string, IndexBuffer> m_indexBuffers = {};

    std::string m_currBindIndexBufferLabel = {};

    void createIndexBuffer(const std::string& label, uint32_t indicesCount);

    void createAllDeclaredIndexBuffers();

private:
    void createDescriptorSetLayout();

    struct UniformBuffer {
        UniformBufferObject data = {};
        std::vector<BufferResource> resources = {};
    };

    UniformBuffer m_uniformBuffer = {};

    void createUniformBuffers();

    uint32_t m_currSwapchainImageIndex = 0;

    void updateUniformBuffers();

    VkDescriptorPool m_descriptorPool = {};

    void createDescriptorPool();

    std::vector<VkDescriptorSet> m_descriptorSets = {};

    void createDescriptorSets();

public:
    void translateCamera(float dx, float dy, float dz);

    void rotateCamera(float dx, float dy);

    void zoomCamera(float delta);

private:
    std::unique_ptr<Camera> m_camera = nullptr;

    void buildCamera();
};

#endif // VULKAN_ENGINE_H
