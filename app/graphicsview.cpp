// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "graphicsview.h"

#include "graphicsscene.h"
#include "settings.h"

#include <QStyleHints>
#include <QOperatingSystemVersion>
#include <QDebug>
#include <QColorSpace>
#include <QMouseEvent>
#include <QScrollBar>
#include <QMimeData>
#include <QImageReader>
#include <QStyleOptionGraphicsItem>

GraphicsView::GraphicsView(QWidget *parent)
    : QGraphicsView (parent)
{
    setDragMode(QGraphicsView::ScrollHandDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setAcceptDrops(false);
    setCheckerboardEnabled(false);

    updateStyle(qGuiApp->styleHints()->colorScheme());
    connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, this, &GraphicsView::updateStyle);

    connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, &GraphicsView::viewportRectChanged);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, &GraphicsView::viewportRectChanged);
}

void GraphicsView::showFileFromPath(const QString &filePath)
{
    emit navigatorViewRequired(false, transform());

    if (filePath.endsWith(".svg")) {
        showSvg(filePath);
    } else {
        QImageReader imageReader(filePath);
        imageReader.setAutoTransform(true);
        imageReader.setDecideFormatFromContent(true);
        imageReader.setAllocationLimit(0);

        // Since if the image format / plugin does not support this feature, imageFormat() will returns an invalid format.
        // So we cannot use imageFormat() and check if it returns QImage::Format_Invalid to detect if we support the file.
        // QImage::Format imageFormat = imageReader.imageFormat();
        if (imageReader.format().isEmpty()) {
            showText(tr("File is not a valid image"));
        } else if (imageReader.supportsAnimation() && imageReader.imageCount() > 1) {
            showAnimated(filePath);
        } else if (!imageReader.canRead()) {
            showText(tr("Image data is invalid or currently unsupported"));
        } else {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            QImage && img = imageReader.read();
            if (img.isNull()) {
                showText(tr("Image data is invalid or currently unsupported"));
            } else {
                if (img.format() == QImage::Format_CMYK8888) {
                    img.convertToColorSpace(QColorSpace::SRgb);
                }
                img.setDevicePixelRatio(devicePixelRatioF());
                showImage(img);
            }
#else
            QPixmap && pixmap = QPixmap::fromImageReader(&imageReader);
            if (pixmap.isNull()) {
                showText(tr("Image data is invalid or currently unsupported"));
            } else {
                pixmap.setDevicePixelRatio(devicePixelRatioF());
                showImage(pixmap);
            }
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        }
    }
}

void GraphicsView::showImage(const QPixmap &pixmap)
{
    resetTransform();
    scene()->showImage(pixmap);
    displayScene();
}

void GraphicsView::showImage(const QImage &image)
{
    resetTransform();
    scene()->showImage(QPixmap::fromImage(image));
    displayScene();
}

void GraphicsView::showText(const QString &text)
{
    resetTransform();
    scene()->showText(text);
    displayScene();
}

void GraphicsView::showSvg(const QString &filepath)
{
    resetTransform();
    scene()->showSvg(filepath);
    displayScene();
}

void GraphicsView::showAnimated(const QString &filepath)
{
    resetTransform();
    scene()->showAnimated(filepath);
    displayScene();
}

GraphicsScene *GraphicsView::scene() const
{
    return qobject_cast<GraphicsScene*>(QGraphicsView::scene());
}

void GraphicsView::setScene(GraphicsScene *scene)
{
    return QGraphicsView::setScene(scene);
}

qreal GraphicsView::scaleFactor() const
{
    return QStyleOptionGraphicsItem::levelOfDetailFromTransform(transform());
}

void GraphicsView::resetTransform()
{
    if (!shouldAvoidTransform()) {
        QGraphicsView::resetTransform();
    }
}

void GraphicsView::zoomView(qreal scaleFactor)
{
    m_enableFitToScreen = false;
    scale(scaleFactor, scaleFactor);
    applyTransformationModeByScaleFactor();
    emit navigatorViewRequired(!isThingSmallerThanWindowWith(transform()), transform());
}

// This is always according to user's view.
// the direction of the rotation will NOT be clockwise because the y-axis points downwards.
void GraphicsView::rotateView(bool clockwise)
{
    resetScale();

    QTransform tf(0,                  clockwise ? 1 : -1, 0,
                  clockwise ? -1 : 1, 0,                  0,
                  0,                  0,                  1);
    tf = transform() * tf;
    setTransform(tf);
    
    // Apply transformation mode but don't emit navigator signal here
    // Let displayScene() handle the navigator visibility correctly
    applyTransformationModeByScaleFactor();
}

void GraphicsView::flipView(bool horizontal)
{
    QTransform tf(horizontal ? -1 : 1, 0,                   0,
                  0,                   horizontal ? 1 : -1, 0,
                  0,                   0,                   1);
    tf = transform() * tf;
    setTransform(tf);

    // Ensure the navigation view is also flipped.
    emit navigatorViewRequired(!isThingSmallerThanWindowWith(transform()), transform());
}

void GraphicsView::resetScale()
{
    setTransform(resetScale(transform()));
    applyTransformationModeByScaleFactor();
    emit navigatorViewRequired(!isThingSmallerThanWindowWith(transform()), transform());
}

void GraphicsView::fitInView(const QRectF &rect, Qt::AspectRatioMode aspectRadioMode)
{
    QGraphicsView::fitInView(rect, aspectRadioMode);
    applyTransformationModeByScaleFactor();
}

void GraphicsView::fitByOrientation(Qt::Orientation ori, bool scaleDownOnly)
{
    resetScale();

    QRectF viewRect = this->viewport()->rect();
    QRectF imageRect = transform().mapRect(sceneRect());
    QSize viewSize = viewRect.size().toSize();

    qreal ratio;

    if (ori == Qt::Horizontal) {
        // Horizontal fit means fit by width
        if (scaleDownOnly && imageRect.width() <= viewSize.width()) {
            // Image width already fits, no scaling needed
            ratio = 1;
        } else {
            ratio = viewRect.width() / imageRect.width();
        }
    } else {
        // Vertical fit means fit by height
        if (scaleDownOnly && imageRect.height() <= viewSize.height()) {
            // Image height already fits, no scaling needed
            ratio = 1;
        } else {
            ratio = viewRect.height() / imageRect.height();
        }
    }

    if (ratio != 1) {
        scale(ratio, ratio);
    }

    // Position the image correctly based on orientation with rotation consideration
    QRectF originalScene = sceneRect();
    QTransform currentTransform = transform();
    
    // TODO: May be optimized by matrix arithmetic operation?

    if (ori == Qt::Horizontal) {
        // For horizontal fit (fit by width), position at top (for tall images)
        // Find the scene point that corresponds to the top-center of the transformed image
        QPointF sceneTopCenter;

        if (qFuzzyIsNull(currentTransform.m12()) && qFuzzyIsNull(currentTransform.m21())) {
            // 0° or 180° rotation
            if (currentTransform.m11() > 0 && currentTransform.m22() > 0) {
                // 0° rotation: use original top-center
                sceneTopCenter = QPointF(originalScene.center().x(), originalScene.top());
            } else {
                // 180° rotation: the visual "top" is now at the scene bottom
                sceneTopCenter = QPointF(originalScene.center().x(), originalScene.bottom());
            }
        } else {
            // 90/270 degree rotation: the "top" in view corresponds to left/right in scene
            if (currentTransform.m12() > 0) {
                // 90 degree: top in view = left in scene
                sceneTopCenter = QPointF(originalScene.left(), originalScene.center().y());
            } else {
                // 270 degree: top in view = right in scene
                sceneTopCenter = QPointF(originalScene.right(), originalScene.center().y());
            }
        }
        centerOn(sceneTopCenter);
    } else {
        // For vertical fit (fit by height), position at left (for wide images)
        // Find the scene point that corresponds to the left-center of the transformed image
        QPointF sceneLeftCenter;

        if (qFuzzyIsNull(currentTransform.m12()) && qFuzzyIsNull(currentTransform.m21())) {
            // 0° or 180° rotation
            if (currentTransform.m11() > 0 && currentTransform.m22() > 0) {
                // 0° rotation: use original left-center
                sceneLeftCenter = QPointF(originalScene.left(), originalScene.center().y());
            } else {
                // 180° rotation: the visual "left" is now at the scene right
                sceneLeftCenter = QPointF(originalScene.right(), originalScene.center().y());
            }
        } else {
            // 90/270 degree rotation: the "left" in view corresponds to top/bottom in scene
            if (currentTransform.m21() > 0) {
                // 90 degree: left in view = top in scene
                sceneLeftCenter = QPointF(originalScene.center().x(), originalScene.top());
            } else {
                // 270 degree: left in view = bottom in scene
                sceneLeftCenter = QPointF(originalScene.center().x(), originalScene.bottom());
            }
        }
        centerOn(sceneLeftCenter);
    }

    m_enableFitToScreen = false;

    applyTransformationModeByScaleFactor();
    emit navigatorViewRequired(!isThingSmallerThanWindowWith(transform()), transform());
}

void GraphicsView::displayScene()
{
    if (shouldAvoidTransform()) {
        emit navigatorViewRequired(!isThingSmallerThanWindowWith(transform()), transform());
        return;
    }

    if (isSceneBiggerThanView()) {
        // Do fit-in-view
        fitInView(sceneRect(), Qt::KeepAspectRatio);
        // After fitInView, the image should fit the window, so hide navigator
        emit navigatorViewRequired(false, transform());
    } else {
        // Image is already smaller than window, no navigator needed
        emit navigatorViewRequired(false, transform());
    }

    m_enableFitToScreen = true;
    m_firstUserMediaLoaded = true;
}

bool GraphicsView::isSceneBiggerThanView() const
{
    if (!isThingSmallerThanWindowWith(transform())) {
        return true;
    } else {
        return false;
    }
}

// Automately do fit in view when viewport(window) smaller than image original size.
void GraphicsView::setEnableAutoFitToScreen(bool enable)
{
    m_enableFitToScreen = enable;
}

bool GraphicsView::avoidResetTransform() const
{
    return m_avoidResetTransform;
}

void GraphicsView::setAvoidResetTransform(bool avoidReset)
{
    m_avoidResetTransform = avoidReset;
}

bool GraphicsView::checkerboard() const {
    return m_checkerboardEnabled;
}

void GraphicsView::toggleCheckerboard(bool invertCheckerboardColor)
{
    setCheckerboardEnabled(!m_checkerboardEnabled, invertCheckerboardColor);
}

inline double zeroOrOne(double number)
{
    return qFuzzyIsNull(number) ? 0 : (number > 0 ? 1 : -1);
}

// Note: this only works if we only have 90 degree based rotation
//       and no shear/translate.
QTransform GraphicsView::resetScale(const QTransform & orig)
{
    return QTransform(zeroOrOne(orig.m11()), zeroOrOne(orig.m12()),
                      zeroOrOne(orig.m21()), zeroOrOne(orig.m22()),
                      orig.dx(),             orig.dy());
}

void GraphicsView::updateStyle(Qt::ColorScheme colorScheme) {
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
background-color: black;
)#");
        break;
    case Qt::ColorScheme::Light:
    case Qt::ColorScheme::Unknown:
    default:
        setStyleSheet(R"#(
border-style: none;
background-color: white;
)#");
        break;
    }
}

void GraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (shouldIgnoreMousePressMoveEvent(event)) {
        event->ignore();
        // blumia: return here, or the QMouseEvent event transparency won't
        //         work if we set a QGraphicsView::ScrollHandDrag drag mode.
        return;
    }

    return QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (shouldIgnoreMousePressMoveEvent(event)) {
        event->ignore();
    }

    return QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::ForwardButton || event->button() == Qt::BackButton) {
        event->ignore();
    } else {
        QGraphicsItem *item = itemAt(event->pos());
        if (!item) {
            event->ignore();
        }
    }

    return QGraphicsView::mouseReleaseEvent(event);
}

void GraphicsView::wheelEvent(QWheelEvent *event)
{
    event->ignore();
    // blumia: no need for calling parent method.
}

void GraphicsView::resizeEvent(QResizeEvent *event)
{
    if (m_enableFitToScreen) {
        bool originalSizeSmallerThanWindow = isThingSmallerThanWindowWith(resetScale(transform()));
        if (originalSizeSmallerThanWindow && scaleFactor() >= 1) {
            // no longer need to do fitInView()
            // but we leave the m_enableToScreen value unchanged in case
            // user resize down the window again.
        } else if (originalSizeSmallerThanWindow && scaleFactor() < 1) {
            resetScale();
        } else {
            fitInView(sceneRect(), Qt::KeepAspectRatio);
        }
    } else {
        emit navigatorViewRequired(!isThingSmallerThanWindowWith(transform()), transform());
    }
    return QGraphicsView::resizeEvent(event);
}

bool GraphicsView::isThingSmallerThanWindowWith(const QTransform &transform) const
{
    return rect().size().expandedTo(transform.mapRect(sceneRect()).size().toSize())
            == rect().size();
}

bool GraphicsView::shouldIgnoreMousePressMoveEvent(const QMouseEvent *event) const
{
    if (event->buttons() == Qt::NoButton) {
        return true;
    }

    QGraphicsItem *item = itemAt(event->pos());
    if (!item) {
        return true;
    }

    if (isThingSmallerThanWindowWith(transform())) {
        return true;
    }

    return false;
}

void GraphicsView::setCheckerboardEnabled(bool enabled, bool invertColor)
{
    m_checkerboardEnabled = enabled;
    bool isLightCheckerboard = Settings::instance()->useLightCheckerboard() ^ invertColor;
    if (m_checkerboardEnabled) {
        // Prepare background check-board pattern
        QPixmap tilePixmap(0x20, 0x20);
        tilePixmap.fill(isLightCheckerboard ? QColor(220, 220, 220, 170) : QColor(35, 35, 35, 170));
        QPainter tilePainter(&tilePixmap);
        constexpr QColor color(45, 45, 45, 170);
        constexpr QColor invertedColor(210, 210, 210, 170);
        tilePainter.fillRect(0, 0, 0x10, 0x10, isLightCheckerboard ? invertedColor : color);
        tilePainter.fillRect(0x10, 0x10, 0x10, 0x10, isLightCheckerboard ? invertedColor : color);
        tilePainter.end();

        setBackgroundBrush(tilePixmap);
    } else {
        setBackgroundBrush(Qt::transparent);
    }
}

void GraphicsView::applyTransformationModeByScaleFactor()
{
    if (this->scaleFactor() < 1) {
        scene()->trySetTransformationMode(Qt::SmoothTransformation, this->scaleFactor());
    } else {
        scene()->trySetTransformationMode(Qt::FastTransformation, this->scaleFactor());
    }
}

bool GraphicsView::shouldAvoidTransform() const
{
    return m_firstUserMediaLoaded && m_avoidResetTransform;
}
