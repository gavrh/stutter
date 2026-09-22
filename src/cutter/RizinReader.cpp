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

QString RizinReader::disassemble(RVA address, int instructionCount) const {
    return Core()->cmd(
        QStringLiteral("pd %1 @ %2").arg(qMax(1, instructionCount)).arg(addressString(address))
    );
}

QString RizinReader::disassembleFunction(RVA address) const {
    return Core()->cmd(QStringLiteral("pdf @ %1").arg(addressString(address)));
}

QString RizinReader::decompile(RVA address) const {
    return Core()->cmd(QStringLiteral("pdc @ %1").arg(addressString(address)));
}

QString RizinReader::hexdump(RVA address, int length) const {
    return Core()->cmd(
        QStringLiteral("px %1 @ %2").arg(qMax(1, length)).arg(addressString(address))
    );
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

QString RizinReader::functions() const {
    return Core()->cmd(QStringLiteral("afl"));
}

QString RizinReader::strings(int limit) const {
    const int count = qMax(1, limit);
    return Core()->cmd(QStringLiteral("izz~^0[%1]").arg(count));
}

QString RizinReader::imports() const {
    return Core()->cmd(QStringLiteral("ii"));
}

QString RizinReader::exports() const {
    return Core()->cmd(QStringLiteral("iE"));
}

QString RizinReader::sections() const {
    return Core()->cmd(QStringLiteral("iS"));
}

QByteArray RizinReader::readBytes(RVA address, int length) const {
    return Core()->ioRead(address, qMax(1, length));
}
