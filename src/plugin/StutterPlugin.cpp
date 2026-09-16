#include <plugin/StutterPlugin.hpp>
#include <ui/StutterWidget.hpp>
#include <providers/Provider.hpp>

#include <MainWindow.h>
#include <QTimer>

Stutter::Stutter() {
    std::unique_ptr<Provider> openai_provider = Provider::create(ProviderType::OpenAI);
    std::unique_ptr<Provider> anthropic_provider = Provider::create(ProviderType::Anthropic);
};

Stutter::~Stutter() = default;

void Stutter::setupPlugin() {
    new Stutter();
}

void Stutter::setupInterface(MainWindow* main) {
    Window* win = new Window(main);
    main->addPluginDockWidget(win);
}
