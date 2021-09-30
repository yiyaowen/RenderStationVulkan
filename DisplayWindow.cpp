/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#include <QApplication>
#include <QDesktopWidget>
#include <QResizeEvent>
#include <QWindow>

#include "DisplayWindow.h"
#include "Platforms/SurfaceCompatible.h"

DisplayWindow::DisplayWindow() {
    setAttribute(Qt::WA_NativeWindow);
    setAttribute(Qt::WA_PaintOnScreen);
    // Replace fullscreen button with maximization button on macOS.
    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    windowHandle()->setSurfaceType(QWindow::VulkanSurface);

    this->setWindowTitle("Render Station 渲染作坊");

    int width = 800, height = 600;
    this->resize(width, height);

    const auto& desktopSize = QApplication::desktop()->size();
    this->move((desktopSize.width() - width) / 2, (desktopSize.height() - height) / 2);
}

void DisplayWindow::initVulkanEngine() {
    void* surfaceHandle = reinterpret_cast<void*>(windowHandle()->winId());
    int dpr = QApplication::desktop()->screen()->devicePixelRatio();

    VulkanEngineStructs::CreateInfo info = {};

    info.surface.handle = makePlatformSurfaceVulkanCompatible(surfaceHandle, dpr);
    info.surface.DPR = dpr;
    info.surface.screenCoordWidth = this->width();
    info.surface.screenCoordHeight = this->height();
    info.surface.pixelWidth = dpr * this->width();
    info.surface.pixelHeight = dpr * this->height();

    engine.init(info);
}

void DisplayWindow::paintEvent(QPaintEvent* event) {
    engine.renderFrame();
    this->update();
}

QPaintEngine* DisplayWindow::paintEngine() const {
    // Discard Qt paint engine in case of it conflicts with Vulkan.
    return nullptr;
}

void DisplayWindow::resizeEvent(QResizeEvent* event) {
    if (!engine.isInited()) return;

    const auto& size = event->size();
    engine.resize(size.width(), size.height());
}
