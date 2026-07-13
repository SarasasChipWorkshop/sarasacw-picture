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
#include <QVBoxLayout>

#ifdef HAVE_QTDBUS
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#endif // HAVE_QTDBUS

using namespace Qt::Literals::StringLiterals;

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
    , m_am(new ActionManager)
    , m_pm(new PlaylistManager(this))
    , m_fileSystemWatcher(new QFileSystemWatcher(this))
{
    this->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    m_centralLayout = new QVBoxLayout(this);
    // YYC MARK:
    // 72 px is the height of Windows Photo Viewer's bottom line.
    m_centralLayout->setContentsMargins(QMargins(0, 0, 0, 72)); // TODO: This value may be declared as a constant.

    // YYC MARK:
    // Blumis set original value is 350 x 330.
    // It is too small for modern device,
    // so I level it up to standard VGA resolution 640 x 480.
    this->setMinimumSize(640, 480);
    this->setWindowIcon(QIcon(u":/icons/app-icon.svg"_s));
    this->setAcceptDrops(true);

    m_pm->setAllowedSuffixes(supportedImageFormats());

    GraphicsScene * scene = new GraphicsScene(this);

    m_gv = new GraphicsView(this);
    m_gv->setScene(scene);
    m_centralLayout->addWidget(m_gv);

    m_nav = new NavigatorView(this);
    // YYC MARK:
    // Blumia set original value is 220 x 160.
    // So if we set its height to 72 for respecting the height of Windows Photo Viewer,
    // the width should be set to 99 for keeping aspect ratio.
    m_nav->setFixedSize(99, 72);
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
        m_am->actionFitToScreen,
        m_am->actionZoomIn,
        m_am->actionZoomOut,
        m_am->actionPrevPicture,
        m_am->actionProperties,
        m_am->actionNextPicture,
        m_am->actionRotateCounterClockwise,
        m_am->actionRotateClockwise,
        m_am->actionFlipHorizontal,
        m_am->actionToggleCheckerboard,
    }, this);

    m_menu = new QMenu(this);

    {
        m_menu->addAction(m_am->actionCopyPixmap);
        m_menu->addAction(m_am->actionCopyFilePath);

        m_menu->addSeparator();

        m_menu->addAction(m_am->actionPrevPicture);
        m_menu->addAction(m_am->actionNextPicture);

        m_menu->addSeparator();

        m_menu->addAction(m_am->actionActualSize);
        m_menu->addAction(m_am->actionFitToScreen);
        m_menu->addAction(m_am->actionFitByWidth);
        m_menu->addAction(m_am->actionFitByHeight);
        m_menu->addAction(m_am->actionZoomIn);
        m_menu->addAction(m_am->actionZoomOut);

        m_menu->addSeparator();

        m_menu->addAction(m_am->actionFlipHorizontal);
        m_menu->addAction(m_am->actionFlipVertical);
        m_menu->addAction(m_am->actionRotateClockwise);
        m_menu->addAction(m_am->actionRotateCounterClockwise);

        m_menu->addSeparator();

        m_menu->addAction(m_am->actionToggleAvoidResetTransform);
        m_menu->addAction(m_am->actionToggleCheckerboard);
        m_menu->addAction(m_am->actionTogglePauseAnimation);
        m_menu->addAction(m_am->actionAnimationNextFrame);

        m_menu->addSeparator();

        m_menu->addAction(m_am->actionTrash);
        m_menu->addAction(m_am->actionLocateInFileManager);
        m_menu->addAction(m_am->actionProperties);

        m_menu->addSeparator();

        m_menu->addAction(m_am->actionSettings);
        m_menu->addAction(m_am->actionHelp);
    }

    connect(m_pm, &PlaylistManager::playlistChanged, this, std::bind(&MainWindow::updateActionState, this));

    connect(m_pm, &PlaylistManager::playlistChanged, this, std::bind(&MainWindow::galleryCurrent, this, false));
    connect(m_pm, &PlaylistManager::currentIndexChanged, this, std::bind(&MainWindow::galleryCurrent, this, false));

    connect(m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, [this](){
        QTimer::singleShot(500, this, std::bind(&MainWindow::galleryCurrent, this, true));
    });

    QShortcut * fullscreenShorucut = new QShortcut(QKeySequence(QKeySequence::FullScreen), this);
    connect(fullscreenShorucut, &QShortcut::activated,
            this, &MainWindow::toggleFullscreen);

    centerWindow();

    QTimer::singleShot(0, this, [this](){
        m_am->setupShortcuts();
    });

}

MainWindow::~MainWindow()
{

}

void MainWindow::showFiles(const QStringList &files)
{
    m_pm->loadPlaylist(files);
    m_nav->fitInView(m_nav->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::showFiles(const QList<QUrl>& urls)
{
    m_pm->loadPlaylist(urls);
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

void MainWindow::clearGallery()
{
    m_pm->clearPlaylist();
}

void MainWindow::galleryPrev()
{
    const bool loopGallery = Settings::instance()->loopGallery();
    if (!loopGallery && m_pm->isFirstIndex()) return;

    auto index = m_pm->previousIndex();
    if (index.has_value()) {
        m_pm->setCurrentIndex(*index);
    }
}

void MainWindow::galleryNext()
{
    const bool loopGallery = Settings::instance()->loopGallery();
    if (!loopGallery && m_pm->isLastIndex()) return;

    auto index = m_pm->nextIndex();
    if (index.has_value()) {
        m_pm->setCurrentIndex(*index);
    }
}

void MainWindow::galleryCurrent(bool fromFileWatcher)
{
    auto index = m_pm->currentIndex();
    if (index.has_value()) {
        QString localFilePath = m_pm->localFileByIndex(*index);
        m_gv->showFileFromPath(localFilePath);
        if (!fromFileWatcher) {
            updateFileWatcher(localFilePath);
            setWindowTitle(m_pm->urlByIndex(*index).fileName());
        }
    } else {
        m_gv->showText(tr("Drag image here"));
        setWindowTitle(QString());
        updateFileWatcher();
    }

    updateActionState();
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

    return QWidget::showEvent(event);
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
        closeWindow();
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
        QWidget::wheelEvent(event);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    updateWidgetsPosition();

    return QWidget::resizeEvent(event);
}

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    m_menu->exec(mapToGlobal(event->pos()));

    return QWidget::contextMenuEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }

    return QWidget::dragEnterEvent(event);
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
        showFiles(mimeData->urls());
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
                              (72 - m_bottomButtonGroup->height()) / 2 + (height() - 72));
    m_nav->move(width() - m_nav->width(), height() - m_nav->height());
}

void MainWindow::toggleAvoidResetTransform()
{
    m_gv->setAvoidResetTransform(!m_gv->isAvoidResetTransform());
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

// region: Action Trigger

void MainWindow::on_actionCopyPixmap_triggered()
{
    QClipboard *cb = QApplication::clipboard();
    cb->setPixmap(m_gv->scene()->renderToPixmap());
}

void MainWindow::on_actionCopyFilePath_triggered()
{
    auto index = m_pm->currentIndex();
    if (!index.has_value()) return;

    QClipboard *cb = QApplication::clipboard();
    cb->setText(m_pm->localFileByIndex(*index));
}

void MainWindow::on_actionPrevPicture_triggered()
{
    galleryPrev();
}

void MainWindow::on_actionNextPicture_triggered()
{
    galleryNext();
}

void MainWindow::on_actionActualSize_triggered()
{
    m_gv->resetScale();
    m_gv->setEnableAutoFitToScreen(false);
}

void MainWindow::on_actionFitToScreen_triggered()
{
    m_gv->fitInView(m_nav->sceneRect(), Qt::KeepAspectRatio);
    m_gv->setEnableAutoFitToScreen(m_gv->scaleFactor() <= 1);
}

void MainWindow::on_actionFitByWidth_triggered()
{
    m_gv->fitByOrientation(Qt::Orientation::Horizontal);
}

void MainWindow::on_actionFitByHeight_triggered()
{
    m_gv->fitByOrientation(Qt::Orientation::Vertical);
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

void MainWindow::on_actionFlipHorizontal_triggered()
{
    m_gv->flipView(true);
}

void MainWindow::on_actionFlipVertical_triggered()
{
    m_gv->flipView(false);
}

void MainWindow::on_actionRotateClockwise_triggered()
{
    m_gv->rotateView(true);
    m_gv->displayScene();
}

void MainWindow::on_actionRotateCounterClockwise_triggered()
{
    m_gv->rotateView(false);
    m_gv->displayScene();
}

void MainWindow::on_actionToggleAvoidResetTransform_triggered()
{
    toggleAvoidResetTransform();
}

void MainWindow::on_actionToggleCheckerboard_triggered()
{
    m_gv->toggleCheckerboard(QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier));
}

void MainWindow::on_actionTogglePauseAnimation_triggered()
{
    m_gv->scene()->togglePauseAnimation();
}

void MainWindow::on_actionAnimationNextFrame_triggered()
{
    m_gv->scene()->skipAnimationFrame(1);
}

void MainWindow::on_actionTrash_triggered()
{
    auto index = m_pm->currentIndex();
    if (!index.has_value()) return;
    if (!m_pm->urlByIndex(*index).isLocalFile()) return;

    QFile file(m_pm->localFileByIndex(*index));
    QFileInfo fileInfo(file.fileName());

    QMessageBox::StandardButton result = QMessageBox::question(this, tr("Move to Trash"),
                                                               tr("Are you sure you want to move \"%1\" to recycle bin?").arg(fileInfo.fileName()));
    if (result == QMessageBox::Yes) {
        bool succ = file.moveToTrash();
        if (!succ) {
            QMessageBox::warning(this, tr("Failed to move file to trash"),
                                 tr("Move to trash failed, it might caused by file permission issue, file system limitation, or platform limitation."));
        } else {
            m_pm->removeFromPlaylist(*index);
        }
    }
}

void MainWindow::on_actionLocateInFileManager_triggered()
{
    auto index = m_pm->currentIndex();
    if (!index.has_value()) return;
    QUrl currentFileUrl = m_pm->urlByIndex(*index);
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

void MainWindow::on_actionProperties_triggered()
{
    auto index = m_pm->currentIndex();
    if (!index.has_value()) return;
    QUrl currentFileUrl = m_pm->urlByIndex(*index);
    if (!currentFileUrl.isValid()) return;

    MetadataModel * md = new MetadataModel();
    md->setFile(currentFileUrl.toLocalFile());

    MetadataDialog * ad = new MetadataDialog(this);
    ad->setMetadataModel(md);
    ad->exec();
    ad->deleteLater();
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

// endregion

bool MainWindow::updateFileWatcher(const QString &basePath)
{
    m_fileSystemWatcher->removePaths(m_fileSystemWatcher->files());
    if (!basePath.isEmpty()) return m_fileSystemWatcher->addPath(basePath);
    return false;
}

void MainWindow::updateActionState() {
    m_am->updateActionState(m_pm, m_gv);
}
