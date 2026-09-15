#include <stutter.h>
#include <ui.h>
#include <provider.h>
#include <MainWindow.h>

Stutter::Stutter() = default;

Stutter::~Stutter() = default;

void Stutter::setupPlugin() {
    new Stutter();
    std::unique_ptr<Provider> openai_provider = Provider::create(ProviderType::OpenAI);
    std::unique_ptr<Provider> anthropic_provider = Provider::create(ProviderType::Anthropic);
}

void Stutter::setupInterface(MainWindow* main) {
    Window* win = new Window(main);
    main->addPluginDockWidget(win);
}
