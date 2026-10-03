/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "Gui/RegionDetector.h"
#include "OcrUtils.h"

#include <QPainter>
#include <QTest>

using namespace Qt::StringLiterals;

class RegionDetectorTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void blankImage()
    {
        QImage image(800, 600, QImage::Format_RGB32);
        image.fill(Qt::white);
        QVERIFY(RegionDetector::detect(image, QRectF(0, 0, 800, 600)).isEmpty());
        QVERIFY(RegionDetector::detect({}, {}).isEmpty());
    }

    void nestedRegionsAndCoordinates()
    {
        QImage image(1600, 1000, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setPen(QPen(Qt::black, 3));
        painter.drawRect(100, 100, 1100, 700);
        painter.drawRect(300, 250, 500, 300);
        painter.end();
        // Negative monitor origin and fractional scale.
        const QRectF canvas(-1000, 50, 1280, 800);
        const auto regions = RegionDetector::detect(image, canvas);
        QVERIFY(regions.size() >= 2);
        const QPointF point(-600, 350);
        const auto candidates = RegionDetector::candidatesAt(regions, point);
        QVERIFY(candidates.size() >= 2);
        qreal previousArea = 0;
        for (const auto &region : candidates) {
            QVERIFY(canvas.contains(region));
            QVERIFY(region.contains(point));
            QVERIFY(region.width() * region.height() >= previousArea);
            previousArea = region.width() * region.height();
        }
        QVERIFY(RegionDetector::candidatesAt(regions, QPointF(1000, 1000)).isEmpty());
    }

    void languageDefaults()
    {
        QCOMPARE(OcrUtils::preferredLanguages({u"osd"_s, u"jpn"_s, u"eng"_s, u"chi_sim"_s}), (QStringList{u"chi_sim"_s, u"eng"_s, u"jpn"_s}));
        QVERIFY(OcrUtils::preferredLanguages({u"osd"_s}).isEmpty());
        QCOMPARE(OcrUtils::configuredLanguages({u"missing"_s, u"jpn"_s, u"jpn"_s}, {u"eng"_s, u"jpn"_s}), QStringList{u"jpn"_s});
    }

    void transparentImage()
    {
        QImage image(100, 50, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        image.setDevicePixelRatio(2);
        image.setPixelColor(50, 25, Qt::black);
        const auto prepared = OcrUtils::prepareImage(image);
        QCOMPARE(prepared.size(), QSize(200, 100));
        QCOMPARE(prepared.pixelColor(0, 0), QColor(Qt::white));
        QVERIFY(prepared.pixelColor(100, 50).lightness() < 180);
    }

    void largePanelSurvivesBusyScreenshot()
    {
        QImage image(1800, 1000, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setPen(QPen(Qt::black, 2));
        for (int row = 0; row < 26; ++row) {
            for (int column = 0; column < 22; ++column) {
                painter.drawRect(5 + column * 54, 5 + row * 37, 46, 29);
            }
        }
        painter.drawRect(1300, 100, 400, 750);
        painter.end();
        const auto regions = RegionDetector::detect(image, QRectF(image.rect()));
        const auto candidates = RegionDetector::candidatesAt(regions, QPointF(1500, 450));
        QVERIFY(!candidates.isEmpty());
        QVERIFY(candidates.first().width() >= 395);
        QVERIFY(candidates.first().height() >= 745);
    }

    void perScreenDetectionKeepsSmallRegions()
    {
        QImage image(4800, 1200, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setPen(QPen(Qt::black, 2));
        painter.drawRect(300, 400, 64, 42);
        painter.drawRect(3300, 400, 64, 42);
        painter.end();
        const auto regions = RegionDetector::detect(image, QRectF(image.rect()), {QRectF(0, 0, 2400, 1200), QRectF(2400, 0, 2400, 1200)});
        QVERIFY(!RegionDetector::candidatesAt(regions, QPointF(3320, 420)).isEmpty());
        QVERIFY(!RegionDetector::candidatesAt(regions, QPointF(320, 420)).isEmpty());
    }

    void lowContrastPanel()
    {
        QImage image(800, 600, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setPen(QPen(QColor(239, 239, 239), 2));
        painter.drawRoundedRect(120, 100, 420, 300, 12, 12);
        painter.end();
        const auto regions = RegionDetector::detect(image, QRectF(image.rect()));
        QVERIFY(!RegionDetector::candidatesAt(regions, QPointF(300, 200)).isEmpty());
    }

    void asyncCandidatesKeepSelection()
    {
        const QRectF selected(100, 100, 400, 200);
        const auto candidates =
            RegionDetector::candidatesAt({selected.adjusted(-1, -1, 1, 1), QRectF(0, 0, 800, 600), QRectF(150, 130, 250, 50)}, QPointF(200, 150), selected);
        QCOMPARE(candidates.size(), 3);
        QVERIFY(candidates.contains(selected));
        QVERIFY(!candidates.contains(selected.adjusted(-1, -1, 1, 1)));
    }
};

QTEST_MAIN(RegionDetectorTest)
#include "RegionDetectorTest.moc"
