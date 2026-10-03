/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "Gui/OcrResultDialog.h"
#include "Gui/PinnedImageWindow.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPointer>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextEdit>

using namespace Qt::StringLiterals;

class GuiWorkflowTest : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.spectacle.PinWindowTest")
    QString mWindowState;

public Q_SLOTS:
    void reportWindowState(const QString &state)
    {
        mWindowState = state;
    }

private:
    static void savePreview(QWidget *widget, const QString &name)
    {
        const auto directory = qEnvironmentVariable("SPECTACLE_TEST_ARTIFACT_DIR");
        if (!directory.isEmpty()) {
            QDir().mkpath(directory);
            QVERIFY(widget->grab().save(directory + u'/' + name + u".png"_s));
        }
    }

private Q_SLOTS:
    void editableOcrResult()
    {
        QPointer<OcrResultDialog> dialog = new OcrResultDialog;
        dialog->setResult(u"中文截图识别\nSpectacle screenshot\n日本語のテスト"_s, u"Text copied to clipboard. Languages: chi_sim + eng + jpn"_s);
        dialog->show();
        QTest::qWait(50);
        auto editor = dialog->findChild<QTextEdit *>();
        QVERIFY(editor);
        QVERIFY(!editor->isReadOnly());
        QCOMPARE(editor->toPlainText().split(u'\n').size(), 3);
        savePreview(dialog, u"ocr-result"_s);
        dialog->close();
        QTRY_VERIFY(dialog.isNull());
    }

    void pinCopiesOriginalAndCloses()
    {
        QImage image(480, 240, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPainter painter(&image);
        QFont font(u"Noto Sans CJK SC"_s);
        font.setPixelSize(26);
        painter.setFont(font);
        painter.drawText(QRect(20, 20, 440, 200), Qt::AlignCenter, u"中文截图 · Hello · 日本語\nPinned reference"_s);
        painter.end();
        QPointer<PinnedImageWindow> pin = new PinnedImageWindow(image, QPoint(30, 30));
        QTest::qWait(50);
        QVERIFY(pin->isVisible());
        QVERIFY(pin->windowFlags().testFlag(Qt::WindowStaysOnTopHint));
        pin->activateWindow();
        QTest::qWait(50);
        QTest::keyClick(pin, Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(QApplication::clipboard()->image(), image);
        savePreview(pin, u"pinned-image"_s);
        QTest::keyClick(pin, Qt::Key_Escape);
        QTRY_VERIFY(pin.isNull());
    }

    void doubleClickClosesPin()
    {
        QImage image(240, 120, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPointer<PinnedImageWindow> pin = new PinnedImageWindow(image, QPoint(100, 100));
        QTest::qWait(50);
        QTest::mouseDClick(pin, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
        QTRY_VERIFY(pin.isNull());
    }

    void ocrRetryKeepsOriginalImage()
    {
        QPointer<OcrResultDialog> dialog = new OcrResultDialog;
        QImage image(200, 100, QImage::Format_RGB32);
        image.fill(Qt::red);
        dialog->setSourceImage(image, {});
        image.fill(Qt::blue); // Simulate a newer screenshot in the viewer.
        dialog->setResult(u"Previous result"_s, u"Ready"_s);
        auto languages = dialog->findChild<QComboBox *>();
        auto retry = dialog->findChild<QPushButton *>(u"ocrRetryButton"_s);
        QVERIFY(languages);
        QVERIFY(retry);
        QSignalSpy request(dialog, &OcrResultDialog::recognitionRequested);
        languages->setCurrentIndex(languages->findData(u"jpn+eng"_s));
        retry->click();
        QCOMPARE(request.count(), 1);
        QCOMPARE(qvariant_cast<QImage>(request.first().at(0)).pixelColor(0, 0), QColor(Qt::red));
        QCOMPARE(request.first().at(1).toString(), u"jpn+eng"_s);
        dialog->setBusy(true);
        QVERIFY(!retry->isEnabled());
        QVERIFY(!languages->isEnabled());
        dialog->close();
        QTRY_VERIFY(dialog.isNull());
    }

    void compositorDragAndClose()
    {
        const auto inputTool = qEnvironmentVariable("SPECTACLE_TEST_WAYLAND_INPUT");
        if (inputTool.isEmpty() || QGuiApplication::platformName() != u"wayland"_s) {
            QSKIP("Run in an isolated KWin session with SPECTACLE_TEST_WAYLAND_INPUT to verify compositor input");
        }
        auto input = [&inputTool](const QStringList &arguments) {
            QProcess process;
            process.start(inputTool, arguments);
            QVERIFY(process.waitForStarted());
            // Process native Wayland events while the separate client moves the pointer.
            QTRY_COMPARE_WITH_TIMEOUT(process.state(), QProcess::NotRunning, 5000);
            QCOMPARE(process.exitCode(), 0);
        };
        QImage image(240, 120, QImage::Format_RGB32);
        image.fill(Qt::white);
        QPointer<PinnedImageWindow> pin = new PinnedImageWindow(image, QPoint(100, 100));
        QTest::qWait(300);
        input({u"drag"_s, u"120"_s, u"120"_s, u"420"_s, u"320"_s});
        QTest::qWait(200);
        // The click is outside the original pin. Closing proves the compositor
        // moved the actual surface, rather than just our cached coordinates.
        input({u"doubleclick"_s, u"440"_s, u"340"_s});
        QTRY_VERIFY(pin.isNull());

        pin = new PinnedImageWindow(image, QPoint(100, 100));
        QTest::qWait(300);
        input({u"click"_s, u"140"_s, u"140"_s});
        input({u"key"_s, u"1"_s}); // Linux KEY_ESC, delivered by the compositor.
        QTRY_VERIFY(pin.isNull());

        QPointer<PinnedImageWindow> oldWindow = new PinnedImageWindow(image, QPoint(100, 100));
        QTest::qWait(300);
        input({u"click"_s, u"140"_s, u"140"_s});
        pin = new PinnedImageWindow(image, QPoint(400, 300));
        oldWindow->close();
        QTest::qWait(300);
        // No activation or click on the new pin: Escape must work immediately
        // after replacing the previously focused capture window.
        input({u"key"_s, u"1"_s});
        QTRY_VERIFY(pin.isNull());
    }

    void compositorScreenAndWindowType()
    {
        const auto inputTool = qEnvironmentVariable("SPECTACLE_TEST_WAYLAND_INPUT");
        if (inputTool.isEmpty() || QGuiApplication::platformName() != u"wayland"_s) {
            QSKIP("Run tests/run-wayland-tests.sh for compositor window and output checks");
        }
        QVERIFY(QGuiApplication::screens().size() >= 2);
        // Match the real setup's unequal fractional scales inside the isolated
        // compositor. The user's output configuration is never involved.
        const auto firstScreen = QGuiApplication::screens().first();
        const auto secondScreen = QGuiApplication::screens().at(1);
        QProcess scale;
        scale.start(u"kscreen-doctor"_s, {u"output.%1.scale.1.5"_s.arg(firstScreen->name()), u"output.%1.scale.1.25"_s.arg(secondScreen->name())});
        QVERIFY(scale.waitForStarted());
        QTRY_COMPARE_WITH_TIMEOUT(scale.state(), QProcess::NotRunning, 10000);
        QCOMPARE(scale.exitCode(), 0);
        QTest::qWait(250);
        auto bus = QDBusConnection::sessionBus();
        QVERIFY(bus.registerService(u"org.kde.spectacle.PinWindowTest"_s));
        QVERIFY(bus.registerObject(u"/probe"_s, this, QDBusConnection::ExportAllSlots));
        QDBusInterface scripting(u"org.kde.KWin"_s, u"/Scripting"_s, u"org.kde.kwin.Scripting"_s);
        if (qEnvironmentVariableIsSet("SPECTACLE_TEST_KAROUSEL")) {
            QDBusReply<bool> loaded = scripting.call(u"isScriptLoaded"_s, u"karousel"_s);
            QVERIFY(loaded.isValid() && loaded.value());
        }
        QTemporaryDir directory;
        QFile script(directory.filePath(u"query.js"_s));
        QVERIFY(script.open(QIODevice::WriteOnly));
        script.write(
            uR"(
const states = workspace.windowList().filter(w => w.pid === %1).map(w => ({
    normal: w.normalWindow, utility: w.utility, output: w.output.name, scale: w.output.devicePixelRatio,
    x: w.frameGeometry.x, y: w.frameGeometry.y,
    width: w.frameGeometry.width, height: w.frameGeometry.height,
    skipTaskbar: w.skipTaskbar, skipPager: w.skipPager
}));
callDBus("org.kde.spectacle.PinWindowTest", "/probe", "org.kde.spectacle.PinWindowTest", "reportWindowState", JSON.stringify(states));
)"_s.arg(QCoreApplication::applicationPid())
                .toUtf8());
        script.close();

        QImage image(450, 270, QImage::Format_RGB32);
        image.setDevicePixelRatio(1.5);
        image.fill(Qt::white);
        // Creating a normal window also exercises Karousel's layout updates.
        QWidget ordinaryWindow;
        ordinaryWindow.show();
        for (auto targetScreen : QGuiApplication::screens()) {
            const auto expected = targetScreen->geometry().topLeft() + QPoint(120, 100);
            QPointer<PinnedImageWindow> pin = new PinnedImageWindow(image, expected, targetScreen);
            QTest::qWait(500);
            mWindowState.clear();
            QDBusReply<int> id = scripting.call(u"loadScript"_s, script.fileName(), u"spectacle-pin-probe"_s);
            QVERIFY(id.isValid() && id.value() >= 0);
            QDBusInterface probe(u"org.kde.KWin"_s, u"/Scripting/Script%1"_s.arg(id.value()), u"org.kde.kwin.Script"_s);
            probe.call(u"run"_s);
            QTRY_VERIFY(!mWindowState.isEmpty());
            scripting.call(u"unloadScript"_s, u"spectacle-pin-probe"_s);
            const auto windows = QJsonDocument::fromJson(mWindowState.toUtf8()).array();
            int tools = 0;
            for (const auto &value : windows) {
                const auto window = value.toObject();
                if (!window.value(u"utility"_s).toBool()) {
                    continue;
                }
                ++tools;
                QVERIFY(!window.value(u"normal"_s).toBool());
                QVERIFY(window.value(u"skipTaskbar"_s).toBool());
                QCOMPARE(window.value(u"output"_s).toString(), targetScreen->name());
                QCOMPARE(window.value(u"scale"_s).toDouble(), targetScreen == firstScreen ? 1.5 : 1.25);
                QCOMPARE(window.value(u"x"_s).toInt(), expected.x());
                QCOMPARE(window.value(u"y"_s).toInt(), expected.y());
                QCOMPARE(window.value(u"width"_s).toInt(), image.deviceIndependentSize().width());
                QCOMPARE(window.value(u"height"_s).toInt(), image.deviceIndependentSize().height());
            }
            QCOMPARE(tools, 1);
            pin->close();
            QTRY_VERIFY(pin.isNull());
        }
        bus.unregisterObject(u"/probe"_s);
        bus.unregisterService(u"org.kde.spectacle.PinWindowTest"_s);
    }
};

QTEST_MAIN(GuiWorkflowTest)
#include "GuiWorkflowTest.moc"
