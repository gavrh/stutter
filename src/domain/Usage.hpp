#pragma once

#include <QtGlobal>

namespace stutter {

struct Usage {
    qint64 inputTokens = 0;
    qint64 outputTokens = 0;
    double estimatedCost = 0.0;

    qint64 totalTokens() const { return inputTokens + outputTokens; }
};

}
