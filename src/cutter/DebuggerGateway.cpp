#include <cutter/DebuggerGateway.hpp>

#include <core/Cutter.h>

#include <rz_core.h>

bool DebuggerGateway::isDebugging() const {
    RzCoreLocked core(Core());
    return core->dbg && core->dbg->pid > 0;
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
