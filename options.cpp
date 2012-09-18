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

#include <QString>

#include "settings.h"
#include "options.h"

#include "ui_options.h"

Options::Options(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Options)
{
    ui->setupUi(this);

    load();
}

Options::~Options()
{
    delete ui;
}

void Options::load()
{
    for(int i = 0;i < Settings::instance()->maximumNumberOfLists();i++)
        ui->comboNumberOfLists->addItem(QString::number(i+1));

    ui->comboNumberOfLists->setCurrentIndex(Settings::instance()->numberOfLists() - 1);
    ui->checkOnTop->setChecked(Settings::instance()->onTop());
    ui->checkTray->setChecked(Settings::instance()->hideToTray());
    ui->checkRestore->setChecked(Settings::instance()->restoreNeighborsAtStartup());
    ui->checkSave->setChecked(Settings::instance()->saveGeometry());
    ui->checkSaveTickers->setChecked(Settings::instance()->saveTickers());
    ui->checkAllowDups->setChecked(Settings::instance()->allowDuplicates());
    ui->checkMini->setChecked(Settings::instance()->miniTickerEntry());

    const QMap<QString, QString> tsmap = Settings::instance()->translations();
    QString ts = Settings::instance()->translation();
    QMap<QString, QString>::const_iterator itEnd = tsmap.end();

    for(QMap<QString, QString>::const_iterator it = tsmap.begin();it != itEnd;++it)
    {
        ui->comboLang->addItem(it.value(), it.key());

        if(it.key() == ts)
            ui->comboLang->setCurrentIndex(ui->comboLang->count()-1);
    }
}

void Options::save()
{
    Settings::instance()->setNumberOfLists(ui->comboNumberOfLists->currentIndex()+1, Settings::NoSync);
    Settings::instance()->setOnTop(ui->checkOnTop->isChecked(), Settings::NoSync);
    Settings::instance()->setHideToTray(ui->checkTray->isChecked(), Settings::NoSync);
    Settings::instance()->setRestoreNeighborsAtStartup(ui->checkRestore->isChecked(), Settings::NoSync);
    Settings::instance()->setSaveGeometry(ui->checkSave->isChecked(), Settings::NoSync);
    Settings::instance()->setSaveTickers(ui->checkSaveTickers->isChecked(), Settings::NoSync);
    Settings::instance()->setAllowDuplicates(ui->checkAllowDups->isChecked(), Settings::NoSync);
    Settings::instance()->setTranslation(ui->comboLang->itemData(ui->comboLang->currentIndex()).toString(), Settings::NoSync);
    Settings::instance()->setMiniTickerEntry(ui->checkMini->isChecked()); // also sync
}
