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

#include <QDesktopServices>
#include <QListWidgetItem>
#include <QLinearGradient>
#include <QApplication>
#include <QFontMetrics>
#include <QFileDialog>
#include <QInputEvent>
#include <QMessageBox>
#include <QTextStream>
#include <QMouseEvent>
#include <QStringList>
#include <QClipboard>
#include <QDateTime>
#include <QGradient>
#include <QFileInfo>
#include <QKeyEvent>
#include <QPalette>
#include <QPainter>
#include <QEvent>
#include <QColor>
#include <QMenu>
#include <QSize>
#include <QUrl>
#include <QPen>

#include "tickerinformationtooltip.h"
#include "tickerinput.h"
#include "settings.h"
#include "listitem.h"
#include "list.h"
#include "ui_list.h"

List::List(int group, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::List),
    m_section(group),
    m_saveTickers(Settings::instance()->saveTickers()),
    m_ignoreInput(false),
    m_dragging(false)
{
    ui->setupUi(this);

    ui->pushSave->setEnabled(!m_saveTickers);

    QMenu *menu = new QMenu(this);
    menu->addAction(QIcon(":/images/clear.png"), tr("Clear") + "\tN", this, SLOT(clear()));
    menu->addAction(tr("Sort") + "\tR", this, SLOT(slotSortList()));
    menu->addSeparator();
    menu->addAction(tr("Reset priorities") + "\tAlt+U", this, SLOT(slotResetPriorities()));

    ui->pushList->setMenu(menu);

    QIcon file_icon(":/images/file.png");

    menu = new QMenu(this);
    menu->addAction(tr("Add one ticker...") + "\tO", this, SLOT(slotAddOne()));
    menu->addSeparator();
    menu->addAction(file_icon, tr("Add from file...") + "\tA", this, SLOT(slotAddFromFile()));
    menu->addAction(tr("Add from clipboard") + "\tP", this, SLOT(slotAddFromClipboard()));
    ui->pushAdd->setMenu(menu);

    menu = new QMenu(this);
    menu->addAction(file_icon, tr("Export to file...") + "\tE", this, SLOT(slotExportToFile()));
    menu->addAction(tr("Export to clipboard") + "\tC", this, SLOT(slotExportToClipboard()));
    ui->pushSaveAs->setMenu(menu);

    setFocusProxy(ui->list);

    load();

    // catch keyboard events
    ui->list->installEventFilter(this);
    ui->list->viewport()->installEventFilter(this);
}

List::~List()
{
    delete ui;
}

bool List::haveTickers() const
{
    return ui->list->count();
}

void List::addTicker(const QString &ticker, ListItem::Priority p)
{
    if(ui->list->findItems(ticker, Qt::MatchFixedString).size())
    {
        qDebug("Will not add duplicate item");
        return;
    }

    if(Settings::instance()->tickerValidator().exactMatch(ticker))
    {
        addItem(ticker + ',' + QString::number(p));
        numberOfItemsChanged();
        save();
    }
}

QString List::currentTicker() const
{
    QListWidgetItem *item = ui->list->currentItem();

    return item ? item->text() : QString();
}

ListItem::Priority List::currentPriority() const
{
    ListItem *item = static_cast<ListItem *>(ui->list->currentItem());

    return item ? item->priority() : ListItem::PriorityNormal;
}

void List::setSaveTickers(bool dosave)
{
    if(m_saveTickers == dosave)
        return;

    bool oldsave = m_saveTickers;
    m_saveTickers = dosave;
    ui->pushSave->setEnabled(!m_saveTickers);

    if(!oldsave)
        save();
}

bool List::contains(const QPoint &p)
{
    QPoint l = mapFromGlobal(p);

    return ui->list->geometry().contains(l);
}

void List::initialSelect()
{
    setFocus();

    QListWidgetItem *item = ui->list->item(0);

    if(!item)
        return;

    ui->list->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);
}

bool List::eventFilter(QObject *obj, QEvent *event)
{
    QEvent::Type type = event->type();

    // eat input events
    if(m_ignoreInput && obj == ui->list &&
                            (type == QEvent::KeyPress
                            || type == QEvent::KeyRelease
                            || type == QEvent::ShortcutOverride
                            || type == QEvent::MouseButtonPress
                            || type == QEvent::MouseButtonRelease
                            || type == QEvent::MouseButtonDblClick
                            || type == QEvent::MouseMove
                            || type == QEvent::Wheel
                            || type == QEvent::TabletPress
                            || type == QEvent::TabletRelease
                            || type == QEvent::TabletMove
                            || type == QEvent::TouchBegin
                            || type == QEvent::TouchEnd
                            || type == QEvent::TouchUpdate
                         ))
        return true;

    if(obj == ui->list)
    {
        if(type == QEvent::KeyPress)
        {
            QKeyEvent *ke = static_cast<QKeyEvent *>(event);

            if(ke->matches(QKeySequence::Paste))
                paste();
            else if(ke->matches(QKeySequence::New))
                clear();
            else if(ke->matches(QKeySequence::Open))
                slotAddFromFile();
            else if(ke->modifiers() == Qt::NoModifier
                    || ke->modifiers() == Qt::KeypadModifier) // disallow all modifiers except keypad
            {
                switch(ke->key())
                {
                case Qt::Key_1:
                case Qt::Key_2:
                case Qt::Key_3:
                case Qt::Key_4:
                case Qt::Key_5:
                    emit copyTo(currentTicker(), currentPriority(), ke->key() - Qt::Key_1);
                break;

                case Qt::Key_A:
                    slotAddFromFile();
                break;

                case Qt::Key_P:
                    paste();
                break;

                case Qt::Key_C:
                    slotExportToClipboard();
                break;

                case Qt::Key_E:
                    slotExportToFile();
                break;

                case Qt::Key_O:
                case Qt::Key_Insert:
                    slotAddOne();
                break;

                case Qt::Key_Right:
                    emit copyRight(currentTicker(), currentPriority());
                break;

                case Qt::Key_Left:
                    emit copyLeft(currentTicker(), currentPriority());
                break;

                case Qt::Key_Delete:
                {
                    QListWidgetItem *i = ui->list->currentItem();

                    if(i)
                    {
                        delete i;
                        numberOfItemsChanged();
                        save();
                        loadItem(LoadItemCurrent);
                    }
                }
                break;

                case Qt::Key_Return:
                case Qt::Key_Enter:
                    loadItem(LoadItemCurrent);
                break;

                case Qt::Key_N:
                    clear();
                break;

                case Qt::Key_R:
                    slotSortList();
                break;

                    // Yahoo finance
                case Qt::Key_Y:
                {
                    QString t = currentTicker();

                    if(!t.isEmpty())
                        QDesktopServices::openUrl(QUrl(QString("http://finance.yahoo.com/q?s=%1").arg(t.replace('.', '-'))));

                    break;
                }

                case Qt::Key_Up:
                    loadItem(LoadItemPrevious);
                break;

                case Qt::Key_Down:
                    loadItem(LoadItemNext);
                break;

                case Qt::Key_Home:
                    loadItem(LoadItemFirst);
                break;

                case Qt::Key_End:
                    loadItem(LoadItemLast);
                break;

                case Qt::Key_Space:
                    showTickerInfo();
                break;

                case Qt::Key_PageUp:
                    loadItem(LoadItemPageUp);
                break;

                case Qt::Key_PageDown:
                    loadItem(LoadItemPageDown);
                break;

                case Qt::Key_Plus:
                case Qt::Key_Equal:
                    changePriority(+1);
                break;

                case Qt::Key_Minus:
                    changePriority(-1);
                break;

                case Qt::Key_U:
                    slotResetPriority();
                break;

                // default processing
                case Qt::Key_Tab:
                    return QObject::eventFilter(obj, event);
                } // switch
            }
            else if(ke->modifiers() == Qt::ControlModifier)
            {
                switch(ke->key())
                {
                    case Qt::Key_Up:
                        moveItem(MoveItemPreviuos);
                    break;

                    case Qt::Key_Down:
                        moveItem(MoveItemNext);
                    break;

                    case Qt::Key_Home:
                        moveItem(MoveItemFirst);
                    break;

                    case Qt::Key_End:
                        moveItem(MoveItemLast);
                    break;

                    case Qt::Key_PageUp:
                        moveItem(MoveItemPageUp);
                    break;

                    case Qt::Key_PageDown:
                        moveItem(MoveItemPageDown);
                    break;
                }
            }
            else if(ke->modifiers() == Qt::AltModifier)
            {
                switch(ke->key())
                {
                    case Qt::Key_U:
                        slotResetPriorities();
                    break;
                }
            }

            return true;
        }
        else if(type == QEvent::FocusIn)
            ui->list->setAlternatingRowColors(true);
        else if(type == QEvent::FocusOut)
            ui->list->setAlternatingRowColors(false);
    }
    else if(obj == ui->list->viewport())
    {
        if(type == QEvent::MouseButtonPress)
        {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);

            if(me->buttons() & Qt::LeftButton)
            {
                ListItem *i = static_cast<ListItem *>(ui->list->itemAt(me->pos()));
                m_startDragText = i ? i->text() : QString();

                if(!m_startDragText.isEmpty())
                {
                    m_startDragPriority = i->priority();
                    m_startPos = me->pos();
                }
            }
        }
        else if(type == QEvent::MouseMove)
        {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);

            if(!m_dragging)
            {
                if(!m_startPos.isNull()
                        && (me->buttons() & Qt::LeftButton)
                        && (me->pos() - m_startPos).manhattanLength() > QApplication::startDragDistance())
                {
                    qDebug("Start dragging \"%s\"", qPrintable(m_startDragText));

                    QPixmap pix = createDragCursor();

                    m_dragging = true;
                    QApplication::setOverrideCursor(QCursor(pix, pix.width()/2, pix.height()/2));
                }
            }
            else
            {
                if(me->buttons() == Qt::NoButton)
                {
                    QApplication::restoreOverrideCursor();
                    m_dragging = false;
                    m_startPos = QPoint();
                }
            }

            return true;
        }
        else if(type == QEvent::MouseButtonRelease)
        {
            QMouseEvent *me = static_cast<QMouseEvent *>(event);

            if(me->button() == Qt::LeftButton || (me->buttons() & Qt::LeftButton))
            {
                if(m_dragging)
                {
                    QPoint p = QCursor::pos();

                    qDebug("Dropped at %d,%d", p.x(), p.y());
                    QApplication::restoreOverrideCursor();

                    emit dropped(m_startDragText, m_startDragPriority, p);
                }

                m_dragging = false;
                m_startPos = QPoint();
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

void List::numberOfItemsChanged()
{
    ui->labelElements->setNum(ui->list->count());
}

QStringList List::toStringList(bool withPriority)
{
    int i = 0;
    QStringList items;
    ListItem *item;

    while((item = static_cast<ListItem *>(ui->list->item(i++))))
    {
        items.append((withPriority && item->priority() != ListItem::PriorityNormal)
                     ? (item->text() + ',' + QString::number(item->priority()))
                     : item->text());
    }

    return items;
}

void List::save()
{
    // don't save tickers automatically
    if(!m_saveTickers)
    {
        qDebug("Autosave is disabled");
        showSaved(false);
        return;
    }

    slotSave();
}

void List::load()
{
    qDebug("Loading tickers from section \"%d\"", m_section);

    QStringList items = Settings::instance()->tickersForGroup(m_section);

    ui->list->setUpdatesEnabled(false);

    foreach(QString t, items)
        addItem(t);

    ui->list->setUpdatesEnabled(true);

    numberOfItemsChanged();
}

void List::paste()
{
    QString text = QApplication::clipboard()->text();

    qDebug("Paste");

    QTextStream t(&text);
    QString ticker;

    ui->list->setUpdatesEnabled(false);

    bool changed = false;

    while(!t.atEnd())
    {
        t >> ticker;

        if(Settings::instance()->tickerValidator().exactMatch(ticker))
        {
            changed = true;
            addItem(ticker, true);
        }
    }

    ui->list->setUpdatesEnabled(true);

    if(changed)
    {
        numberOfItemsChanged();
        save();
    }
}

void List::showSaved(bool isSaved)
{
    QPalette pal = ui->labelUnsaved->palette();
    pal.setColor(QPalette::Window, isSaved ? palette().color(QPalette::Window) : Qt::red);
    ui->labelUnsaved->setPalette(pal);
}

void List::showTickerInfo()
{
    QListWidgetItem *item = ui->list->currentItem();

    if(!item)
    {
        qDebug("Nothing to load");
        return;
    }

    QPoint p = ui->list->viewport()->mapToGlobal(ui->list->visualItemRect(item).bottomLeft());

    if(p.isNull())
    {
        qDebug("Cannot find where to show the information");
        return;
    }

    TickerInformationToolTip::showText(p, item->text());
}

QPixmap List::createDragCursor()
{
    QFont fnt = ui->list->font();
    int size = fnt.pointSize();

    if(size < 0)
        size = fnt.pixelSize();

    fnt.setPointSize(size+2);

    QFontMetrics fm(fnt);
    QSize dragCursorSize = fm.boundingRect(m_startDragText).adjusted(0,0, 10,4).size();

    QColor textColor = palette().color(QPalette::WindowText);
    QColor borderColor = QColor::fromRgb(0xffefef);

    QPixmap px(dragCursorSize);
    px.fill(Qt::transparent);
    QPainter p(&px);

    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    // bounding rect, background
    QLinearGradient gr = QLinearGradient(QPointF(0, 0), QPointF(0, px.height()));
    gr.setColorAt(0, borderColor);
    gr.setColorAt(0.5, QColor::fromRgb(0xe6c207));
    gr.setColorAt(1, borderColor);

    p.setBrush(gr);
    p.setPen(Qt::NoPen);
    p.drawRect(px.rect());

    p.setPen(QPen(textColor, 1));
    p.setBrush(Qt::transparent);
    p.drawRect(px.rect());

    // text
    p.setFont(fnt);
    p.drawText(px.rect(), Qt::AlignCenter, m_startDragText);
    p.end();

    return px;
}

void List::addItem(const QString &text, bool fix)
{
    QStringList txt = text.toUpper().split(',', QString::SkipEmptyParts);

    if(txt.isEmpty())
        return;

    ListItem *item = new ListItem(fix
                                  ? txt[0].replace('-', '.')
                                  : txt[0],
                                  ui->list);

    if(txt.size() > 1)
    {
        bool ok;
        int p = txt[1].toInt(&ok);

        if(ok)
            item->setPriority(static_cast<ListItem::Priority>(p));
    }

    ui->list->addItem(item);
}

void List::changePriority(int p)
{
    ListItem *li = static_cast<ListItem *>(ui->list->currentItem());

    if(!li)
    {
        qDebug("Cannot find ticker");
        return;
    }

    // fix the value
    if(p < -1)
        p = -1;
    else if(p > +1)
        p = +1;

    if((p < 0 && li->priority() == ListItem::PriorityNormal)
            || (p > 0 && li->priority() == ListItem::PriorityHighest))
    {
        qDebug("Priority is on the edge");
        return;
    }

    qDebug("Changing priority %+d", p);

    li->setPriority(static_cast<ListItem::Priority>(li->priority() + p));

    save();
}

void List::loadItem(LoadItem litem)
{
    QListWidgetItem *item = ui->list->currentItem();

    if(!item)
        item = ui->list->item(0);

    QListWidgetItem *citem = item;

    switch(litem)
    {
        case LoadItemCurrent:
        break;

        case LoadItemNext:
            item = ui->list->item(ui->list->row(item)+1);
        break;

        case LoadItemPrevious:
            item = ui->list->item(ui->list->row(item)-1);
        break;

        case LoadItemFirst:
            item = ui->list->item(0);
        break;

        case LoadItemLast:
            item = ui->list->item(ui->list->count()-1);
        break;

        case LoadItemPageUp:
        case LoadItemPageDown:
            item = ui->list->itemAt(ui->list->visualItemRect(item).adjusted(
                                        0,
                                        (litem == LoadItemPageUp ? -ui->list->viewport()->height() : ui->list->viewport()->height()),
                                        0,
                                        0
                                        ).topLeft());

            if(!item)
                item = (litem == LoadItemPageUp) ? ui->list->item(0) : ui->list->item(ui->list->count()-1);
        break;
    }

    if(!item)
    {
        qDebug("Cannot find item to load");
        return;
    }

    if(item == citem && litem != LoadItemCurrent)
    {
        qDebug("Will not load the same item");
        return;
    }

    ui->list->setCurrentItem(item);
    item->setSelected(true);
    emit loadTicker(item->text());
}

void List::moveItem(MoveItem mi)
{
    QListWidgetItem *item = ui->list->currentItem();

    if(!item)
    {
        qDebug("Cannot find the ticker to move");
        return;
    }

    int crow = ui->list->currentRow();
    int row = -1;

    switch(mi)
    {
        case MoveItemNext:
            row = ui->list->row(item)+1;

            if(row >= ui->list->count())
                row = ui->list->count()-1;
        break;

        case MoveItemPreviuos:
            row = ui->list->row(item)-1;

            if(row <= 0)
                row = 0;
        break;

        case MoveItemFirst:
            row = 0;
        break;

        case MoveItemLast:
            row = ui->list->count()-1;
        break;

        case MoveItemPageUp:
        case MoveItemPageDown:
            item = ui->list->itemAt(ui->list->visualItemRect(item).adjusted(
                                        0,
                                        (mi == MoveItemPageUp ? -ui->list->viewport()->height() : ui->list->viewport()->height()),
                                        0,
                                        0
                                        ).topLeft());

            if(!item)
                item = (mi == MoveItemPageUp) ? ui->list->item(0) : ui->list->item(ui->list->count()-1);

            row = ui->list->row(item);
        break;
    }

    if(row == crow)
    {
        qDebug("Won't move to the same position");
        return;
    }

    if(row < 0)
    {
        qDebug("Won't move to the invalid position");
        return;
    }

    qDebug("Moving ticker from position %d to %d", crow, row);

    item = ui->list->takeItem(crow);

    if(!item)
        return;

    ui->list->insertItem(row, item);
    ui->list->setCurrentItem(item, QItemSelectionModel::ClearAndSelect);

    save();
}

void List::slotAddOne()
{
    qDebug("Adding one ticker");

    TickerInput ti(this);

    if(ti.exec() != QDialog::Accepted)
        return;

    QString ticker = ti.ticker();

    if(Settings::instance()->tickerValidator().exactMatch(ticker))
    {
        addItem(ticker, true);
        numberOfItemsChanged();
        save();
    }
}

void List::slotAddFromFile()
{
    qDebug("Adding new tickers from file");

    QStringList fileNames = QFileDialog::getOpenFileNames(this, tr("Choose a file"), Settings::instance()->lastTickerDirectory());

    if(fileNames.isEmpty())
        return;

    bool changed = false, error = false;
    QStringList errorFiles;

    Settings::instance()->setLastTickerDirectory(QFileInfo(fileNames[0]).absolutePath());

    ui->list->setUpdatesEnabled(false);

    foreach(QString fileName, fileNames)
    {
        QString ticker;
        QFile file(fileName);

        if(!file.open(QIODevice::ReadOnly))
        {
            qWarning("Cannot open file \"%s\" for reading", qPrintable(fileName));
            error = true;
            errorFiles.append(fileName);
            continue;
        }

        QTextStream t(&file);

        while(!t.atEnd())
        {
            t >> ticker;

            if(Settings::instance()->tickerValidator().exactMatch(ticker))
            {
                changed = true;
                addItem(ticker, true);
            }
        }
    }

    ui->list->setUpdatesEnabled(true);

    if(changed)
    {
        numberOfItemsChanged();
        save();
    }

    if(error)
        QMessageBox::warning(this, tr("Error"), tr("Cannot open the following files: %1").arg(errorFiles.join(",")));
}

void List::slotAddFromClipboard()
{
    paste();
}

void List::clear()
{
    qDebug("Clear tickers for section \"%d\"", m_section);

    // nothing to do
    if(!ui->list->count())
        return;

    ui->list->clear();
    numberOfItemsChanged();
    save();
}

void List::slotSave()
{
    qDebug("Saving tickers to section \"%d\"", m_section);

    qint64 t = QDateTime::currentMSecsSinceEpoch();

    Settings::instance()->saveTickersForGroup(m_section, toStringList(true));

    showSaved(true);

    qDebug("Saved in %ld ms.", static_cast<long int>(QDateTime::currentMSecsSinceEpoch() - t));
}

void List::slotExportToFile()
{
    qDebug("Exporting tickers to file");

    QString fileName = QFileDialog::getSaveFileName(this, tr("Choose a file"), Settings::instance()->lastTickerDirectory());

    if(fileName.isEmpty())
        return;

    QFile file(fileName);

    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        qWarning("Cannot open file for writing");
        QMessageBox::warning(this, tr("Error"), tr("Cannot save to file %1").arg(fileName));
        return;
    }

    Settings::instance()->setLastTickerDirectory(QFileInfo(fileName).absolutePath());

    QTextStream t(&file);

    QStringList items = toStringList(false);

    foreach(QString item, items)
    {
        t << item << '\n';
    }

    t.flush();
}

void List::slotSortList()
{
    ui->list->sortItems();
    save();
}

void List::slotResetPriorities()
{
    int row = 0;
    ListItem *i;

    while((i = static_cast<ListItem *>(ui->list->item(row++))))
        i->setPriority(ListItem::PriorityNormal);

    save();
}

void List::slotResetPriority()
{
    ListItem *i = static_cast<ListItem *>(ui->list->currentItem());

    if(!i || i->priority() == ListItem::PriorityNormal)
        return;

    qDebug("Resetting priority for ticker \"%s\"", qPrintable(i->text()));

    i->setPriority(ListItem::PriorityNormal);
    save();
}

void List::slotExportToClipboard()
{
    qDebug("Exporting tickers to clipboard");

    QApplication::clipboard()->setText(toStringList(false).join("\n"));
}