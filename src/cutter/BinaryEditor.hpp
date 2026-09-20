#pragma once

#include <core/CutterCommon.h>

#include <QByteArray>
#include <QVector>

class BinaryEditor {
public:
    struct Patch {
        RVA address = RVA_INVALID;
        QByteArray bytes;
    };

    bool isWriteModeEnabled() const;
    void setWriteMode(bool enabled);

    void stage(RVA address, const QByteArray& bytes);
    void discard();
    QVector<Patch> staged() const;
    bool commit();
    QString preview() const;

private:
    QVector<Patch> patches_;
};
