// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

#include "settings.h"

#include <QApplication>
#include <QTimer>
#include <QStringListModel>
#include <type_traits>

template <typename E, typename = std::enable_if_t<std::is_enum_v<E>>>
class EnumComboBoxTransformer {
public:
    using Pair = QPair<E, QString>;
    using Pairs = QList<QPair<E, QString>>;

    EnumComboBoxTransformer(Pairs &&pairs) : pairs(std::move(pairs)) {}
    ~EnumComboBoxTransformer() = default;
    Q_DISABLE_COPY_MOVE(EnumComboBoxTransformer)

    /// Build Qt model used for QComboBox
    QStringListModel* buildComboBoxModel(QObject *parent = nullptr) const {
        QStringList dropDownEntries;
        for (const auto& pair : std::as_const(this->pairs)) {
            dropDownEntries.append(pair.second);
        }
        return new QStringListModel(dropDownEntries, parent);
    }
    /// Convert QComboBox index to enum value.
    E to_value(int index) const {
        return this->pairs.at(static_cast<qsizetype>(index)).first;
    }
    /// Convert enum value to QComboBox index.
    int to_index(E value) const {
        for (qsizetype i = 0; i < pairs.size(); ++i) {
            if (pairs.at(i).first == value) {
                return static_cast<int>(i);
            }
        }
        // Value not found
        throw std::invalid_argument("invalid enum value for finding index");
    }

private:
    Pairs pairs;
};

static EnumComboBoxTransformer<Settings::DoubleClickBehavior> DC_OPTIONS {
    {
        {Settings::DoubleClickBehavior::Ignore, QCoreApplication::translate("SettingsDialog", "Do nothing")},
        {Settings::DoubleClickBehavior::Close, QCoreApplication::translate("SettingsDialog", "Close the window")},
        {Settings::DoubleClickBehavior::Maximize, QCoreApplication::translate("SettingsDialog", "Toggle maximize")},
        {Settings::DoubleClickBehavior::FullScreen, QCoreApplication::translate("SettingsDialog", "Toggle fullscreen")}
    }
};

static EnumComboBoxTransformer<Settings::MouseWheelBehavior, QString> MW_OPTIONS {
    {
        { Settings::MouseWheelBehavior::Zoom, QCoreApplication::translate("SettingsDialog", "Zoom in and out") },
        { Settings::MouseWheelBehavior::Switch, QCoreApplication::translate("SettingsDialog", "View next or previous item") }
    }
};

static EnumComboBoxTransformer<Settings::WindowSizeBehavior, QString> IWS_OPTIONS {
    {
        { Settings::WindowSizeBehavior::Auto, QCoreApplication::translate("SettingsDialog", "Auto size") },
        { Settings::WindowSizeBehavior::Maximized, QCoreApplication::translate("SettingsDialog", "Maximized") },
        { Settings::WindowSizeBehavior::Windowed, QCoreApplication::translate("SettingsDialog", "Windowed") }
    }
};

static EnumComboBoxTransformer<Qt::HighDpiScaleFactorRoundingPolicy, QString> HIDPI_OPTIONS {
    {
        { Qt::HighDpiScaleFactorRoundingPolicy::Round, QCoreApplication::translate("SettingsDialog", "Round (Integer scaling)", "This option means round up for .5 and above") },
        { Qt::HighDpiScaleFactorRoundingPolicy::Ceil, QCoreApplication::translate("SettingsDialog", "Ceil (Integer scaling)", "This option means always round up") },
        { Qt::HighDpiScaleFactorRoundingPolicy::Floor, QCoreApplication::translate("SettingsDialog", "Floor (Integer scaling)", "This option means always round down") },
        { Qt::HighDpiScaleFactorRoundingPolicy::PassThrough, QCoreApplication::translate("SettingsDialog", "Follow system (Fractional scaling)", "This option means don't round") }
    }
};

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent), ui(new QT_PREPEND_NAMESPACE(Ui::SettingsDialog))
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    auto* settings = Settings::instance();
    ui->m_useLightCheckerboard->setChecked(settings->useLightCheckerboard());
    ui->m_loopGallery->setChecked(settings->loopGallery());
    ui->m_svgTiny12Only->setChecked(settings->svgTiny12Only());
    ui->m_doubleClickBehavior->setModel(DC_OPTIONS.buildComboBoxModel(this));
    Settings::DoubleClickBehavior dcb = settings->doubleClickBehavior();
    ui->m_doubleClickBehavior->setCurrentIndex(DC_OPTIONS.to_index(dcb));
    ui->m_mouseWheelBehavior->setModel(MW_OPTIONS.buildComboBoxModel(this));
    Settings::MouseWheelBehavior mwb = settings->mouseWheelBehavior();
    ui->m_mouseWheelBehavior->setCurrentIndex(MW_OPTIONS.to_index(mwb));
    ui->m_initWindowSizeBehavior->setModel(IWS_OPTIONS.buildComboBoxModel(this));
    Settings::WindowSizeBehavior iwsb = settings->initWindowSizeBehavior();
    ui->m_initWindowSizeBehavior->setCurrentIndex(IWS_OPTIONS.to_index(iwsb));
    ui->m_hiDpiRoundingPolicyBehavior->setModel(HIDPI_OPTIONS.buildComboBoxModel(this));
    Qt::HighDpiScaleFactorRoundingPolicy hidpi = settings->hiDpiScaleFactorBehavior();
    ui->m_hiDpiRoundingPolicyBehavior->setCurrentIndex(HIDPI_OPTIONS.to_index(hidpi));

#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
# define QCHECKBOX_CHECKSTATECHANGED QCheckBox::checkStateChanged
# define QT_CHECKSTATE Qt::CheckState
#else
# define QCHECKBOX_CHECKSTATECHANGED QCheckBox::stateChanged
# define QT_CHECKSTATE int
#endif // QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)

    connect(ui->m_useLightCheckerboard, &QCHECKBOX_CHECKSTATECHANGED, this, [ = ](QT_CHECKSTATE state){
        Settings::instance()->setUseLightCheckerboard(state == Qt::Checked);
    });

    connect(ui->m_loopGallery, &QCHECKBOX_CHECKSTATECHANGED, this, [ = ](QT_CHECKSTATE state){
        Settings::instance()->setLoopGallery(state == Qt::Checked);
    });

    connect(ui->m_svgTiny12Only, &QCHECKBOX_CHECKSTATECHANGED, this, [ = ](QT_CHECKSTATE state){
        Settings::instance()->setSvgTiny12Only(state == Qt::Checked);
    });

    connect(ui->m_doubleClickBehavior, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [ = ](int index){
        Settings::instance()->setDoubleClickBehavior(DC_OPTIONS.to_value(index));
    });

    connect(ui->m_mouseWheelBehavior, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [ = ](int index){
        Settings::instance()->setMouseWheelBehavior(MW_OPTIONS.to_value(index));
    });

    connect(ui->m_initWindowSizeBehavior, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [ = ](int index){
        Settings::instance()->setInitWindowSizeBehavior(IWS_OPTIONS.to_value(index));
    });

    connect(ui->m_hiDpiRoundingPolicyBehavior, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [ = ](int index){
        Settings::instance()->setHiDpiScaleFactorBehavior(HIDPI_OPTIONS.to_value(index));
    });

    // YYC MARK:
    // Use adjustSize() to make sure all ComboBox are expanded by its items.
    // Because items are added after inserting them into layout.
    // It is inviable to call this function in ctor because this relys on Qt event system.
    // It only can be done by QTimer::singleShot().
    QTimer::singleShot(0, this, [this](){
        this->adjustSize();
    });
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}
