/* SPDX-License-Identifier: LGPL-2.0-or-later */
#pragma once

#include <QImage>
#include <QStringList>

namespace OcrUtils
{
QString findTessdataPath();
QStringList availableLanguages(const QString &path);
QStringList preferredLanguages(const QStringList &available);
QStringList configuredLanguages(const QStringList &configured, const QStringList &available);
QImage prepareImage(const QImage &image);
}
