// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#include "bottombuttongroup.h"

#include <QToolButton>
#include <QVBoxLayout>
#include <QDebug>

BottomButtonGroup::BottomButtonGroup(QWidget *parent)
    : QGroupBox (parent)
{
    QHBoxLayout * mainLayout = new QHBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    this->setLayout(mainLayout);
    this->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    this->setStyleSheet(R"#(
BottomButtonGroup {
    border-style: none;
    background: transparent;
}
QToolButton {
    background: transparent;
    border-style: none;
}
)#");
}

void BottomButtonGroup::addActions(const std::vector<QAction *> & actionList) {
    for (QAction * action : actionList) {
        // Create button
        QToolButton * btn = new QToolButton(this);
        btn->setDefaultAction(action);
        btn->setIconSize(QSize(32, 32));
        btn->setFixedSize(40, 40);
        // Add button
        layout()->addWidget(btn);
    }
    updateGeometry();
}

void BottomButtonGroup::addAction(QAction* action) {
    addActions({action});
}
