#include <tools/ToolValidator.hpp>

#include <QJsonArray>
#include <QJsonValue>

namespace stutter::ToolValidator {

bool addressValue(const QJsonValue& value, RVA& out, QString& error) {
    if (value.isDouble()) {
        const double number = value.toDouble();
        if (number < 0) {
            error = QStringLiteral("address must not be negative");
            return false;
        }
        out = static_cast<RVA>(number);
        return true;
    }
    if (value.isString()) {
        const QString text = value.toString().trimmed();
        if (text.isEmpty()) {
            error = QStringLiteral("address is empty");
            return false;
        }
        bool ok = false;
        const RVA parsed = text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)
            ? text.mid(2).toULongLong(&ok, 16)
            : text.toULongLong(&ok, 10);
        if (!ok) {
            error = QStringLiteral("not a valid address: %1").arg(text);
            return false;
        }
        out = parsed;
        return true;
    }
    error = QStringLiteral("address must be a string or integer");
    return false;
}

bool address(const QJsonObject& arguments, const QString& key, RVA& out, QString& error) {
    const QJsonValue value = arguments.value(key);
    if (value.isUndefined() || value.isNull()) {
        error = QStringLiteral("%1 is required").arg(key);
        return false;
    }
    return addressValue(value, out, error);
}

bool addresses(
    const QJsonObject& arguments,
    QVector<RVA>& out,
    QString& error,
    int maxCount
) {
    const QJsonValue multi = arguments.value(QStringLiteral("addresses"));
    if (multi.isArray()) {
        const QJsonArray array = multi.toArray();
        if (array.isEmpty()) {
            error = QStringLiteral("addresses is empty");
            return false;
        }
        if (array.size() > maxCount) {
            error = QStringLiteral("too many addresses (maximum %1)").arg(maxCount);
            return false;
        }
        for (const QJsonValue& value : array) {
            RVA parsed = RVA_INVALID;
            if (!addressValue(value, parsed, error)) return false;
            out.append(parsed);
        }
        return true;
    }
    if (multi.isString() || multi.isDouble()) {
        RVA parsed = RVA_INVALID;
        if (!addressValue(multi, parsed, error)) return false;
        out.append(parsed);
        return true;
    }
    RVA single = RVA_INVALID;
    if (!address(arguments, QStringLiteral("address"), single, error)) return false;
    out.append(single);
    return true;
}

bool requiredString(const QJsonObject& arguments, const QString& key, QString& out, QString& error) {
    const QJsonValue value = arguments.value(key);
    if (!value.isString() || value.toString().isEmpty()) {
        error = QStringLiteral("%1 is required").arg(key);
        return false;
    }
    out = value.toString();
    return true;
}

QString string(const QJsonObject& arguments, const QString& key, const QString& fallback) {
    const QJsonValue value = arguments.value(key);
    return value.isString() ? value.toString() : fallback;
}

int boundedInt(
    const QJsonObject& arguments,
    const QString& key,
    int fallback,
    int minimum,
    int maximum
) {
    const QJsonValue value = arguments.value(key);
    if (!value.isDouble()) return fallback;
    return qBound(minimum, static_cast<int>(value.toDouble()), maximum);
}

QString hexBytes(const QByteArray& bytes) {
    return QString::fromLatin1(bytes.toHex(' '));
}

}
