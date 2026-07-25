// SPDX-FileCopyrightText: 2026 yyc12345 <yyc12321@outlook.com>
//
// SPDX-License-Identifier: MIT

#include "playlistscout.h"

#include "playlistscouts/qtscout.h"
#include "playlistscouts/explorerscout.h"

std::optional<PlaylistScout::ScoutResult> PlaylistScout::scout(
    const QUrl &url, const QStringList &allowedSuffixes) {

#ifdef Q_OS_WIN
    // In Windows, Qt approach can not aware Windows Explorer custom sort order.
    // So we need to try using Win32 functions for fetching it at first.
    if (auto rv = PlaylistScout::Explorer::scout(url, allowedSuffixes); rv.has_value()) {
        return rv;
    }
#endif // Q_OS_WIN

    // TODO: We may add macOS Finder, KDE Dolphin and etc supports in future.

    // Qt way is the last fallback.
    return PlaylistScout::Qt::scout(url, allowedSuffixes);
}
