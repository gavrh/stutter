#pragma once

#include <core/CutterCommon.h>

#include <QByteArray>
#include <QString>

class DebuggerGateway {
public:
    bool isDebugging() const;

    QString registers() const;
    QString backtrace() const;

    void continueExecution() const;
    void stepIn() const;
    void stepOver() const;
    void stepOut() const;
    void stop() const;

    bool setBreakpoint(RVA address) const;
    bool removeBreakpoint(RVA address) const;

    QByteArray readMemory(RVA address, int length) const;
};
