#include <providers/CodexAuthService.hpp>

#include <providers/CodexProcess.hpp>
#include <providers/CodexRpcClient.hpp>

#include <QDesktopServices>
#include <QJsonObject>
#include <QUrl>

CodexAuthService::CodexAuthService(
    CodexProcess& process,
    CodexRpcClient& rpc,
    QObject* parent
) : QObject(parent), process_(process), rpc_(rpc) {
    setStatus(
        process_.isAvailable() ? tr("Codex CLI found") : tr("Codex CLI not found on PATH"),
        false
    );
    connect(&rpc_, &CodexRpcClient::ready, this, [this] {
        readAccount(loginRequested_);
    });
    connect(&rpc_, &CodexRpcClient::protocolError, this, [this](const QString& message) {
        loginRequested_ = false;
        setStatus(message, false);
    });
    connect(&rpc_, &CodexRpcClient::notificationReceived, this,
        [this](const QString& method, const QJsonObject& params) {
            if (method == QStringLiteral("account/login/completed") &&
                (loginId_.isEmpty() || params.value(QStringLiteral("loginId")).toString() == loginId_)) {
                loginRequested_ = false;
                loginId_.clear();
                if (params.value(QStringLiteral("success")).toBool()) refresh();
                else setStatus(params.value(QStringLiteral("error")).toString(), false);
            } else if (method == QStringLiteral("account/updated")) {
                const bool connected = params.value(QStringLiteral("authMode")).toString()
                    == QStringLiteral("chatgpt");
                if (connected) {
                    const QString plan = params.value(QStringLiteral("planType")).toString();
                    setStatus(plan.isEmpty() ? tr("Connected to ChatGPT")
                                             : tr("Connected to ChatGPT (%1)").arg(plan), true);
                }
            }
        }
    );
}

bool CodexAuthService::isAvailable() const {
    return process_.isAvailable();
}

void CodexAuthService::connectChatGpt() {
    if (!process_.isAvailable()) {
        setStatus(tr("Install Codex CLI and ensure 'codex' is on PATH"), false);
        return;
    }
    loginRequested_ = true;
    setStatus(tr("Starting Codex..."), false, true);
    if (rpc_.isReady()) readAccount(true);
    else rpc_.start();
}

void CodexAuthService::disconnect() {
    loginRequested_ = false;
    loginId_.clear();
    process_.stop();
    setStatus(
        process_.isAvailable() ? tr("Codex CLI found") : tr("Codex CLI not found on PATH"),
        false
    );
}

void CodexAuthService::refresh() {
    if (rpc_.isReady()) readAccount(false);
}

void CodexAuthService::logout() {
    if (!rpc_.isReady()) return;
    setStatus(tr("Disconnecting..."), connected_, true);
    rpc_.request(QStringLiteral("account/logout"), {},
        [this](const QJsonObject&, const QJsonObject& error) {
            if (!error.isEmpty()) {
                setStatus(error.value(QStringLiteral("message")).toString(), connected_);
                return;
            }
            setStatus(tr("Not connected to ChatGPT"), false);
        });
}

void CodexAuthService::readAccount(bool loginIfMissing) {
    setStatus(tr("Checking ChatGPT connection..."), connected_, true);
    rpc_.request(
        QStringLiteral("account/read"),
        QJsonObject {{QStringLiteral("refreshToken"), false}},
        [this, loginIfMissing](const QJsonObject& result, const QJsonObject& error) {
            if (!error.isEmpty()) {
                setStatus(error.value(QStringLiteral("message")).toString(), false);
                return;
            }
            const QJsonObject account = result.value(QStringLiteral("account")).toObject();
            if (account.value(QStringLiteral("type")).toString() == QStringLiteral("chatgpt")) {
                loginRequested_ = false;
                const QString plan = account.value(QStringLiteral("planType")).toString();
                setStatus(plan.isEmpty() ? tr("Connected to ChatGPT")
                                         : tr("Connected to ChatGPT (%1)").arg(plan), true);
            } else if (loginIfMissing) {
                beginLogin();
            } else {
                setStatus(tr("Not connected to ChatGPT"), false);
            }
        }
    );
}

void CodexAuthService::beginLogin() {
    setStatus(tr("Preparing ChatGPT sign-in..."), false, true);
    rpc_.request(
        QStringLiteral("account/login/start"),
        QJsonObject {
            {QStringLiteral("type"), QStringLiteral("chatgpt")},
            {QStringLiteral("useHostedLoginSuccessPage"), true},
            {QStringLiteral("appBrand"), QStringLiteral("chatgpt")}
        },
        [this](const QJsonObject& result, const QJsonObject& error) {
            if (!error.isEmpty()) {
                loginRequested_ = false;
                setStatus(error.value(QStringLiteral("message")).toString(), false);
                return;
            }
            loginId_ = result.value(QStringLiteral("loginId")).toString();
            const QUrl authUrl(result.value(QStringLiteral("authUrl")).toString());
            if (!authUrl.isValid() || !QDesktopServices::openUrl(authUrl)) {
                setStatus(tr("Could not open the ChatGPT sign-in page"), false);
                return;
            }
            setStatus(tr("Complete sign-in in your browser"), false, true);
        }
    );
}

void CodexAuthService::setStatus(const QString& text, bool connected, bool busy) {
    statusText_ = text;
    connected_ = connected;
    emit statusChanged(text, connected, busy);
}
