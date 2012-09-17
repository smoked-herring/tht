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
#include <QImageWriter>
#include <QFileDialog>
#include <QMessageBox>
#include <QClipboard>
#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QPixmap>
#include <QList>
#include <QDir>

#include "screenshoteditor.h"
#include "savescreenshot.h"
#include "settings.h"

#include "ui_savescreenshot.h"

SaveScreenshot::SaveScreenshot(const QPixmap &px, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SaveScreenshot)
{
    ui->setupUi(this);

    m_editor = new ScreenshotEditor(px, this);

    connect(ui->pushEdit, SIGNAL(clicked()), m_editor, SLOT(exec()));
}

SaveScreenshot::~SaveScreenshot()
{
    delete ui;
}

void SaveScreenshot::slotClipboard()
{
    QApplication::clipboard()->setPixmap(m_editor->pixmap());
    accept();
}

void SaveScreenshot::slotFile()
{
    QString filter;
    QList<QByteArray> formats = QImageWriter::supportedImageFormats();
    QString selectedFilter, current;

    foreach(QByteArray ba, formats)
    {
        current = tr("Image %1 (*.%2)").arg(ba.toUpper().constData()).arg(ba.toLower().constData());

        if(selectedFilter.isEmpty() && ba.toLower() == "png")
            selectedFilter = current;

        filter += current + ";;";
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save as"),
                                                    Settings::instance()->lastScreenShotDirectory()
                                                    + QDir::separator()
                                                    + QDateTime::currentDateTime().toString("yyyy-MM-dd-"),
                                                    filter, &selectedFilter);

    if(fileName.isEmpty())
        return;

    Settings::instance()->setLastScreenShotDirectory(QFileInfo(fileName).absolutePath());

    if(m_editor->pixmap().save(fileName))
    {
        qDebug("Screenshot has been saved to \"%s\"", qPrintable(fileName));
        accept();
    }
    else
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot save screenshot"));
        qDebug("Cannot save screenshot");
    }
}