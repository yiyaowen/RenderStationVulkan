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
#include <QDebug>

#include <exception>

#include "DisplayWindow.h"

int main(int argc, char** argv) {
    try {
        QApplication a(argc, argv);
        DisplayWindow w;
        w.initVulkanEngine();
        w.show();
        return a.exec();
    }
    catch (const std::exception& e) {
        qDebug() << e.what();
    }
}
