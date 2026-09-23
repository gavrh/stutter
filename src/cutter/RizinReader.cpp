#include <cutter/RizinReader.hpp>

#include <core/Cutter.h>

namespace {
QString addressString(RVA value) {
    return QStringLiteral("0x") + QString::number(value, 16);
}
}

bool RizinReader::isReady() const {
    return !fileName().isEmpty();
}

QString RizinReader::fileName() const {
    const CutterJson info = Core()->cmdj(QStringLiteral("ij"));
    return info[QStringLiteral("core")][QStringLiteral("file")].toString();
}

RVA RizinReader::currentAddress() const {
    return Core()->getOffset();
}

RVA RizinReader::functionStart(RVA address) const {
    return Core()->getFunctionStart(address);
}

RVA RizinReader::functionEnd(RVA address) const {
    return Core()->getFunctionEnd(address);
}

QString RizinReader::functionName(RVA address) const {
    const CutterJson info = Core()->cmdj(
        QStringLiteral("afij @ %1").arg(addressString(address))
    );
    return info[QStringLiteral("name")].toString();
}

QString RizinReader::disassembleFunction(RVA address) const {
    return Core()->cmd(QStringLiteral("pdf @ %1").arg(addressString(address)));
}

QString RizinReader::decompile(RVA address) const {
    return Core()->cmd(QStringLiteral("pdc @ %1").arg(addressString(address)));
}

QString RizinReader::xrefs(RVA address) const {
    return Core()->cmd(QStringLiteral("axt @ %1").arg(addressString(address)));
}

QString RizinReader::commentAt(RVA address) const {
    return Core()->getCommentAt(address);
}

QString RizinReader::command(const QString& command) const {
    return Core()->cmdRaw(command.toUtf8().constData());
}

QByteArray RizinReader::readBytes(RVA address, int length) const {
    return Core()->ioRead(address, qMax(1, length));
}
