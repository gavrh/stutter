#include <cutter/CutterGateway.hpp>

#include <core/Cutter.h>

#include <QFileInfo>
#include <QStringList>

CutterGateway::CutterGateway(QObject* parent) : QObject(parent) {
    CutterCore* core = Core();
    if (!core) return;
    connect(core, &CutterCore::seekChanged, this, [this] { emit contextChanged(); });
    connect(core, &CutterCore::functionsChanged, this, &CutterGateway::contextChanged);
    connect(core, &CutterCore::commentsChanged, this, [this] { emit contextChanged(); });
    connect(core, &CutterCore::flagsChanged, this, &CutterGateway::contextChanged);
    connect(core, &CutterCore::instructionChanged, this, [this] { emit contextChanged(); });
}

bool CutterGateway::isReady() const {
    return reader_.isReady();
}

QString CutterGateway::analysisContext(const QString& binaryPath) const {
    if (!reader_.isReady()) return {};

    const RVA address = reader_.currentAddress();
    const RVA start = reader_.functionStart(address);

    QStringList lines;
    if (!binaryPath.isEmpty()) {
        lines.append(QStringLiteral("binary: %1").arg(QFileInfo(binaryPath).fileName()));
        lines.append(QStringLiteral("path: %1").arg(binaryPath));
    }
    lines.append(QStringLiteral("current address: 0x%1").arg(address, 0, 16));
    if (start != RVA_INVALID) {
        const QString name = reader_.functionName(address);
        lines.append(QStringLiteral("current function: %1 (0x%2-0x%3)")
            .arg(name.isEmpty() ? QStringLiteral("unknown") : name)
            .arg(start, 0, 16)
            .arg(reader_.functionEnd(address), 0, 16));
    }
    lines.append(QStringLiteral("debugging: %1")
        .arg(debugger_.isDebugging() ? QStringLiteral("yes") : QStringLiteral("no")));
    return lines.join(QLatin1Char('\n'));
}
