/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "OcrManager.h"
#include "OcrUtils.h"
#include "settings.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QPainter>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class OcrTest : public QObject
{
    Q_OBJECT
    QString mDataPath;
    QByteArray mOldPrefix;

    static QImage textImage(const QString &text)
    {
        QImage image(1000, 130, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        QFont font(u"Noto Sans CJK SC"_s);
        font.setPixelSize(38);
        painter.setFont(font);
        painter.setPen(Qt::black);
        painter.drawText(QRect(25, 20, 950, 90), Qt::AlignVCenter, text);
        return image;
    }

private Q_SLOTS:
    void initTestCase()
    {
        qputenv("SPECTACLE_OCR_HELPER", "/nonexistent/spectacle-test-helper");
        mDataPath = OcrUtils::findTessdataPath();
        mOldPrefix = qgetenv("TESSDATA_PREFIX");
        QStandardPaths::setTestModeEnabled(true);
        if (mDataPath.isEmpty() || !TesseractRuntimeLoader::instance().ensureLoaded()) {
            QSKIP("Tesseract runtime and language data are required for recognition integration tests");
        }
    }

    void init()
    {
        qputenv("TESSDATA_PREFIX", QFile::encodeName(mDataPath));
        Settings::setOcrLanguages({u"eng"_s});
    }

    void cleanupTestCase()
    {
        if (mOldPrefix.isNull()) {
            qunsetenv("TESSDATA_PREFIX");
        } else {
            qputenv("TESSDATA_PREFIX", mOldPrefix);
        }
    }

    void initializationNotifies()
    {
        OcrManager manager;
        QSignalSpy status(&manager, &OcrManager::statusChanged);
        QCOMPARE(manager.status(), OcrManager::OcrStatus::Initializing);
        QTRY_VERIFY(manager.isAvailable());
        QCOMPARE(manager.status(), OcrManager::OcrStatus::Ready);
        QCOMPARE(status.count(), 1);
    }

    void missingLanguageData()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        qputenv("TESSDATA_PREFIX", QFile::encodeName(dir.path()));
        OcrManager manager;
        QTRY_COMPARE(manager.status(), OcrManager::OcrStatus::Error);
        QVERIFY(!manager.isAvailable());
        QVERIFY(!manager.errorMessage().isEmpty());
    }

    void initializesWithoutEnglish()
    {
        const auto japanese = mDataPath + u"/jpn.traineddata"_s;
        if (!QFile::exists(japanese)) {
            QSKIP("Japanese language data is not installed");
        }
        QTemporaryDir dir;
        QVERIFY(QFile::link(japanese, dir.filePath(u"jpn.traineddata"_s)));
        qputenv("TESSDATA_PREFIX", QFile::encodeName(dir.path()));
        OcrManager manager;
        QTRY_VERIFY(manager.isAvailable());
        QCOMPARE(manager.currentLanguageCode(), u"jpn"_s);
    }

    void brokenLanguagePreservesWorkingEngine()
    {
        QTemporaryDir dir;
        QVERIFY(QFile::link(mDataPath + u"/eng.traineddata"_s, dir.filePath(u"eng.traineddata"_s)));
        QFile broken(dir.filePath(u"jpn.traineddata"_s));
        QVERIFY(broken.open(QIODevice::WriteOnly));
        broken.write("invalid model");
        broken.close();
        qputenv("TESSDATA_PREFIX", QFile::encodeName(dir.path()));
        OcrManager manager;
        QTRY_VERIFY(manager.isAvailable());
        QSignalSpy result(&manager, &OcrManager::textRecognized);
        manager.recognizeTextWithLanguage(textImage(u"Hello"_s), u"eng+jpn"_s);
        QCOMPARE(result.count(), 1);
        QVERIFY(!result.first().at(2).toBool());
        QCOMPARE(manager.currentLanguageCode(), u"eng"_s);
        QVERIFY(manager.isAvailable());
    }

    void recognition_data()
    {
        QTest::addColumn<QString>("languages");
        QTest::addColumn<QString>("input");
        QTest::addColumn<QStringList>("expected");
        QTest::newRow("english") << u"eng"_s << u"Spectacle screenshot 12345"_s << QStringList{u"Spectacle"_s, u"12345"_s};
        QTest::newRow("chinese") << u"chi_sim+eng"_s << u"中文截图识别 Hello 12345"_s << QStringList{u"中文"_s, u"截图"_s, u"Hello"_s};
        QTest::newRow("japanese") << u"jpn+eng"_s << u"日本語のテスト Hello 12345"_s << QStringList{u"日本語"_s, u"テスト"_s, u"Hello"_s};
        QTest::newRow("mixed") << u"chi_sim+eng+jpn"_s << u"中文截图 Hello 日本語テスト"_s << QStringList{u"中文"_s, u"Hello"_s, u"テスト"_s};
    }

    void recognition()
    {
        QFETCH(QString, languages);
        QFETCH(QString, input);
        QFETCH(QStringList, expected);
        const auto available = OcrUtils::availableLanguages(mDataPath);
        for (const auto &code : languages.split(u'+')) {
            if (!available.contains(code)) {
                QSKIP("Language data for this case is not installed");
            }
        }
        OcrManager manager;
        QTRY_VERIFY(manager.isAvailable());
        QSignalSpy result(&manager, &OcrManager::textRecognized);
        manager.recognizeTextWithLanguage(textImage(input), languages);
        QCOMPARE(manager.status(), OcrManager::OcrStatus::Processing);
        QTRY_COMPARE_WITH_TIMEOUT(result.count(), 1, 35000);
        QVERIFY(result.first().at(2).toBool());
        auto text = result.first().at(0).toString();
        for (const auto &word : expected) {
            QVERIFY2(text.remove(u' ').contains(word), qPrintable(text));
        }
        QCOMPARE(QApplication::clipboard()->text(), result.first().at(0).toString());
        QCOMPARE(manager.currentLanguageCode(), u"eng"_s);
        QCOMPARE(Settings::ocrLanguages(), QStringList{u"eng"_s});
    }

    void blankImagePreservesClipboard()
    {
        OcrManager manager;
        QTRY_VERIFY(manager.isAvailable());
        QSignalSpy result(&manager, &OcrManager::textRecognized);
        QApplication::clipboard()->setText(u"keep me"_s);
        QImage blank(300, 200, QImage::Format_RGB32);
        blank.fill(Qt::white);
        manager.recognizeText(blank);
        QTRY_COMPARE_WITH_TIMEOUT(result.count(), 1, 35000);
        QVERIFY(result.first().at(0).toString().isEmpty());
        QCOMPARE(QApplication::clipboard()->text(), u"keep me"_s);
    }

    void languageChangeWaitsForRecognition()
    {
        if (!OcrUtils::availableLanguages(mDataPath).contains(u"jpn"_s)) {
            QSKIP("Japanese language data is not installed");
        }
        OcrManager manager;
        QTRY_VERIFY(manager.isAvailable());
        QSignalSpy result(&manager, &OcrManager::textRecognized);
        manager.recognizeText(textImage(u"Spectacle screenshot 12345"_s));
        manager.setLanguagesByCode({u"jpn"_s});
        QCOMPARE(manager.currentLanguageCode(), u"eng"_s);
        // A duplicate request must not emit a false completion for the active job.
        manager.recognizeText(textImage(u"Second request"_s));
        QCOMPARE(result.count(), 0);
        QTRY_COMPARE_WITH_TIMEOUT(result.count(), 1, 35000);
        QCOMPARE(result.first().at(1).toStringList(), QStringList{u"eng"_s});
        QCOMPARE(manager.currentLanguageCode(), u"jpn"_s);
        QCOMPARE(Settings::ocrLanguages(), QStringList{u"jpn"_s});
    }

    void missingRequestedLanguageIsReported()
    {
        OcrManager manager;
        QTRY_VERIFY(manager.isAvailable());
        QSignalSpy result(&manager, &OcrManager::textRecognized);
        manager.recognizeTextWithLanguage(textImage(u"Hello"_s), u"eng+missing_language"_s);
        QCOMPARE(result.count(), 1);
        QVERIFY(!result.first().at(2).toBool());
        QVERIFY(manager.errorMessage().contains(u"missing_language"_s));
        QCOMPARE(manager.currentLanguageCode(), u"eng"_s);
    }
};

QTEST_MAIN(OcrTest)
#include "OcrTest.moc"
