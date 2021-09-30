/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#ifndef SURFACE_COMPATIBLE_H
#define SURFACE_COMPATIBLE_H

void* makePlatformSurfaceVulkanCompatible(void* surfaceHandle, int dpr);

#endif // SURFACE_COMPATIBLE_H