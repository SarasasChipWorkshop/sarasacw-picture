// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "aboutdialog.h"
#include "ui_aboutdialog.h"

#include <QApplication>
#include <QFile>

using namespace Qt::Literals::StringLiterals;

static QString loadEmbeddedHtml(QString&& path) {
    QFile htmlFile(path);
    bool canOpenFile = htmlFile.open(QIODevice::ReadOnly);
    QByteArray htmlRawContent = canOpenFile ? htmlFile.readAll() : QByteArrayLiteral("");
    return QString::fromUtf8(htmlRawContent);
}

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent), ui(new QT_PREPEND_NAMESPACE(Ui::AboutDialog))
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // blumia: Chain two arg() here since it seems lupdate will remove one of them if we use
    //         the old `arg(QCoreApp::translate(), tr())` way, but it's worth to mention
    //         `arg(QCoreApp::translate(), this->tr())` works, but lupdate will complain about the usage.
    const QString HELP_HTML = loadEmbeddedHtml(u":/plain/help.html"_s)
        .arg(tr("General Help"))
        .arg(tr("Launch application with image file path as argument to load the file."))
        .arg(tr("Passing in a single image file will load all the images in that directory and start viewing from that image."))
        .arg(tr("Passing in multiple image files lets you view them one by one."))
        .arg(tr("Drag and drop image file onto the window is also supported."))
        .arg(tr("None of the operations in this application will alter the pictures on disk."))
        .arg(tr("Context Menu Help"))
        .arg(tr("Keep transformation", "The same meaning of MainWindow's Keep transformation."))
        .arg(tr("Avoid resetting the zoom/rotation/flip state that was applied to the image view when switching between images."))
        .arg(tr("Checkerboard", "The same meaning of MainWindow's Checkerboard."))
        .arg(tr("Checkerboard light and dark theme can be adjusted in the configuration window, or easily switch it by holding Shift and clicking the button."));

    const QString APP_NAME_ARG = qApp->applicationDisplayName();
    const QString APP_VERSION_ARG =
#ifdef GIT_DESCRIBE_VERSION_STRING
        GIT_DESCRIBE_VERSION_STRING;
#else
        qApp->applicationVersion();
#endif // GIT_DESCRIBE_VERSION_STRING
    const QString QT_VERSION_ARG = QT_VERSION_STR;
    const QString QT_ARCH_ARG = QSysInfo::buildCpuArchitecture();
    const QString ABOUT_HTML = loadEmbeddedHtml(u":/plain/about.html"_s)
        .arg(APP_NAME_ARG)
        .arg(tr("Version: %1").arg(APP_VERSION_ARG))
        .arg(tr("Upstream author"))
        .arg(tr("Hard fork author"))
        .arg(tr("Logo designer"))
        .arg(tr("Built with Qt %1 (%2)").arg(QT_VERSION_ARG).arg(QT_ARCH_ARG))
        .arg(tr("Source code"));

    const QString SPECIAL_THANKS_HTML = loadEmbeddedHtml(u":/plain/special-thanks.html"_s)
        .arg(tr("Contributors"))
        .arg(tr("List of contributors on GitHub"))
        .arg(tr("Thanks to all people who contributed to this project."))
        .arg(tr("Translators"))
        .arg(tr("I would like to thank the following people who volunteered to translate this application."));

    const QString LICENSE_HTML = loadEmbeddedHtml(u":/plain/license.html"_s)
        .arg(tr("License"))
        .arg(tr("This application is released under the MIT License."))
        .arg(tr("License Verbatim Copy"));

    const QString THIRD_PARTY_LIBS_HTML = loadEmbeddedHtml(u":/plain/third-party-libs.html"_s)
        .arg(tr("Third-party Libraries"))
        .arg(tr("This application is built on the following free software libraries.", "Free as in freedom"))
        .arg(tr("Some of them are optional and might not be included with this distribution depending on the build environment."));

    ui->m_helpTextEdit->setHtml(HELP_HTML);
    ui->m_aboutTextEdit->setHtml(ABOUT_HTML);
    ui->m_specialThanksTextEdit->setHtml(SPECIAL_THANKS_HTML);
    ui->m_licenseTextEdit->setHtml(LICENSE_HTML);
    ui->m_3rdPartyLibsTextEdit->setHtml(THIRD_PARTY_LIBS_HTML);

    connect(ui->m_buttonBox, QOverload<QAbstractButton *>::of(&QDialogButtonBox::clicked), this, [this](QAbstractButton * btn){
        Q_UNUSED(btn)
        this->close();
    });

    setMinimumSize(361, 161); // not sure why it complain "Unable to set geometry"
}

AboutDialog::~AboutDialog()
{
    delete ui;
}

QSize AboutDialog::sizeHint() const
{
    return QSize(520, 350);
}
