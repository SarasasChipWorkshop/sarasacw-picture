// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#ifndef GRAPHICSSCENE_H
#define GRAPHICSSCENE_H

#include <QGraphicsScene>

class GraphicsScene : public QGraphicsScene
{
    Q_OBJECT
public:
    GraphicsScene(QObject *parent = nullptr);
    ~GraphicsScene();

    void showImage(const QPixmap &pixmap);
    void showText(const QString &text);
    void showSvg(const QString &filepath);
    void showAnimated(const QString &filepath);

    bool trySetTransformationMode(Qt::TransformationMode mode, float scaleHint);

    /// Check whether current thing is animation.
    /// Only animation thing can be paused.
    bool canPauseAnimation() const;
    /// Check whether we are pasuing an animation graphics.
    bool isPauseAnimation() const;
    /// Toggle animation graphics pause state.
    /// Return true for successfully toggle, otherwise false.
    bool togglePauseAnimation();
    /// Check whether we can skip animation frame.
    /// Only paused animation graphics can skip frame.
    bool canSkipAnimationFrame() const;
    /// Skip paused animation graphics with given frame.
    /// Return true for successfully skip, otherwise false.
    bool skipAnimationFrame(int delta = 1);

    QPixmap renderToPixmap();

public slots:
    void updateStyle(Qt::ColorScheme colorScheme);

private:
    QGraphicsItem * m_theThing = nullptr;
};

#endif // GRAPHICSSCENE_H
