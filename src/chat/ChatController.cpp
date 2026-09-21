#include <chat/ChatController.hpp>

#include <config/ModelCatalog.hpp>
#include <providers/CodexProvider.hpp>
#include <providers/ProviderFactory.hpp>
#include <storage/BinaryRepository.hpp>
#include <storage/ConversationRepository.hpp>
#include <storage/MessageRepository.hpp>
#include <tools/ToolExecutor.hpp>
#include <tools/ToolProtocol.hpp>
#include <tools/ToolRegistry.hpp>
#include <ui/ChatMessageWidget.hpp>
#include <ui/ChatWidget.hpp>
#include <ui/SettingsDialog.hpp>

#include <QJsonDocument>

#include <utility>

namespace {
constexpr int maxToolResultChars = 12000;
constexpr int keepRecentToolResults = 6;

QString truncateToolResult(const QString& text) {
    if (text.size() <= maxToolResultChars) return text;
    return text.left(maxToolResultChars)
        + QStringLiteral("\n... [truncated %1 characters]").arg(text.size() - maxToolResultChars);
}

stutter::ToolActivityCategory toolCategory(stutter::ToolPermission permission) {
    switch (permission) {
    case stutter::ToolPermission::Read: return stutter::ToolActivityCategory::Reader;
    case stutter::ToolPermission::Analysis: return stutter::ToolActivityCategory::Analysis;
    case stutter::ToolPermission::Binary: return stutter::ToolActivityCategory::Binary;
    case stutter::ToolPermission::Debugger: return stutter::ToolActivityCategory::Debugger;
    }
    return stutter::ToolActivityCategory::Provider;
}

QString describeCall(const ToolCall& call) {
    if (call.arguments.isEmpty()) return call.rawArguments;
    return QString::fromUtf8(
        QJsonDocument(call.arguments).toJson(QJsonDocument::Indented)
    );
}

QString formatTokenCount(qint64 count) {
    if (count < 1000) {
        return QString::number(count);
    }
    if (count < 1000000) {
        return QString::number(count / 1000.0, 'f', 1) + QStringLiteral("k");
    }
    return QString::number(count / 1000000.0, 'f', 1) + QStringLiteral("m");
}
}

ChatController::ChatController(
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
    QObject* parent
) : QObject(parent),
    widget_(widget),
    settings_(settings),
    modelCatalog_(modelCatalog),
    codexProvider_(codexProvider),
    conversations_(this),
    tools_(&tools),
    toolExecutor_(&toolExecutor),
    binaryProvider_(std::move(binaryProvider)),
    analysisContextProvider_(std::move(analysisContextProvider)) {
    conversations_.setRepositories(&binaries, &conversations, &messages);
    connect(&widget_, &ChatWidget::messageSubmitted, this, &ChatController::submit);
    connect(&widget_, &ChatWidget::stopRequested, this, &ChatController::stop);
    connect(&widget_, &ChatWidget::conversationCleared, this, &ChatController::clear);
    connect(&conversations_, &ConversationService::conversationLoaded, this, [this] {
        renderConversation();
    });
}

ChatController::~ChatController() = default;

void ChatController::submit(const QString& text) {
    if (!requestId_.isEmpty()) return;
    if (!promptBuilder_.isValid()) {
        widget_.addErrorMessage(promptBuilder_.error());
        return;
    }
    if (binaryProvider_) {
        conversations_.setBinary(binaryProvider_());
    }
    conversations_.appendMessage(stutter::MessageRole::User, text);
    widget_.addUserMessage(text);
    if (!selectProvider()) return;

    const stutter::Model model = selectedModel();
    const ChatContext context = contextBuilder_.build(
        conversations_.messages(),
        conversations_.currentConversation().summary,
        analysisContextProvider_ ? analysisContextProvider_() : QString(),
        model
    );
    activeRequest_ = promptBuilder_.build(context, providerConfig());
    QVector<ToolDefinition> allowedTools;
    if (tools_ && toolExecutor_) {
        for (const ToolDefinition& definition : tools_->definitions()) {
            if (toolExecutor_->canUse(definition.name)) allowedTools.append(definition);
        }
    }
    const bool codex = providerConfig().providerId == QStringLiteral("codex");
    if (codex) {
        if (!allowedTools.isEmpty()) {
            ChatMessage protocol;
            protocol.role = MessageRole::System;
            protocol.content = stutter::toolProtocolInstructions(allowedTools);
            activeRequest_.messages.append(protocol);
        }
    } else {
        activeRequest_.tools = allowedTools;
    }

    streamedResponse_.clear();
    cancellationRequested_ = false;
    widget_.setBusy(true);
    widget_.beginAssistantMessage()->setTitle(assistantTitle());
    requestId_ = provider_->send(activeRequest_);
}

void ChatController::stop() {
    if (provider_ && !requestId_.isEmpty()) {
        cancellationRequested_ = true;
        provider_->cancel(requestId_);
    }
}

void ChatController::clear() {
    if (provider_ && !requestId_.isEmpty()) provider_->cancel(requestId_);
    conversations_.clear();
    sessionUsage_ = {};
    resetRequest();
    widget_.setUsageText({});
}

void ChatController::renderConversation() {
    widget_.clearMessages();
    sessionUsage_ = {};
    widget_.setUsageText({});
    for (const stutter::Message& message : conversations_.messages()) {
        switch (message.role) {
        case stutter::MessageRole::User:
            widget_.addUserMessage(message.content);
            break;
        case stutter::MessageRole::Assistant:
            widget_.addAssistantMessage(message.content);
            break;
        case stutter::MessageRole::Tool:
            widget_.addToolMessage(message.toolName, message.content);
            break;
        case stutter::MessageRole::System:
            break;
        }
    }
}

bool ChatController::selectProvider() {
    if (provider_) disconnect(provider_, nullptr, this, nullptr);
    ownedProvider_.reset();
    provider_ = nullptr;

    const stutter::ProviderConfig config = providerConfig();
    if (!config.isValid()) {
        widget_.addErrorMessage(tr("Select a provider and model in Settings"));
        return false;
    }
    if (config.providerId == QStringLiteral("codex")) {
        if (!codexProvider_.isConnected()) {
            widget_.addErrorMessage(tr("Select Codex in Settings and wait for it to connect"));
            return false;
        }
        provider_ = &codexProvider_;
    } else {
        if (settings_.apiKey().isEmpty()) {
            widget_.addErrorMessage(tr("Add an API key in Settings before sending a message"));
            return false;
        }
        try {
            ownedProvider_ = ProviderFactory::create(config, settings_.apiKey());
        } catch (const std::exception& exception) {
            widget_.addErrorMessage(QString::fromUtf8(exception.what()));
            return false;
        }
        provider_ = ownedProvider_.get();
    }
    connectProvider(*provider_);
    return true;
}

stutter::ProviderConfig ChatController::providerConfig() const {
    stutter::ProviderConfig config;
    config.providerId = settings_.providerId();
    config.modelId = settings_.model();
    config.endpoint = settings_.endpoint().isEmpty()
        ? modelCatalog_.defaultEndpoint(config.providerId)
        : QUrl(settings_.endpoint());
    config.effort = settings_.effort();
    const stutter::Model model = selectedModel();
    if (model.maxOutputTokens > 0) {
        config.maxOutputTokens = static_cast<int>(qMin<qint64>(model.maxOutputTokens, 16384));
    }
    return config;
}

QString ChatController::assistantTitle() const {
    const stutter::Model model = selectedModel();
    QString title = QStringLiteral("Stutter");
    const QString modelName = model.name.isEmpty() ? settings_.model() : model.name;
    if (!modelName.isEmpty()) {
        title += QStringLiteral(" - ") + modelName;
    }
    const QString effort = settings_.effort();
    if (!effort.isEmpty() && effort != QStringLiteral("none")) {
        title += QStringLiteral(" (") + effort + QStringLiteral(")");
    }
    return title;
}

stutter::Model ChatController::selectedModel() const {
    for (const stutter::Model& model : modelCatalog_.modelsForProvider(settings_.providerId())) {
        if (model.id == settings_.model()) return model;
    }
    return {};
}

void ChatController::connectProvider(Provider& provider) {
    connect(&provider, &Provider::eventReceived, this, &ChatController::handleEvent);
    connect(&provider, &Provider::finished, this, &ChatController::handleFinished);
    connect(&provider, &Provider::failed, this, &ChatController::handleFailure);
    connect(&provider, &Provider::activityStarted, this, [this](const stutter::ToolActivity& activity) {
        widget_.addActivity(activity);
        emit activityStarted(activity);
    });
    connect(&provider, &Provider::activityUpdated, this, [this](const stutter::ToolActivity& activity) {
        widget_.updateActivity(activity);
        emit activityUpdated(activity);
    });
    connect(&provider, &Provider::activityFinished, this, [this](const stutter::ToolActivity& activity) {
        widget_.updateActivity(activity);
        emit activityFinished(activity);
    });
}

void ChatController::handleEvent(const ProviderEvent& event) {
    if (event.requestId != requestId_) return;
    if (event.type == ProviderEventType::TextDelta) {
        streamedResponse_.append(event.textDelta);
        widget_.appendAssistantDelta(event.textDelta);
    }
}

void ChatController::handleFinished(const QString& requestId, const ChatResponse& response) {
    if (requestId != requestId_) return;
    if (streamedResponse_.isEmpty() && !response.content.isEmpty()) {
        widget_.appendAssistantDelta(response.content);
    }
    const QString assistantText = stutter::stripToolBlocks(response.content);
    if (!assistantText.isEmpty()) {
        conversations_.appendMessage(stutter::MessageRole::Assistant, assistantText);
    }
    sessionUsage_.inputTokens += response.usage.inputTokens;
    sessionUsage_.outputTokens += response.usage.outputTokens;
    if (provider_ == &codexProvider_ && runTextToolCall(response)) return;
    if (runToolCalls(response)) return;
    widget_.setUsageText(tr("%1 input / %2 output (session)")
        .arg(formatTokenCount(sessionUsage_.inputTokens))
        .arg(formatTokenCount(sessionUsage_.outputTokens)));
    widget_.finishAssistantMessage();
    resetRequest();
}

void ChatController::handleFailure(const QString& requestId, const ProviderError& error) {
    if (requestId != requestId_) return;
    if (cancellationRequested_) {
        widget_.finishAssistantMessage();
        resetRequest();
        return;
    }

    QString message = error.message;
    const bool contextLimit =
        message.contains(QStringLiteral("maximum length"), Qt::CaseInsensitive)
        || message.contains(QStringLiteral("context length"), Qt::CaseInsensitive)
        || message.contains(QStringLiteral("contextwindowexceeded"), Qt::CaseInsensitive)
        || error.code.contains(QStringLiteral("context"), Qt::CaseInsensitive);
    if (contextLimit) {
        message = tr("The conversation exceeded the model's context limit. "
                     "Start a new conversation or narrow the request.");
    }
    widget_.addErrorMessage(message.isEmpty() ? tr("Provider request failed") : message);
    resetRequest();
}

bool ChatController::runToolCalls(const ChatResponse& response) {
    if (response.toolCalls.isEmpty()) return false;
    if (!tools_ || !toolExecutor_) return false;

    ChatMessage assistantMessage;
    assistantMessage.role = MessageRole::Assistant;
    assistantMessage.content = response.content;
    assistantMessage.toolCalls = response.toolCalls;
    activeRequest_.messages.append(assistantMessage);

    for (const ToolCall& call : response.toolCalls) {
        stutter::ToolActivity activity;
        activity.id = call.id;
        activity.name = call.name;
        activity.category = toolCategory(toolExecutor_->permissionFor(call.name));
        activity.status = stutter::ToolActivityStatus::Running;
        activity.detail = describeCall(call);
        widget_.addActivity(activity);
        emit activityStarted(activity);

        const stutter::ToolResult result = toolExecutor_->execute(call);
        activity.status = result.success
            ? stutter::ToolActivityStatus::Succeeded
            : stutter::ToolActivityStatus::Failed;
        activity.result = result.success ? result.content : result.error;
        widget_.updateActivity(activity);
        emit activityFinished(activity);

        ChatMessage toolMessage;
        toolMessage.role = MessageRole::Tool;
        toolMessage.toolCallId = call.id;
        toolMessage.content = result.success
            ? truncateToolResult(result.content)
            : QStringLiteral("Error: %1").arg(result.error);
        activeRequest_.messages.append(toolMessage);
    }

    shrinkOldToolResults();
    streamedResponse_.clear();
    requestId_ = provider_->send(activeRequest_);
    return true;
}

void ChatController::shrinkOldToolResults() {
    int seen = 0;
    for (int index = activeRequest_.messages.size() - 1; index >= 0; --index) {
        ChatMessage& message = activeRequest_.messages[index];
        if (message.role != MessageRole::Tool) continue;
        ++seen;
        if (seen > keepRecentToolResults && message.content.size() > 160) {
            message.content = QStringLiteral("[older tool output omitted]");
        }
    }
}

bool ChatController::runTextToolCall(const ChatResponse& response) {
    if (!tools_ || !toolExecutor_) return false;
    ToolCall call;
    if (!stutter::parseToolCall(response.content, call)) return false;

    stutter::ToolActivity activity;
    activity.id = call.id;
    activity.name = call.name;
    activity.category = toolCategory(toolExecutor_->permissionFor(call.name));
    activity.status = stutter::ToolActivityStatus::Running;
    activity.detail = describeCall(call);
    widget_.addActivity(activity);
    emit activityStarted(activity);

    const stutter::ToolResult result = toolExecutor_->execute(call);
    activity.status = result.success
        ? stutter::ToolActivityStatus::Succeeded
        : stutter::ToolActivityStatus::Failed;
    activity.result = result.success ? result.content : result.error;
    widget_.updateActivity(activity);
    emit activityFinished(activity);

    const QString toolResult = result.success
        ? QStringLiteral("Tool result: %1").arg(truncateToolResult(result.content))
        : QStringLiteral("Tool error: %1").arg(result.error);

    streamedResponse_.clear();
    requestId_ = codexProvider_.continueTurn(toolResult);
    return true;
}

void ChatController::resetRequest() {
    requestId_.clear();
    streamedResponse_.clear();
    cancellationRequested_ = false;
    activeRequest_ = {};
    widget_.setBusy(false);
}
