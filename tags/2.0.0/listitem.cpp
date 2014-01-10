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

#include <QColor>

#include "listitem.h"
#include "settings.h"

ListItem::ListItem(QListWidget *parent) :
    QListWidgetItem(parent),
    m_priority(Ticker::PriorityNormal)
{
    setPriority(m_priority, true);
}

ListItem::ListItem(const QString &text, QListWidget *parent) :
    QListWidgetItem(text, parent),
    m_priority(Ticker::PriorityNormal)
{
    setPriority(m_priority, true);
}

ListItem::~ListItem()
{}

void ListItem::setPriority(Ticker::Priority p, bool force)
{
    if(p == m_priority && !force)
        return;

    m_priority = p;

    QBrush brush;

    switch(m_priority)
    {
        case Ticker::PriorityMedium:
            brush = QColor(255, 215, 68);
        break;

        case Ticker::PriorityHigh:
            brush = QColor(30, 245, 65);
        break;

        case Ticker::PriorityHighest:
            brush = QColor(255, 89, 95);
        break;

        default:
        break;
    }

    setBackground(brush);
}

bool ListItem::operator<(const QListWidgetItem &other) const
{
    const ListItem *o = static_cast<const ListItem *>(&other);

    return o ? ((priority() == o->priority()) ? text() < o->text() : priority() > o->priority())
             : text() < other.text();
}