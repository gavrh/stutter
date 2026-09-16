#pragma once

#include <QObject>
#include <QProcess>
#include <QString>

class CodexProcess final : public QObject {
    Q_OBJECT

public:
    explicit CodexProcess(QObject* parent = nullptr);
    ~CodexProcess() override;

    QString executablePath() const;
    bool isAvailable() const;
    bool isRunning() const;
    void start();
    void stop();
    qint64 write(const QByteArray& bytes);

signals:
    void started();
    void stopped();
    void standardOutput(const QByteArray& bytes);
    void diagnostic(const QString& message);
    void errorOccurred(const QString& message);

private:
    QProcess process_;
};
