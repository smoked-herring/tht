/*

Copyright (C) 2009, Etienne Moutot <e.moutot@gmail.com>

This file is part of colorPickerWidget.

    colorPickerWidget is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    colorPickerWidget is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*/

/***************************************************************************
 *   Copyright (C) 2013 by Dmitry 'Krasu' Baryshev                         *
 *   linuxsquirrel.dev@gmail.com                                           *
 ***************************************************************************/

#include <QGridLayout>

#include "colorpickerdialog.h"
#include "qtcolortriangle.h"
#include "colorviewer.h"
#include "tools.h"

ColorPickerDialog::ColorPickerDialog(QWidget *parent)
    : QDialog(parent)
{
    //: Command addressed to the user, means "Please choose a color here:"
    setWindowTitle(tr("Choose a color"));

    colorTriangle = new QtColorTriangle(this);

    connect(colorTriangle, SIGNAL(colorChanged(QColor)), this, SLOT(colorChgd()));

    colorView = new ColorViewer(this);
    colorView->setColor(colorTriangle->color());
        connect(colorTriangle, SIGNAL(colorChanged(QColor)), colorView, SLOT(changeColor(QColor)));

    QPushButton *ok = new QPushButton(Tools::oKTitle(), this);
    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(colorTriangle,  0, 0, 3, 2);
    layout->addWidget(colorView,      0, 2, 2, 1);
    layout->addWidget(ok,             2, 2, 1, 1);

    setLayout(layout);
}

ColorPickerDialog::~ColorPickerDialog()
{
}

QColor ColorPickerDialog::color() const
{
    return colorView->color();
}

void ColorPickerDialog::setColor(const QColor &col)
{
    colorView->setColor(col);
    colorTriangle->setColor(col);
}


void ColorPickerDialog::colorChgd()
{
    emit colorChanged(colorView->color());
}
