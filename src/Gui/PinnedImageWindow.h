/* SPDX-License-Identifier: LGPL-2.0-or-later */
#pragma once

#include <QEventLoopLocker>
#include <QImage>
#include <QPointer>
#include <QWidget>

namespace LayerShellQt
{
class Window;
}

class PinnedImageWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PinnedImageWindow(const QImage &image, const QPoint &position, QScreen *targetScreen = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setZoom(qreal zoom);
    void movePin(const QPoint &position);
    QImage mImage;
    QEventLoopLocker mLoopLocker;
    LayerShellQt::Window *mLayerWindow = nullptr;
    QPointer<QScreen> mTargetScreen;
    QPoint mPosition;
    QPoint mDragOffset;
    bool mDragging = false;
    qreal mZoom = 1;
};
