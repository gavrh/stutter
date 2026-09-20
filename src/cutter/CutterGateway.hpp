#pragma once

#include <cutter/AnalysisEditor.hpp>
#include <cutter/BinaryEditor.hpp>
#include <cutter/DebuggerGateway.hpp>
#include <cutter/RizinReader.hpp>

#include <QObject>
#include <QString>

class CutterGateway final : public QObject {
    Q_OBJECT

public:
    explicit CutterGateway(QObject* parent = nullptr);

    bool isReady() const;
    RizinReader& reader() { return reader_; }
    AnalysisEditor& editor() { return editor_; }
    BinaryEditor& binaryEditor() { return binaryEditor_; }
    DebuggerGateway& debugger() { return debugger_; }

    QString analysisContext(const QString& binaryPath) const;

signals:
    void contextChanged();

private:
    RizinReader reader_;
    AnalysisEditor editor_;
    BinaryEditor binaryEditor_;
    DebuggerGateway debugger_;
};
