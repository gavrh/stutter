#include <cutter/CutterGateway.hpp>
#include <tools/ToolRegistry.hpp>
#include <tools/ToolValidator.hpp>

#include <QJsonObject>
#include <QStringList>

namespace {

bool hasForbiddenCharacters(const QString& text) {
    for (const QChar character : text) {
        if (character == QLatin1Char('\n') || character == QLatin1Char('\r')
            || character == QLatin1Char('|') || character == QLatin1Char('>')
            || character == QLatin1Char('<') || character == QLatin1Char('`')
            || character == QLatin1Char('$') || character == QLatin1Char('&')
            || character == QLatin1Char('!') || character == QLatin1Char('#')) {
            return true;
        }
    }
    return false;
}

bool startsWithAny(const QString& token, const QStringList& prefixes) {
    for (const QString& prefix : prefixes) {
        if (token.startsWith(prefix)) return true;
    }
    return false;
}

bool hasAssignment(const QString& segment) {
    return segment.contains(QLatin1Char('='));
}

bool isReadOnlyAnalysis(const QString& token, const QString& segment) {
    if (startsWithAny(token, {
            QStringLiteral("afl"), QStringLiteral("afo"), QStringLiteral("afx"),
            QStringLiteral("afM"), QStringLiteral("afd")
        })) {
        return true;
    }
    // `afi*` reads, except `afii-` which deletes imports used by the function.
    if (token.startsWith(QStringLiteral("afii-"))) return false;
    if (token.startsWith(QStringLiteral("afi"))) return true;
    if (startsWithAny(token, {QStringLiteral("afna"), QStringLiteral("afns")})) return true;
    if (startsWithAny(token, {
            QStringLiteral("afvl"), QStringLiteral("afv="), QStringLiteral("afvd"),
            QStringLiteral("afvf"), QStringLiteral("afvR"), QStringLiteral("afvW"),
            QStringLiteral("afvx")
        })) {
        return true;
    }
    if (startsWithAny(token, {
            QStringLiteral("axl"), QStringLiteral("axt"), QStringLiteral("axf"),
            QStringLiteral("axg")
        })) {
        return true;
    }
    if (token == QStringLiteral("ai") || token == QStringLiteral("aij")) return true;
    if (token.startsWith(QStringLiteral("aii-"))) return false;
    if (token.startsWith(QStringLiteral("aii"))) return true;
    if (startsWithAny(token, {
            QStringLiteral("abi"), QStringLiteral("abl"), QStringLiteral("abt")
        })) {
        return true;
    }
    if (startsWithAny(token, {
            QStringLiteral("ao"), QStringLiteral("aO"), QStringLiteral("a8"),
            QStringLiteral("aL")
        })) {
        return true;
    }
    if (token == QStringLiteral("adk")) return true;
    if (startsWithAny(token, {
            QStringLiteral("aai"), QStringLiteral("aau"), QStringLiteral("aaT")
        })) {
        return true;
    }
    // Graphs, except the stateful custom-graph subcommands.
    if (token.startsWith(QStringLiteral("ag"))) {
        return !startsWithAny(token, {
            QStringLiteral("ag-"), QStringLiteral("agn"), QStringLiteral("age"),
            QStringLiteral("agw")
        });
    }
    // Registers: reads unless assigning.
    if (token.startsWith(QStringLiteral("ar"))) {
        if (hasAssignment(segment)) return false;
        return !startsWithAny(token, {
            QStringLiteral("ara"), QStringLiteral("arp"), QStringLiteral("arf")
        });
    }
    // Syscalls: reads, except the ones that dump to files.
    if (token.startsWith(QStringLiteral("as"))) {
        return !token.startsWith(QStringLiteral("asc"));
    }
    // ESIL: statistics and state reads only.
    if (token.startsWith(QStringLiteral("ae"))) {
        if (token.contains(QLatin1Char('-'))) return false;
        return startsWithAny(token, {
            QStringLiteral("aei"), QStringLiteral("aek"), QStringLiteral("aea"),
            QStringLiteral("aeH")
        });
    }
    // Classes: reads unless removing or renaming.
    if (token.startsWith(QStringLiteral("ac"))) {
        if (token.contains(QLatin1Char('-'))) return false;
        return startsWithAny(token, {
            QStringLiteral("acl"), QStringLiteral("aci"), QStringLiteral("acg"),
            QStringLiteral("acm"), QStringLiteral("acb"), QStringLiteral("acv")
        });
    }
    if (startsWithAny(token, {
            QStringLiteral("ahl"), QStringLiteral("ahts")
        })) {
        return true;
    }
    if (token == QStringLiteral("av") || token == QStringLiteral("avj")
        || token == QStringLiteral("avg") || token == QStringLiteral("avr")
        || token.startsWith(QStringLiteral("avrD"))) {
        return true;
    }
    return false;
}

bool isReadOnlyDebug(const QString& token, const QString& segment) {
    if (token == QStringLiteral("dr")) return !hasAssignment(segment);
    if (token.startsWith(QStringLiteral("dbt"))) return true;
    if (token.startsWith(QStringLiteral("di"))) return true;
    if (token.startsWith(QStringLiteral("dbl"))) return true;
    if (token == QStringLiteral("db.")) return true;
    if (startsWithAny(token, {QStringLiteral("ddl"), QStringLiteral("del")})) return true;
    if (startsWithAny(token, {
            QStringLiteral("dkl"), QStringLiteral("dkn"), QStringLiteral("dkN")
        })) {
        return true;
    }
    if (token == QStringLiteral("dt")) return true;
    if (startsWithAny(token, {
            QStringLiteral("dtl"), QStringLiteral("dte"), QStringLiteral("dtg")
        })) {
        return true;
    }
    if (token == QStringLiteral("dm")) return true;
    if (startsWithAny(token, {
            QStringLiteral("dmj"), QStringLiteral("dmq"), QStringLiteral("dmt"),
            QStringLiteral("dm="), QStringLiteral("dm."), QStringLiteral("dmm"),
            QStringLiteral("dmh"), QStringLiteral("dmw"), QStringLiteral("dmx"),
            QStringLiteral("dmS")
        })) {
        return true;
    }
    if (token == QStringLiteral("dp")) return true;
    if (startsWithAny(token, {
            QStringLiteral("dpj"), QStringLiteral("dpl"), QStringLiteral("dpe"),
            QStringLiteral("dpT")
        })) {
        return true;
    }
    return false;
}

bool isReadOnlyCommand(const QString& segment) {
    const QString token = segment.section(QLatin1Char(' '), 0, 0);
    if (token.isEmpty()) return false;
    if (token == QStringLiteral("help")) return true;

    const QChar first = token.at(0);

    // Families that only ever read: info, print, alias for px, math, help,
    // last output, command specifiers and the search commands.
    if (first == QLatin1Char('i') || first == QLatin1Char('p') || first == QLatin1Char('x')
        || first == QLatin1Char('?') || first == QLatin1Char('%')
        || first == QLatin1Char('_') || first == QLatin1Char(':')) {
        return true;
    }
    if (first == QLatin1Char('/')) return true;

    if (first == QLatin1Char('c')) return !token.startsWith(QStringLiteral("cw"));

    // Code metadata listing only, e.g. `C`, `Cj`, `Cl`, `C.`, `C.j`.
    if (first == QLatin1Char('C')) {
        const QString rest = token.mid(1);
        for (const QChar character : rest) {
            if (character != QLatin1Char('j') && character != QLatin1Char('*')
                && character != QLatin1Char('l') && character != QLatin1Char('.')) {
                return false;
            }
        }
        return true;
    }

    if (first == QLatin1Char('e')) {
        if (token == QStringLiteral("el") || token == QStringLiteral("es")
            || token == QStringLiteral("et")) {
            return true;
        }
        return token == QStringLiteral("e") && !hasAssignment(segment);
    }

    if (first == QLatin1Char('k')) {
        if (token == QStringLiteral("kj")) return true;
        if (startsWithAny(token, {
                QStringLiteral("ks"), QStringLiteral("kd"), QStringLiteral("ko")
            })) {
            return false;
        }
        return !hasAssignment(segment);
    }

    if (first == QLatin1Char('H')) return token == QStringLiteral("H");
    if (first == QLatin1Char('R')) return token == QStringLiteral("R");

    if (first == QLatin1Char('L')) {
        if (!startsWithAny(token, {
                QStringLiteral("Ll"), QStringLiteral("La"), QStringLiteral("Lc"),
                QStringLiteral("LC"), QStringLiteral("Ld"), QStringLiteral("Lh"),
                QStringLiteral("Li"), QStringLiteral("Lo"), QStringLiteral("Lp"),
                QStringLiteral("LD")
            })) {
            return false;
        }
        // These two also load/register plugins when given an argument.
        if (token == QStringLiteral("Ld") || token == QStringLiteral("Lo")) {
            return !segment.contains(QLatin1Char(' '));
        }
        return true;
    }

    if (first == QLatin1Char('F')) {
        return startsWithAny(token, {
            QStringLiteral("Fl"), QStringLiteral("Fd"), QStringLiteral("Ff")
        });
    }

    if (first == QLatin1Char('f')) {
        if (token == QStringLiteral("f")) return !segment.contains(QLatin1Char(' '));
        if (token.startsWith(QStringLiteral("f-"))) return false;
        return startsWithAny(token, {
            QStringLiteral("fl"), QStringLiteral("fi"), QStringLiteral("fd"),
            QStringLiteral("fe"), QStringLiteral("ff"), QStringLiteral("fg"),
            QStringLiteral("fx"), QStringLiteral("fO"), QStringLiteral("f.")
        });
    }

    if (first == QLatin1Char('t')) {
        if (token == QStringLiteral("t")) return true;
        if (startsWithAny(token, {
                QStringLiteral("td"), QStringLiteral("tn"), QStringLiteral("to"),
                QStringLiteral("t-")
            })) {
            return false;
        }
        if (token.startsWith(QStringLiteral("tf-"))) return false;
        return startsWithAny(token, {
            QStringLiteral("tc"), QStringLiteral("te"), QStringLiteral("tf"),
            QStringLiteral("tp"), QStringLiteral("ts"), QStringLiteral("tt"),
            QStringLiteral("tu"), QStringLiteral("tx"), QStringLiteral("tl")
        });
    }

    if (first == QLatin1Char('a')) return isReadOnlyAnalysis(token, segment);
    if (first == QLatin1Char('d')) return isReadOnlyDebug(token, segment);

    return false;
}

QStringList splitCommands(const QString& command) {
    QStringList segments;
    for (const QString& part : command.split(QLatin1Char(';'))) {
        const QString trimmed = part.trimmed();
        if (!trimmed.isEmpty()) segments.append(trimmed);
    }
    return segments;
}
}

namespace stutter {

void registerConsoleTools(ToolRegistry& registry, CutterGateway& gateway) {
    RizinReader& reader = gateway.reader();

    registry.add(makeTool(
        QStringLiteral("run_rizin"),
        QStringLiteral(
            "Run read-only Rizin console commands. Separate multiple commands with ';'. Prefer the "
            "specific Cutter tools; use them for anything that changes the analysis, bytes, or "
            "debugger state so Cutter stays in sync."
        ),
        toolSchema(
            {{QStringLiteral("command"), stringProperty(QStringLiteral("Rizin command to run"))}},
            {QStringLiteral("command")}
        ),
        ToolPermission::Read,
        [&reader](const QJsonObject& arguments) {
            QString command;
            QString error;
            if (!ToolValidator::requiredString(arguments, QStringLiteral("command"), command, error)) {
                return ToolResult::fail(error);
            }
            if (command.size() > 1024 || hasForbiddenCharacters(command)) {
                return ToolResult::fail(QStringLiteral("Command contains unsupported characters"));
            }

            const QStringList segments = splitCommands(command);
            if (segments.isEmpty()) {
                return ToolResult::fail(QStringLiteral("Command is empty"));
            }
            if (segments.size() > 32) {
                return ToolResult::fail(QStringLiteral("Too many commands in one call"));
            }

            QStringList outputs;
            for (const QString& segment : segments) {
                if (!isReadOnlyCommand(segment)) {
                    return ToolResult::fail(QStringLiteral(
                        "Command is not allowed by the read-only console policy: %1"
                    ).arg(segment.section(QLatin1Char(' '), 0, 0)));
                }
                outputs.append(reader.command(segment));
            }
            const QString output = outputs.join(QLatin1Char('\n'));
            return ToolResult::ok(output.isEmpty() ? QStringLiteral("No output.") : output);
        }
    ));
}

}
