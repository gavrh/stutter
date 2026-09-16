#include <providers/Provider.hpp>
#include <stdexcept>

std::unique_ptr<Provider> Provider::create(ProviderType type) {
    switch(type) {
        case ProviderType::OpenAI:
            return std::make_unique<OpenAI>();

        case ProviderType::Anthropic:
            return std::make_unique<Anthropic>();
    }

    throw std::invalid_argument("Incorrect provider provided");
};
