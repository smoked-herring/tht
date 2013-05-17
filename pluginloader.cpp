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

#include <QApplication>
#include <QTranslator>
#include <QFileInfo>
#include <QLibrary>
#include <QVariant>
#include <QDebug>
#include <QDir>

#include "pluginloader.h"

PluginLoader* PluginLoader::instance()
{
    static PluginLoader *m_inst = new PluginLoader;

    return m_inst;
}

PluginLoader::PluginLoader(QObject *parent) : QObject(parent)
{
    QDir pluginsDir(QCoreApplication::applicationDirPath() + QDir::separator() + "plugins");

    QFileInfoList plugins = pluginsDir.entryInfoList(QStringList() << "*.dll",
                                                     QDir::Files | QDir::NoDotAndDotDot | QDir::Readable,
                                                     QDir::Name);

    foreach(QFileInfo fi, plugins)
    {
        PluginLibrary p;

        p.library = new QLibrary(fi.absoluteFilePath());
        p.library->load();

        p.plugin_create = (Plugin* (*)())(p.library)->resolve("plugin_create");
        p.plugin_destroy = (void (*)(Plugin *))(p.library)->resolve("plugin_destroy");

        if(!p.plugin_create || !p.plugin_destroy)
        {
            qWarning("Cannot load library (%s)", qPrintable(p.library->errorString()));
            p.library->unload();
            delete p.library;
        }
        else
        {
            p.plugin = p.plugin_create();

            if(!p.plugin)
            {
                qWarning("Plugin \"%s\" creation failed", qPrintable(fi.fileName()));
                continue;
            }

            append(p);

            qDebug("Loaded plugin \"%s\"", qPrintable(fi.fileName()));
        }
    }
}

PluginLoader::~PluginLoader()
{
    iterator itEnd = end();

    for(iterator it = begin();it != itEnd;++it)
    {
        qDebug("Removing plugin \"%s\"", qPrintable((*it).library->fileName()));

        delete (*it).translator;
        (*it).plugin_destroy((*it).plugin);
        delete (*it).library;
    }

    clear();
}

void PluginLoader::init()
{
    QString ts = qApp->property("tht-translation").toString();
    QString tsDir = QCoreApplication::applicationDirPath() + QDir::separator() + "translations";

    iterator itEnd = end();

    for(iterator it = begin();it != itEnd;++it)
    {
        QFileInfo fi((*it).library->fileName());

        (*it).translator = new QTranslator;

        if(!(*it).translator)
        {
            qWarning("Translator for \"%s\" plugin creation failed", qPrintable(fi.fileName()));
            continue;
        }

        qDebug("Loading plugin translation: %s",
               (*it).translator->load(fi.baseName().replace('-', '_') + '_' + ts, tsDir) ? "ok" : "failed");

        qApp->installTranslator((*it).translator);

        (*it).plugin->init();
    }
}

QList<Plugin *> PluginLoader::byType(Plugin::Type type)
{
    QList<Plugin *> list;

    iterator itEnd = end();

    for(iterator it = begin();it != itEnd;++it)
    {
        if((*it).plugin->type() == type)
            list.append((*it).plugin);
    }

    return list;
}
