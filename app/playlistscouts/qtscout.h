// SPDX-FileCopyrightText: 2026 yyc12345 <yyc12321@outlook.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "../playlistscout.h"

namespace PlaylistScout::Qt {
    std::optional<ScoutResult> scout(const QUrl &url, const QStringList &allowedSuffixes);
}
