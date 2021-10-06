/*
** Render Station @ https://github.com/yiyaowen/render-station
**
** Create fantastic animation and game.
**
** yiyaowen (c) 2021 All Rights Reserved.
**
** Developed with Qt5 and Vulkan on macOS.
*/

#ifndef DISPLAY_WINDOW_H
#define DISPLAY_WINDOW_H

#include <QWidget>

#include "VulkanEngine.h"

class DisplayWindow : public QWidget {
    Q_OBJECT
public:
    DisplayWindow();

    void declareRenderResourceData();

    void initVulkanEngine();

protected:
    void paintEvent(QPaintEvent* event) override;

    QPaintEngine* paintEngine() const override;

    void resizeEvent(QResizeEvent *event) override;

    void keyPressEvent(QKeyEvent *event) override;

    void keyReleaseEvent(QKeyEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

private:
    VulkanEngine engine = {};

    void handleInputEvent();

    std::unordered_map<Qt::Key, bool> m_keyStatusTable = {
            { Qt::Key::Key_A, false }, { Qt::Key::Key_D, false },
            { Qt::Key::Key_Q, false }, { Qt::Key::Key_E, false },
            { Qt::Key::Key_W, false }, { Qt::Key::Key_S, false } };

    QPointF m_currMousePos = {};
    QPointF m_lastMousePos = {};

    float m_cameraMoveSpeedScale = 0.02f;
    float m_cameraRotateSpeedScale = 0.005f;
    float m_cameraZoomSpeedScale = 0.002f;
};

#endif // DISPLAY_WINDOW_H
