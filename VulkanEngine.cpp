/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#include <algorithm>
#include <cstdint>
#include <exception>
#include <set>

#include <QApplication>
#include <QDesktopWidget>

#include "VulkanEngine.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkanEngineDebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
{
    FUNC_PARAM_UNUSED(messageSeverity);
    FUNC_PARAM_UNUSED(messageType);
    FUNC_PARAM_UNUSED(pUserData);

    qDebug() << "Vulkan Engine Debug Callback:\n" << pCallbackData->pMessage << "\n";

    return VK_FALSE;
}

void VulkanEngine::init(const VulkanEngineStructs::CreateInfo& info) {
    m_isInited = true;

    m_originInfo = info;

    // Init surface info.
    m_surfaceInfo = info.surface;

    // Query supported layers and extensions.
    enumerateSupportedLayers();

    enumerateSupportedExtensions();

    // Init vulkan core.
    initCore();
}

VulkanEngine::~VulkanEngine() {
    destroyCore();
}

void VulkanEngine::renderFrame() {
    if (m_instance == VK_NULL_HANDLE || !m_renderEnable) return;
    mRenderFrame();
}

void VulkanEngine::resize(uint32_t width, uint32_t height) {
    m_surfaceInfo.screenCoordWidth = width;
    m_surfaceInfo.screenCoordHeight = height;

    m_surfaceInfo.pixelWidth = m_surfaceInfo.DPR * width;
    m_surfaceInfo.pixelHeight = m_surfaceInfo.DPR * height;

    recreateSwapchain();
}

void VulkanEngine::initCore() {
    createInstance();

    bindDebugCallbackFunc();

    createSurface();

    selectPhysicalDevice();

    createLogicalDevice();

    createSwapchain();

    createImageViews();

    createRenderPasses();

    createFramebuffers();

    createGraphicsPipelines();

    createCommandPool();

    createCommandBuffers();

    createFencesAndSemaphores();
}

void VulkanEngine::destroyCore() {
    // Disable rendering frame when destroy starts.
    m_renderEnable = false;

    // Wait until all works done.
    vkDeviceWaitIdle(m_device);

    // Destroy: shader modules created.
    m_shaderContainer.destroyAllShaderModules();

    // Destroy: createFencesAndSemaphores()
    for (auto& fenceGroup : m_fences) {
        for (auto& fence : fenceGroup.second) {
            vkDestroyFence(m_device, fence, nullptr);
        }
    }

    for (auto& semaphoreGroup : m_semaphores) {
        for (auto& semaphore : semaphoreGroup.second) {
            vkDestroySemaphore(m_device, semaphore, nullptr);
        }
    }

    // Destroy: createCommandPool()
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);

    // Destroy: createGraphicsPipelines()
    for (auto& pipelineLayout : m_pipelineLayouts) {
        vkDestroyPipelineLayout(m_device, pipelineLayout.second, nullptr);
    }

    for (auto& graphicsPipeline : m_graphicsPipelines) {
        vkDestroyPipeline(m_device, graphicsPipeline.second, nullptr);
    }

    // Destroy: createFramebuffers()
    for (auto& framebuffer : m_swapchainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    // Destroy: createRenderPasses()
    for (auto& renderPass : m_renderPasses) {
        vkDestroyRenderPass(m_device, renderPass.second, nullptr);
    }

    // Destroy: createImageViews()
    for (auto& imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }

    // Destroy: createSwapchain()
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    // Destroy: createLogicalDevice()
    vkDestroyDevice(m_device, nullptr);

    // Destroy: createSurface()
    vkDestroySurfaceKHR(m_instance, m_surface, nullptr);

    // Destroy: bindDebugCallbackFunc()
    if (EnableValidationLayers) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
    }

    // Destroy: createInstance()
    vkDestroyInstance(m_instance, nullptr);
}

void VulkanEngine::mRenderFrame() {

    // Render

    vkWaitForFences(m_device, 1, &m_fences["frame_in_flight"][m_currFrameIndex], VK_TRUE, UINT64_MAX);

    uint32_t  imageIndex;
    vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_semaphores["image_available"][m_currFrameIndex], VK_NULL_HANDLE, &imageIndex);

    if (m_fenceRefs["image_in_flight"][imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(m_device, 1, &m_fenceRefs["image_in_flight"][imageIndex], VK_TRUE, UINT64_MAX);
    }
    m_fenceRefs["image_in_flight"][imageIndex] = m_fences["frame_in_flight"][m_currFrameIndex];

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_semaphores["image_available"][m_currFrameIndex] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[] = { m_semaphores["render_finish"][m_currFrameIndex] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(m_device, 1, &m_fences["frame_in_flight"][m_currFrameIndex]);

    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_fences["frame_in_flight"][m_currFrameIndex]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit queue.");
    }

    // Preset

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapchains[] = { m_swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    vkQueuePresentKHR(m_presentQueue, &presentInfo);

    m_currFrameIndex = (m_currFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanEngine::recreateSwapchain() {
    m_renderEnable = false;

    // Wait util all works done.
    vkDeviceWaitIdle(m_device);

    // Destroy old swapchain.
    destroyOldSwapchain();

    // Create new swapchain.
    createSwapchain();

    createImageViews();

    createRenderPasses();

    createFramebuffers();

    createGraphicsPipelines();

    // Note that command pool is not recreated here for the sake of efficiency.
    // Consequently, all command buffers should be freed in destroySwapchain.

    createCommandBuffers();

    m_renderEnable = true;
}

void VulkanEngine::destroyOldSwapchain() {
    // Destroy: shader modules created.
    m_shaderContainer.destroyAllShaderModules();

    // Free: vkAllocateCommandBuffers()
    vkFreeCommandBuffers(m_device, m_commandPool, m_commandBuffers.size(), m_commandBuffers.data());

    // Destroy: createGraphicsPipelines()
    for (auto& pipelineLayout : m_pipelineLayouts) {
        vkDestroyPipelineLayout(m_device, pipelineLayout.second, nullptr);
    }

    for (auto& graphicsPipeline : m_graphicsPipelines) {
        vkDestroyPipeline(m_device, graphicsPipeline.second, nullptr);
    }

    // Destroy: createFramebuffers()
    for (auto& framebuffer : m_swapchainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }

    // Destroy: createRenderPasses()
    for (auto& renderPass : m_renderPasses) {
        vkDestroyRenderPass(m_device, renderPass.second, nullptr);
    }

    // Destroy: createImageViews()
    for (auto& imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }

    // Destroy: createSwapchain()
    vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
}

void VulkanEngine::createInstance() {
    if (EnableValidationLayers && !isLayersSupported(validationLayers)) {
        throw std::runtime_error("Designated validation layers not supported.");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Render Station";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Vulkan Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    // Enable creating surface from Qt on macOS.
    std::vector<const char*> extensions = { "VK_KHR_surface", "VK_EXT_metal_surface" };
    // Enable validation layer debug callback func.
    if (EnableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    instanceInfo.enabledExtensionCount = extensions.size();
    instanceInfo.ppEnabledExtensionNames = extensions.data();

    // Note that creating a debug messenger depends on an initialized instance,
    // which means that we can not get debug messages of instance creating and destroying.
    // However, we can populate pNext field of instance create info to debug instance.
    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo ={};
    if (EnableValidationLayers) {
        instanceInfo.enabledLayerCount = validationLayers.size();
        instanceInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugMessengerInfo);
        instanceInfo.pNext = &debugMessengerInfo;
    }
    else {
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.ppEnabledLayerNames = nullptr;
    }

    if (vkCreateInstance(&instanceInfo, nullptr, &m_instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create instance.");
    }
}

void VulkanEngine::bindDebugCallbackFunc() {
    if (!EnableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {};
    populateDebugMessengerCreateInfo(debugMessengerInfo);

    // PFN: I guess it is the abbreviation of pointer to function.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func == nullptr) {
        throw std::runtime_error("Failed to find vkCreateDebugUtilsMessengerEXT func.");
    }
    if (func(m_instance, &debugMessengerInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create debug utils messenger.");
    }
}

void VulkanEngine::createSurface() {
    VkMetalSurfaceCreateInfoEXT metalSurfaceInfo = {};
    metalSurfaceInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
    metalSurfaceInfo.pLayer = m_surfaceInfo.handle;
    vkCreateMetalSurfaceEXT(m_instance, &metalSurfaceInfo, nullptr, &m_surface);
}

void VulkanEngine::selectPhysicalDevice() {
    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

    if (physicalDeviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support.");
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

    for (const auto& device : physicalDevices) {
        auto deviceInfo = queryPhysicalDeviceInfo(device);
        // If already find an adequate device then use it.
        if (isDeviceAdequate(deviceInfo)) {
            m_physicalDevice = device;
            m_physicalDeviceInfo = deviceInfo;
            break;
        }
    }

    // When no physical device is adequate then exit.
    if (m_physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find an adequate GPU.");
    }
}

void VulkanEngine::createLogicalDevice() {
    // There may be a queue family that can support both graphics and present,
    // which means it is possible that graphics.value() == present.value().
    const auto& indices = m_physicalDeviceInfo.queueFamilyIndices;
    std::set<uint32_t> uniqueQueueFamilyIndices = { indices.graphics.value(), indices.present.value() };

    std::vector<VkDeviceQueueCreateInfo> queueInfos = {};

    float queuePriority = 1.0f; // Simply give all queues the same priority.
    for (uint32_t i : uniqueQueueFamilyIndices) {

        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = i;
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;

        queueInfos.push_back(queueInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = queueInfos.size();
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.pEnabledFeatures = &deviceFeatures;

    // If the [VK_KHR_portability_subset] extension is included in pProperties of vkEnumerateDeviceExtensionProperties,
    // ppEnabledExtensions must include "VK_KHR_portability_subset". (Debugged with Molten Vulkan SDK on macOS)
    auto requiredExtensions = deviceMinimumRequiredExtensions;
    if (isPropertyInSupportedProperties("VK_KHR_portability_subset", m_physicalDeviceInfo.supportedExtensions)) {
        requiredExtensions.push_back("VK_KHR_portability_subset");
    }
    deviceInfo.enabledExtensionCount = requiredExtensions.size();
    deviceInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (EnableValidationLayers) {
        deviceInfo.enabledLayerCount = validationLayers.size();
        deviceInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        deviceInfo.enabledLayerCount = 0;
        deviceInfo.ppEnabledLayerNames = nullptr;
    }

    if (vkCreateDevice(m_physicalDevice, &deviceInfo, nullptr, &m_device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device.");
    }

    // Bind device with shader container by the way.
    m_shaderContainer.setDevice(&m_device);

    // Store required queues in created device by the way.
    // Get first queue in each queue family by default.
    vkGetDeviceQueue(m_device, indices.graphics.value(), 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, indices.present.value(), 0, &m_presentQueue);
}

void VulkanEngine::createSwapchain() {
    const auto& details = m_physicalDeviceInfo.swapchainDetails;

    VkSurfaceFormatKHR surfaceFormat = selectSwapchainSurfaceFormat(details.formats);

    VkPresentModeKHR presentMode = selectSwapchainPresentMode(details.presentModes);

    VkExtent2D extent2D = selectSwapchainExtent2D(details.capabilities);

    uint32_t  imageCount = details.capabilities.minImageCount + 1;
    // Note maxImageCount == 0 means there is no maximum.
    if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount) {
        imageCount = details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = m_surface;

    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageFormat = surfaceFormat.format;
    swapchainInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = extent2D;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    const auto& indices = m_physicalDeviceInfo.queueFamilyIndices;
    uint32_t queueFamilyIndices[] = { indices.graphics.value(), indices.present.value() };

    if (indices.graphics != indices.present) {
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        // These 2 args designate which queue families can share this swapchain.
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        // Exclusive: An image is owned by one queue family at a time and ownership must be explicitly
        // transferred before using it in another queue family. This option offers the best performance.
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.queueFamilyIndexCount = 0;
        swapchainInfo.pQueueFamilyIndices = nullptr;
    }

    swapchainInfo.preTransform = details.capabilities.currentTransform;

    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    swapchainInfo.presentMode = presentMode;
    swapchainInfo.clipped = VK_TRUE;

    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(m_device, &swapchainInfo, nullptr, &m_swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain.");
    }

    // Store images and their properties in this swapchain by the way.
    uint32_t swapchainImageCount;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, nullptr);

    m_swapchainImages.resize(swapchainImageCount); // imageCount must not be 0 since the swapchain is created successfully.
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &swapchainImageCount, m_swapchainImages.data());

    m_swapchainImageFormat = surfaceFormat.format;

    m_swapchainExtent2D = extent2D;
}

void VulkanEngine::createImageViews() {
    m_swapchainImageViews.resize(m_swapchainImages.size());

    for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
        VkImageViewCreateInfo imageViewInfo = {};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = m_swapchainImages[i];
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = m_swapchainImageFormat;

        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

        imageViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = 1;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(m_device, &imageViewInfo, nullptr, &m_swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views.");
        }
    }
}

void VulkanEngine::createRenderPasses() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Disable anti-alias;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDesc = {};
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0; // There is only one subpass in this application, so 0 points to itself.
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpassDesc;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPasses["main"]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render passes.");
    }
}

void VulkanEngine::createFramebuffers() {
    m_swapchainFramebuffers.resize(m_swapchainImageViews.size());

    for (size_t i = 0; i < m_swapchainImageViews.size(); ++i) {
        VkImageView attachments[] = { m_swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPasses["main"];
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent2D.width;
        framebufferInfo.height = m_swapchainExtent2D.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffers.");
        }
    }
}

void VulkanEngine::createGraphicsPipelines() {
    // Make shader infos.
    m_shaderContainer.addNewShader("vert", "vert.spv", "main", ShaderContainer::Vertex);
    m_shaderContainer.addNewShader("frag", "frag.spv", "main", ShaderContainer::Fragment);

    auto shaderStageInfos = m_shaderContainer.generateAllCreateInfos();

    // Make input assembly info.
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
    vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputStateInfo.vertexBindingDescriptionCount = 0;
    vertexInputStateInfo.pVertexBindingDescriptions = nullptr;
    vertexInputStateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputStateInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo ={};
    inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;

    // Make viewport and scissor info.
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = m_swapchainExtent2D.width;
    viewport.height = m_swapchainExtent2D.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = m_swapchainExtent2D;

    VkPipelineViewportStateCreateInfo viewportStateInfo = {};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.pViewports = &viewport;
    viewportStateInfo.scissorCount = 1;
    viewportStateInfo.pScissors = &scissor;

    // Make rasterization info.
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
    rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationStateInfo.depthClampEnable = VK_FALSE;
    rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizationStateInfo.lineWidth = 1.0f;
    rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStateInfo.depthBiasClamp = 0.0f;
    rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;

    // Make multisample info.
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
    multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleStateInfo.sampleShadingEnable = VK_FALSE;
    multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampleStateInfo.minSampleShading = 1.0f;
    multisampleStateInfo.pSampleMask = nullptr;
    multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;
    multisampleStateInfo.alphaToOneEnable = VK_FALSE;

    // Make color blend info.
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {};
    colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendStateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateInfo.attachmentCount = 1;
    colorBlendStateInfo.pAttachments = &colorBlendAttachment;
    colorBlendStateInfo.blendConstants[0] = 0.0f;
    colorBlendStateInfo.blendConstants[1] = 0.0f;
    colorBlendStateInfo.blendConstants[2] = 0.0f;
    colorBlendStateInfo.blendConstants[3] = 0.0f;

    // Make dynamic info.
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };

    VkPipelineDynamicStateCreateInfo dynamicStateInfo ={};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//    dynamicStateInfo.dynamicStateCount = 2;
//    dynamicStateInfo.pDynamicStates = dynamicStates;
    dynamicStateInfo.dynamicStateCount = 0;
    dynamicStateInfo.pDynamicStates = nullptr;

    // Create pipeline layout.
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = 0;
    layoutInfo.pSetLayouts = nullptr;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(m_device, &layoutInfo, nullptr, &m_pipelineLayouts["main"]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout.");
    }

    // Create pipeline.
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = shaderStageInfos.size();
    pipelineInfo.pStages = shaderStageInfos.data();
    pipelineInfo.pVertexInputState = &vertexInputStateInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyStateInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizationStateInfo;
    pipelineInfo.pMultisampleState = &multisampleStateInfo;
    pipelineInfo.pDepthStencilState = nullptr;
    pipelineInfo.pColorBlendState = &colorBlendStateInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = m_pipelineLayouts["main"];
    pipelineInfo.renderPass = m_renderPasses["main"];
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipelines["main"]) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create graphics pipelines.");
    }
}

void VulkanEngine::createCommandPool() {
    const auto& indices = m_physicalDeviceInfo.queueFamilyIndices;

    VkCommandPoolCreateInfo cmdPoolInfo = {};
    cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPoolInfo.queueFamilyIndex = indices.graphics.value();
    cmdPoolInfo.flags = 0;

    if (vkCreateCommandPool(m_device, &cmdPoolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool.");
    }
}

void VulkanEngine::createCommandBuffers() {
    m_commandBuffers.resize(m_swapchainFramebuffers.size());

    VkCommandBufferAllocateInfo cmdBufferAllocInfo = {};
    cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdBufferAllocInfo.commandPool = m_commandPool;
    cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferAllocInfo.commandBufferCount = m_commandBuffers.size();

    if (vkAllocateCommandBuffers(m_device, &cmdBufferAllocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers.");
    }

    for (size_t i = 0; i < m_commandBuffers.size(); ++i) {
        VkCommandBufferBeginInfo bufferBeginInfo = {};
        bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bufferBeginInfo.flags = 0;
        bufferBeginInfo.pInheritanceInfo = nullptr;

        if (vkBeginCommandBuffer(m_commandBuffers[i], &bufferBeginInfo) != VK_SUCCESS) {
            throw std::runtime_error("Failed to begin command buffer.");
        }

        VkRenderPassBeginInfo passBeginInfo = {};
        passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        passBeginInfo.renderPass = m_renderPasses["main"];
        passBeginInfo.framebuffer = m_swapchainFramebuffers[i];
        passBeginInfo.renderArea.offset = { 0, 0 };
        passBeginInfo.renderArea.extent = m_swapchainExtent2D;

        VkClearValue clearColor = { { { 0.0f, 0.0f, 0.0f, 1.0f } } };
        passBeginInfo.clearValueCount = 1;
        passBeginInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(m_commandBuffers[i], &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(m_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelines["main"]);

        vkCmdDraw(m_commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(m_commandBuffers[i]);

        if (vkEndCommandBuffer(m_commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to record command buffer.");
        }
    }
}

void VulkanEngine::createFencesAndSemaphores() {
    m_fences["frame_in_flight"].resize(MAX_FRAMES_IN_FLIGHT);
    m_fenceRefs["image_in_flight"].resize(m_swapchainImages.size(), VK_NULL_HANDLE);

    m_semaphores["image_available"].resize(MAX_FRAMES_IN_FLIGHT);
    m_semaphores["render_finish"].resize(MAX_FRAMES_IN_FLIGHT);

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (vkCreateFence(m_device, &fenceInfo, nullptr, &m_fences["frame_in_flight"][i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_semaphores["image_available"][i]) != VK_SUCCESS ||
            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_semaphores["render_finish"][i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create fences and semaphores.");
        }
    }
}

void VulkanEngine::enumerateSupportedLayers() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> layers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, layers.data());

    m_supportedLayers = layers;
}

bool VulkanEngine::isLayerSupported(const char* layerName) {
    return std::find_if(m_supportedLayers.begin(), m_supportedLayers.end(),
                        [&](const VkLayerProperties& layer) -> bool {
        return strcmp(layer.layerName, layerName) == 0;
    }) != m_supportedLayers.end();
}

bool VulkanEngine::isLayersSupported(const std::vector<const char*>& layerNames) {
    return std::all_of(layerNames.begin(), layerNames.end(),
                       [&](const char* name)  {
        return isLayerSupported(name);
    });
}

void VulkanEngine::enumerateSupportedExtensions() {
    uint32_t extensionCount;
    // Set pLayerName to nullptr to search all layers.
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    m_supportedExtensions = extensions;
}

bool VulkanEngine::isExtensionSupported(const char* extensionName) {
    return std::find_if(m_supportedExtensions.begin(), m_supportedExtensions.end(),
                        [&](const VkExtensionProperties& extension) -> bool {
        return strcmp(extension.extensionName, extensionName) == 0;
    }) != m_supportedExtensions.end();
}

bool VulkanEngine::isExtensionsSupported(const std::vector<const char*>& extensionNames) {
    return std::all_of(extensionNames.begin(), extensionNames.end(),
                       [&](const char* name) {
        return isExtensionSupported(name);
    });
}

void VulkanEngine::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &info) {
    info = {};
    info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    info.messageSeverity =
//            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    info.pfnUserCallback = vulkanEngineDebugCallback;
    info.pUserData = nullptr;
}

VulkanEngineStructs::PhysicalDeviceInfo VulkanEngine::queryPhysicalDeviceInfo(VkPhysicalDevice const& device) {
    VulkanEngineStructs::PhysicalDeviceInfo info = {};

    info.supportedExtensions = queryDeviceSupportedExtensions(device);

    info.queueFamilyIndices = queryDeviceQueueFamilyIndices(device);

    info.swapchainDetails = queryDeviceSwapchainDetails(device);

    return info;
}

std::vector<const char*> VulkanEngine::queryDeviceSupportedExtensions(VkPhysicalDevice const& device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, supportedExtensions.data());

    std::vector<const char*> extensions(extensionCount);
    for (size_t i = 0; i < extensionCount; ++i) {
        extensions[i] = (supportedExtensions[i].extensionName);
    }

    return extensions;
}

VulkanEngineStructs::QueueFamilyIndices VulkanEngine::queryDeviceQueueFamilyIndices(VkPhysicalDevice const& device) {
    VulkanEngineStructs::QueueFamilyIndices indices = {};

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (size_t i = 0; i < queueFamilies.size(); ++i) {
        const auto& queueFamily = queueFamilies[i];
        // Check graphics support.
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics = i;
        }

        // Check present support.
        VkBool32 isPresentSupported = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &isPresentSupported);
        if (isPresentSupported) {
            indices.present = i;
        }

        // If already find a fully supported queue family then use it.
        if (indices.isFullySupported()) break;
    }

    return indices;
}

VulkanEngineStructs::SwapchainDetails VulkanEngine::queryDeviceSwapchainDetails(VkPhysicalDevice const& device) {
    VulkanEngineStructs::SwapchainDetails details = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

    uint32_t  formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
    }

    uint32_t  presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool VulkanEngine::isDeviceAdequate(const VulkanEngineStructs::PhysicalDeviceInfo& info) {
    bool supportMinimumRequiredExtensions =
            isPropertiesAllInSupportedProperties(deviceMinimumRequiredExtensions, info.supportedExtensions);

    bool hasFullySupportedQueueFamily = info.queueFamilyIndices.isFullySupported();

    const auto& details = info.swapchainDetails;
    bool atLeastOneFormatAndOnePresentMode = !details.formats.empty() && !details.presentModes.empty();

    return supportMinimumRequiredExtensions && hasFullySupportedQueueFamily && atLeastOneFormatAndOnePresentMode;
}

bool VulkanEngine::isPropertyInSupportedProperties(const char* name, const std::vector<const char*>& names) {
    return std::find_if(names.begin(), names.end(),
                        [&](const char* supportedName) -> bool {
        return strcmp(name, supportedName) == 0;
    }) != names.end();
}

bool VulkanEngine::isPropertiesAllInSupportedProperties(const std::vector<const char*>& subset,
                                                        const std::vector<const char*>& superset) {
    return std::all_of(subset.begin(), subset.end(),
                       [&](const char* sub) -> bool {
        return isPropertyInSupportedProperties(sub, superset);
    });
}

VkSurfaceFormatKHR VulkanEngine::selectSwapchainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& candidateFormats) {
    for (const auto& format : candidateFormats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT) {
            return format;
        }
    }
    // If there is no candidate satisfies requirement then simply return first one.
    return candidateFormats[0];
}

VkPresentModeKHR VulkanEngine::selectSwapchainPresentMode(const std::vector<VkPresentModeKHR>& candidateModes) {
    for (const auto& mode : candidateModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return mode;
        }
    }
    // If there is no candidate support mailbox mode then simply return fifo mode.
    // Note that all GPUs support Vulkan will support fifo mode.
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::selectSwapchainExtent2D(const VkSurfaceCapabilitiesKHR& capabilities) {
    // In most cases, Vulkan will decide the swap chain extent automatically.
    // More exactly, when screen DPR (Device Pixel Ratio) == 1, Vulkan will set the extent itself;
    // in other cases, Vulkan will set currentExtent.width to UINT32_MAX, which means that
    // we must decide the value ourselves. (Vulkan_Extent == Surface_Pixel_Size == Surface_Screen_Coord_Size * DPR)

//    if (capabilities.currentExtent.width != UINT32_MAX) {
//        return capabilities.currentExtent;
//    }
//    else {

    // Always select swapchain extent manually here since the swapchain image size can be changed.

        uint32_t width = std::clamp(m_surfaceInfo.pixelWidth,
                                    capabilities.minImageExtent.width, capabilities.maxImageExtent.width);

        uint32_t height = std::clamp(m_surfaceInfo.pixelHeight,
                                     capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        VkExtent2D extent2D = { width, height };

        return extent2D;

//    }
}
