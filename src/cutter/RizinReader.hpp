#pragma once

#include <core/CutterCommon.h>

#include <QByteArray>
#include <QString>

class RizinReader {
public:
    struct CommandResult {
        QString output;
        bool success = true;
    };

    bool isReady() const;
    QString fileName() const;
    RVA currentAddress() const;
    RVA functionStart(RVA address) const;
    RVA functionEnd(RVA address) const;
    QString functionName(RVA address) const;

    QString disassembleFunction(RVA address) const;
    QString decompile(RVA address) const;
    QString xrefs(RVA address) const;
    QString commentAt(RVA address) const;

    CommandResult command(const QString& command) const;

    QByteArray readBytes(RVA address, int length) const;
};
