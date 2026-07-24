// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class ActionManager;
class PlaylistManager;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    void showFiles(const QStringList& files);
    void showFiles(const QList<QUrl>& urls);
    void initWindowSize();

    void clearGallery();
    void galleryPrev();
    void galleryNext();
    void galleryCurrent(bool fromFileWatcher);

    static QStringList supportedImageFormats();

protected slots:
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

    void centerWindow();
    void closeWindow();
    void toggleAvoidResetTransform();
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
    QT_PREPEND_NAMESPACE(Ui::MainWindow) *ui;
    
    ActionManager *m_am;
    PlaylistManager *m_pm;
    QFileSystemWatcher *m_fileSystemWatcher;
};

#endif // MAINWINDOW_H
