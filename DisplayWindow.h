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

    void initVulkanEngine();

protected:
    void paintEvent(QPaintEvent* event) override;

    QPaintEngine* paintEngine() const override;

    void resizeEvent(QResizeEvent *event) override;

private:
    VulkanEngine engine = {};
};

#endif // DISPLAY_WINDOW_H
