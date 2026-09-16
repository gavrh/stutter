#include <chat/ChatController.hpp>

#include <config/ModelCatalog.hpp>
#include <providers/CodexProvider.hpp>
#include <providers/ProviderFactory.hpp>
#include <ui/ChatWidget.hpp>
#include <ui/SettingsDialog.hpp>

#include <QJsonDocument>

namespace {
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
    QObject* parent
) : QObject(parent),
    widget_(widget),
    settings_(settings),
    modelCatalog_(modelCatalog),
    codexProvider_(codexProvider),
    conversations_(this) {
    connect(&widget_, &ChatWidget::messageSubmitted, this, &ChatController::submit);
    connect(&widget_, &ChatWidget::stopRequested, this, &ChatController::stop);
    connect(&widget_, &ChatWidget::conversationCleared, this, &ChatController::clear);
}

ChatController::~ChatController() = default;

void ChatController::submit(const QString& text) {
    if (!requestId_.isEmpty()) return;
    if (!promptBuilder_.isValid()) {
        widget_.addErrorMessage(promptBuilder_.error());
        return;
    }
    if (!selectProvider()) return;

    conversations_.appendMessage(stutter::MessageRole::User, text);
    const stutter::Model model = selectedModel();
    const ChatContext context = contextBuilder_.build(
        conversations_.messages(),
        conversations_.currentConversation().summary,
        {},
        model
    );
    const ChatRequest request = promptBuilder_.build(context, providerConfig());

    streamedResponse_.clear();
    cancellationRequested_ = false;
    widget_.setBusy(true);
    widget_.beginAssistantMessage();
    requestId_ = provider_->send(request);
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
    resetRequest();
    widget_.setUsageText({});
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
    if (!response.content.isEmpty()) {
        conversations_.appendMessage(stutter::MessageRole::Assistant, response.content);
    }
    for (const ToolCall& tool : response.toolCalls) {
        const QString arguments = tool.rawArguments.isEmpty()
            ? QString::fromUtf8(QJsonDocument(tool.arguments).toJson(QJsonDocument::Indented))
            : tool.rawArguments;
        widget_.addToolMessage(tool.name, arguments);
    }
    widget_.setUsageText(tr("%1 input / %2 output tokens")
        .arg(formatTokenCount(response.usage.inputTokens))
        .arg(formatTokenCount(response.usage.outputTokens)));
    widget_.finishAssistantMessage();
    resetRequest();
}

void ChatController::handleFailure(const QString& requestId, const ProviderError& error) {
    if (requestId != requestId_) return;
    if (cancellationRequested_) widget_.finishAssistantMessage();
    else widget_.addErrorMessage(error.message.isEmpty() ? tr("Provider request failed") : error.message);
    resetRequest();
}

void ChatController::resetRequest() {
    requestId_.clear();
    streamedResponse_.clear();
    cancellationRequested_ = false;
    widget_.setBusy(false);
}
