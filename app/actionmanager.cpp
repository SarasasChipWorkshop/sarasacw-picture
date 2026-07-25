// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#include "actionmanager.h"

#include "settings.h"
#include "mainwindow.h"
#include "playlistmanager.h"
#include "graphicsview.h"
#include "graphicsscene.h"

#include <QGuiApplication>
#include <QSvgRenderer>
#include <QPainter>

static QIcon loadHidpiIcon(const QString &resp, QSize sz = QSize(32, 32))
{
    QSvgRenderer r(resp);
    QPixmap pm = QPixmap(sz * qApp->devicePixelRatio());
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    r.render(&p);
    pm.setDevicePixelRatio(qApp->devicePixelRatio());
    return QIcon(pm);
}

void ActionManager::setupAction(MainWindow *mainWindow)
{
    // region: Action and Its Icon Creation

    // Declare some convenient macros and lambdas.

    #define ICON_NAME(name)\
        QStringLiteral(":/icons/" #name ".svg")

    #define ACTION_NAME(s) QStringLiteral(STRIFY(s))
    #define STRIFY(s) #s

    auto create_action = [] (QWidget *w, QAction **a, QString i, QString an, bool iconFromTheme = false) {
        *a = new QAction(w);
        if (!i.isNull())
            (*a)->setIcon(iconFromTheme ? QIcon::fromTheme(i) : loadHidpiIcon(i));
        (*a)->setObjectName(an);
        w->addAction(*a);
    };

    /// This macro for register normal action.
    #define CREATE_NEW_ACTION(w, a) create_action(w, &a, QString(), ACTION_NAME(a))
    /// This macro for register Qt theme action.
    #define CREATE_NEW_THEMEICON_ACTION(w, a, i) create_action(w, &a, QLatin1String(STRIFY(i)), ACTION_NAME(a), true)
    /// This macro for register action with our custom icon.
    #define CREATE_NEW_ICON_ACTION(w, a, i) create_action(w, &a, ICON_NAME(i), ACTION_NAME(a), false)

    // TODO: We may replace all Qt theme icons to our custom icon for visual consistency.
    // TODO: Some icons are invalid.

    CREATE_NEW_ACTION(mainWindow, actionCopyPixmap);
    CREATE_NEW_ACTION(mainWindow, actionCopyFilePath);

    CREATE_NEW_ICON_ACTION(mainWindow, actionPrevPicture, go-previous);
    CREATE_NEW_ICON_ACTION(mainWindow, actionNextPicture, go-next);

    CREATE_NEW_ICON_ACTION(mainWindow, actionActualSize, zoom-original);
    CREATE_NEW_ICON_ACTION(mainWindow, actionFitToScreen, app-icon);
    CREATE_NEW_ICON_ACTION(mainWindow, actionFitByWidth, app-icon);
    CREATE_NEW_ICON_ACTION(mainWindow, actionFitByHeight, app-icon);
    CREATE_NEW_ICON_ACTION(mainWindow, actionZoomIn, zoom-in);
    CREATE_NEW_ICON_ACTION(mainWindow, actionZoomOut, zoom-out);

    CREATE_NEW_ICON_ACTION(mainWindow, actionFlipHorizontal, app-icon);
    CREATE_NEW_ICON_ACTION(mainWindow, actionFlipVertical, app-icon);
    CREATE_NEW_ICON_ACTION(mainWindow, actionRotateClockwise, object-rotate-right);
    CREATE_NEW_ICON_ACTION(mainWindow, actionRotateCounterClockwise, app-icon);

    CREATE_NEW_ACTION(mainWindow, actionToggleAvoidResetTransform);
    CREATE_NEW_ACTION(mainWindow, actionToggleCheckerboard);
    CREATE_NEW_ACTION(mainWindow, actionTogglePauseAnimation);
    CREATE_NEW_ACTION(mainWindow, actionAnimationNextFrame);

    CREATE_NEW_THEMEICON_ACTION(mainWindow, actionTrash, edit-delete);
    CREATE_NEW_THEMEICON_ACTION(mainWindow, actionLocateInFileManager, system-file-manager);
    CREATE_NEW_THEMEICON_ACTION(mainWindow, actionProperties, document-properties);

    CREATE_NEW_ACTION(mainWindow, actionSettings);
    CREATE_NEW_THEMEICON_ACTION(mainWindow, actionHelp, system-help);

    // Remove convenient macros

    #undef CREATE_NEW_ICON_ACTION
    #undef CREATE_NEW_THEMEICON_ACTION
    #undef CREATE_NEW_ACTION

    #undef STRIFY
    #undef ACTION_NAME
    #undef ICON_NAME

    // endregion

    // region: Setup Checkable Action

    actionToggleAvoidResetTransform->setCheckable(true);
    actionToggleCheckerboard->setCheckable(true);
    actionTogglePauseAnimation->setCheckable(true);

    // endregion

    // region: Action Translatable Text Setup

    actionCopyPixmap->setText(QCoreApplication::translate("MainWindow", "Copy P&ixmap", nullptr));
    actionCopyFilePath->setText(QCoreApplication::translate("MainWindow", "Copy &File Path", nullptr));

    actionPrevPicture->setText(QCoreApplication::translate("MainWindow", "Previous image", nullptr));
    actionNextPicture->setText(QCoreApplication::translate("MainWindow", "Next image", nullptr));

    actionActualSize->setText(QCoreApplication::translate("MainWindow", "Actual size", nullptr));
    actionFitToScreen->setText(QCoreApplication::translate("MainWindow", "Fit to screen", nullptr));
    actionFitByWidth->setText(QCoreApplication::translate("MainWindow", "Fit by width", nullptr));
    actionFitByHeight->setText(QCoreApplication::translate("MainWindow", "Fit by height", nullptr));
    actionZoomIn->setText(QCoreApplication::translate("MainWindow", "Zoom in", nullptr));
    actionZoomOut->setText(QCoreApplication::translate("MainWindow", "Zoom out", nullptr));

    actionFlipHorizontal->setText(QCoreApplication::translate("MainWindow", "Flip &Horizontally", nullptr));
    actionFlipVertical->setText(QCoreApplication::translate("MainWindow", "Flip &Vertically", nullptr));
    actionRotateClockwise->setText(QCoreApplication::translate("MainWindow", "Rotate right", nullptr));
    actionRotateCounterClockwise->setText(QCoreApplication::translate("MainWindow", "Rotate left", nullptr));

    actionToggleAvoidResetTransform->setText(QCoreApplication::translate("MainWindow", "Keep transformation", "The 'transformation' means the flip/rotation status that currently applied to the image view"));
    actionToggleCheckerboard->setText(QCoreApplication::translate("MainWindow", "Toggle Checkerboard", nullptr));
    actionTogglePauseAnimation->setText(QCoreApplication::translate("MainWindow", "Pause/Resume Animation", nullptr));
    actionAnimationNextFrame->setText(QCoreApplication::translate("MainWindow", "Animation Go to Next Frame", nullptr));

    actionTrash->setText(QCoreApplication::translate("MainWindow", "Move to Trash", nullptr));
#ifdef Q_OS_WIN
    actionLocateInFileManager->setText(
        QCoreApplication::translate(
            "MainWindow", "Show in File Explorer",
            "File Explorer is the name of explorer.exe under Windows"
            )
        );
#else
    actionLocateInFileManager->setText(QCoreApplication::translate("MainWindow", "Show in directory", nullptr));
#endif // Q_OS_WIN
    actionProperties->setText(QCoreApplication::translate("MainWindow", "Properties", nullptr));

    actionSettings->setText(QCoreApplication::translate("MainWindow", "Configure...", nullptr));
    actionHelp->setText(QCoreApplication::translate("MainWindow", "Help", nullptr));
    // endregion

    QMetaObject::connectSlotsByName(mainWindow);
}

void ActionManager::setupShortcuts()
{
    actionCopyPixmap->setShortcut(QKeySequence::Copy);

    actionPrevPicture->setShortcuts({
        QKeySequence(Qt::Key_Left),
        QKeySequence(Qt::Key_PageUp),
    });
    actionNextPicture->setShortcuts({
        QKeySequence(Qt::Key_Right),
        QKeySequence(Qt::Key_PageDown),
    });

    actionActualSize->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    actionZoomIn->setShortcut(QKeySequence::ZoomIn);
    actionZoomOut->setShortcut(QKeySequence::ZoomOut);

    actionTrash->setShortcut(QKeySequence::Delete);
    actionProperties->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));

    actionSettings->setShortcut(QKeySequence::Preferences);
    actionHelp->setShortcut(QKeySequence::HelpContents);
}

void ActionManager::updateActionState(PlaylistManager* pm, GraphicsView* gv)
{
    const auto* gs = gv->scene();
    const auto* settings = Settings::instance();

    // Update copy actions
    bool canCopy = pm->currentIndex().has_value();
    actionCopyPixmap->setEnabled(canCopy);
    actionCopyFilePath->setEnabled(canCopy);

    // Update Prev/Next Picture State
    const int galleryFileCount = pm->totalCount();
    const bool loopGallery = settings->loopGallery();
    actionPrevPicture->setEnabled(galleryFileCount > 1 && (loopGallery || !pm->isFirstIndex()));
    actionNextPicture->setEnabled(galleryFileCount > 1 && (loopGallery || !pm->isLastIndex()));

    // Update Avoid Reset Transform
    actionToggleAvoidResetTransform->setChecked(gv->isAvoidResetTransform());
    // Update Checkerboard
    actionToggleCheckerboard->setChecked(gv->isCheckerboard());
    // Update Animation Graphics Action
    bool canPauseAnimation = gs->canPauseAnimation();
    actionTogglePauseAnimation->setEnabled(canPauseAnimation);
    if (canPauseAnimation) {
        actionTogglePauseAnimation->setChecked(gs->isPauseAnimation());
    } else {
        actionTogglePauseAnimation->setChecked(false);
    }
    bool canSkipAnimation = gs->canSkipAnimationFrame();
    actionAnimationNextFrame->setEnabled(canSkipAnimation);

    // Update trash, located in explorer and properties.
    bool hasLoadedFile = pm->currentIndex().has_value();
    actionTrash->setEnabled(hasLoadedFile);
    actionLocateInFileManager->setEnabled(hasLoadedFile);
    actionProperties->setEnabled(hasLoadedFile);
}

