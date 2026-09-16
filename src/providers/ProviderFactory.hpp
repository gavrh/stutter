#pragma once

#include <domain/ProviderConfig.hpp>

#include <memory>

class QObject;
class Provider;

class ProviderFactory {
public:
    static std::unique_ptr<Provider> create(
        const stutter::ProviderConfig& config,
        const QString& apiKey
    );
};
