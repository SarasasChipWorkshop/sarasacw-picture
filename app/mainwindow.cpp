// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    : QWidget(parent)
    , ui(new QT_PREPEND_NAMESPACE(Ui::MainWindow))
    , m_am(new ActionManager)
    , m_pm(new PlaylistManager(this))
    , m_fileSystemWatcher(new QFileSystemWatcher(this))
{
    ui->setupUi(this);
    this->setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    // YYC MARK:
    // Blumis set original value is 350 x 330.
    // It is too small for modern device,
    // so I level it up to SVGA resolution 800 x 600.
    this->setMinimumSize(800, 600);
    this->setWindowIcon(QIcon(u":/icons/app-icon.svg"_s));
    this->setAcceptDrops(true);

    // Initialize playlist manager
    m_pm->setAllowedSuffixes(supportedImageFormats());

    // Initialize graphics scene
    GraphicsScene * scene = new GraphicsScene(this);
    // Setup graphics view (main photo view)
    ui->m_gv->setScene(scene);
    // Setup navigator view (right-bottom corner thumbnail view)
    ui->m_nav->setScene(scene);
    ui->m_nav->setMainView(ui->m_gv);
    ui->m_nav->fitInView(ui->m_nav->sceneRect(), Qt::KeepAspectRatio);
    // Binding graphics and navigator views signals
    connect(ui->m_gv, &GraphicsView::navigatorViewRequired,
            this, [this](bool required, const QTransform & tf){
        ui->m_nav->setTransform(GraphicsView::resetScale(tf));
        ui->m_nav->fitInView(ui->m_nav->sceneRect(), Qt::KeepAspectRatio);
        ui->m_nav->setVisible(required);
        ui->m_navPH->setVisible(required);
        ui->m_nav->updateMainViewportRegion();
    });
    connect(ui->m_gv, &GraphicsView::viewportRectChanged,
            ui->m_nav, &NavigatorView::updateMainViewportRegion);

    // Setup action manager
    m_am->setupAction(this);
    // Setup bottom button group
    ui->m_bottomButtonGroup->addActions({
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
    });
    // Setup right click menus
    {
        ui->m_subMenuCopy->addAction(m_am->actionCopyPixmap);
        ui->m_subMenuCopy->addAction(m_am->actionCopyFilePath);
    }
    {
        ui->m_subMenuTransform->addAction(m_am->actionActualSize);
        ui->m_subMenuTransform->addAction(m_am->actionFitToScreen);
        ui->m_subMenuTransform->addAction(m_am->actionFitByWidth);
        ui->m_subMenuTransform->addAction(m_am->actionFitByHeight);
        ui->m_subMenuTransform->addAction(m_am->actionZoomIn);
        ui->m_subMenuTransform->addAction(m_am->actionZoomOut);

        ui->m_subMenuTransform->addSeparator();

        ui->m_subMenuTransform->addAction(m_am->actionFlipHorizontal);
        ui->m_subMenuTransform->addAction(m_am->actionFlipVertical);
        ui->m_subMenuTransform->addAction(m_am->actionRotateClockwise);
        ui->m_subMenuTransform->addAction(m_am->actionRotateCounterClockwise);
    }
    {
        ui->m_menu->addMenu(ui->m_subMenuCopy);

        ui->m_menu->addSeparator();

        ui->m_menu->addAction(m_am->actionPrevPicture);
        ui->m_menu->addAction(m_am->actionNextPicture);

        ui->m_menu->addSeparator();

        ui->m_menu->addMenu(ui->m_subMenuTransform);

        ui->m_menu->addSeparator();

        ui->m_menu->addAction(m_am->actionToggleAvoidResetTransform);
        ui->m_menu->addAction(m_am->actionToggleCheckerboard);
        ui->m_menu->addAction(m_am->actionTogglePauseAnimation);
        ui->m_menu->addAction(m_am->actionAnimationNextFrame);

        ui->m_menu->addSeparator();

        ui->m_menu->addAction(m_am->actionTrash);
        ui->m_menu->addAction(m_am->actionLocateInFileManager);
        ui->m_menu->addAction(m_am->actionProperties);

        ui->m_menu->addSeparator();

        ui->m_menu->addAction(m_am->actionSettings);
        ui->m_menu->addAction(m_am->actionHelp);
    }

    // Connect playlist manager and file watcher signals for photo browsering.
    connect(m_pm, &PlaylistManager::playlistChanged, this, std::bind(&MainWindow::updateActionState, this));

    connect(m_pm, &PlaylistManager::playlistChanged, this, std::bind(&MainWindow::galleryCurrent, this, false));
    connect(m_pm, &PlaylistManager::currentIndexChanged, this, std::bind(&MainWindow::galleryCurrent, this, false));

    connect(m_fileSystemWatcher, &QFileSystemWatcher::fileChanged, this, [this](){
        QTimer::singleShot(500, this, std::bind(&MainWindow::galleryCurrent, this, true));
    });

    // Change window location
    centerWindow();

    QTimer::singleShot(0, this, [this](){
        m_am->setupShortcuts();
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::showFiles(const QStringList &files)
{
    m_pm->loadPlaylist(files);
    ui->m_nav->fitInView(ui->m_nav->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::showFiles(const QList<QUrl>& urls)
{
    m_pm->loadPlaylist(urls);
    ui->m_nav->fitInView(ui->m_nav->sceneRect(), Qt::KeepAspectRatio);
}

void MainWindow::initWindowSize()
{
    switch (Settings::instance()->initWindowSizeBehavior()) {
    case Settings::WindowSizeBehavior::Maximized:
        showMaximized();
        break;
    case Settings::WindowSizeBehavior::Windowed:
    default:
        showNormal();
        break;
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
        ui->m_gv->showFileFromPath(localFilePath);
        if (!fromFileWatcher) {
            updateFileWatcher(localFilePath);
            setWindowTitle(m_pm->urlByIndex(*index).fileName());
        }
    } else {
        ui->m_gv->showText(tr("Drag image here"));
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

void MainWindow::contextMenuEvent(QContextMenuEvent *event)
{
    ui->m_menu->exec(mapToGlobal(event->pos()));

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
        ui->m_gv->showText(tr("Not supported mimedata: %1").arg(mimeData->formats().first()));
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

void MainWindow::toggleAvoidResetTransform()
{
    ui->m_gv->setAvoidResetTransform(!ui->m_gv->isAvoidResetTransform());
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
    cb->setPixmap(ui->m_gv->scene()->renderToPixmap());
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
    ui->m_gv->resetScale();
    ui->m_gv->setEnableAutoFitToScreen(false);
}

void MainWindow::on_actionFitToScreen_triggered()
{
    ui->m_gv->fitInView(ui->m_nav->sceneRect(), Qt::KeepAspectRatio);
    ui->m_gv->setEnableAutoFitToScreen(ui->m_gv->scaleFactor() <= 1);
}

void MainWindow::on_actionFitByWidth_triggered()
{
    ui->m_gv->fitByOrientation(Qt::Orientation::Horizontal);
}

void MainWindow::on_actionFitByHeight_triggered()
{
    ui->m_gv->fitByOrientation(Qt::Orientation::Vertical);
}

void MainWindow::on_actionZoomIn_triggered()
{
    if (ui->m_gv->scaleFactor() < 1000) {
        ui->m_gv->zoomView(1.25);
    }
}

void MainWindow::on_actionZoomOut_triggered()
{
    ui->m_gv->zoomView(0.8);
}

void MainWindow::on_actionFlipHorizontal_triggered()
{
    ui->m_gv->flipView(true);
}

void MainWindow::on_actionFlipVertical_triggered()
{
    ui->m_gv->flipView(false);
}

void MainWindow::on_actionRotateClockwise_triggered()
{
    ui->m_gv->rotateView(true);
    ui->m_gv->displayScene();
}

void MainWindow::on_actionRotateCounterClockwise_triggered()
{
    ui->m_gv->rotateView(false);
    ui->m_gv->displayScene();
}

void MainWindow::on_actionToggleAvoidResetTransform_triggered()
{
    toggleAvoidResetTransform();
}

void MainWindow::on_actionToggleCheckerboard_triggered()
{
    ui->m_gv->toggleCheckerboard(QGuiApplication::queryKeyboardModifiers().testFlag(Qt::ShiftModifier));
}

void MainWindow::on_actionTogglePauseAnimation_triggered()
{
    ui->m_gv->scene()->togglePauseAnimation();
}

void MainWindow::on_actionAnimationNextFrame_triggered()
{
    ui->m_gv->scene()->skipAnimationFrame(1);
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
    m_am->updateActionState(m_pm, ui->m_gv);
}
