#include <stutter.h>
#include <ui.h>
#include <MainWindow.h>

Stutter::Stutter() = default;

Stutter::~Stutter() = default;

void Stutter::setupPlugin() {}

void Stutter::setupInterface(MainWindow* main) {
    StutterWidget* widget = new StutterWidget(main);
    main->addPluginDockWidget(widget);
}
