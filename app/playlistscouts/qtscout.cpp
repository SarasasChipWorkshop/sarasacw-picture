// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
// SPDX-FileCopyrightText: 2026 yyc12345 <yyc12321@outlook.com>
//
// SPDX-License-Identifier: MIT

#include "qtscout.h"

#include <QFileInfo>
#include <QDir>
#include <QCollator>

std::optional<PlaylistScout::ScoutResult> PlaylistScout::Qt::scout(
    const QUrl &url, const QStringList &allowedSuffixes) {
    PlaylistScout::ScoutResult rv;

    QFileInfo info(url.toLocalFile());
    QDir dir(info.path());
    QString currentFileName = info.fileName();

    QStringList entryList = dir.entryList(
        allowedSuffixes, QDir::Files | QDir::NoSymLinks, QDir::NoSort);

    QCollator collator;
    collator.setNumericMode(true);

    std::sort(entryList.begin(), entryList.end(), collator);

    for (qsizetype i = 0; i < entryList.count(); i++) {
        const QString &fileName = entryList.at(i);
        const QString &oneEntry = dir.absoluteFilePath(fileName);
        const QUrl &url = QUrl::fromLocalFile(oneEntry);

        rv.playlist.append(url);
        if (fileName == currentFileName) {
            rv.indexHint = i;
        }
    }

    return rv;
}
