#include <providers/OpenAIProvider.hpp>

#include <providers/StreamParser.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUuid>

#include <utility>

namespace {
QString roleName(MessageRole role) {
    switch (role) {
    case MessageRole::System: return QStringLiteral("system");
    case MessageRole::User: return QStringLiteral("user");
    case MessageRole::Assistant: return QStringLiteral("assistant");
    case MessageRole::Tool: return QStringLiteral("tool");
    }
    return QStringLiteral("user");
}

QUrl endpointFor(QUrl baseUrl) {
    if (baseUrl.isEmpty()) {
        baseUrl = QUrl(QStringLiteral("https://api.openai.com/"));
    }
    QString path = baseUrl.path();
    if (!path.endsWith(QLatin1Char('/'))) {
        path.append(QLatin1Char('/'));
    }
    if (!path.endsWith(QStringLiteral("v1/chat/completions/"))) {
        path.append(QStringLiteral("v1/chat/completions"));
    } else {
        path.chop(1);
    }
    baseUrl.setPath(path);
    return baseUrl;
}

QJsonObject requestBody(const ChatRequest& request, bool includeStreamUsage) {
    QJsonArray messages;
    for (const ChatMessage& message : request.messages) {
        QJsonObject object {
            {QStringLiteral("role"), roleName(message.role)},
            {QStringLiteral("content"), message.content}
        };
        if (!message.toolCallId.isEmpty()) {
            object.insert(QStringLiteral("tool_call_id"), message.toolCallId);
        }
        if (!message.toolCalls.isEmpty()) {
            QJsonArray calls;
            for (const ToolCall& call : message.toolCalls) {
                calls.append(QJsonObject {
                    {QStringLiteral("id"), call.id},
                    {QStringLiteral("type"), QStringLiteral("function")},
                    {QStringLiteral("function"), QJsonObject {
                        {QStringLiteral("name"), call.name},
                        {QStringLiteral("arguments"), call.rawArguments.isEmpty()
                            ? QString::fromUtf8(QJsonDocument(call.arguments).toJson(QJsonDocument::Compact))
                            : call.rawArguments}
                    }}
                });
            }
            object.insert(QStringLiteral("tool_calls"), calls);
        }
        messages.append(object);
    }

    QJsonObject body {
        {QStringLiteral("model"), request.model},
        {QStringLiteral("messages"), messages},
        {QStringLiteral("stream"), true},
        {QStringLiteral("max_tokens"), request.maxTokens}
    };
    if (includeStreamUsage) {
        body.insert(
            QStringLiteral("stream_options"),
            QJsonObject {{QStringLiteral("include_usage"), true}}
        );
    }
    if (request.temperature >= 0.0) {
        body.insert(QStringLiteral("temperature"), request.temperature);
    }
    if (!request.effort.isEmpty()) {
        body.insert(QStringLiteral("reasoning_effort"), request.effort);
    }
    if (!request.tools.isEmpty()) {
        QJsonArray tools;
        for (const ToolDefinition& tool : request.tools) {
            tools.append(QJsonObject {
                {QStringLiteral("type"), QStringLiteral("function")},
                {QStringLiteral("function"), QJsonObject {
                    {QStringLiteral("name"), tool.name},
                    {QStringLiteral("description"), tool.description},
                    {QStringLiteral("parameters"), tool.inputSchema}
                }}
            });
        }
        body.insert(QStringLiteral("tools"), tools);
    }
    return body;
}

ProviderError responseError(QNetworkReply* reply, const QByteArray& body) {
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QJsonObject root = QJsonDocument::fromJson(body).object();
    const QJsonObject object = root.value(QStringLiteral("error")).toObject();
    ProviderError error;
    error.httpStatus = status;
    error.code = object.value(QStringLiteral("code")).toVariant().toString();
    error.message = object.value(QStringLiteral("message")).toString();
    if (error.message.isEmpty()) {
        error.message = reply->errorString();
    }
    error.retryable = status == 408 || status == 409 || status == 429 || status >= 500;
    return error;
}
}

struct OpenAIProvider::RequestState {
    QString id;
    OpenAIStreamParser parser;
    ChatResponse response;
    QMap<int, ToolCall> tools;
    QByteArray errorBody;
    bool failed = false;
};

OpenAIProvider::OpenAIProvider(
    QString apiKey,
    QUrl baseUrl,
    bool includeStreamUsage,
    QObject* parent
) : Provider(parent),
    apiKey_(std::move(apiKey)),
    baseUrl_(endpointFor(std::move(baseUrl))),
    includeStreamUsage_(includeStreamUsage),
    network_(this) {}

OpenAIProvider::~OpenAIProvider() = default;

QString OpenAIProvider::send(const ChatRequest& request) {
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (apiKey_.isEmpty() || request.model.isEmpty()) {
        ProviderError error;
        error.code = QStringLiteral("invalid_request");
        error.message = apiKey_.isEmpty()
            ? QStringLiteral("OpenAI API key is empty")
            : QStringLiteral("Model is empty");
        QTimer::singleShot(0, this, [this, id, error] { emit failed(id, error); });
        return id;
    }

    QNetworkRequest networkRequest(baseUrl_);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    networkRequest.setRawHeader("Accept", "text/event-stream");
    networkRequest.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey_.toUtf8());
    QNetworkReply* reply = network_.post(
        networkRequest,
        QJsonDocument(requestBody(request, includeStreamUsage_)).toJson(QJsonDocument::Compact)
    );
    auto state = std::make_unique<RequestState>();
    state->id = id;
    requests_.emplace(reply, std::move(state));
    connect(reply, &QNetworkReply::readyRead, this, [this, reply] { readReply(reply); });
    connect(reply, &QNetworkReply::finished, this, [this, reply] { finishReply(reply); });
    return id;
}

void OpenAIProvider::cancel(const QString& requestId) {
    for (const auto& entry : requests_) {
        if (entry.second->id == requestId) {
            entry.first->abort();
            return;
        }
    }
}

void OpenAIProvider::readReply(QNetworkReply* reply) {
    const auto iterator = requests_.find(reply);
    if (iterator == requests_.end()) {
        return;
    }
    const QByteArray bytes = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 400) {
        iterator->second->errorBody.append(bytes);
    } else {
        processEvents(*iterator->second, iterator->second->parser.push(bytes));
    }
}

void OpenAIProvider::processEvents(RequestState& state, QVector<ProviderEvent> events) {
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

void OpenAIProvider::finishReply(QNetworkReply* reply) {
    const auto iterator = requests_.find(reply);
    if (iterator == requests_.end()) {
        reply->deleteLater();
        return;
    }
    std::unique_ptr<RequestState> state = std::move(iterator->second);
    requests_.erase(iterator);

    const QByteArray remaining = reply->readAll();
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 400) {
        state->errorBody.append(remaining);
    } else {
        processEvents(*state, state->parser.push(remaining));
    }

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
