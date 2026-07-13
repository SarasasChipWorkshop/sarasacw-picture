// SPDX-FileCopyrightText: 2025 Gary Wang <git@blumia.net>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QUrl>
#include <QObject>
#include <optional>

/// The manager of playlist.
class PlaylistManager : public QObject
{
    Q_OBJECT
public:
    /// Constructs a PlaylistManager with an optional parent QObject.
    explicit PlaylistManager(QObject* parent = nullptr);
    /// Destructor.
    ~PlaylistManager();

    /// Get the allowed file suffixes of this playlist.
    /// @return The current list of allowed suffixes.
    const QStringList& allowedSuffixes() const;
    /// Set the allowed file suffixes of this playlist.
    /// @remark This only works for "load the whole directory by one file" case.
    /// @param[in] nameFilters The new list of allowed suffixes.
    void setAllowedSuffixes(const QStringList& nameFilters);

    /// Load user specified files as playlist.
    /// @remark The wrapper for another overload.
    /// @return True for successful load, otherwise false.
    bool loadPlaylist(const QStringList& files);
    /// Load user specified URLs as playlist.
    /// @remark This function automatically dispatch these files into correct sub-load functions.
    /// @return True for successful load, otherwise false.
    bool loadPlaylist(const QList<QUrl>& urls);
    /// Clear the entire playlist and reset current index.
    void clearPlaylist();

    /// Append a local file path to the playlist.
    void appendToPlaylist(const QString& file);
    /// Append a URL to the playlist.
    void appendToPlaylist(const QUrl& url);
    /// Insert a local file path at the given index.
    /// @return True if inserted successfully, false if index is out of range.
    bool insertIntoPlaylist(qsizetype index, const QString& file);
    /// Insert a URL at the given index.
    /// @return True if inserted successfully, false if index is out of range.
    bool insertIntoPlaylist(qsizetype index, const QUrl& url);
    /// Remove the entry at the given index.
    /// @return True if removed successfully, false if index is out of range.
    bool removeFromPlaylist(qsizetype index);

    /// Returns the total number of items in the playlist.
    /// @return Total count of entries in the playlist.
    qsizetype totalCount() const;
    /// Try fetching previous index.
    /// @details This previous index is gallery-loop-aware.
    /// @return std::nullopt if playlist is empty, otherwise the previous index considering gallery loop.
    std::optional<qsizetype> previousIndex() const;
    /// Try fetching next index.
    /// @details This next index is gallery-loop-aware.
    /// @return std::nullopt if playlist is empty, otherwise the next index considering gallery loop.
    std::optional<qsizetype> nextIndex() const;
    /// Check whether current index is the first index.
    /// @return False if current index is not the first index or playlist is empty, otherwise true.
    bool isFirstIndex() const;
    /// Check whether current index is the last index.
    /// @return False if current index is not the last index or playlist is empty, otherwise true.
    bool isLastIndex() const;
    /// Returns the current index.
    /// @return std::nullopt if the playlist is empty, otherwise the current index.
    std::optional<qsizetype> currentIndex() const;
    /// Set the current index.
    /// @remark Index only can be set when playlist is not empty.
    /// @return True if successful set, including "not changed" set, otherwise false.
    /// This return value only indicates whether we can set index and given index is valid for setting.
    /// If "no changed" set occurs, this function returns true, but doesn't raise current index changed signal.
    bool setCurrentIndex(qsizetype index);
    /// Returns the URL at the given index.
    /// @return The URL if index is valid.
    /// This class guarantee that this QUrl is pointing to a local file.
    QUrl urlByIndex(qsizetype index) const;
    /// Returns the local file path at the given index.
    /// @return The local file path if index is valid.
    /// This class guarantee that local file path always is valid.
    QString localFileByIndex(qsizetype index) const;

signals:
    void allowedSuffixesChanged(const QStringList& suffixes);
    /// Raised when index was changed bu setter.
    /// The argument holds the new current index.
    /// Please note that this signal will NOT be raised by the whole playlist change caused index change.
    void currentIndexChanged(qsizetype index);
    /// The whole playlist was changed, involving loading, editing and deleting some or full entries.
    /// The argument holds the total count of new play list.
    void playlistChanged(qsizetype count);

private:
    /// The fundamental playlist setter.
    /// @details All playlist load functions will call this function to set playlist at last.
    /// @param[in] urls ALl urls in this list should be checked by checkUrl() function.
    /// @param[in] indexHint Try to set current index to this index if possible.
    void setPlaylist(const QList<QUrl>&& urls, std::optional<qsizetype> indexHint = std::nullopt);
    /// Load playlist from a single file.
    /// @details This function scans the same directory for siblings matching allowed suffixes, sorted naturally.
    /// @return True if the playlist was loaded successfully, false otherwise.
    bool loadSingleFilePlaylist(const QUrl& url);
    /// Load playlist from multiple URLs directly.
    /// @return True if all URLs are valid, false otherwise.
    bool loadMultipleFilesPlaylist(const QList<QUrl>&& urls);
    /// Load playlist from an M3U/M3U8 playlist file.
    /// @return True if loaded successfully, false otherwise.
    bool loadM3U8Playlist(const QUrl& url);

    /// Make sure that current index is valid.
    /// @details This function usually is used by those functions manipulating playlist.
    void sanitizeIndex();

    /// Check whether given QUrl is proper to be put into playlist.
    /// @details Only the url pointing to local file can be put into playlist.
    static bool checkUrl(const QUrl& url);
    /// Convert a string to local file into a QUrl.
    /// @param[in] file The string pointing to local file.
    static QUrl urlFromString(const QString& file);

    std::optional<qsizetype> m_currentIndex;
    QList<QUrl> m_playlist;
    QStringList m_allowedSuffixes;
};
