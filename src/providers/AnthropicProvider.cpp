#include <providers/AnthropicProvider.hpp>

#include <providers/StreamParser.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <QUuid>

#include <utility>

static QUrl endpointFor(QUrl baseUrl) {
    if (baseUrl.isEmpty()) {
        baseUrl = QUrl(QStringLiteral("https://api.anthropic.com/"));
    }
    QString path = baseUrl.path();
    if (!path.endsWith(QLatin1Char('/'))) path.append(QLatin1Char('/'));
    if (!path.endsWith(QStringLiteral("v1/messages/"))) {
        path.append(QStringLiteral("v1/messages"));
    } else {
        path.chop(1);
    }
    baseUrl.setPath(path);
    return baseUrl;
}

static QJsonObject requestBody(const ChatRequest& request) {
    QJsonArray messages;
    QStringList systemParts;
    for (const ChatMessage& message : request.messages) {
        if (message.role == MessageRole::System) {
            systemParts.append(message.content);
            continue;
        }

        QJsonArray content;
        if (!message.content.isEmpty()) {
            if (message.role == MessageRole::Tool) {
                content.append(QJsonObject {
                    {QStringLiteral("type"), QStringLiteral("tool_result")},
                    {QStringLiteral("tool_use_id"), message.toolCallId},
                    {QStringLiteral("content"), message.content}
                });
            } else {
                content.append(QJsonObject {
                    {QStringLiteral("type"), QStringLiteral("text")},
                    {QStringLiteral("text"), message.content}
                });
            }
        }
        for (const ToolCall& call : message.toolCalls) {
            content.append(QJsonObject {
                {QStringLiteral("type"), QStringLiteral("tool_use")},
                {QStringLiteral("id"), call.id},
                {QStringLiteral("name"), call.name},
                {QStringLiteral("input"), call.arguments}
            });
        }
        messages.append(QJsonObject {
            {QStringLiteral("role"), message.role == MessageRole::Assistant
                ? QStringLiteral("assistant") : QStringLiteral("user")},
            {QStringLiteral("content"), content}
        });
    }

    QJsonObject body {
        {QStringLiteral("model"), request.model},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("max_tokens"), request.maxTokens},
        {QStringLiteral("stream"), true}
    };
    if (!systemParts.isEmpty()) body.insert(QStringLiteral("system"), systemParts.join(QLatin1Char('\n')));
    if (request.temperature >= 0.0) body.insert(QStringLiteral("temperature"), request.temperature);
    if (!request.effort.isEmpty()) {
        body.insert(
            QStringLiteral("output_config"),
            QJsonObject {{QStringLiteral("effort"), request.effort}}
        );
    }
    if (!request.tools.isEmpty()) {
        QJsonArray tools;
        for (const ToolDefinition& tool : request.tools) {
            tools.append(QJsonObject {
                {QStringLiteral("name"), tool.name},
                {QStringLiteral("description"), tool.description},
                {QStringLiteral("input_schema"), tool.inputSchema}
            });
        }
        body.insert(QStringLiteral("tools"), tools);
    }
    return body;
}

static ProviderError responseError(QNetworkReply* reply, const QByteArray& body) {
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QJsonObject object = QJsonDocument::fromJson(body).object()
        .value(QStringLiteral("error")).toObject();
    ProviderError error;
    error.httpStatus = status;
    error.code = object.value(QStringLiteral("type")).toString();
    error.message = object.value(QStringLiteral("message")).toString();
    if (error.message.isEmpty()) error.message = reply->errorString();
    error.retryable = status == 408 || status == 409 || status == 429 || status >= 500;
    return error;
}

struct AnthropicProvider::RequestState {
    QString id;
    AnthropicStreamParser parser;
    ChatResponse response;
    QMap<int, ToolCall> tools;
    QByteArray errorBody;
    bool failed = false;
};

AnthropicProvider::AnthropicProvider(QString apiKey, QString apiVersion, QUrl baseUrl, QObject* parent)
    : Provider(parent),
      apiKey_(std::move(apiKey)),
      apiVersion_(std::move(apiVersion)),
      baseUrl_(endpointFor(std::move(baseUrl))),
      network_(this) {}

AnthropicProvider::~AnthropicProvider() = default;

QString AnthropicProvider::send(const ChatRequest& request) {
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (apiKey_.isEmpty() || request.model.isEmpty()) {
        ProviderError error;
        error.code = QStringLiteral("invalid_request");
        error.message = apiKey_.isEmpty()
            ? QStringLiteral("Anthropic API key is empty")
            : QStringLiteral("Model is empty");
        QTimer::singleShot(0, this, [this, id, error] { emit failed(id, error); });
        return id;
    }

    QNetworkRequest networkRequest(baseUrl_);
    networkRequest.setTransferTimeout(120000);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    networkRequest.setRawHeader("Accept", "text/event-stream");
    networkRequest.setRawHeader("x-api-key", apiKey_.toUtf8());
    networkRequest.setRawHeader("anthropic-version", apiVersion_.toUtf8());
    QNetworkReply* reply = network_.post(
        networkRequest,
        QJsonDocument(requestBody(request)).toJson(QJsonDocument::Compact)
    );
    auto state = std::make_unique<RequestState>();
    state->id = id;
    requests_.emplace(reply, std::move(state));
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] { readReply(reply); });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { finishReply(reply); });
    return id;
}

void AnthropicProvider::cancel(const QString& requestId) {
    for (const auto& entry : requests_) {
        if (entry.second->id == requestId) {
            entry.first->abort();
            return;
        }
    }
}

void AnthropicProvider::readReply(QNetworkReply* reply) {
    const auto iterator = requests_.find(reply);
    if (iterator == requests_.end()) return;
    const QByteArray bytes = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 400) iterator->second->errorBody.append(bytes);
    else processEvents(*iterator->second, iterator->second->parser.push(bytes));
}

void AnthropicProvider::processEvents(RequestState& state, QVector<ProviderEvent> events) {
    for (ProviderEvent& event : events) {
        event.requestId = state.id;
        if (event.type == ProviderEventType::TextDelta) {
            state.response.content.append(event.textDelta);
        } else if (event.type == ProviderEventType::ToolCallDelta) {
            ToolCall& tool = state.tools[event.toolIndex];
            if (!event.toolCallId.isEmpty()) tool.id = event.toolCallId;
            if (!event.toolName.isEmpty()) tool.name = event.toolName;
            tool.rawArguments.append(event.argumentsDelta);
        } else if (event.type == ProviderEventType::Completed) {
            state.response.usage = event.usage;
            state.response.stopReason = event.stopReason;
        } else if (event.type == ProviderEventType::Error) {
            state.failed = true;
            emit failed(state.id, event.error);
        }
        emit eventReceived(event);
    }
}

void AnthropicProvider::finishReply(QNetworkReply* reply) {
    const auto iterator = requests_.find(reply);
    if (iterator == requests_.end()) {
        reply->deleteLater();
        return;
    }
    std::unique_ptr<RequestState> state = std::move(iterator->second);
    requests_.erase(iterator);
    const QByteArray remaining = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 400) state->errorBody.append(remaining);
    else processEvents(*state, state->parser.push(remaining));

    if ((reply->error() != QNetworkReply::NoError || status >= 400) && !state->failed) {
        state->failed = true;
        emit failed(state->id, responseError(reply, state->errorBody));
    } else if (!state->failed) {
        processEvents(*state, state->parser.finish());
        for (ToolCall tool : state->tools) {
            const QJsonDocument arguments = QJsonDocument::fromJson(tool.rawArguments.toUtf8());
            if (arguments.isObject()) tool.arguments = arguments.object();
            state->response.toolCalls.append(tool);
        }
        emit finished(state->id, state->response);
    }
    reply->deleteLater();
}
