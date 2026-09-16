#pragma once

#include <chat/ContextBuilder.hpp>
#include <chat/ConversationService.hpp>
#include <chat/HistorySummarizer.hpp>
#include <chat/PromptBuilder.hpp>

#include <QObject>

#include <memory>

class ChatWidget;
class CodexProvider;
class ModelCatalog;
class Provider;
class SettingsDialog;

class ChatController final : public QObject {
    Q_OBJECT

public:
    explicit ChatController(
        ChatWidget& widget,
        SettingsDialog& settings,
        const ModelCatalog& modelCatalog,
        CodexProvider& codexProvider,
        QObject* parent = nullptr
    );
    ~ChatController() override;

private:
    void submit(const QString& text);
    void stop();
    void clear();
    bool selectProvider();
    stutter::ProviderConfig providerConfig() const;
    stutter::Model selectedModel() const;
    void connectProvider(Provider& provider);
    void handleEvent(const ProviderEvent& event);
    void handleFinished(const QString& requestId, const ChatResponse& response);
    void handleFailure(const QString& requestId, const ProviderError& error);
    void resetRequest();

    ChatWidget& widget_;
    SettingsDialog& settings_;
    const ModelCatalog& modelCatalog_;
    CodexProvider& codexProvider_;
    ConversationService conversations_;
    ContextBuilder contextBuilder_;
    PromptBuilder promptBuilder_;
    HistorySummarizer historySummarizer_;
    std::unique_ptr<Provider> ownedProvider_;
    Provider* provider_ = nullptr;
    QString requestId_;
    QString streamedResponse_;
    bool cancellationRequested_ = false;
};
