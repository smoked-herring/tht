/*
 * This file is part of THT.
 *
 * THT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * THT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with THT.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QStringList>
#include <QSettings>
#include <QRegExp>
#include <QPoint>
#include <QSize>

#include <windows.h>

class Settings
{
public:
    static Settings* instance();

    ~Settings();

    enum SyncType { SyncTypeNoSync, SyncTypeSync };

    void sync();

    OSVERSIONINFO version() const;

    void setCheckBoxState(const QString &checkbox, bool checked, SyncType sync = SyncTypeSync);
    int checkBoxState(const QString &checkbox);

    QString tickersPersistentDatabaseName() const;
    QString tickersPersistentDatabasePath() const;

    QString tickersMutableDatabaseName() const;
    QString tickersMutableDatabasePath() const;

    bool preloadMode() const;

    QRegExp tickerValidator() const;

    void setLastTickerInput(QString, SyncType sync = SyncTypeSync);
    QString lastTickerInput();

    void setLastTickerDirectory(QString, SyncType sync = SyncTypeSync);
    QString lastTickerDirectory();

    void setLastScreenShotDirectory(QString, SyncType sync = SyncTypeSync);
    QString lastScreenShotDirectory();

    void setMiniTickerEntry(bool, SyncType sync = SyncTypeSync);
    bool miniTickerEntry();

    void setAllowDuplicates(bool, SyncType sync = SyncTypeSync);
    bool allowDuplicates();

    void setNyseOnly(bool, SyncType sync = SyncTypeSync);
    bool nyseOnly();

    void setOnTop(bool, SyncType sync = SyncTypeSync);
    bool onTop();

    void setHideToTray(bool, SyncType sync = SyncTypeSync);
    bool hideToTray();

    void setTrayNoticeSeen(bool, SyncType sync = SyncTypeSync);
    bool trayNoticeSeen();

    void setSaveGeometry(bool, SyncType sync = SyncTypeSync);
    bool saveGeometry();

    int maximumNumberOfLists() const;

    void setNumberOfLists(int, SyncType sync = SyncTypeSync);
    int numberOfLists();

    void setWindowSize(const QSize &, SyncType sync = SyncTypeSync);
    QSize windowSize();

    void setWindowPosition(const QPoint &, SyncType sync = SyncTypeSync);
    QPoint windowPosition();

    void setSaveTickers(bool, SyncType sync = SyncTypeSync);
    bool saveTickers();

    void saveTickersForGroup(int group, const QStringList &tickers, SyncType sync = SyncTypeSync);
    QStringList tickersForGroup(int group);
    void removeTickers(int group, SyncType sync = SyncTypeSync);

private:
    Settings();

    template <typename T>
    T load(const QString &key, const T &def = T());

    template <typename T>
    void save(const QString &key, const T &value, SyncType sync = SyncTypeSync);

private:
    QSettings *m_settings;
    QRegExp m_rxTicker;
    OSVERSIONINFO m_version;
    QString m_tickersPersistentDatabaseName;
    QString m_tickersPersistentDatabasePath;
    QString m_tickersMutableDatabaseName;
    QString m_tickersMutableDatabasePath;
};

inline
QString Settings::tickersPersistentDatabaseName() const
{
    return m_tickersPersistentDatabaseName;
}

inline
QString Settings::tickersMutableDatabaseName() const
{
    return m_tickersMutableDatabaseName;
}

inline
QString Settings::tickersMutableDatabasePath() const
{
    return m_tickersMutableDatabasePath;
}

inline
QString Settings::tickersPersistentDatabasePath() const
{
    return m_tickersPersistentDatabasePath;
}

inline
OSVERSIONINFO Settings::version() const
{
    return m_version;
}

inline
QRegExp Settings::tickerValidator() const
{
    return m_rxTicker;
}

inline
int Settings::maximumNumberOfLists() const
{
    return 5;
}

#endif // SETTINGS_H
