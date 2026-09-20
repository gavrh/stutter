#pragma once

#include <core/CutterCommon.h>

#include <QByteArray>
#include <QString>

class RizinReader {
public:
    bool isReady() const;
    QString fileName() const;
    RVA currentAddress() const;
    RVA functionStart(RVA address) const;
    RVA functionEnd(RVA address) const;
    QString functionName(RVA address) const;

    QString disassemble(RVA address, int instructionCount) const;
    QString disassembleFunction(RVA address) const;
    QString decompile(RVA address) const;
    QString hexdump(RVA address, int length) const;
    QString xrefs(RVA address) const;

    QString functions() const;
    QString strings(int limit) const;
    QString imports() const;
    QString exports() const;
    QString sections() const;

    QByteArray readBytes(RVA address, int length) const;
};
