#pragma once

#include <chat/ContextBuilder.hpp>
#include <domain/ProviderConfig.hpp>
#include <providers/ProviderTypes.hpp>

class PromptBuilder {
public:
    PromptBuilder();

    bool isValid() const { return error_.isEmpty(); }
    QString error() const { return error_; }
    ChatRequest build(const ChatContext& context, const stutter::ProviderConfig& config) const;

private:
    static QString loadResource(const QString& path, QString& error);

    QString systemPrompt_;
    QString reverseEngineerPrompt_;
    QString error_;
};
