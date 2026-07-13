// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#include "playlistmanager.h"

#include <QCollator>
#include <QDir>
#include <QFileInfo>
#include <QUrl>

PlaylistManager::PlaylistManager(QObject *parent)
    : QObject(parent)
    , m_currentIndex(std::nullopt)
    , m_playlist()
    , m_allowedSuffixes()
{

}

PlaylistManager::~PlaylistManager()
{

}

// region: Allowed Suffixes Operations

const QStringList& PlaylistManager::allowedSuffixes() const {
    return m_allowedSuffixes;
}

void PlaylistManager::setAllowedSuffixes(const QStringList& nameFilters) {
    m_allowedSuffixes = nameFilters;
    emit allowedSuffixesChanged(m_allowedSuffixes);
}

// endregion

// region: Playlist Loader and Reset

bool PlaylistManager::loadPlaylist(const QStringList& files) {
    // Convert string list to url list first
    QList<QUrl> urls;
    for (const QString & file : std::as_const(files)) {
        urls.append(urlFromString(file));
    }

    return loadPlaylist(urls);
}

bool PlaylistManager::loadPlaylist(const QList<QUrl>& urls) {
    // Check its count, and check M3U8 playlist for single file case.
    if (urls.isEmpty()) {
        clearPlaylist();
        return true;
    } else if (urls.count() == 1) {
        const QUrl& firstUrl = urls.first();
        const QString lowerCaseUrlPath(firstUrl.path().toLower());
        if (lowerCaseUrlPath.endsWith(".m3u8") || lowerCaseUrlPath.endsWith(".m3u")) {
            return loadM3U8Playlist(firstUrl);
        } else {
            return loadSingleFilePlaylist(firstUrl);
        }
    } else {
        return loadMultipleFilesPlaylist(std::move(urls));
    }
}

bool PlaylistManager::loadSingleFilePlaylist(const QUrl& url) {
    // Check URL at first
    if (!checkUrl(url))
        return false;

    QFileInfo info(url.toLocalFile());
    QDir dir(info.path());
    QString currentFileName = info.fileName();

    QStringList entryList = dir.entryList(
        m_allowedSuffixes, QDir::Files | QDir::NoSymLinks, QDir::NoSort);

    QCollator collator;
    collator.setNumericMode(true);

    std::sort(entryList.begin(), entryList.end(), collator);

    QList<QUrl> playlist;
    std::optional<qsizetype> idx = std::nullopt;
    for (qsizetype i = 0; i < entryList.count(); i++) {
        const QString &fileName = entryList.at(i);
        const QString &oneEntry = dir.absoluteFilePath(fileName);
        const QUrl &url = QUrl::fromLocalFile(oneEntry);
        if (!checkUrl(url)) {
            return false;
        }
        playlist.append(url);
        if (fileName == currentFileName) {
            idx = i;
        }
    }

    setPlaylist(std::move(playlist), idx);
    return true;
}

bool PlaylistManager::loadMultipleFilesPlaylist(const QList<QUrl>&& urls) {
    // Check all of them are valid
    for (const auto& url : std::as_const(urls)) {
        if (!checkUrl(url)) {
            return false;
        }
    }

    // Set playlist and return
    setPlaylist(std::move(urls));
    return true;
}

bool PlaylistManager::loadM3U8Playlist(const QUrl& url) {
    // Check URL at first
    if (!checkUrl(url))
        return false;

    QList<QUrl> urls;
    QFile file(url.toLocalFile());
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!file.atEnd()) {
            QString line = file.readLine();
            if (line.startsWith('#')) {
                continue;
            }
            QFileInfo fileInfo(file);
            QUrl item = QUrl::fromUserInput(line, fileInfo.absolutePath());
            if (!checkUrl(item)) {
                return false;
            }
            urls.append(item);
        }
    } else {
        return false;
    }

    setPlaylist(std::move(urls));
    return true;
}

void PlaylistManager::setPlaylist(const QList<QUrl>&& urls, std::optional<qsizetype> indexHint) {
    // Setup playlist
    m_playlist = std::move(urls);

    // Set index with hint
    if (indexHint.has_value()) {
        m_currentIndex = *indexHint;
    }
    sanitizeIndex();

    // Raise signal
    emit playlistChanged(totalCount());
}

void PlaylistManager::clearPlaylist() {
    m_playlist.clear();
    sanitizeIndex();
    emit playlistChanged(this->totalCount());
}

// endregion

// region: Playlist Operations

void PlaylistManager::appendToPlaylist(const QString &file)
{
    return appendToPlaylist(urlFromString(file));
}

void PlaylistManager::appendToPlaylist(const QUrl &url)
{
    m_playlist.append(url);
    sanitizeIndex();
    emit playlistChanged(totalCount());
}

bool PlaylistManager::insertIntoPlaylist(qsizetype index, const QString &file)
{
    return insertIntoPlaylist(index, urlFromString(file));
}

bool PlaylistManager::insertIntoPlaylist(qsizetype index, const QUrl &url)
{
    if (index < 0 || index > totalCount()) {
        return false;
    } else {
        m_playlist.insert(index, url);
        sanitizeIndex();
        emit playlistChanged(totalCount());
        return true;
    }
}

bool PlaylistManager::removeFromPlaylist(qsizetype index)
{
    if (index < 0 || index >= totalCount()) {
        return false;
    } else {
        m_playlist.removeAt(index);
        sanitizeIndex();
        emit playlistChanged(totalCount());
        return true;
    }
}

// endregion

// region: Index Operations

qsizetype PlaylistManager::totalCount() const
{
    return m_playlist.count();
}

std::optional<qsizetype> PlaylistManager::previousIndex() const
{
    if (!m_currentIndex.has_value() || totalCount() <= 0) return std::nullopt;
    return (isFirstIndex() ? totalCount() - 1 : *m_currentIndex - 1);
}

std::optional<qsizetype> PlaylistManager::nextIndex() const
{
    if (!m_currentIndex.has_value() || totalCount() <= 0) return std::nullopt;
    return (isLastIndex() ? 0 : *m_currentIndex + 1);
}

bool PlaylistManager::isFirstIndex() const
{
    return m_currentIndex.has_value() && *m_currentIndex == 0;
}

bool PlaylistManager::isLastIndex() const
{
    return m_currentIndex.has_value() && *m_currentIndex + 1 == totalCount();
}

std::optional<qsizetype> PlaylistManager::currentIndex() const
{
    return m_currentIndex;
}

bool PlaylistManager::setCurrentIndex(qsizetype index)
{
    if (!m_currentIndex.has_value()) {
        return false;
    }
    if (index < 0 || index >= totalCount()) {
        return false;
    }
    if (*m_currentIndex == index) {
        return true;
    }

    m_currentIndex = index;
    emit currentIndexChanged(index);
    return true;
}

QUrl PlaylistManager::urlByIndex(qsizetype index) const
{
    return m_playlist.value(index);
}

QString PlaylistManager::localFileByIndex(qsizetype index) const
{
    return urlByIndex(index).toLocalFile();
}

// endregion

// region: Misc

void PlaylistManager::sanitizeIndex()
{
    auto cnt = totalCount();
    if (cnt == 0) {
        // If playlist is empty, current index should be invalid.
        m_currentIndex = std::nullopt;
    } else {
        if (m_currentIndex.has_value()) {
            // Clamp current index in playlist range.
            // If it is already in range, not change was done.
            auto currentIndex = *m_currentIndex;
            if (currentIndex < 0) {
                m_currentIndex = 0;
            } else if (currentIndex >= cnt) {
                m_currentIndex = cnt - 1;
            } else {
                ; // No change.
            }
        } else {
            // If there is no set value for current index,
            // set it at the beginning of playlist.
            m_currentIndex = 0;
        }
    }
}

bool PlaylistManager::checkUrl(const QUrl& url)
{
    return url.isValid() && url.isLocalFile();
}

QUrl PlaylistManager::urlFromString(const QString& file)
{
    return QUrl::fromLocalFile(file);
}

// endregion
