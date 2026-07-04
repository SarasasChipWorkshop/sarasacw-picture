// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "mainwindow.h"

#include "settings.h"
#include "bottombuttongroup.h"
#include "graphicsview.h"
#include "navigatorview.h"
#include "graphicsscene.h"
#include "settingsdialog.h"
#include "aboutdialog.h"
#include "metadatamodel.h"
#include "metadatadialog.h"
#include "actionmanager.h"
#include "playlistmanager.h"

#include <QMouseEvent>
#include <QMovie>
#include <QDebug>
#include <QGraphicsTextItem>
#include <QApplication>
#include <QStyle>
#include <QScreen>
#include <QMenu>
#include <QShortcut>
#include <QClipboard>
#include <QMimeData>
#include <QWindow>
#include <QFile>
#include <QTimer>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QProcess>
#include <QDesktopServices>
#include <QMessageBox>

#ifdef HAVE_QTDBUS
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#endif // HAVE_QTDBUS

using namespace Qt::Literals::StringLiterals;

MainWindow::MainWindow(QWidget *parent)
    : FramelessWindow(parent)
    , m_am(new ActionManager)
    , m_pm(new PlaylistManager(this))
    , m_fileSystemWatcher(new QFileSystemWatcher(this))
{
    this->setAttribute(Qt::WA_TranslucentBackground, true);
    this->setMinimumSize(350, 330);
    this->setWindowIcon(QIcon(u":/icons/app-icon.svg"_s));
    this->setMouseTracking(true);
    this->setAcceptDrops(true);

    m_pm->setAutoLoadFilterSuffixes(supportedImageFormats());

    GraphicsScene * scene = new GraphicsScene(this);

    m_gv = new GraphicsView(this);
    m_gv->setScene(scene);
    this->setCentralWidget(m_gv);

    m_nav = new NavigatorView(this);
    m_nav->setFixedSize(220, 160);
    m_nav->setScene(scene);
    m_nav->setMainView(m_gv);
    m_nav->fitInView(m_nav->sceneRect(), Qt::KeepAspectRatio);

    connect(m_gv, &GraphicsView::navigatorViewRequired,
            this, [this](bool required, const QTransform & tf){
        m_nav->setTransform(GraphicsView::resetScale(tf));
        m_nav->fitInView(m_nav->sceneRect(), Qt::KeepAspectRatio);
        m_nav->setVisible(required);
        m_nav->updateMainViewportRegion();
    });

    connect(m_gv, &GraphicsView::viewportRectChanged,
            m_nav, &NavigatorView::updateMainViewportRegion);

    m_am->setupAction(this);

    m_bottomButtonGroup = new BottomButtonGroup({
        m_am->actionActualSize,
        m_am->actionToggleMaximize,
        m_am->actionZoomIn,
        m_am->actionZoomOut,
        m_am->actionPrevPicture,
        m_am->actionNextPicture,
        m_am->actionToggleCheckerboard,
        m_am->actionRotateClockwise
    }, this);

    connect(m_pm, &PlaylistManager::totalCountChanged, this, &MainWindow::updateGalleryButtonsVisibility);

    connect(m_pm->model(), &PlaylistModel::modelReset, this, std::bind(&MainWindow::galleryCurrent, this, false, false));
    connect(m_pm, &PlaylistManager::currentIndexChanged, this, std::bind(&MainWindow::galleryCurrent, this, true, false));

    connect(m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, [this](){
        QTimer::singleShot(500, this, std::bind(&MainWindow::galleryCurrent, this, false, true));
    });

    QShortcut * fullscreenShorucut = new QShortcut(QKeySequence(QKeySequence::FullScreen), this);
    connect(fullscreenShorucut, &QShortcut::activated,
            this, &MainWindow::toggleFullscreen);

    centerWindow();

    QTimer::singleShot(0, this, [this](){
        m_am->setupShortcuts();
        Settings::instance()->applyUserShortcuts(this);
    });

}

MainWindow::~MainWindow()
{

}

void MainWindow::showUrls(const QList<QUrl> &urls)
{
    if (!urls.isEmpty()) {
        const QUrl & firstUrl = urls.first();
        if (urls.count() == 1) {
            const QString lowerCaseUrlPath(firstUrl.path().toLower());
            if (lowerCaseUrlPath.endsWith(".m3u8") || lowerCaseUrlPath.endsWith(".m3u")) {
                m_pm->loadM3U8Playlist(firstUrl);
                galleryCurrent(true, true);
                return;
            }
        }
        m_gv->showFileFromPath(firstUrl.toLocalFile());
        m_pm->loadPlaylist(urls);
    } else {
        m_gv->showText(tr("File url list is empty"));
        m_pm->setPlaylist(urls);
        return;
    }

    m_nav->fitInView(m_nav->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::initWindowSize()
{
    switch (Settings::instance()->initWindowSizeBehavior()) {
    case Settings::WindowSizeBehavior::Auto:
        adjustWindowSizeBySceneRect();
        break;
    case Settings::WindowSizeBehavior::Maximized:
        showMaximized();
        break;
    case Settings::WindowSizeBehavior::Windowed:
        showNormal();
        break;
    default:
        adjustWindowSizeBySceneRect();
        break;
    }
}

void MainWindow::adjustWindowSizeBySceneRect()
{
    if (m_pm->totalCount() < 1) return;

    QSize sceneSize = m_gv->sceneRect().toRect().size();
    QSize sceneSizeWithMargins = sceneSize + QSize(130, 125);

    if (m_gv->scaleFactor() < 1 || size().expandedTo(sceneSizeWithMargins) != size()) {
        // if it scaled down by the resize policy:
        QSize screenSize = qApp->screenAt(QCursor::pos())->availableSize();
        if (screenSize.expandedTo(sceneSize) == screenSize) {
            // we can show the picture by increase the window size.
            QSize finalSize = (screenSize.expandedTo(sceneSizeWithMargins) == screenSize) ?
                              sceneSizeWithMargins : screenSize;
            // We have a very reasonable sizeHint() value ;P
            this->resize(finalSize.expandedTo(this->sizeHint()));

            // We're sure the window can display the whole thing with 1:1 scale.
            // The old window size may cause fitInView call from resize() and the
            // above resize() call won't reset the scale back to 1:1, so we
            // just call resetScale() here to ensure the thing is no longer scaled.
            m_gv->resetScale();
            centerWindow();
        } else {
            // toggle maximum
            showMaximized();
        }
    }
}

// can be empty if it is NOT from a local file.
QUrl MainWindow::currentImageFileUrl() const
{
    return m_pm->urlByIndex(m_pm->curIndex());
}

void MainWindow::clearGallery()
{
    m_pm->setPlaylist({});
}

void MainWindow::galleryPrev()
{
    const bool loopGallery = Settings::instance()->loopGallery();
    if (!loopGallery && m_pm->isFirstIndex()) return;

    QModelIndex index = m_pm->previousIndex();
    if (index.isValid()) {
        m_pm->setCurrentIndex(index);
        m_gv->showFileFromPath(m_pm->localFileByIndex(index));
    }
}

void MainWindow::galleryNext()
{
    const bool loopGallery = Settings::instance()->loopGallery();
    if (!loopGallery && m_pm->isLastIndex()) return;

    QModelIndex index = m_pm->nextIndex();
    if (index.isValid()) {
        m_pm->setCurrentIndex(index);
        m_gv->showFileFromPath(m_pm->localFileByIndex(index));
    }
}

// Only use this to update minor information.
void MainWindow::galleryCurrent(bool showLoadImageHintWhenEmpty, bool reloadImage)
{
    QModelIndex index = m_pm->curIndex();
    bool shouldResetfileWatcher = true;
    if (index.isValid()) {
        const QString & localFilePath(m_pm->localFileByIndex(index));
        if (reloadImage) m_gv->showFileFromPath(localFilePath);
        shouldResetfileWatcher = !updateFileWatcher(localFilePath);
        setWindowTitle(m_pm->urlByIndex(index).fileName());
    } else if (showLoadImageHintWhenEmpty && m_pm->totalCount() <= 0) {
        m_gv->showText(QCoreApplication::translate("GraphicsScene", "Drag image here"));
        setWindowTitle(QString());
    }

    updateGalleryButtonsVisibility();

    if (shouldResetfileWatcher) updateFileWatcher();
}

QStringList MainWindow::supportedImageFormats()
{
    QStringList formatFilters {
#if QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
        QStringLiteral("*.jfif")
#endif // QT_VERSION < QT_VERSION_CHECK(6, 8, 0)
    };
    for (const QByteArray &item : QImageReader::supportedImageFormats()) {
        formatFilters.append(QStringLiteral("*.") % QString::fromLocal8Bit(item));
    }
    return formatFilters;
}

void MainWindow::showEvent(QShowEvent *event)
{
    updateWidgetsPosition();

    return FramelessWindow::showEvent(event);
}

void MainWindow::enterEvent(QEnterEvent *event)
{
    return FramelessWindow::enterEvent(event);
}

void MainWindow::leaveEvent(QEvent *event)
{
    return FramelessWindow::leaveEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    // The forward/back mouse button can also used to trigger a mouse double-click event
    // Since we use that for gallery navigation so we ignore these two buttons.
    if (event->buttons() & Qt::ForwardButton || event->buttons() & Qt::BackButton) {
        return;
    }

    switch (Settings::instance()->doubleClickBehavior()) {
    case Settings::DoubleClickBehavior::Close:
        quitAppAction();
        event->accept();
        break;
    case Settings::DoubleClickBehavior::Maximize:
        toggleMaximize();
        event->accept();
        break;
    case Settings::DoubleClickBehavior::FullScreen:
        toggleFullscreen();
        event->accept();
        break;
    case Settings::DoubleClickBehavior::Ignore:
        break;
    }

    // blumia: don't call parent constructor here, seems it will cause mouse move
    //         event get called even if we set event->accept();
    // return QMainWindow::mouseDoubleClickEvent(event);
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;
    bool needWeelEvent = false, wheelUp = false;
    bool actionIsZoom = event->modifiers().testFlag(Qt::ControlModifier)
            || Settings::instance()->mouseWheelBehavior() == Settings::MouseWheelBehavior::Zoom;

    // NOTE: Only checking angleDelta since the QWheelEvent::pixelDelta() doc says
    //       pixelDelta() value is driver specific and unreliable on X11...
    //       We are not scrolling the canvas, just zoom in or out, so it probably
    //       doesn't matter here.
    if (!numDegrees.isNull() && numDegrees.y() != 0) {
        needWeelEvent = true;
        wheelUp = numDegrees.y() > 0;
    }

    if (needWeelEvent) {
        if (actionIsZoom) {
            if (wheelUp) {
                on_actionZoomIn_triggered();
            } else {
                on_actionZoomOut_triggered();
            }
        } else {
            if (wheelUp) {
                galleryPrev();
            } else {
                galleryNext();
            }
        }
        event->accept();
    } else {
        FramelessWindow::wheelEvent(event);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    updateWidgetsPosition();

    return FramelessWindow::resizeEvent(event);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu * menu = new QMenu;
    QMenu * copyMenu = new QMenu(tr("&Copy"));
    QUrl currentFileUrl = currentImageFileUrl();

    QAction * copyPixmap = m_am->actionCopyPixmap;
    QAction * copyFilePath = m_am->actionCopyFilePath;

    copyMenu->setIcon(QIcon::fromTheme(u"edit-copy"_s));
    copyMenu->addAction(copyPixmap);
    if (currentFileUrl.isValid()) {
        copyMenu->addAction(copyFilePath);
    }

    QAction * paste = m_am->actionPaste;

    QAction * trash = m_am->actionTrash;

    QAction * avoidResetTransform = m_am->actionToggleAvoidResetTransform;
    avoidResetTransform->setCheckable(true);
    avoidResetTransform->setChecked(m_gv->avoidResetTransform());

    QAction * toggleSettings = m_am->actionSettings;
    QAction * helpAction = m_am->actionHelp;
    QAction * propertiesAction = m_am->actionProperties;

#if 0
    menu->addAction(m_am->actionOpen);
#endif // 0

    if (copyMenu->actions().count() == 1) {
        menu->addActions(copyMenu->actions());
    } else {
        menu->addMenu(copyMenu);
    }

    if (canPaste()) {
        menu->addAction(paste);
    }

    menu->addSeparator();

    menu->addAction(m_am->actionHorizontalFlip);
#if 0
    menu->addAction(m_am->actionFitInView);
    menu->addAction(m_am->actionFitByWidth);
#endif // 0
    menu->addSeparator();
    menu->addAction(avoidResetTransform);
    menu->addSeparator();
    menu->addAction(toggleSettings);
    menu->addAction(helpAction);
    if (currentFileUrl.isValid()) {
        menu->addSeparator();
        if (currentFileUrl.isLocalFile()) {
            menu->addAction(trash);
            menu->addAction(m_am->actionLocateInFileManager);
        }
        menu->addAction(propertiesAction);
    }
    menu->exec(mapToGlobal(event->pos()));
    menu->deleteLater();
    copyMenu->deleteLater();

    return FramelessWindow::contextMenuEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() || event->mimeData()->hasImage() || event->mimeData()->hasText()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }

    return FramelessWindow::dragEnterEvent(event);
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    Q_UNUSED(event)
}

void MainWindow::dropEvent(QDropEvent *event)
{
    event->acceptProposedAction();

    const QMimeData * mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        const QList<QUrl> &urls = mimeData->urls();
        if (urls.isEmpty()) {
            m_gv->showText(tr("File url list is empty"));
        } else {
            showUrls(urls);
        }
    } else if (mimeData->hasImage()) {
        QImage img = qvariant_cast<QImage>(mimeData->imageData());
        QPixmap pixmap = QPixmap::fromImage(img);
        if (pixmap.isNull()) {
            m_gv->showText(tr("Image data is invalid"));
        } else {
            m_gv->showImage(pixmap);
        }
    } else if (mimeData->hasText()) {
        m_gv->showText(mimeData->text());
    } else {
        m_gv->showText(tr("Not supported mimedata: %1").arg(mimeData->formats().first()));
    }
}

void MainWindow::centerWindow()
{
    this->setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            this->size(),
            qApp->screenAt(QCursor::pos())->availableGeometry()
        )
    );
}

void MainWindow::closeWindow()
{
}

void MainWindow::updateWidgetsPosition()
{
    m_bottomButtonGroup->move((width() - m_bottomButtonGroup->width()) / 2,
                              height() - m_bottomButtonGroup->height());
    m_nav->move(width() - m_nav->width(), height() - m_nav->height());
}

void MainWindow::toggleAvoidResetTransform()
{
    m_gv->setAvoidResetTransform(!m_gv->avoidResetTransform());
}

bool MainWindow::canPaste() const
{
    const QMimeData * clipboardData = QApplication::clipboard()->mimeData();
    if (clipboardData->hasImage()) {
        return true;
    } else if (clipboardData->hasText()) {
        QString clipboardText(clipboardData->text());
        if (clipboardText.startsWith("PICTURE:")) {
            QString maybeFilename(clipboardText.mid(8));
            if (QFile::exists(maybeFilename)) {
                return true;
            }
        }
    }

    return false;
}

void MainWindow::quitAppAction(bool force)
{
    closeWindow();
}

void MainWindow::toggleFullscreen()
{
    if (isFullScreen()) {
        showNormal();
    } else {
        showFullScreen();
    }
}

void MainWindow::toggleMaximize()
{
    if (isMaximized()) {
        showNormal();
    } else {
        showMaximized();
    }
}

QSize MainWindow::sizeHint() const
{
    return QSize(710, 530);
}

void MainWindow::on_actionOpen_triggered()
{
    QStringList picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    QUrl pictureUrl = picturesLocations.isEmpty() ? QUrl::fromLocalFile(picturesLocations.first())
                                                  : QUrl::fromLocalFile(QDir::homePath());
    QList<QUrl> urls(QFileDialog::getOpenFileUrls(this, QString(), pictureUrl));
    if (!urls.isEmpty()) {
        showUrls(urls);
    }
}

void MainWindow::on_actionActualSize_triggered()
{
    m_gv->resetScale();
    m_gv->setEnableAutoFitInView(false);
}

void MainWindow::on_actionToggleMaximize_triggered()
{
    toggleMaximize();
}

void MainWindow::on_actionZoomIn_triggered()
{
    if (m_gv->scaleFactor() < 1000) {
        m_gv->zoomView(1.25);
    }
}

void MainWindow::on_actionZoomOut_triggered()
{
    m_gv->zoomView(0.8);
}

void MainWindow::on_actionHorizontalFlip_triggered()
{
    m_gv->flipView();
}

void MainWindow::on_actionFitInView_triggered()
{
    m_gv->fitInView(m_nav->sceneRect(), Qt::KeepAspectRatio);
    m_gv->setEnableAutoFitInView(m_gv->scaleFactor() <= 1);
}

void MainWindow::on_actionFitByWidth_triggered()
{
    m_gv->fitByOrientation();
}

void MainWindow::on_actionCopyPixmap_triggered()
{
    QClipboard *cb = QApplication::clipboard();
    cb->setPixmap(m_gv->scene()->renderToPixmap());
}

void MainWindow::on_actionCopyFilePath_triggered()
{
    QUrl currentFileUrl(currentImageFileUrl());
    if (currentFileUrl.isValid()) {
        QClipboard *cb = QApplication::clipboard();
        cb->setText(currentFileUrl.toLocalFile());
    }
}

void MainWindow::on_actionPaste_triggered()
{
    QImage clipboardImage;
    QUrl clipboardFileUrl;

    const QMimeData * clipboardData = QApplication::clipboard()->mimeData();
    if (clipboardData->hasImage()) {
        QVariant imageVariant(clipboardData->imageData());
        if (imageVariant.isValid()) {
            clipboardImage = qvariant_cast<QImage>(imageVariant);
        }
    } else if (clipboardData->hasText()) {
        QString clipboardText(clipboardData->text());
        if (clipboardText.startsWith("PICTURE:")) {
            QString maybeFilename(clipboardText.mid(8));
            if (QFile::exists(maybeFilename)) {
                clipboardFileUrl = QUrl::fromLocalFile(maybeFilename);
            }
        }
    }

    if (!clipboardImage.isNull()) {
        setWindowTitle(tr("Image From Clipboard"));
        m_gv->showImage(clipboardImage);
        clearGallery();
    } else if (clipboardFileUrl.isValid()) {
        m_gv->showFileFromPath(clipboardFileUrl.toLocalFile());
        m_pm->loadPlaylist(clipboardFileUrl);
    }
}

void MainWindow::on_actionTrash_triggered()
{
    QModelIndex index = m_pm->curIndex();
    if (!m_pm->urlByIndex(index).isLocalFile()) return;

    QFile file(m_pm->localFileByIndex(index));
    QFileInfo fileInfo(file.fileName());

    QMessageBox::StandardButton result = QMessageBox::question(this, tr("Move to Trash"),
                                                               tr("Are you sure you want to move \"%1\" to recycle bin?").arg(fileInfo.fileName()));
    if (result == QMessageBox::Yes) {
        bool succ = file.moveToTrash();
        if (!succ) {
            QMessageBox::warning(this, tr("Failed to move file to trash"),
                                 tr("Move to trash failed, it might caused by file permission issue, file system limitation, or platform limitation."));
        } else {
            m_pm->removeAt(index);
            galleryCurrent(true, true);
        }
    }
}

void MainWindow::on_actionToggleCheckerboard_triggered()
{
    // TODO: is that okay to do this since we plan to support custom shortcuts?
    m_gv->toggleCheckerboard(QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier));
}

void MainWindow::on_actionRotateClockwise_triggered()
{
    m_gv->rotateView();
    m_gv->displayScene();
}

void MainWindow::on_actionRotateCounterClockwise_triggered()
{
    m_gv->rotateView(false);
    m_gv->displayScene();
}

void MainWindow::on_actionPrevPicture_triggered()
{
    galleryPrev();
}

void MainWindow::on_actionNextPicture_triggered()
{
    galleryNext();
}

void MainWindow::on_actionTogglePauseAnimation_triggered()
{
    m_gv->scene()->togglePauseAnimation();
}

void MainWindow::on_actionAnimationNextFrame_triggered()
{
    m_gv->scene()->skipAnimationFrame(1);
}

void MainWindow::on_actionToggleAvoidResetTransform_triggered()
{
    toggleAvoidResetTransform();
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog * sd = new SettingsDialog(this);
    sd->exec();
    sd->deleteLater();
}

void MainWindow::on_actionHelp_triggered()
{
    AboutDialog * ad = new AboutDialog(this);
    ad->exec();
    ad->deleteLater();
}

void MainWindow::on_actionProperties_triggered()
{
    QUrl currentFileUrl = currentImageFileUrl();
    if (!currentFileUrl.isValid()) return;

    MetadataModel * md = new MetadataModel();
    md->setFile(currentFileUrl.toLocalFile());

    MetadataDialog * ad = new MetadataDialog(this);
    ad->setMetadataModel(md);
    ad->exec();
    ad->deleteLater();
}

void MainWindow::on_actionLocateInFileManager_triggered()
{
    QUrl currentFileUrl = currentImageFileUrl();
    if (!currentFileUrl.isValid()) return;

    QFileInfo fileInfo(currentFileUrl.toLocalFile());
    if (!fileInfo.exists()) return;

    QUrl && folderUrl = QUrl::fromLocalFile(fileInfo.absolutePath());

#ifdef Q_OS_WIN
    QProcess::startDetached("explorer", QStringList() << "/select," << QDir::toNativeSeparators(fileInfo.absoluteFilePath()));
#elif defined(Q_OS_LINUX) and defined(HAVE_QTDBUS)
    // Use https://www.freedesktop.org/wiki/Specifications/file-manager-interface/ if possible
    const QDBusConnectionInterface * dbusIface = QDBusConnection::sessionBus().interface();
    if (!dbusIface || !dbusIface->isServiceRegistered(QLatin1String("org.freedesktop.FileManager1"))) {
        QDesktopServices::openUrl(folderUrl);
        return;
    }
    QDBusInterface fm1Iface(u"org.freedesktop.FileManager1"_s,
                            u"/org/freedesktop/FileManager1"_s,
                            u"org.freedesktop.FileManager1"_s);
    fm1Iface.setTimeout(1000);
    fm1Iface.callWithArgumentList(QDBus::Block, "ShowItems", {
                                      QStringList{currentFileUrl.toString()},
                                      QString()
                                  });
    if (fm1Iface.lastError().isValid()) {
        QDesktopServices::openUrl(folderUrl);
    }
#else
    QDesktopServices::openUrl(folderUrl);
#endif // Q_OS_WIN
}

void MainWindow::on_actionQuitApp_triggered()
{
    quitAppAction(false);
}

bool MainWindow::updateFileWatcher(const QString &basePath)
{
    m_fileSystemWatcher->removePaths(m_fileSystemWatcher->files());
    if (!basePath.isEmpty()) return m_fileSystemWatcher->addPath(basePath);
    return false;
}

void MainWindow::updateGalleryButtonsVisibility()
{
    const int galleryFileCount = m_pm->totalCount();
    const bool loopGallery = Settings::instance()->loopGallery();
    m_am->enablePrevNextPictureAction(
        galleryFileCount > 1 && (loopGallery || !m_pm->isFirstIndex()),
        galleryFileCount > 1 && (loopGallery || !m_pm->isLastIndex()));
}
