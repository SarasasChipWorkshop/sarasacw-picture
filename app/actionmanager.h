// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#ifndef ACTIONMANAGER_H
#define ACTIONMANAGER_H

#include <QAction>

class MainWindow;
class PlaylistManager;
class GraphicsView;

/// Action Manager is separated from Main Window
/// to manage all actions specifically.
class ActionManager
{
public:
    explicit ActionManager() = default;
    ~ActionManager() = default;

    void setupAction(MainWindow * mainWindow);
    void setupShortcuts();

public slots:
    void updateActionState(PlaylistManager* pm, GraphicsView* gv);

public:
    QAction *actionCopyPixmap;
    QAction *actionCopyFilePath;

    QAction *actionPrevPicture;
    QAction *actionNextPicture;

    QAction *actionActualSize;
    QAction *actionFitToScreen;
    QAction *actionFitByWidth;
    QAction *actionFitByHeight;
    QAction *actionZoomIn;
    QAction *actionZoomOut;

    QAction *actionFlipHorizontal;
    QAction *actionFlipVertical;
    QAction *actionRotateClockwise;
    QAction *actionRotateCounterClockwise;

    QAction *actionToggleAvoidResetTransform;
    QAction *actionToggleCheckerboard;
    QAction *actionTogglePauseAnimation;
    QAction *actionAnimationNextFrame;

    QAction *actionTrash;
    QAction *actionLocateInFileManager;
    QAction *actionProperties;

    QAction *actionSettings;
    QAction *actionHelp;
};

#endif // ACTIONMANAGER_H
