/* SPDX-License-Identifier: LGPL-2.0-or-later */
#pragma once

#include <QImage>
#include <QList>
#include <QRectF>

namespace RegionDetector
{
struct Detection {
    QList<QRectF> panels;
    QList<QRectF> contents;
};
Detection detectDetailed(const QImage &image, const QRectF &canvasRect, const QList<QRectF> &viewports = {});

// Input pixels are mapped to the same logical canvas coordinates as selection.
QList<QRectF> detect(const QImage &image, const QRectF &canvasRect, const QList<QRectF> &viewports = {});
QList<QRectF> candidatesAt(const QList<QRectF> &regions, const QPointF &point, const QRectF &selected = {});
}
