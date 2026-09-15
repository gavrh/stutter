#ifndef STUTTER_PROVIDER_H
#define STUTTER_PROVIDER_H

#include <string>
#include <memory>

enum class ProviderType {
    OpenAI,
    Anthropic
};

class Provider {

    public:
        virtual ~Provider() = default;
        virtual ProviderType type() const = 0;
        virtual std::string_view name() const = 0;

        static std::unique_ptr<Provider> create(ProviderType type);
};

class OpenAI : public Provider {

    public:
        ProviderType type() const override { return ProviderType::OpenAI; };
        std::string_view name() const override { return "OpenAI"; };
};

class Anthropic : public Provider {

    public: 
        ProviderType type() const override { return ProviderType::Anthropic; };
        std::string_view name() const override { return "Anthropic"; };

};

#endif // STUTTER_PROVIDER_H
