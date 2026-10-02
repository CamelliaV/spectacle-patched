/*
 * SPDX-License-Identifier: GPL-2.0-only OR LGPL-2.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QRectF>
#include <QTest>

#include "Gui/SelectionRectUtils.h"

class SelectionRectUtilsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void returnsValidSavedSelection();
    void normalizesSavedSelection();
    void clipsSavedSelectionToScreens();
    void rejectsEmptySavedSelection();
    void rejectsSavedSelectionOutsideScreens();
};

void SelectionRectUtilsTest::returnsValidSavedSelection()
{
    QCOMPARE(SelectionRectUtils::restorableSelectionRect(QRectF(10, 20, 30, 40), QRectF(0, 0, 100, 100)), QRectF(10, 20, 30, 40));
}

void SelectionRectUtilsTest::normalizesSavedSelection()
{
    QCOMPARE(SelectionRectUtils::restorableSelectionRect(QRectF(40, 60, -30, -40), QRectF(0, 0, 100, 100)), QRectF(10, 20, 30, 40));
}

void SelectionRectUtilsTest::clipsSavedSelectionToScreens()
{
    QCOMPARE(SelectionRectUtils::restorableSelectionRect(QRectF(80, 70, 40, 50), QRectF(0, 0, 100, 100)), QRectF(80, 70, 20, 30));
}

void SelectionRectUtilsTest::rejectsEmptySavedSelection()
{
    QVERIFY(SelectionRectUtils::restorableSelectionRect(QRectF(10, 20, 0, 40), QRectF(0, 0, 100, 100)).isEmpty());
}

void SelectionRectUtilsTest::rejectsSavedSelectionOutsideScreens()
{
    QVERIFY(SelectionRectUtils::restorableSelectionRect(QRectF(120, 120, 20, 20), QRectF(0, 0, 100, 100)).isEmpty());
}

QTEST_GUILESS_MAIN(SelectionRectUtilsTest)

#include "SelectionRectUtilsTest.moc"
