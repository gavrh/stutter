#include <cutter/DebuggerGateway.hpp>

#include <core/Cutter.h>

#include <rz_core.h>

namespace {
QString addressString(RVA value) {
    return QStringLiteral("0x") + QString::number(value, 16);
}
}

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
    Core()->cmd(QStringLiteral("db %1").arg(addressString(address)));
    return true;
}

bool DebuggerGateway::removeBreakpoint(RVA address) const {
    if (address == RVA_INVALID) return false;
    Core()->cmd(QStringLiteral("db- %1").arg(addressString(address)));
    return true;
}

QByteArray DebuggerGateway::readMemory(RVA address, int length) const {
    return Core()->ioRead(address, qMax(1, length));
}
