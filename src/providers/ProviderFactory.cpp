#include <providers/ProviderFactory.hpp>

#include <providers/AnthropicProvider.hpp>
#include <providers/CodexProvider.hpp>
#include <providers/OpenAIProvider.hpp>

#include <stdexcept>

std::unique_ptr<Provider> ProviderFactory::create(
    const stutter::ProviderConfig& config,
    const QString& apiKey
) {
    if (config.providerId == QStringLiteral("openai")) {
        return std::make_unique<OpenAIProvider>(apiKey, config.endpoint);
    }
    if (config.providerId == QStringLiteral("deepseek")) {
        return std::make_unique<OpenAIProvider>(apiKey, config.endpoint);
    }
    if (config.providerId == QStringLiteral("openrouter")) {
        return std::make_unique<OpenAIProvider>(apiKey, config.endpoint, false);
    }
    if (config.providerId == QStringLiteral("anthropic")) {
        return std::make_unique<AnthropicProvider>(
            apiKey,
            config.options.value(
                QStringLiteral("api_version"),
                QStringLiteral("2023-06-01")
            ).toString(),
            config.endpoint
        );
    }
    if (config.providerId == QStringLiteral("codex")) {
        return std::make_unique<CodexProvider>();
    }

    throw std::invalid_argument("Unknown provider type");
}
