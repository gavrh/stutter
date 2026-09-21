#pragma once

#include <core/CutterCommon.h>

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVector>

namespace stutter::ToolValidator {

bool addressValue(const QJsonValue& value, RVA& out, QString& error);
bool address(const QJsonObject& arguments, const QString& key, RVA& out, QString& error);
bool addresses(
    const QJsonObject& arguments,
    QVector<RVA>& out,
    QString& error,
    int maxCount = 50
);
bool requiredString(const QJsonObject& arguments, const QString& key, QString& out, QString& error);
QString string(const QJsonObject& arguments, const QString& key, const QString& fallback = {});
int boundedInt(
    const QJsonObject& arguments,
    const QString& key,
    int fallback,
    int minimum,
    int maximum
);
QString hexBytes(const QByteArray& bytes);

}
