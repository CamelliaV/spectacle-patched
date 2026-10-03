/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "MultilingualOcr.h"

#include <KLocalizedString>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <cmath>

using namespace Qt::StringLiterals;

namespace
{
QString helperPath()
{
    if (qEnvironmentVariableIsSet("SPECTACLE_OCR_HELPER")) {
        return qEnvironmentVariable("SPECTACLE_OCR_HELPER");
    }
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, u"spectacle/ocr/recognize.py"_s);
}

QString assetsPath()
{
    return qEnvironmentVariable("SPECTACLE_OCR_ASSETS", QFileInfo(helperPath()).absolutePath());
}
}

bool MultilingualOcr::isAvailable()
{
    if (!QFileInfo::exists(helperPath()) || QStandardPaths::findExecutable(u"python3"_s).isEmpty()) {
        return false;
    }
    const QDir assets(assetsPath());
    for (const auto &file : {u"vendor/rapidocr/__init__.py"_s,
                             u"vendor/cv2/__init__.py"_s,
                             u"models/ch_PP-OCRv5_det_mobile.onnx"_s,
                             u"models/ch_PP-OCRv5_rec_server.onnx"_s,
                             u"models/en_PP-OCRv5_rec_mobile.onnx"_s,
                             u"models/ch_PP-LCNet_x0_25_textline_ori_cls_mobile.onnx"_s}) {
        if (!QFileInfo::exists(assets.filePath(file))) {
            return false;
        }
    }
    return true;
}

MultilingualOcr::Result MultilingualOcr::recognize(const QImage &image)
{
    if (image.isNull()) {
        return {{}, i18n("No image to recognize."), false};
    }
    if (!isAvailable()) {
        return {{}, i18n("The multilingual OCR models are not installed."), false};
    }
    QTemporaryDir directory;
    const auto input = directory.filePath(u"screenshot.png"_s);
    if (!directory.isValid() || !image.save(input)) {
        return {{}, i18n("Could not prepare the screenshot for text recognition."), false};
    }
    // Run entirely locally and off the GUI thread. The private temporary image
    // is removed after the child has exited, including failures and timeouts.
    QProcess process;
    process.start(QStandardPaths::findExecutable(u"python3"_s), {helperPath(), u"--assets"_s, assetsPath(), input});
    if (!process.waitForStarted(5000)) {
        return {{}, i18n("Could not start multilingual text recognition."), false};
    }
    if (!process.waitForFinished(60000)) {
        process.kill();
        process.waitForFinished();
        return {{}, i18n("Multilingual text recognition timed out. Try a smaller region."), false};
    }
    const auto output = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || output.value(u"version"_s).toInt() != 1
        || !output.value(u"success"_s).toBool() || !output.value(u"text"_s).isString()) {
        auto detail = output.value(u"error"_s).toString();
        if (detail.isEmpty()) {
            detail = QString::fromUtf8(process.readAllStandardError()).right(1000).trimmed();
        }
        return {{}, i18n("Multilingual text recognition failed: %1", detail), false};
    }
    // Leading spaces can be reconstructed command/code indentation.
    return {output.value(u"text"_s).toString(), {}, true};
}

MultilingualOcr::RegionResult MultilingualOcr::detectTextRegions(const QImage &image, const QRectF &canvasRect, const QList<QRectF> &viewports)
{
    if (image.isNull() || canvasRect.isEmpty() || !isAvailable()) {
        return {};
    }
    const qreal sx = image.width() / canvasRect.width();
    const qreal sy = image.height() / canvasRect.height();
    const auto screens = viewports.isEmpty() ? QList<QRectF>{canvasRect} : viewports;
    QJsonArray slices;
    for (const auto &screen : screens) {
        const auto viewport = screen.intersected(canvasRect);
        const auto pixels = QRectF((viewport.x() - canvasRect.x()) * sx, (viewport.y() - canvasRect.y()) * sy, viewport.width() * sx, viewport.height() * sy)
                                .toAlignedRect()
                                .intersected(image.rect());
        if (!pixels.isEmpty()) {
            slices.append(QJsonArray{pixels.x(), pixels.y(), pixels.width(), pixels.height()});
        }
    }
    if (slices.isEmpty()) {
        return {};
    }
    QTemporaryDir directory;
    const auto input = directory.filePath(u"regions.png"_s);
    if (!directory.isValid() || !image.save(input)) {
        return {{}, i18n("Could not prepare region detection."), false};
    }
    QProcess process;
    process.start(QStandardPaths::findExecutable(u"python3"_s),
                  {helperPath(),
                   u"--assets"_s,
                   assetsPath(),
                   u"--regions"_s,
                   u"--viewports"_s,
                   QString::fromUtf8(QJsonDocument(slices).toJson(QJsonDocument::Compact)),
                   input});
    if (!process.waitForStarted(5000) || !process.waitForFinished(15000)) {
        process.kill();
        process.waitForFinished();
        return {{}, i18n("Text region detection did not finish."), false};
    }
    const auto output = QJsonDocument::fromJson(process.readAllStandardOutput()).object();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || output.value(u"version"_s).toInt() != 1
        || !output.value(u"success"_s).toBool() || !output.value(u"regions"_s).isArray()) {
        return {{}, output.value(u"error"_s).toString(i18n("Text region detection failed.")), false};
    }
    QList<QRectF> regions;
    const auto detected = output.value(u"regions"_s).toArray();
    for (const auto &value : detected) {
        const auto coordinates = value.toArray();
        if (coordinates.size() != 4) {
            continue;
        }
        bool valid = true;
        for (const auto &coordinate : coordinates) {
            valid &= coordinate.isDouble() && std::isfinite(coordinate.toDouble());
        }
        if (!valid || coordinates[2].toDouble() <= 0 || coordinates[3].toDouble() <= 0) {
            continue;
        }
        QRectF rect(canvasRect.x() + coordinates[0].toDouble() / sx,
                    canvasRect.y() + coordinates[1].toDouble() / sy,
                    coordinates[2].toDouble() / sx,
                    coordinates[3].toDouble() / sy);
        for (const auto &screen : screens) {
            if (screen.contains(rect.center())) {
                rect = rect.intersected(screen).intersected(canvasRect);
                if (!rect.isEmpty()) {
                    regions.append(rect);
                }
                break;
            }
        }
        if (regions.size() >= 2000) {
            break;
        }
    }
    return {regions, {}, true};
}
