// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#include "metadatadialog.h"
#include "ui_metadatadialog.h"

#include <QDialogButtonBox>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QHeaderView>

#include "metadatamodel.h"

class PropertyTreeView : public QTreeView
{
public:
    explicit PropertyTreeView(QWidget* parent) : QTreeView(parent) {}
    ~PropertyTreeView() {}

protected:
    void rowsInserted(const QModelIndex& parent, int start, int end) override
    {
        QTreeView::rowsInserted(parent, start, end);
        if (!parent.isValid()) {
            // we are inserting a section group
            for (int row = start; row <= end; ++row) {
                setupSection(row);
            }
        } else {
            // we are inserting a property
            setRowHidden(parent.row(), QModelIndex(), false);
        }
    }

    void reset() override
    {
        QTreeView::reset();
        if (model()) {
            for (int row = 0; row < model()->rowCount(); ++row) {
                setupSection(row);
            }
        }
    }

private:
    void setupSection(int row)
    {
        expand(model()->index(row, 0));
        setFirstColumnSpanned(row, QModelIndex(), true);
        setRowHidden(row, QModelIndex(), !model()->hasChildren(model()->index(row, 0)));
    }
};

class PropertyTreeItemDelegate : public QStyledItemDelegate
{
public:
    PropertyTreeItemDelegate(QObject* parent)
        : QStyledItemDelegate(parent)
    {}

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override
    {
        QStyleOptionViewItem opt = option;
        if (!index.parent().isValid()) {
            opt.font.setBold(true);
            opt.features.setFlag(QStyleOptionViewItem::Alternate);
        }
        QStyledItemDelegate::paint(painter, opt, index);
    }
};

MetadataDialog::MetadataDialog(QWidget *parent)
    : QDialog(parent), ui(new QT_PREPEND_NAMESPACE(Ui::MetadataDialog))
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // YYC MARK:
    // Because we use custom QTreeView and we do not want to expose it to public domain.
    // So I have to use this way as the compromise.

    // Create our custom QTreeView and insert them
    m_treeView = new PropertyTreeView(this);
    auto* layout = ui->verticalLayout;
    const int index = layout->indexOf(ui->m_buttonBox);
    layout->insertWidget(index, m_treeView);

    // Setup our custom QTreeView
    m_treeView->setRootIsDecorated(false);
    m_treeView->setIndentation(0);
    m_treeView->setItemDelegate(new PropertyTreeItemDelegate(m_treeView));
    m_treeView->header()->resizeSection(0, sizeHint().width() / 2);

    connect(ui->m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::close);
}

MetadataDialog::~MetadataDialog()
{
    delete ui;
}

void MetadataDialog::setMetadataModel(MetadataModel * model)
{
    m_treeView->setModel(model);
}

QSize MetadataDialog::sizeHint() const
{
    return QSize(520, 350);
}
