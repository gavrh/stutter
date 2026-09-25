#include <cutter/RizinReader.hpp>

#include <core/Cutter.h>

#include <rz_cmd.h>
#include <rz_cons.h>
#include <rz_core.h>

#include <cstdlib>

static QString addressString(RVA value) {
    return QStringLiteral("0x") + QString::number(value, 16);
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

RizinReader::CommandResult RizinReader::command(const QString& command) const {
    RzCoreLocked core(Core());
    rz_cons_push();
    const RzCmdStatus status = rz_core_cmd0_rzshell(core, command.toUtf8().constData());
    char* buffer = rz_cons_get_buffer_dup();
    rz_cons_pop();

    CommandResult result;
    result.output = buffer ? QString::fromUtf8(buffer) : QString();
    result.success = status == RZ_CMD_STATUS_OK;
    std::free(buffer);
    return result;
}

QByteArray RizinReader::readBytes(RVA address, int length) const {
    return Core()->ioRead(address, qMax(1, length));
}
