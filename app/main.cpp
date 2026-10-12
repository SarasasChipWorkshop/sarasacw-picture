// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#include "mainwindow.h"

#include "settings.h"

#ifdef Q_OS_MACOS
#include "fileopeneventhandler.h"
#endif // Q_OS_MACOS

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QTranslator>
#include <QUrl>

using namespace Qt::Literals::StringLiterals;

int main(int argc, char *argv[])
{
    QCoreApplication::setApplicationName(u"SCW Photo Viewer"_s);
    QCoreApplication::setApplicationVersion(SCWPIC_VERSION_STRING);
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Settings::instance()->hiDpiScaleFactorBehavior());

    QApplication a(argc, argv);

    QTranslator translator;
#if defined(TRANSLATION_RESOURCE_EMBEDDING)
    const QString qmDir = u":/i18n/"_s;
#elif defined(QM_FILE_INSTALL_ABSOLUTE_DIR)
    const QString qmDir = QT_STRINGIFY(QM_FILE_INSTALL_ABSOLUTE_DIR);
#else
    const QString qmDir = QDir(QCoreApplication::applicationDirPath()).absoluteFilePath("translations");
#endif
    if (translator.load(QLocale(), u"SarasaCWPicture"_s, u"_"_s, qmDir)) {
        QCoreApplication::installTranslator(&translator);
    }

    QGuiApplication::setApplicationDisplayName(QCoreApplication::translate("main", "SCW Photo Viewer"));

    // commandline options
    QCommandLineOption supportedImageFormats(u"supported-image-formats"_s, QCoreApplication::translate("main", "List supported image format suffixes, and quit program."));
    // parse commandline arguments
    QCommandLineParser parser;
    parser.addOption(supportedImageFormats);
    parser.addPositionalArgument("File list", QCoreApplication::translate("main", "File list."));
    parser.addHelpOption();
    parser.process(a);

    if (parser.isSet(supportedImageFormats)) {
#if QT_VERSION < QT_VERSION_CHECK(6, 9, 0)
        fputs(qPrintable(MainWindow::supportedImageFormats().join(QChar('\n'))), stdout);
        ::exit(EXIT_SUCCESS);
#else
        QCommandLineParser::showMessageAndExit(QCommandLineParser::MessageType::Information,
                                               MainWindow::supportedImageFormats().join(QChar('\n')));
#endif
    }

    MainWindow w;
    w.show();

#ifdef Q_OS_MACOS
    FileOpenEventHandler * fileOpenEventHandler = new FileOpenEventHandler(&a);
    a.installEventFilter(fileOpenEventHandler);
    a.connect(fileOpenEventHandler, &FileOpenEventHandler::fileOpen, [&w](const QUrl & url){
        if (w.isHidden()) {
            w.showNormal();
        } else {
            w.activateWindow();
        }
        w.showFiles({url});
        w.initWindowSize();
    });

    // Handle dock icon clicks to show hidden window
    a.connect(&a, &QApplication::applicationStateChanged, [&w](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive && w.isHidden()) {
            // YYC MARK: 
            // We can't use `{}` because there is an ambiguity between 2 overloads:
            // `void showFiles(const QStringList& files);` and `void showFiles(const QList<QUrl>& urls);`.
            QStringList emptyFileList;
            w.showFiles(emptyFileList);
            w.galleryCurrent(false);
            w.showNormal();
            w.raise();
            w.activateWindow();
        }
    });
#endif // Q_OS_MACOS

    QStringList fileList = parser.positionalArguments();
    w.showFiles(fileList);

    w.initWindowSize();

    return QApplication::exec();
}
