// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#include "graphicsscene.h"

#include "settings.h"

#include <QStyleHints>
#include <QOperatingSystemVersion>
#include <QGuiApplication>
#include <QGraphicsSceneMouseEvent>
#include <QMimeData>
#include <QDebug>
#include <QGraphicsItem>
#include <QUrl>
#include <QGraphicsSvgItem>
#include <QSvgRenderer>
#include <QMovie>
#include <QPainter>

static const QColor& ColorSchemaToTextForeground(Qt::ColorScheme colorScheme) {
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
        static QColor WHITE = QColor("White");
        return WHITE;
    case Qt::ColorScheme::Light:
    case Qt::ColorScheme::Unknown:
    default:
        static QColor BLACK = QColor("Black");
        return BLACK;
    }
}

class PGraphicsPixmapItem : public QGraphicsPixmapItem
{
public:
    PGraphicsPixmapItem(const QPixmap &pixmap, QGraphicsItem *parent = nullptr)
        : QGraphicsPixmapItem(pixmap, parent)
    {}

    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    void setScaleHint(float scaleHint) {
        m_scaleHint = scaleHint;
    }

    const QPixmap & scaledPixmap(float scaleHint) {
        if (qFuzzyCompare(scaleHint, m_cachedScaleHint)) return m_cachedPixmap;
        QSizeF resizedScale(boundingRect().size());
        resizedScale *= scaleHint;
        QPixmap && sourcePixmap = pixmap();
        m_cachedPixmap = sourcePixmap.scaled(
             resizedScale.toSize() * sourcePixmap.devicePixelRatioF(),
             Qt::KeepAspectRatio,
             Qt::SmoothTransformation);
        m_cachedScaleHint = scaleHint;
        return m_cachedPixmap;
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
               QWidget *widget) override
    {
        if (transformationMode() == Qt::FastTransformation) {
            return QGraphicsPixmapItem::paint(painter, option, widget);
        } else {
//            painter->setRenderHints(QPainter::Antialiasing);
            painter->drawPixmap(QRectF(offset(), boundingRect().size()).toRect(),
                                scaledPixmap(m_scaleHint));
        }
    }

private:
    float m_scaleHint = 1;
    float m_cachedScaleHint = -1;
    QPixmap m_cachedPixmap;
};

class PGraphicsMovieItem : public QGraphicsItem
{
public:
    PGraphicsMovieItem(QGraphicsItem *parent = nullptr) : QGraphicsItem(parent) {}

    enum { Type = UserType + 2 };
    int type() const override { return Type; }

    void setMovie(QMovie* movie) {
        if (m_movie) m_movie->disconnect();
        m_movie.reset(movie);
        m_movie->connect(m_movie.data(), &QMovie::updated, [this](){
            this->update();
        });
    }

    QRectF boundingRect() const override {
        if (m_movie) { return m_movie->frameRect(); }
        else { return QRectF(); }
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override {
        if (m_movie) {
            painter->drawPixmap(m_movie->frameRect(), m_movie->currentPixmap(), m_movie->frameRect());
        }
    }

    inline QMovie * movie() const {
        return m_movie.data();
    }

private:
    QScopedPointer<QMovie> m_movie;
};

GraphicsScene::GraphicsScene(QObject *parent)
    : QGraphicsScene(parent)
{
    showText(tr("Drag image here", "The same meaning of MainWindow's hint."));
    connect(qGuiApp->styleHints(), &QStyleHints::colorSchemeChanged, this, &GraphicsScene::updateStyle);
}

GraphicsScene::~GraphicsScene()
{

}

void GraphicsScene::showImage(const QPixmap &pixmap)
{
    this->clear();
    PGraphicsPixmapItem * pixmapItem = new PGraphicsPixmapItem(pixmap);
    this->addItem(pixmapItem);
    pixmapItem->setShapeMode(QGraphicsPixmapItem::BoundingRectShape);
    m_theThing = pixmapItem;
    this->setSceneRect(m_theThing->boundingRect());
}

void GraphicsScene::showText(const QString &text)
{
    this->clear();
    QGraphicsTextItem * textItem = this->addText(text);
    textItem->setDefaultTextColor(ColorSchemaToTextForeground(qGuiApp->styleHints()->colorScheme()));
    m_theThing = textItem;
    this->setSceneRect(m_theThing->boundingRect());
}

void GraphicsScene::showSvg(const QString &filepath)
{
    this->clear();
    QGraphicsSvgItem * svgItem = new QGraphicsSvgItem();
    QSvgRenderer * render = new QSvgRenderer(svgItem);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    if (Settings::instance()->svgTiny12Only()) {
        render->setOptions(QtSvg::Tiny12FeaturesOnly);
    }
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    render->load(filepath);
    svgItem->setSharedRenderer(render);
    this->addItem(svgItem);
    m_theThing = svgItem;
    this->setSceneRect(m_theThing->boundingRect());
}

void GraphicsScene::showAnimated(const QString &filepath)
{
    this->clear();

    PGraphicsMovieItem * animatedItem = new PGraphicsMovieItem();
    QMovie * movie = new QMovie(filepath);
    movie->start();
    animatedItem->setMovie(movie);
    this->addItem(animatedItem);
    m_theThing = animatedItem;

    this->setSceneRect(m_theThing->boundingRect());
}

bool GraphicsScene::trySetTransformationMode(Qt::TransformationMode mode, float scaleHint)
{
    PGraphicsPixmapItem * pixmapItem = qgraphicsitem_cast<PGraphicsPixmapItem *>(m_theThing);
    if (pixmapItem) {
        pixmapItem->setTransformationMode(mode);
        pixmapItem->setScaleHint(scaleHint);
        return true;
    }

    return false;
}

/// Fetch graphics item as our custom animated item.
/// Return nullptr means this item is not animated item.
static PGraphicsMovieItem* fetchAnimatedItem(QGraphicsItem *item) {
    return qgraphicsitem_cast<PGraphicsMovieItem *>(item);
}

bool GraphicsScene::canPauseAnimation() const {
    return fetchAnimatedItem(m_theThing) != nullptr;
}

bool GraphicsScene::isPauseAnimation() const {
    auto* animatedItem = fetchAnimatedItem(m_theThing);
    if (animatedItem != nullptr) {
        return animatedItem->movie()->state() == QMovie::Paused;
    } else {
        return false;
    }
}

bool GraphicsScene::togglePauseAnimation()
{
    auto* animatedItem = fetchAnimatedItem(m_theThing);
    if (animatedItem != nullptr) {
        animatedItem->movie()->setPaused(animatedItem->movie()->state() != QMovie::Paused);
        return true;
    } else {
        return false;
    }
}

bool GraphicsScene::canSkipAnimationFrame() const {
    return isPauseAnimation();
}

bool GraphicsScene::skipAnimationFrame(int delta)
{
    auto* animatedItem = fetchAnimatedItem(m_theThing);

    // Only paused animated item can skip frame
    if (animatedItem == nullptr) {
        return false;
    }
    if (animatedItem->movie()->state() != QMovie::Paused) {
        return false;
    }

    // Perform skip
    const int frameCount = animatedItem->movie()->frameCount();
    const int currentFrame = animatedItem->movie()->currentFrameNumber();
    const int targetFrame = (currentFrame + delta) % frameCount;
    return animatedItem->movie()->jumpToFrame(targetFrame);
}

QPixmap GraphicsScene::renderToPixmap()
{
    PGraphicsPixmapItem * pixmapItem = qgraphicsitem_cast<PGraphicsPixmapItem *>(m_theThing);
    if (pixmapItem) {
        return pixmapItem->pixmap();
    }

    QPixmap pixmap(sceneRect().toRect().size());
    pixmap.fill(Qt::transparent);
    QPainter p(&pixmap);
    render(&p, sceneRect());

    return pixmap;
}

void GraphicsScene::updateStyle(Qt::ColorScheme colorScheme) {
    Q_UNUSED(colorScheme)

    QGraphicsItem * item = this->m_theThing;
    if (item == nullptr) return;
    QGraphicsTextItem * textItem = qgraphicsitem_cast<QGraphicsTextItem*>(item);
    if (textItem == nullptr) return;

    textItem->setDefaultTextColor(ColorSchemaToTextForeground(colorScheme));
}
