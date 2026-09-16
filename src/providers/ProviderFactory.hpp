#pragma once

#include <providers/ProviderTypes.hpp>

#include <memory>

class QObject;
class Provider;

class ProviderFactory {
public:
    static std::unique_ptr<Provider> create(const ProviderConfig& config);
};
