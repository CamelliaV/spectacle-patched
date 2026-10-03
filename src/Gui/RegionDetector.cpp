/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "RegionDetector.h"

#include <algorithm>
#include <cmath>
#include <opencv2/imgproc.hpp>

namespace
{
qreal area(const QRectF &rect)
{
    return rect.width() * rect.height();
}

bool equivalent(const QRectF &a, const QRectF &b)
{
    const auto intersection = a.intersected(b);
    const qreal overlap = area(intersection);
    const qreal combined = area(a) + area(b) - overlap;
    return combined > 0 && overlap / combined > 0.9;
}

struct Candidate {
    QRectF rect;
    bool panel;
};

RegionDetector::Detection detectViewport(const QImage &image, const QRectF &viewport)
{
    auto small = image;
    if (qMax(small.width(), small.height()) > 2048) {
        small = small.scaled(2048, 2048, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    small = small.convertToFormat(QImage::Format_Grayscale8);
    cv::Mat gray(small.height(), small.width(), CV_8UC1, small.bits(), small.bytesPerLine());
    const qreal sx = viewport.width() / small.width();
    const qreal sy = viewport.height() / small.height();
    QList<Candidate> candidates;
    for (const auto &thresholds : {std::pair{40, 120}, std::pair{8, 24}}) {
        cv::Mat edges;
        cv::Canny(gray, edges, thresholds.first, thresholds.second);
        for (const auto &kernel : {cv::Size(1, 1), cv::Size(9, 3), cv::Size(21, 11), cv::Size(31, 21)}) {
            const bool panel = kernel.width == 1;
            // The low-contrast pass is only for closed panels, not text noise.
            if (thresholds.first == 8 && !panel) {
                continue;
            }
            cv::Mat joined;
            const cv::Size scaledKernel(qMax(1, qRound((panel ? 3 : kernel.width) / sx)), qMax(1, qRound((panel ? 3 : kernel.height) / sy)));
            cv::morphologyEx(edges, joined, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, scaledKernel));
            std::vector<std::vector<cv::Point>> contours;
            cv::findContours(joined, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
            for (const auto &contour : contours) {
                const auto box = cv::boundingRect(contour);
                const qreal width = box.width * sx;
                const qreal height = box.height * sy;
                if (width < 32 || height < 16 || width * height < 600 || box.area() > small.width() * small.height() * 0.98) {
                    continue;
                }
                // A closed panel has a substantial interior. Discard letter
                // outlines and irregular, disconnected details in the panel pass.
                if (panel) {
                    std::vector<cv::Point> outline;
                    cv::approxPolyDP(contour, outline, cv::arcLength(contour, true) * 0.02, true);
                    if (std::abs(cv::contourArea(contour)) / box.area() < 0.7 || !cv::isContourConvex(outline)) {
                        continue;
                    }
                }
                QRectF rect(viewport.x() + box.x * sx, viewport.y() + box.y * sy, width, height);
                if (!panel) {
                    rect = rect.adjusted(-3, -3, 3, 3);
                }
                candidates.append({rect.intersected(viewport), panel});
            }
        }
    }
    // Preserve large panels before applying the budget. Previously the smallest
    // 256 contours could crowd out the very content block the user wanted.
    std::stable_sort(candidates.begin(), candidates.end(), [](const Candidate &a, const Candidate &b) {
        return a.panel != b.panel ? a.panel : area(a.rect) > area(b.rect);
    });
    QList<QRectF> result;
    RegionDetector::Detection detected;
    int panels = 0;
    int contents = 0;
    for (const auto &candidate : candidates) {
        int &count = candidate.panel ? panels : contents;
        if (count >= 192 || std::any_of(result.cbegin(), result.cend(), [&candidate](const QRectF &other) {
                return equivalent(candidate.rect, other);
            })) {
            continue;
        }
        result.append(candidate.rect);
        (candidate.panel ? detected.panels : detected.contents).append(candidate.rect);
        ++count;
    }
    return detected;
}
}

RegionDetector::Detection RegionDetector::detectDetailed(const QImage &image, const QRectF &canvasRect, const QList<QRectF> &viewports)
{
    if (image.isNull() || canvasRect.isEmpty()) {
        return {};
    }
    const auto screens = viewports.isEmpty() ? QList<QRectF>{canvasRect} : viewports;
    const qreal sx = image.width() / canvasRect.width();
    const qreal sy = image.height() / canvasRect.height();
    Detection result;
    for (const auto &screen : screens) {
        const auto viewport = screen.intersected(canvasRect);
        if (viewport.isEmpty()) {
            continue;
        }
        const QRect pixels = QRectF((viewport.x() - canvasRect.x()) * sx, (viewport.y() - canvasRect.y()) * sy, viewport.width() * sx, viewport.height() * sy)
                                 .toAlignedRect()
                                 .intersected(image.rect());
        if (pixels.isEmpty()) {
            continue;
        }
        // Account for rounding the crop to pixels before mapping back to logical coordinates.
        const QRectF actualViewport(canvasRect.x() + pixels.x() / sx, canvasRect.y() + pixels.y() / sy, pixels.width() / sx, pixels.height() / sy);
        const auto detected = detectViewport(image.copy(pixels), actualViewport);
        for (const auto &rect : detected.panels) {
            result.panels.append(rect.intersected(viewport));
        }
        for (const auto &rect : detected.contents) {
            result.contents.append(rect.intersected(viewport));
        }
    }
    return result;
}

QList<QRectF> RegionDetector::detect(const QImage &image, const QRectF &canvasRect, const QList<QRectF> &viewports)
{
    const auto result = detectDetailed(image, canvasRect, viewports);
    return result.panels + result.contents;
}

QList<QRectF> RegionDetector::candidatesAt(const QList<QRectF> &regions, const QPointF &point, const QRectF &selected)
{
    QList<QRectF> result;
    for (const auto &rect : regions) {
        if (!rect.isEmpty() && rect.contains(point)) {
            result.append(rect);
        }
    }
    std::stable_sort(result.begin(), result.end(), [](const QRectF &a, const QRectF &b) {
        return area(a) < area(b);
    });
    QList<QRectF> unique;
    for (const auto &rect : result) {
        if (!std::any_of(unique.cbegin(), unique.cend(), [&rect](const QRectF &other) {
                return equivalent(rect, other);
            })) {
            unique.append(rect);
        }
    }
    if (!selected.isEmpty() && selected.contains(point)) {
        unique.removeIf([&selected](const QRectF &rect) {
            return equivalent(rect, selected);
        });
        unique.append(selected);
        std::stable_sort(unique.begin(), unique.end(), [](const QRectF &a, const QRectF &b) {
            return area(a) < area(b);
        });
    }
    return unique;
}
