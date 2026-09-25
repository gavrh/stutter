#pragma once

#include <core/CutterCommon.h>

class DebuggerGateway {
public:
    bool isDebugging() const;

    void continueExecution() const;
    void stepIn() const;
    void stepOver() const;
    void stepOut() const;
    void stop() const;

    bool setBreakpoint(RVA address) const;
    bool removeBreakpoint(RVA address) const;
};
