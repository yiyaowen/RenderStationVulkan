#
# Render Station @ https://github.com/yiyaowen/render-station
#
# Create fantastic animation and game.
#
# yiyaowen (c) 2021 All Rights Reserved.
#
# Developed with Qt5 and Vulkan on macOS.
#

cmake_minimum_required(VERSION 3.5)

project(RenderStation VERSION 1.0 LANGUAGES CXX OBJCXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Widgets Gui REQUIRED)

link_libraries(Qt5::Core Qt5::Widgets Qt5::Gui)

set(VULKAN_INCLUDE_PATH "/Users/fort.w/VulkanSDK/1.2.189.0/macOS/include")
set(VULKAN_LIB_PATH "/Users/fort.w/VulkanSDK/1.2.189.0/macOS/lib")

set(GLM_INCLUDE_PATH "/Users/fort.w/Downloads/glm-master")

include_directories(${VULKAN_INCLUDE_PATH} ${GLM_INCLUDE_PATH})

link_directories(${VULKAN_LIB_PATH})

set(VULKAN_SHARED_LIB "${VULKAN_LIB_PATH}/libvulkan.1.2.189.dylib")

link_libraries(${VULKAN_SHARED_LIB})

file(GLOB_RECURSE VULKAN_INCLUDE_FILES "${VULKAN_INCLUDE_PATH}/*")
file(GLOB_RECURSE GLM_INCLUDE_FILES "${GLM_INCLUDE_PATH}/*")

add_executable(RenderStation
    # SDK
    ${VULKAN_INCLUDE_FILES}

    # Headers
    Camera.h
    DisplayWindow.h
    GraphicsResource.h
    Platforms/ExecuteCommand.h
    Platforms/SurfaceCompatible.h
    ShaderContainer.h
    VulkanEngine.h

    # Sources
    Camera.cpp
    DisplayWindow.cpp
    Main.cpp
    Platforms/ExecuteCommand.mm
    Platforms/SurfaceCompatible.mm
    ShaderContainer.cpp
    VulkanEngine.cpp
)

if(APPLE)
target_link_libraries(RenderStation
    PRIVATE "-framework AppKit"
    PRIVATE "-framework Foundation"
    PRIVATE "-framework QuartzCore"
)
endif()