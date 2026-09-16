#include <plugin/StutterPlugin.hpp>
#include <ui/StutterWidget.hpp>

#include <MainWindow.h>
#include <QTimer>

Stutter::Stutter() = default;

Stutter::~Stutter() = default;

void Stutter::setupPlugin() {
    new Stutter();
}

void Stutter::setupInterface(MainWindow* main) {
    Window* win = new Window(main);
    main->addPluginDockWidget(win);
}
