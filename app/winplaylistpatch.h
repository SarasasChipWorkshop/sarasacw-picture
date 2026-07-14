// SPDX-FileCopyrightText: 2026 yyc12345 <yyc12321@outlook.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QtSystemDetection>

#ifdef Q_OS_WIN

#include <QUrl>
class WinPlaylistPatch
{
public:
    static bool loadSingleFilePlaylist(const QUrl &url,
                                       const QStringList &allowedSuffixes,
                                       QList<QUrl> &playlist,
                                       std::optional<qsizetype> &indexHint);
};

#endif // Q_OS_WIN
