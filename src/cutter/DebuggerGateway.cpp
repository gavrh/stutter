#include <cutter/DebuggerGateway.hpp>

#include <core/Cutter.h>

#include <rz_core.h>

bool DebuggerGateway::isDebugging() const {
    RzCoreLocked core(Core());
    return core->dbg && core->dbg->pid > 0;
}

QString DebuggerGateway::registers() const {
    if (!isDebugging()) return {};
    return Core()->cmd(QStringLiteral("drj"));
}

QString DebuggerGateway::backtrace() const {
    if (!isDebugging()) return {};
    return Core()->cmd(QStringLiteral("dbt"));
}

void DebuggerGateway::continueExecution() const {
    if (isDebugging()) Core()->continueDebug();
}

void DebuggerGateway::stepIn() const {
    if (isDebugging()) Core()->stepDebug();
}

void DebuggerGateway::stepOver() const {
    if (isDebugging()) Core()->stepOverDebug();
}

void DebuggerGateway::stepOut() const {
    if (isDebugging()) Core()->stepOutDebug();
}

void DebuggerGateway::stop() const {
    if (isDebugging()) Core()->stopDebug();
}

bool DebuggerGateway::setBreakpoint(RVA address) const {
    if (address == RVA_INVALID) return false;
    BreakpointDescription breakpoint;
    breakpoint.addr = address;
    Core()->addBreakpoint(breakpoint);
    return Core()->getBreakpointsAddresses().contains(address);
}

bool DebuggerGateway::removeBreakpoint(RVA address) const {
    if (address == RVA_INVALID) return false;
    Core()->delBreakpoint(address);
    return !Core()->getBreakpointsAddresses().contains(address);
}

QByteArray DebuggerGateway::readMemory(RVA address, int length) const {
    return Core()->ioRead(address, qMax(1, length));
}
