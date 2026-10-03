/* SPDX-License-Identifier: LGPL-2.0-or-later */
#pragma once

#include <QImage>
#include <QList>
#include <QRectF>
#include <QString>

namespace MultilingualOcr
{
struct Result {
    QString text;
    QString error;
    bool success = false;
};

struct RegionResult {
    QList<QRectF> regions;
    QString error;
    bool success = false;
};

bool isAvailable();
Result recognize(const QImage &image);
RegionResult detectTextRegions(const QImage &image, const QRectF &canvasRect, const QList<QRectF> &viewports);
}
