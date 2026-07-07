// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#ifndef METADATADIALOG_H
#define METADATADIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
class QTreeView;
QT_END_NAMESPACE

QT_BEGIN_NAMESPACE
namespace Ui {
class MetadataDialog;
}
QT_END_NAMESPACE

class MetadataModel;
class MetadataDialog : public QDialog
{
    Q_OBJECT
public:
    explicit MetadataDialog(QWidget * parent);
    ~MetadataDialog() override;

    void setMetadataModel(MetadataModel * model);

    QSize sizeHint() const override;

private:
    QTreeView * m_treeView = nullptr;
    QT_PREPEND_NAMESPACE(Ui::MetadataDialog) *ui;
};

#endif // METADATADIALOG_H
