// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "framelesswindow.h"

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>

QT_BEGIN_NAMESPACE
class QGraphicsView;
class QFileSystemWatcher;
QT_END_NAMESPACE

class ActionManager;
class PlaylistManager;
class GraphicsView;
class NavigatorView;
class BottomButtonGroup;
class MainWindow : public FramelessWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void showFiles(const QStringList& files);
    void showFiles(const QList<QUrl>& urls);
    void initWindowSize();
    void adjustWindowSizeBySceneRect();

    void clearGallery();
    void galleryPrev();
    void galleryNext();
    void galleryCurrent(bool fromFileWatcher);

    static QStringList supportedImageFormats();

protected slots:
    void showEvent(QShowEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void centerWindow();
    void closeWindow();
    void updateWidgetsPosition();
    void toggleAvoidResetTransform();
    void quitAppAction(bool force = false);
    void toggleFullscreen();
    void toggleMaximize();

protected:
    QSize sizeHint() const override;

private slots:
    void on_actionCopyPixmap_triggered();
    void on_actionCopyFilePath_triggered();

    void on_actionPrevPicture_triggered();
    void on_actionNextPicture_triggered();

    void on_actionActualSize_triggered();
    void on_actionFitToScreen_triggered();
    void on_actionFitByWidth_triggered();
    void on_actionFitByHeight_triggered();
    void on_actionZoomIn_triggered();
    void on_actionZoomOut_triggered();

    void on_actionFlipHorizontal_triggered();
    void on_actionFlipVertical_triggered();
    void on_actionRotateClockwise_triggered();
    void on_actionRotateCounterClockwise_triggered();

    void on_actionToggleAvoidResetTransform_triggered();
    void on_actionToggleCheckerboard_triggered();
    void on_actionTogglePauseAnimation_triggered();
    void on_actionAnimationNextFrame_triggered();

    void on_actionTrash_triggered();
    void on_actionLocateInFileManager_triggered();
    void on_actionProperties_triggered();

    void on_actionSettings_triggered();
    void on_actionHelp_triggered();

private:
    bool updateFileWatcher(const QString & basePath = QString());
    void updateActionState();

private:
    ActionManager *m_am;
    PlaylistManager *m_pm;
    GraphicsView *m_gv;
    NavigatorView *m_nav;

    QFileSystemWatcher *m_fileSystemWatcher;
    BottomButtonGroup *m_bottomButtonGroup;
};

#endif // MAINWINDOW_H
