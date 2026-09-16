#include <plugin/StutterPlugin.hpp>
#include <plugin/StutterContext.hpp>

#include <ui/ChatWidget.hpp>

#include <MainWindow.h>

Stutter::Stutter() = default;

Stutter::~Stutter() = default;

void Stutter::setupPlugin() {
    if (!context_) {
        context_ = new StutterContext(this);
    }
}

void Stutter::setupInterface(MainWindow* main) {
    if (!context_) {
        setupPlugin();
    }
    main->addPluginDockWidget(context_->createChatWidget(main));
}
