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
#include <QDateTime>
#include <QSettings>
#include <QString>
#include <QRegExp>
#include <QPoint>
#include <QColor>
#include <QSize>
#include <QList>
#include <QUrl>
#include <QMap>

#include <windows.h>

struct LinkPoint
{
    LinkPoint(const QString &_name = QString(), const QList<QPoint> &_points = QList<QPoint>())
        : name(_name),
          points(_points)
    {}

    QString name;
    QList<QPoint> points;
};

Q_DECLARE_METATYPE(QList<QPoint>)
Q_DECLARE_METATYPE(LinkPoint)
Q_DECLARE_METATYPE(QList<LinkPoint>)
Q_DECLARE_METATYPE(Qt::AlignmentFlag)

#define SETTINGS_GET_BOOL Settings::instance()->value<bool>
#define SETTINGS_SET_BOOL Settings::instance()->setValue<bool>

#define SETTINGS_GET_STRING Settings::instance()->value<QString>
#define SETTINGS_SET_STRING Settings::instance()->setValue<QString>

#define SETTINGS_GET_INT Settings::instance()->value<int>
#define SETTINGS_SET_INT Settings::instance()->setValue<int>

#define SETTINGS_GET_COLOR Settings::instance()->value<QColor>
#define SETTINGS_SET_COLOR Settings::instance()->setValue<QColor>

#define SETTINGS_GET_ALIGNMENT Settings::instance()->value<Qt::AlignmentFlag>
#define SETTINGS_SET_ALIGNMENT Settings::instance()->setValue<Qt::AlignmentFlag>

#define SETTINGS_GET_POINTS Settings::instance()->value<QList<QPoint> >
#define SETTINGS_SET_POINTS Settings::instance()->setValue<QList<QPoint> >

#define SETTINGS_GET_LINKS Settings::instance()->value<QList<LinkPoint> >
#define SETTINGS_SET_LINKS Settings::instance()->setValue<QList<LinkPoint> >

#define SETTINGS_GET_SIZE Settings::instance()->value<QSize>
#define SETTINGS_SET_SIZE Settings::instance()->setValue<QSize>

#define SETTINGS_GET_POINT Settings::instance()->value<QPoint>
#define SETTINGS_SET_POINT Settings::instance()->setValue<QPoint>

#define SETTING_RESTORE_LINKS_AT_STARTUP     "restore-links-at-startup"
#define SETTING_LAST_LINKS                   "last-links"
#define SETTING_SHOW_COMMENTS                "show-comments"
#define SETTING_LIST_HEADER                  "list-header"
#define SETTING_FOOLSDAY_SEEN                "foolsday-seen"
#define SETTING_ELLIPSE_FILL_COLOR           "ellipse-fill-color"
#define SETTING_SCREENSHOT_TEXT_COLOR        "screenshot-text-color"
#define SETTING_SCREENSHOT_TEXT_ALIGNMENT    "screenshot-text-alignment"
#define SETTING_SCREENSHOT_TEXT_SIZE         "screenshot-text-size"
#define SETTING_SCREENSHOT_TEXT              "screenshot-text"
#define SETTING_TRANSLATION                  "translation"
#define SETTING_LAST_TICKER_INPUT            "last-ticker-input"
#define SETTING_LAST_TICKER_DIRECTORY        "last-ticker-directory"
#define SETTING_LAST_SCREENSHOT_DIRECTORY    "last-screenshot-directory"
#define SETTING_RESTORE_NEIGHBORS_AT_STARTUP "restore-neighbors-at-startup"
#define SETTING_SHOW_NEIGHBORS_AT_STARTUP    "neighbors-at-startup"
#define SETTING_LINKS                        "links"
#define SETTING_MINI_TICKER_ENTRY            "mini-ticker-entry"
#define SETTING_ALLOW_DUPLICATES             "allow-duplicates"
#define SETTING_NYSE_ONLY                    "nyse-only"
#define SETTING_ONTOP                        "ontop"
#define SETTING_HIDE_TO_TRAY                 "tray"
#define SETTING_TRAY_NOTICE_SEEN             "tray-notice-seen"
#define SETTING_SAVE_GEOMETRY                "save-geometry"
#define SETTING_SIZE                         "size"
#define SETTING_POSITION                     "position"
#define SETTING_NEIGHBORS_SIZE               "neighbors-size"
#define SETTING_NEIGHBORS_POSITION           "neighbors-position"
#define SETTING_SAVE_TICKERS                 "save-tickers"

class Settings
{
public:
    static Settings* instance();

    ~Settings();

    enum SyncType { NoSync, Sync };

    // some methods for plugins
    template <typename T>
    T value(const QString &key);

    template <typename T>
    T value(const QString &key, const T &def);

    template <typename T>
    void setValue(const QString &key, const T &value, SyncType sync = Sync);

    bool contains(const QString &key) const;

    void sync();

    QMap<QString, QString> translations();

    OSVERSIONINFO windowsVersion() const;

    void rereadTimestamps();

    QString databaseTimestampFormat() const;

    QDateTime persistentDatabaseTimestamp() const;
    QDateTime mutableDatabaseTimestamp() const;

    QString persistentDatabaseName() const;
    QString persistentDatabasePath() const;

    QString mutableDatabaseName() const;
    QString mutableDatabasePath() const;

    bool preloadMode() const;

    QRegExp tickerValidator() const;

    void setCheckBoxState(const QString &checkbox, bool checked, SyncType sync = Sync);
    int checkBoxState(const QString &checkbox);

    int maximumNumberOfLists() const;
    void setNumberOfLists(int, SyncType sync = Sync);
    int numberOfLists();

    void setTickersForGroup(int group, const QStringList &tickers, SyncType sync = Sync);
    QStringList tickersForGroup(int group);
    void removeTickers(int group, SyncType sync = Sync);

    void setHeaderForGroup(int group, const QString &header, SyncType sync = Sync);
    QString headerForGroup(int group);

private:
    Settings();

    QDateTime readTimestamp(const QString &fileName) const;

    void fillTranslations();

    QPoint point(const QString &key);

private:
    QSettings *m_settings;
    QRegExp m_rxTicker;
    OSVERSIONINFO m_windowsVersion;
    QString m_persistentDatabaseName;
    QString m_persistentDatabasePath;
    QString m_mutableDatabaseName;
    QString m_mutableDatabasePath;
    QDateTime m_persistentDatabaseTimestamp;
    QDateTime m_mutableDatabaseTimestamp;
    QString m_databaseTimestampFormat;
    QMap<QString, QString> m_translations;
    QHash<QString, QVariant> m_defaultValues;
};

template <typename T>
T Settings::value(const QString &key)
{
    T def = T();
    QHash<QString, QVariant>::iterator it = m_defaultValues.find(key);

    if(it != m_defaultValues.end())
        def = it.value().value<T>();

    return value<T>(key, def);
}

template <typename T>
T Settings::value(const QString &key, const T &def)
{
    m_settings->beginGroup("settings");
    QVariant value = m_settings->value(key, QVariant::fromValue(def));
    m_settings->endGroup();

    return value.value<T>();
}

template <typename T>
void Settings::setValue(const QString &key, const T &value, Settings::SyncType sync)
{
    m_settings->beginGroup("settings");
    m_settings->setValue(key, QVariant::fromValue(value));
    m_settings->endGroup();

    if(sync == Sync)
        m_settings->sync();
}

inline
bool Settings::contains(const QString &key) const
{
    return m_settings->contains(key);
}

inline
void Settings::rereadTimestamps()
{
    m_persistentDatabaseTimestamp = readTimestamp(m_persistentDatabasePath);
    m_mutableDatabaseTimestamp = readTimestamp(m_mutableDatabasePath);

    qDebug("Database P timestamp: %s", qPrintable(m_persistentDatabaseTimestamp.toString(m_databaseTimestampFormat)));
    qDebug("Database M timestamp: %s", qPrintable(m_mutableDatabaseTimestamp.toString(m_databaseTimestampFormat)));
}

inline
QString Settings::databaseTimestampFormat() const
{
    return m_databaseTimestampFormat;
}

inline
QDateTime Settings::persistentDatabaseTimestamp() const
{
    return m_persistentDatabaseTimestamp;
}

inline
QDateTime Settings::mutableDatabaseTimestamp() const
{
    return m_mutableDatabaseTimestamp;
}

inline
QString Settings::persistentDatabaseName() const
{
    return m_persistentDatabaseName;
}

inline
QString Settings::mutableDatabaseName() const
{
    return m_mutableDatabaseName;
}

inline
QString Settings::mutableDatabasePath() const
{
    return m_mutableDatabasePath;
}

inline
QString Settings::persistentDatabasePath() const
{
    return m_persistentDatabasePath;
}

inline
OSVERSIONINFO Settings::windowsVersion() const
{
    return m_windowsVersion;
}

inline
QRegExp Settings::tickerValidator() const
{
    return m_rxTicker;
}

inline
int Settings::maximumNumberOfLists() const
{
    return 8;
}

inline
QMap<QString, QString> Settings::translations()
{
    if(m_translations.isEmpty())
        fillTranslations();

    return m_translations;
}

#endif // SETTINGS_H
