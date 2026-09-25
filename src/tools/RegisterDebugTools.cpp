#include <cutter/CutterGateway.hpp>
#include <tools/ToolRegistry.hpp>
#include <tools/ToolValidator.hpp>

#include <QJsonArray>
#include <QJsonObject>

static QJsonObject addressProperty(const QString& description) {
    return QJsonObject {
        {QStringLiteral("type"), QJsonArray {QStringLiteral("string"), QStringLiteral("integer")}},
        {QStringLiteral("description"), description}
    };
}

namespace stutter {

void registerDebugTools(ToolRegistry& registry, CutterGateway& gateway) {
    DebuggerGateway& debugger = gateway.debugger();

    registry.add(makeTool(
        QStringLiteral("debug_status"),
        QStringLiteral("Report whether a debug session is active."),
        toolSchema({}, {}),
        ToolPermission::Debugger,
        [&debugger](const QJsonObject&) {
            return ToolResult::ok(debugger.isDebugging()
                ? QStringLiteral("debugging: yes")
                : QStringLiteral("debugging: no"));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("debug_continue"),
        QStringLiteral("Continue execution."),
        toolSchema({}, {}),
        ToolPermission::Debugger,
        [&debugger](const QJsonObject&) {
            if (!debugger.isDebugging()) return ToolResult::fail(QStringLiteral("Not debugging"));
            debugger.continueExecution();
            return ToolResult::ok(QStringLiteral("Continued"));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("debug_step_in"),
        QStringLiteral("Step into the next instruction."),
        toolSchema({}, {}),
        ToolPermission::Debugger,
        [&debugger](const QJsonObject&) {
            if (!debugger.isDebugging()) return ToolResult::fail(QStringLiteral("Not debugging"));
            debugger.stepIn();
            return ToolResult::ok(QStringLiteral("Stepped in"));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("debug_step_over"),
        QStringLiteral("Step over the next instruction."),
        toolSchema({}, {}),
        ToolPermission::Debugger,
        [&debugger](const QJsonObject&) {
            if (!debugger.isDebugging()) return ToolResult::fail(QStringLiteral("Not debugging"));
            debugger.stepOver();
            return ToolResult::ok(QStringLiteral("Stepped over"));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("debug_step_out"),
        QStringLiteral("Step out of the current function."),
        toolSchema({}, {}),
        ToolPermission::Debugger,
        [&debugger](const QJsonObject&) {
            if (!debugger.isDebugging()) return ToolResult::fail(QStringLiteral("Not debugging"));
            debugger.stepOut();
            return ToolResult::ok(QStringLiteral("Stepped out"));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("debug_stop"),
        QStringLiteral("Stop the debug session."),
        toolSchema({}, {}),
        ToolPermission::Debugger,
        [&debugger](const QJsonObject&) {
            if (!debugger.isDebugging()) return ToolResult::fail(QStringLiteral("Not debugging"));
            debugger.stop();
            return ToolResult::ok(QStringLiteral("Stopped debugging"));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("set_breakpoint"),
        QStringLiteral("Set a breakpoint at an address."),
        toolSchema(
            {{QStringLiteral("address"), addressProperty(QStringLiteral("Breakpoint address"))}},
            {QStringLiteral("address")}
        ),
        ToolPermission::Debugger,
        [&debugger](const QJsonObject& arguments) {
            RVA address = RVA_INVALID;
            QString error;
            if (!ToolValidator::address(arguments, QStringLiteral("address"), address, error)) {
                return ToolResult::fail(error);
            }
            if (!debugger.setBreakpoint(address)) {
                return ToolResult::fail(QStringLiteral("Unable to set breakpoint"));
            }
            return ToolResult::ok(QStringLiteral("Set breakpoint at 0x%1").arg(address, 0, 16));
        }
    ));

    registry.add(makeTool(
        QStringLiteral("remove_breakpoint"),
        QStringLiteral("Remove a breakpoint at an address."),
        toolSchema(
            {{QStringLiteral("address"), addressProperty(QStringLiteral("Breakpoint address"))}},
            {QStringLiteral("address")}
        ),
        ToolPermission::Debugger,
        [&debugger](const QJsonObject& arguments) {
            RVA address = RVA_INVALID;
            QString error;
            if (!ToolValidator::address(arguments, QStringLiteral("address"), address, error)) {
                return ToolResult::fail(error);
            }
            if (!debugger.removeBreakpoint(address)) {
                return ToolResult::fail(QStringLiteral("Unable to remove breakpoint"));
            }
            return ToolResult::ok(QStringLiteral("Removed breakpoint at 0x%1").arg(address, 0, 16));
        }
    ));
}

}
