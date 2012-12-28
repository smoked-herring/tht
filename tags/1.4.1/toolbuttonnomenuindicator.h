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

#ifndef TOOLBUTTONNOMENUINDICATOR_H
#define TOOLBUTTONNOMENUINDICATOR_H

#include <QToolButton>

class ToolButtonNoMenuIndicator : public QToolButton
{
public:
    explicit ToolButtonNoMenuIndicator(QWidget *parent = 0);

protected:
    virtual void paintEvent(QPaintEvent *);
};

#endif // TOOLBUTTONNOMENUINDICATOR_H
