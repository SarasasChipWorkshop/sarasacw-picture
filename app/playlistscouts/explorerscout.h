// SPDX-FileCopyrightText: 2026 yyc12345 <yyc12321@outlook.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "../playlistscout.h"
#include <QtSystemDetection>

#ifdef Q_OS_WIN

namespace PlaylistScout::Explorer {
    std::optional<ScoutResult> scout(const QUrl &url, const QStringList &allowedSuffixes);
}

#endif // Q_OS_WIN
