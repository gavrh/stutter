#include <providers/CodexProcess.hpp>

#include <QStandardPaths>

CodexProcess::CodexProcess(QObject* parent) : QObject(parent) {
    process_.setProcessChannelMode(QProcess::SeparateChannels);
    connect(&process_, &QProcess::started, this, &CodexProcess::started);
    connect(&process_, &QProcess::readyReadStandardOutput, this, [this] {
        emit standardOutput(process_.readAllStandardOutput());
    });
    connect(&process_, &QProcess::readyReadStandardError, this, [this] {
        const QString message = QString::fromUtf8(process_.readAllStandardError()).trimmed();
        if (!message.isEmpty()) emit diagnostic(message);
    });
    connect(&process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit errorOccurred(process_.errorString());
    });
    connect(
        &process_,
        qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
        this,
        [this](int exitCode, QProcess::ExitStatus status) {
            if (status == QProcess::CrashExit) {
                emit errorOccurred(tr("Codex app-server crashed with exit code %1").arg(exitCode));
            }
            emit stopped();
        }
    );
}

CodexProcess::~CodexProcess() {
    stop();
}

QString CodexProcess::executablePath() const {
    return QStandardPaths::findExecutable(QStringLiteral("codex"));
}

bool CodexProcess::isAvailable() const {
    return !executablePath().isEmpty();
}

bool CodexProcess::isRunning() const {
    return process_.state() != QProcess::NotRunning;
}

void CodexProcess::start() {
    if (isRunning()) return;
    const QString executable = executablePath();
    if (executable.isEmpty()) {
        emit errorOccurred(tr("Codex CLI was not found on PATH"));
        return;
    }
    process_.setProgram(executable);
    process_.setArguments({QStringLiteral("app-server"), QStringLiteral("--stdio")});
    process_.start();
}

void CodexProcess::stop() {
    if (!isRunning()) return;
    process_.terminate();
    if (!process_.waitForFinished(1000)) {
        process_.kill();
        process_.waitForFinished(1000);
    }
}

qint64 CodexProcess::write(const QByteArray& bytes) {
    return isRunning() ? process_.write(bytes) : -1;
}
