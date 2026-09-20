#include <cutter/BinaryEditor.hpp>

#include <core/Cutter.h>

bool BinaryEditor::isWriteModeEnabled() const {
    return Core()->isWriteModeEnabled();
}

void BinaryEditor::setWriteMode(bool enabled) {
    Core()->setWriteMode(enabled);
}

void BinaryEditor::stage(RVA address, const QByteArray& bytes) {
    if (address == RVA_INVALID || bytes.isEmpty()) return;
    patches_.append(Patch {address, bytes});
}

void BinaryEditor::discard() {
    patches_.clear();
}

QVector<BinaryEditor::Patch> BinaryEditor::staged() const {
    return patches_;
}

bool BinaryEditor::commit() {
    if (patches_.isEmpty()) return true;
    if (!Core()->isWriteModeEnabled()) return false;

    for (const Patch& patch : patches_) {
        Core()->editBytesEndian(
            patch.address,
            QString::fromLatin1(patch.bytes.toHex())
        );
    }
    patches_.clear();
    Core()->triggerRefreshAll();
    return true;
}

QString BinaryEditor::preview() const {
    QString output;
    for (const Patch& patch : patches_) {
        output += QStringLiteral("0x%1: %2\n")
            .arg(patch.address, 0, 16)
            .arg(QString::fromLatin1(patch.bytes.toHex(' ')));
    }
    return output.trimmed();
}
