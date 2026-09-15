#ifndef STUTTER_MODEL_H
#define STUTTER_MODEL_H

#include <provider.h>
#include <string_view>
#include <array>

struct Model {
    std::string_view id;
    std::string_view name;
    ProviderType provider;
};

inline constexpr std::array models {
    Model {
        .id = "gpt-6-astra",
        .name = "GPT-6 Astra",
        .provider = ProviderType::OpenAI
    },
    Model {
        .id = "gpt-5.6-sol",
        .name = "GPT-5.6 Sol",
        .provider = ProviderType::OpenAI
    },
    Model {
        .id = "gpt-5.6-terra",
        .name = "GPT-5.6 Terra",
        .provider = ProviderType::OpenAI
    },
    Model {
        .id = "gpt-5.6-Luna",
        .name = "GPT-5.6 Luna",
        .provider = ProviderType::OpenAI
    },
    Model {
        .id = "claude-fable-5-1",
        .name = "Claude Fable 5.1",
        .provider = ProviderType::Anthropic
    },
    Model {
        .id = "claude-opus-5",
        .name = "Claude Opus 5",
        .provider = ProviderType::Anthropic
    },
    Model {
        .id = "claude-sonnet-5",
        .name = "Claude Sonnet 5",
        .provider = ProviderType::Anthropic
    },
    Model {
        .id = "claude-haiku-4-5",
        .name = "Claude Haiku 4.5",
        .provider = ProviderType::Anthropic
    },
};

#endif // STUTTER_MODEL_H
