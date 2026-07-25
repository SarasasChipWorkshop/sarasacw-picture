// SPDX-FileCopyrightText: 2026 yyc12345 <yyc12321@outlook.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QUrl>
#include <QStringList>
#include <QList>
#include <optional>

namespace PlaylistScout {
    /// The result of scout
    struct ScoutResult {
        /// The final fetched playlist (it can be empty).
        /// Each item must be a QUrl pointing to a local file.
        QList<QUrl> playlist;
        /// The index of given file located in playlist.
        /// Nullopt if it is not presented in playlist.
        std::optional<qsizetype> indexHint;
    };

    /// @brief Scout available image files in given file's parent dir with given allowed suffixes.
    /// @param[in] url The path to file whose parent dir is our target dir for finding image files.
    /// @param[in] allowedSuffixes 
    /// Each items in this list is a wildcard pattern for matching allowed file name, like \c *.png .
    /// The fetched item will be preserved if its name matching any of them.
    /// If this list is empty, there is no restrictions for finding files.
    /// @return Scout result if success, otherwise nullopt.
    std::optional<ScoutResult> scout(const QUrl &url, const QStringList &allowedSuffixes);
}
