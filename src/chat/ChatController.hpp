#pragma once

#include <chat/ContextBuilder.hpp>
#include <chat/ConversationService.hpp>
#include <chat/HistorySummarizer.hpp>
#include <chat/PromptBuilder.hpp>

#include <domain/BinaryIdentity.hpp>
#include <domain/ToolActivity.hpp>
#include <providers/ProviderTypes.hpp>

#include <QObject>

#include <functional>
#include <memory>

class ChatWidget;
class CodexProvider;
class ModelCatalog;
class Provider;
class SettingsDialog;

namespace stutter {
class BinaryRepository;
class ConversationRepository;
class MessageRepository;
class ToolExecutor;
class ToolRegistry;
}

class ChatController final : public QObject {
    Q_OBJECT

public:
    explicit ChatController(
        ChatWidget& widget,
        SettingsDialog& settings,
        const ModelCatalog& modelCatalog,
        CodexProvider& codexProvider,
        stutter::BinaryRepository& binaries,
        stutter::ConversationRepository& conversations,
        stutter::MessageRepository& messages,
        stutter::ToolRegistry& tools,
        stutter::ToolExecutor& toolExecutor,
        std::function<stutter::BinaryIdentity()> binaryProvider,
        std::function<QString()> analysisContextProvider,
        QObject* parent = nullptr
    );
    ~ChatController() override;

signals:
    void activityStarted(const stutter::ToolActivity& activity);
    void activityUpdated(const stutter::ToolActivity& activity);
    void activityFinished(const stutter::ToolActivity& activity);

private:
    void submit(const QString& text);
    void stop();
    void clear();
    void renderConversation();
    bool selectProvider();
    stutter::ProviderConfig providerConfig() const;
    stutter::Model selectedModel() const;
    QString assistantTitle() const;
    void connectProvider(Provider& provider);
    void handleEvent(const ProviderEvent& event);
    void handleFinished(const QString& requestId, const ChatResponse& response);
    void handleFailure(const QString& requestId, const ProviderError& error);
    bool runToolCalls(const ChatResponse& response);
    void shrinkOldToolResults();
    void resetRequest();

    ChatWidget& widget_;
    SettingsDialog& settings_;
    const ModelCatalog& modelCatalog_;
    CodexProvider& codexProvider_;
    ConversationService conversations_;
    ContextBuilder contextBuilder_;
    PromptBuilder promptBuilder_;
    HistorySummarizer historySummarizer_;
    stutter::ToolRegistry* tools_ = nullptr;
    stutter::ToolExecutor* toolExecutor_ = nullptr;
    ChatRequest activeRequest_;
    std::function<stutter::BinaryIdentity()> binaryProvider_;
    std::function<QString()> analysisContextProvider_;
    std::unique_ptr<Provider> ownedProvider_;
    Provider* provider_ = nullptr;
    QString requestId_;
    QString streamedResponse_;
    bool cancellationRequested_ = false;
};
