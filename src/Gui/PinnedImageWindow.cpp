/* SPDX-License-Identifier: LGPL-2.0-or-later */
#include "PinnedImageWindow.h"

#include <KLocalizedString>
#include <KWindowSystem>
#include <LayerShellQt/Window>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QScreen>
#include <QShortcut>
#include <QTimer>
#include <QWindow>

PinnedImageWindow::PinnedImageWindow(const QImage &image, const QPoint &position, QScreen *targetScreen)
    : QWidget(nullptr, Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    , mImage(image)
    , mTargetScreen(targetScreen ? targetScreen : QGuiApplication::screenAt(position))
    , mPosition(position)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setFocusPolicy(Qt::StrongFocus);
    setWindowTitle(i18nc("@title:window", "Pinned Screenshot"));
    setToolTip(i18n("Drag to move · Scroll to zoom · Double-click or Esc to close · Right-click for actions"));
    new QShortcut(QKeySequence::Close, this, this, &QWidget::close);
    new QShortcut(QKeySequence(Qt::Key_Escape), this, this, &QWidget::close);
    new QShortcut(QKeySequence::Copy, this, this, [this] {
        QApplication::clipboard()->setImage(mImage);
    });

    if (!mTargetScreen) {
        mTargetScreen = QGuiApplication::primaryScreen();
    }
    setScreen(mTargetScreen);
    // Create the QWindow, then configure its shell before it is mapped. A plain
    // WindowStaysOnTopHint is insufficient on Wayland.
    winId();
    if (KWindowSystem::isPlatformWayland()) {
        mLayerWindow = LayerShellQt::Window::get(windowHandle());
        // A layer surface's output is chosen when it is created, not when its
        // QWidget is moved. Bind the screenshot's screen before show().
        mLayerWindow->setScreen(mTargetScreen);
        mLayerWindow->setLayer(LayerShellQt::Window::LayerTop);
        mLayerWindow->setAnchors(LayerShellQt::Window::Anchors::fromInt(LayerShellQt::Window::AnchorTop | LayerShellQt::Window::AnchorLeft));
        mLayerWindow->setExclusiveZone(-1);
        mLayerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityOnDemand);
        mLayerWindow->setActivateOnShow(true);
        mLayerWindow->setCloseOnDismissed(true);
        // KWin maps known scopes to window types. A custom scope becomes Normal
        // and tiling scripts can adopt it; Utility is an interactive floating tool.
        mLayerWindow->setScope(QStringLiteral("utility"));
    }
    const auto available = mTargetScreen->availableGeometry().size();
    const auto logicalSize = mImage.deviceIndependentSize();
    setZoom(qMin(1.0, qMin(available.width() / logicalSize.width(), available.height() / logicalSize.height())));
    movePin(position);
    show();
    // The capture overlay is destroyed after constructing the pin. Request focus
    // once that transition has finished, so Escape reaches the new window.
    QTimer::singleShot(0, this, [this] {
        activateWindow();
        setFocus(Qt::OtherFocusReason);
    });
}

void PinnedImageWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.fillRect(rect(), palette().base());
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(rect(), mImage);
    painter.setPen(palette().highlight().color());
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}

void PinnedImageWindow::movePin(const QPoint &position)
{
    if (!mTargetScreen) {
        close();
        return;
    }
    const auto bounds = mTargetScreen->geometry();
    mPosition = QPoint(qBound(bounds.left(), position.x(), qMax(bounds.left(), bounds.right() - width() + 1)),
                       qBound(bounds.top(), position.y(), qMax(bounds.top(), bounds.bottom() - height() + 1)));
    if (mLayerWindow) {
        const auto offset = mPosition - bounds.topLeft();
        mLayerWindow->setMargins(QMargins(offset.x(), offset.y(), 0, 0));
        // Layer-shell margins are double-buffered. A repaint commits the surface;
        // without one, the compositor keeps displaying the pin at its old position.
        update();
    } else {
        move(mPosition);
    }
}

void PinnedImageWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        activateWindow();
        setFocus(Qt::MouseFocusReason);
        mDragging = true;
        mDragOffset = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
    }
}

void PinnedImageWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        mDragging = false;
        event->accept();
        close();
    } else {
        QWidget::mouseDoubleClickEvent(event);
    }
}

void PinnedImageWindow::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        event->accept();
        close();
    } else {
        QWidget::keyPressEvent(event);
    }
}

void PinnedImageWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (mDragging) {
        // Relative motion also works on Wayland, where global positions can be
        // synthetic. Margins move layer surfaces without asking for a WM move.
        movePin(mPosition + event->position().toPoint() - mDragOffset);
    }
}

void PinnedImageWindow::mouseReleaseEvent(QMouseEvent *)
{
    mDragging = false;
    unsetCursor();
}

void PinnedImageWindow::setZoom(qreal zoom)
{
    mZoom = qBound(0.1, zoom, 4.0);
    const auto size = (mImage.deviceIndependentSize() * mZoom).toSize().expandedTo(QSize(32, 32));
    resize(size);
    if (mLayerWindow) {
        mLayerWindow->setDesiredSize(size);
    }
    movePin(mPosition);
    update();
}

void PinnedImageWindow::wheelEvent(QWheelEvent *event)
{
    setZoom(mZoom * (event->angleDelta().y() > 0 ? 1.1 : 1 / 1.1));
    event->accept();
}

void PinnedImageWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(i18nc("@action", "Copy Image"), this, [this] {
        QApplication::clipboard()->setImage(mImage);
    });
    menu.addAction(i18nc("@action", "Actual Size"), this, [this] {
        setZoom(1);
    });
    menu.addSeparator();
    menu.addAction(i18nc("@action", "Close"), this, &QWidget::close);
    menu.exec(event->globalPos());
}

#include "moc_PinnedImageWindow.cpp"
