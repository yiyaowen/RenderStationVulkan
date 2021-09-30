/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#import <AppKit/AppKit.h>
#include <QuartzCore/CAMetalLayer.h>

#include "SurfaceCompatible.h"

void* makePlatformSurfaceVulkanCompatible(void* surfaceHandle, int dpr) {
    NSView* view = (NSView*) surfaceHandle;
    assert([view isKindOfClass:[NSView class]]);
    // In Molten Vulkan SDK, vulkan codes are translated into Metal codes, which requires
    // that the original platform surface (NSView for macOS) must contain a Metal layer.
    if (![view.layer isKindOfClass:[CAMetalLayer class]]) {
        [view setLayer: [CAMetalLayer layer]];
        // On macOS's Retina screen the DPR (Device Pixel Ratio) may not be 1.
        [[view layer] setContentsScale: dpr];
        [view setWantsLayer:YES];
    }
    // Vulkan's vkCreateMetalSurfaceEXT use CAMetalLayer as its base surface object.
    return [view layer];
}