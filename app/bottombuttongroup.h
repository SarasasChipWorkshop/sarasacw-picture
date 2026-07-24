// SPDX-FileCopyrightText: 2022 Gary Wang <wzc782970009@gmail.com>
//
// SPDX-License-Identifier: MIT

#ifndef BOTTOMBUTTONGROUP_H
#define BOTTOMBUTTONGROUP_H

#include <vector>

#include <QAbstractButton>
#include <QGroupBox>

class BottomButtonGroup : public QGroupBox
{
    Q_OBJECT
public:
    explicit BottomButtonGroup(QWidget *parent = nullptr);

    void addActions(const std::vector<QAction *> & actionList);
    void addAction(QAction* action);
};

#endif // BOTTOMBUTTONGROUP_H
