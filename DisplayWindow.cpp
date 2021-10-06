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

    setMouseTracking(true);

    this->setWindowTitle("Render Station 渲染作坊");

    int width = 800, height = 600;
    this->resize(width, height);

    const auto& desktopSize = QApplication::desktop()->size();
    this->move((desktopSize.width() - width) / 2, (desktopSize.height() - height) / 2);
}

static glm::vec3 extractRGB(Qt::GlobalColor color) {
    QColor c(color);
    return { c.redF(), c.greenF(), c.blueF() };
}

void DisplayWindow::declareRenderResourceData() {
    // Note the twist order of declared triangles.
    engine.declareVertices("cube", true,
                           {
                                   { { -0.5f, -0.5f, -0.5f }, extractRGB(Qt::white) },
                                   { { -0.5f, +0.5f, -0.5f }, extractRGB(Qt::black) },
                                   { { +0.5f, +0.5f, -0.5f }, extractRGB(Qt::red) },
                                   { { +0.5f, -0.5f, -0.5f,}, extractRGB(Qt::green) },
                                   { { -0.5f, -0.5f, +0.5f }, extractRGB(Qt::blue) },
                                   { { -0.5f, +0.5f, +0.5f }, extractRGB(Qt::yellow) },
                                   { { +0.5f, +0.5f, +0.5f }, extractRGB(Qt::cyan) },
                                   { { +0.5f, -0.5f, +0.5f }, extractRGB(Qt::magenta) },
                           });
    engine.setCurrBindVertexBufferLabel("cube");

    engine.declareIndices("cube", {
            // front face
            0, 1, 2,
            0, 2, 3,

            // back face
            4, 6, 5,
            4, 7, 6,

            // left face
            4, 5, 1,
            4, 1, 0,

            // right face
            3, 2, 6,
            3, 6, 7,

            // top face
            1, 5, 6,
            1, 6, 2,

            // bottom face
            4, 0, 3,
            4, 3, 7
    });
    engine.setCurrBindIndexBufferLabel("cube");
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
    handleInputEvent();
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

void DisplayWindow::keyPressEvent(QKeyEvent* event) {
    m_keyStatusTable[Qt::Key(event->key())] = true;
}

void DisplayWindow::keyReleaseEvent(QKeyEvent* event) {
    m_keyStatusTable[Qt::Key(event->key())] = false;
}

void DisplayWindow::mouseMoveEvent(QMouseEvent* event) {
    m_currMousePos = event->windowPos();

    if (event->buttons() & Qt::RightButton) {
        QPointF dp = (m_currMousePos - m_lastMousePos) * m_cameraRotateSpeedScale;
        engine.rotateCamera(-static_cast<float>(dp.x()), -static_cast<float>(dp.y()));
    }

    m_lastMousePos = event->windowPos();
}

void DisplayWindow::wheelEvent(QWheelEvent* event) {
    engine.zoomCamera(static_cast<float>(event->delta()) * m_cameraZoomSpeedScale);
}

void DisplayWindow::handleInputEvent() {
    float horizontal =
            (m_keyStatusTable[Qt::Key::Key_A] ? -1.0f : 0.0f) +
            (m_keyStatusTable[Qt::Key::Key_D] ? 1.0f : 0.0f);
    float vertical =
            (m_keyStatusTable[Qt::Key::Key_Q] ? -1.0f : 0.0f) +
            (m_keyStatusTable[Qt::Key::Key_E] ? 1.0f : 0.0f);
    float frontBack =
            (m_keyStatusTable[Qt::Key::Key_W] ? 1.0f : 0.0f) +
            (m_keyStatusTable[Qt::Key::Key_S] ? -1.0f : 0.0f);

    if (qAbs(horizontal) + qAbs(vertical) + qAbs(frontBack) != 0.0f) {
        engine.translateCamera(horizontal * m_cameraMoveSpeedScale,
                               vertical * m_cameraMoveSpeedScale,
                               frontBack * m_cameraMoveSpeedScale);
    }
}
