#include <providers/ProviderFactory.hpp>

#include <providers/AnthropicProvider.hpp>
#include <providers/OpenAIProvider.hpp>

#include <stdexcept>

std::unique_ptr<Provider> ProviderFactory::create(const ProviderConfig& config) {
    switch (config.type) {
    case ProviderType::OpenAI:
        return std::make_unique<OpenAIProvider>(config.apiKey, config.baseUrl);
    case ProviderType::Anthropic:
        return std::make_unique<AnthropicProvider>(
            config.apiKey,
            config.anthropicVersion,
            config.baseUrl
        );
    }

    throw std::invalid_argument("Unknown provider type");
}
