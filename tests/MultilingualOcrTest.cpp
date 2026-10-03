/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "MultilingualOcr.h"
#include "OcrManager.h"

#include <QDir>
#include <QPainter>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <algorithm>
#include <numeric>

using namespace Qt::StringLiterals;

class MultilingualOcrTest : public QObject
{
    Q_OBJECT

    static QString withoutWhitespace(QString text)
    {
        text.removeIf([](QChar character) {
            return character.isSpace();
        });
        return text;
    }

    static int editDistance(const QString &expected, const QString &actual)
    {
        QList<int> row(actual.size() + 1);
        std::iota(row.begin(), row.end(), 0);
        for (int i = 1; i <= expected.size(); ++i) {
            int diagonal = row[0];
            row[0] = i;
            for (int j = 1; j <= actual.size(); ++j) {
                const int previous = row[j];
                row[j] = qMin(qMin(row[j] + 1, row[j - 1] + 1), diagonal + (expected[i - 1] != actual[j - 1]));
                diagonal = previous;
            }
        }
        return row.last();
    }

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        qputenv("SPECTACLE_OCR_HELPER", SPECTACLE_TEST_OCR_HELPER);
        if (!qEnvironmentVariableIsSet("SPECTACLE_OCR_ASSETS")) {
            qputenv("SPECTACLE_OCR_ASSETS", "/usr/share/spectacle/ocr");
        }
        if (!MultilingualOcr::isAvailable()) {
            QSKIP("Install the packaged multilingual models or set SPECTACLE_OCR_ASSETS");
        }
    }

    void mixedScreenshot_data()
    {
        QTest::addColumn<int>("fontSize");
        QTest::addColumn<bool>("dark");
        QTest::newRow("mixed-paragraphs") << 22 << false;
        QTest::newRow("small-ui-text") << 16 << false;
        QTest::newRow("dark-ui-text") << 18 << true;
    }

    void mixedScreenshot()
    {
        QFETCH(int, fontSize);
        QFETCH(bool, dark);
        const QStringList lines{
            u"截图文字识别 Hello world 日本語のテスト"_s,
            u"保存截图到剪贴板 Screenshot copied to clipboard"_s,
            u"日本語の文章を正しく読み取ります。画像から文字を認識します。"_s,
            u"请点击确定按钮，然后选择文件。ファイルを選択してください。"_s,
            u"繁體中文測試：軟體開發與螢幕擷取 Screenshot 12345"_s,
        };
        QImage image(1250, 320, QImage::Format_RGB32);
        image.fill(dark ? QColor(30, 30, 30) : Qt::white);
        QPainter painter(&image);
        QFont font(u"Noto Sans CJK SC"_s);
        font.setPixelSize(fontSize);
        painter.setFont(font);
        painter.setPen(dark ? Qt::white : Qt::black);
        for (int i = 0; i < lines.size(); ++i) {
            painter.drawText(QPoint(20, 40 + i * 56), lines[i]);
        }
        painter.end();
        if (const auto directory = qEnvironmentVariable("SPECTACLE_TEST_ARTIFACT_DIR"); !directory.isEmpty()) {
            QDir().mkpath(directory);
            image.save(directory + u"/mixed-%1-%2.png"_s.arg(fontSize).arg(dark));
        }
        OcrManager manager;
        QTRY_COMPARE(manager.status(), OcrManager::OcrStatus::Ready);
        QSignalSpy results(&manager, &OcrManager::textRecognized);
        manager.recognizeText(image);
        QCOMPARE(manager.engineName(), u"PP-OCRv5"_s);
        QTRY_COMPARE_WITH_TIMEOUT(results.count(), 1, 65000);
        QVERIFY2(results.first().at(2).toBool(), qPrintable(manager.errorMessage()));
        const auto text = results.first().at(0).toString();
        qInfo().noquote() << text;
        const auto expected = withoutWhitespace(lines.join(u'\n'));
        const auto actual = withoutWhitespace(text);
        const auto distance = editDistance(expected, actual);
        QVERIFY2(distance <= expected.size() * 0.04, qPrintable(u"%1 wrong characters: %2"_s.arg(distance).arg(text)));
        QVERIFY(actual.contains(u"Helloworld"_s));
        QVERIFY(text.contains(u"日本語"_s));
        QVERIFY(text.contains(u"ファイル"_s));
        QVERIFY(text.contains(u"截图"_s));
        QVERIFY(text.contains(u"繁體中文"_s));
    }

    void englishScreenshot_data()
    {
        QTest::addColumn<int>("fontSize");
        QTest::addColumn<bool>("dark");
        QTest::newRow("english-small") << 14 << false;
        QTest::newRow("english-normal") << 18 << false;
        QTest::newRow("english-dark") << 18 << true;
    }

    void englishScreenshot()
    {
        QFETCH(int, fontSize);
        QFETCH(bool, dark);
        const QStringList lines{
            u"The quick brown fox jumps over the lazy dog."_s,
            u"Don't merge these words: signal handler, data source, and file name."_s,
            u"Please select a file, then click Save As to continue."_s,
            u"Error: Connection timed out after 30 seconds (retry #2)."_s,
            u"Version 6.7.5 / Ctrl+Shift+P / screenshot_2026-10-03.png"_s,
            u"https://example.com/docs/getting-started?lang=en"_s,
            u"Mixed CASE: OpenAI, KDE Plasma, OCR, Qt6 and Linux."_s,
        };
        QImage image(1150, 360, QImage::Format_RGB32);
        image.fill(dark ? QColor(30, 30, 30) : Qt::white);
        QPainter painter(&image);
        QFont font(u"Noto Sans"_s);
        font.setPixelSize(fontSize);
        painter.setFont(font);
        painter.setPen(dark ? Qt::white : Qt::black);
        for (int i = 0; i < lines.size(); ++i) {
            painter.drawText(QPoint(20, 35 + i * 46), lines[i]);
        }
        painter.end();
        const auto result = MultilingualOcr::recognize(image);
        QVERIFY2(result.success, qPrintable(result.error));
        // Unlike the mixed-script character check, count spaces and punctuation.
        // Joining English words together is a user-visible recognition error.
        const auto expected = lines.join(u'\n');
        const auto distance = editDistance(expected, result.text);
        QVERIFY2(distance <= expected.size() * 0.01, qPrintable(u"%1 errors: %2"_s.arg(distance).arg(result.text)));
        QVERIFY(result.text.contains(u"signal handler, data source, and file name"_s));
        QVERIFY(result.text.contains(u"Connection timed out after 30 seconds"_s));
        QVERIFY(result.text.contains(u"KDE Plasma"_s));
    }

    void commandScreenshotKeepsLines()
    {
        QImage image(1400, 250, QImage::Format_RGB32);
        image.fill(QColor(28, 30, 40));
        QPainter painter(&image);
        QFont font(u"Noto Sans Mono"_s);
        font.setPixelSize(18);
        painter.setFont(font);
        const QStringList command{u"qdbus6"_s, u"org.kde.spectacle"_s, u"/MainApplication"_s, u"org.qtproject.Qt.QCoreApplication.quit"_s};
        int x = 60;
        // Syntax colors and larger gaps encourage the detector to produce
        // separate boxes, as in the reported command screenshot.
        for (int i = 0; i < command.size(); ++i) {
            painter.setPen(i % 2 ? QColor(230, 230, 230) : QColor(120, 190, 240));
            painter.drawText(QPoint(x, 45), command[i]);
            x += painter.fontMetrics().horizontalAdvance(command[i]) + 28;
        }
        painter.setPen(Qt::white);
        painter.drawText(QPoint(20, 95), u"Restarted Spectacle"_s);
        painter.drawText(QPoint(20, 145), u"Show details"_s);
        painter.end();
        const auto result = MultilingualOcr::recognize(image);
        QVERIFY2(result.success, qPrintable(result.error));
        const auto lines = result.text.split(u'\n');
        QCOMPARE(lines.size(), 3);
        QVERIFY2(lines[0].startsWith(u"    "_s), qPrintable(result.text));
        QCOMPARE(lines[0].simplified(), command.join(u' '));
        QCOMPARE(lines[1].trimmed(), u"Restarted Spectacle"_s);
        QCOMPARE(lines[2].trimmed(), u"Show details"_s);
    }

    void detectsBorderlessContentOnBothScreens()
    {
        QImage image(1600, 420, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        QFont font(u"Noto Sans"_s);
        font.setPixelSize(22);
        painter.setFont(font);
        painter.setPen(Qt::black);
        for (const int left : {40, 840}) {
            painter.drawText(QPoint(left, 80), u"A borderless paragraph of text"_s);
            painter.drawText(QPoint(left, 116), u"with several lines to capture."_s);
            painter.drawText(QPoint(left, 152), u"Select the whole content block."_s);
        }
        painter.end();
        const QRectF canvas(-800, 50, 1280, 336);
        const QList<QRectF> screens{QRectF(-800, 50, 640, 336), QRectF(-160, 50, 640, 336)};
        const auto result = MultilingualOcr::detectTextRegions(image, canvas, screens);
        QVERIFY2(result.success, qPrintable(result.error));
        for (const int left : {40, 840}) {
            const QPointF betweenLines(canvas.x() + (left + 80) * 0.8, canvas.y() + 85 * 0.8);
            const QPointF lastLine(canvas.x() + (left + 80) * 0.8, canvas.y() + 144 * 0.8);
            QVERIFY(std::any_of(result.regions.cbegin(), result.regions.cend(), [&](const QRectF &rect) {
                return rect.contains(betweenLines) && rect.contains(lastLine);
            }));
        }
        for (const auto &region : result.regions) {
            QVERIFY(screens[0].contains(region) || screens[1].contains(region));
        }
    }
};

QTEST_MAIN(MultilingualOcrTest)
#include "MultilingualOcrTest.moc"
