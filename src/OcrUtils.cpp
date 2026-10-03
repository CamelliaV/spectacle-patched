/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "OcrUtils.h"

#include <QDir>
#include <QPainter>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

QStringList OcrUtils::availableLanguages(const QString &path)
{
    QStringList result;
    const auto files = QDir(path).entryList({u"*.traineddata"_s}, QDir::Files | QDir::Readable, QDir::Name);
    for (const auto &file : files) {
        const auto code = file.chopped(12);
        if (code != u"osd"_s) {
            result.append(code);
        }
    }
    return result;
}

QString OcrUtils::findTessdataPath()
{
    const auto override = qEnvironmentVariable("TESSDATA_PREFIX");
    if (!override.isEmpty()) {
        return override;
    }
    QStringList paths = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, u"spectacle/tessdata"_s, QStandardPaths::LocateDirectory);
    paths << u"/usr/share/tessdata"_s << u"/usr/local/share/tessdata"_s;
    for (const auto &version : {u"5"_s, u"4.00"_s}) {
        paths << u"/usr/share/tesseract-ocr/%1/tessdata"_s.arg(version);
    }
    for (const auto &path : paths) {
        if (!availableLanguages(path).isEmpty()) {
            return path;
        }
    }
    return {};
}

QStringList OcrUtils::preferredLanguages(const QStringList &available)
{
    QStringList result;
    for (const auto &code : {u"chi_sim"_s, u"eng"_s, u"jpn"_s, u"chi_tra"_s}) {
        if (available.contains(code)) {
            result.append(code);
        }
    }
    if (result.isEmpty()) {
        for (const auto &code : available) {
            if (code != u"osd"_s) {
                result.append(code);
                break;
            }
        }
    }
    return result;
}

QStringList OcrUtils::configuredLanguages(const QStringList &configured, const QStringList &available)
{
    QStringList result;
    for (const auto &code : configured) {
        if (code != u"osd"_s && available.contains(code) && !result.contains(code)) {
            result.append(code);
        }
        if (result.size() == 4) {
            break;
        }
    }
    return result.isEmpty() ? preferredLanguages(available) : result;
}

QImage OcrUtils::prepareImage(const QImage &image)
{
    if (image.isNull()) {
        return {};
    }
    // Composite alpha against white and enlarge small UI text. Bound the extra
    // allocation on large desktop captures; keep the original aspect ratio.
    QImage rgb(image.size(), QImage::Format_RGB888);
    rgb.fill(Qt::white);
    QPainter painter(&rgb);
    QImage source = image;
    source.setDevicePixelRatio(1);
    painter.drawImage(0, 0, source);
    painter.end();
    if (qMax(rgb.width(), rgb.height()) <= 2000) {
        rgb = rgb.scaled(rgb.size() * 2, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return rgb;
}
