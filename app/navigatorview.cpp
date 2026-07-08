// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#include "navigatorview.h"

#include "graphicsview.h"

#include <QStyleHints>
#include <QOperatingSystemVersion>
#include <QMouseEvent>
#include <QDebug>
#include <QTimer>

NavigatorView::NavigatorView(QWidget *parent)
    : QGraphicsView (parent)
    , m_viewportRegion(this->rect())
{
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    updateStyle(qGuiApp->styleHints()->colorScheme());
    connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, this, &NavigatorView::updateStyle);
}

// doesn't take or manage its ownership
void NavigatorView::setMainView(GraphicsView *mainView)
{
    m_mainView = mainView;
}

void NavigatorView::updateMainViewportRegion()
{
    // Use QTimer::singleShot with lambda to delay the update
    // This ensures all geometry updates are complete before calculating viewport region
    QTimer::singleShot(0, [this]() {
        if (m_mainView != nullptr) {
            m_viewportRegion = mapFromScene(m_mainView->mapToScene(m_mainView->rect()));
            update();
        }
    });
}

void NavigatorView::updateStyle(Qt::ColorScheme colorScheme) {
    // YYC MARK:
    // If we are using Windows 10 and lower system or Qt which do not support "Windows 11"
    // theme (lower than Qt 6.7), we should only support light mode.
    // Because Qt's dark-mode-aware "Windows 11" theme is only well-supported on Windows 11.
#if defined(Q_OS_WIN)
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    colorScheme = Qt::ColorScheme::Light;
#else
    if (QOperatingSystemVersion::current() < QOperatingSystemVersion::Windows11) {
        colorScheme = Qt::ColorScheme::Light;
    }
#endif // QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
#endif // defined(Q_OS_WIN)

    switch (colorScheme) {
    case Qt::ColorScheme::Dark:
    setStyleSheet(R"#(
border-style: none;
background-color: rgba(255, 255, 255, 70);
)#");
        break;
    case Qt::ColorScheme::Light:
    case Qt::ColorScheme::Unknown:
    default:
        setStyleSheet(R"#(
border-style: none;
background-color: rgba(0, 0, 0, 70);
)#");
        break;
    }
}

void NavigatorView::mousePressEvent(QMouseEvent *event)
{
    m_mouseDown = true;

    if (m_mainView) {
        m_mainView->centerOn(mapToScene(event->pos()));
        update();
    }

    event->accept();
}

void NavigatorView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_mouseDown && m_mainView) {
        m_mainView->centerOn(mapToScene(event->pos()));
        update();
        event->accept();
    } else {
        event->ignore();
    }
}

void NavigatorView::mouseReleaseEvent(QMouseEvent *event)
{
    m_mouseDown = false;

    event->accept();
}

void NavigatorView::wheelEvent(QWheelEvent *event)
{
    event->ignore();
    return QGraphicsView::wheelEvent(event);
}

void NavigatorView::paintEvent(QPaintEvent *event)
{
    QGraphicsView::paintEvent(event);

    QPainter painter(viewport());
    painter.setPen(QPen(Qt::gray, 2));
    painter.drawRect(m_viewportRegion.boundingRect());
}
