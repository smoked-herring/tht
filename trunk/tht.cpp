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

#include <QxtGlobalShortcut>

#include <QWhatsThisClickedEvent>
#include <QContextMenuEvent>
#include <QDesktopServices>
#include <QApplication>
#include <QKeySequence>
#include <QGridLayout>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFileInfo>
#include <QShortcut>
#include <QDateTime>
#include <QPalette>
#include <QEvent>
#include <QTimer>
#include <QIcon>
#include <QMenu>
#include <QUrl>

#include <windows.h>
#include <winnt.h>
#include <psapi.h>

#include "tickersdatabaseupdater.h"
#include "linkpointmanager.h"
#include "savescreenshot.h"
#include "regionselect.h"
#include "tickerinput.h"
#include "settings.h"
#include "listitem.h"
#include "options.h"
#include "about.h"
#include "tools.h"
#include "list.h"
#include "tht.h"
#include "ui_tht.h"

static const int          THT_WINDOW_STARTUP_TIMEOUT = 1200;
static const char * const THT_PRIVATE_TICKER_PREFIX = "=THT=";

THT::THT(QWidget *parent) :
    QWidget(parent,
            Qt::Window
            | Qt::WindowMinimizeButtonHint
            | Qt::WindowCloseButtonHint
            | Qt::CustomizeWindowHint),
    ui(new Ui::THT),
    m_windows(&m_windowsLoad),
    m_running(false),
    m_locked(false),
    m_lastActiveWindow(0)
{
    if(Settings::instance()->onTop())
        setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    ui->setupUi(this);

    // NYSE only
    ui->checkNyse->setChecked(Settings::instance()->nyseOnly());

    // global shortcuts
    m_takeScreen = new QxtGlobalShortcut(QKeySequence(Qt::CTRL + Qt::ALT + Qt::Key_S), this);
    connect(m_takeScreen, SIGNAL(activated()), this, SLOT(slotTakeScreenshotFromGlobal()));

    QIcon icon_quit(":/images/quit.png");
    QIcon icon_screenshot(":/images/screenshot.png");
    QIcon icon_chart(":/images/chart.ico");

    // context menu
    QShortcut *help_shortcut = new QShortcut(QKeySequence::HelpContents, this, SLOT(slotAbout()));
    QShortcut *quit_shortcut = new QShortcut(Qt::CTRL+Qt::Key_Q, this, SLOT(slotQuit()));
    QShortcut *clear_shortcut = new QShortcut(Qt::ALT+Qt::Key_N, this, SLOT(slotClearLists()));

    m_menu = new QMenu(this);
    m_menu->addAction(QIcon(":/images/options.png"), tr("Options..."), this, SLOT(slotOptions()));
    m_menu->addSeparator();

    m_menu->addAction(icon_screenshot, tr("Take screenshot...") + '\t' + m_takeScreen->shortcut().toString(),
                      this, SLOT(slotTakeScreenshot()));
    m_menu->addAction(tr("Clear links"), this, SLOT(slotClearLinks()));
    m_menu->addSeparator();

    m_menu->addAction(tr("Clear ticker lists") + '\t' + clear_shortcut->key().toString(), this, SLOT(slotClearLists()));
    m_menu->addAction(tr("Load ticker") + "..." + "\tL", this, SLOT(slotLoadTicker()));
    QMenu *menu_load = m_menu->addMenu(tr("Load ticker"));

    m_menu->addSeparator();
    m_menu->addAction(icon_chart, tr("About THT") + '\t' + help_shortcut->key().toString(), this, SLOT(slotAbout()));
    m_menu->addAction(tr("About Qt"), this, SLOT(slotAboutQt()));
    m_menu->addSeparator();
    m_menu->addAction(icon_quit, tr("Quit") + '\t' + quit_shortcut->key().toString(), this, SLOT(slotQuit()));

    m_timerCheckActive = new QTimer(this);
    m_timerCheckActive->setSingleShot(true);
    m_timerCheckActive->setInterval(50);
    connect(m_timerCheckActive, SIGNAL(timeout()), this, SLOT(slotCheckActive()));

    m_timerLoadToNextWindow = new QTimer(this);
    m_timerLoadToNextWindow->setSingleShot(true);
    m_timerLoadToNextWindow->setInterval(10);
    connect(m_timerLoadToNextWindow, SIGNAL(timeout()), this, SLOT(slotLoadToNextWindow()));

    // layout
    m_layout = new QGridLayout(ui->widgetTickers);
    m_layout->setContentsMargins(0, 0, 0, 0);
    ui->widgetTickers->setLayout(m_layout);

    rebuildUi();

    QString startupTicker;

    // set focus
    foreach(List *l, m_lists)
    {
        if(l->hasTickers())
        {
            l->initialSelect();
            startupTicker = l->currentTicker();
            break;
        }
    }

    // restore geometry
    if(Settings::instance()->saveGeometry())
    {
        QSize sz = Settings::instance()->windowSize();

        if(sz.isValid())
            resize(sz);

        Tools::moveWindow(this, Settings::instance()->windowPosition());
    }

    m_tray = new QSystemTrayIcon(icon_chart, this);
    QMenu *trayMenu = new QMenu(this);

    trayMenu->addAction(tr("Restore"), this, SLOT(activate()));
    trayMenu->addAction(icon_screenshot, tr("Take screenshot..."), this, SLOT(slotTakeScreenshot()));
    trayMenu->addSeparator();
    trayMenu->addAction(tr("About THT"), this, SLOT(slotAbout()));
    trayMenu->addSeparator();
    trayMenu->addAction(icon_quit, tr("Quit"), qApp, SLOT(quit()));

    connect(m_tray, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT(slotTrayActivated(QSystemTrayIcon::ActivationReason)));

    m_tray->setContextMenu(trayMenu);
    m_tray->setVisible(Settings::instance()->hideToTray());

    checkWindows();
    rebuildLinkPoints();

    // predefined tickers, menu & shortcuts
    PredefinedTickerMappings mappings;

    // NASDAQ Composite
    mappings.insert(LinkTypeTwinkorswim, "COMP");
    m_predefined.insert("$COMPQ", PredefinedTicker(Qt::Key_Q, mappings));

    // DOW JONES
    mappings.clear();
    mappings.insert(LinkTypeMBTDesktop,    "$DJI");
    mappings.insert(LinkTypeMBTDesktopPro, "$DJI");
    mappings.insert(LinkTypeTwinkorswim,   "DJX");
    m_predefined.insert("$INDU", PredefinedTicker(Qt::Key_I, mappings));

    // S&P 500 Index
    mappings.clear();
    mappings.insert(LinkTypeTwinkorswim, "SPX");
    m_predefined.insert("$SPX", PredefinedTicker(Qt::Key_S, mappings));

    // NYSE Volume
    m_predefined.insert("$TVOL", PredefinedTicker(Qt::Key_T));

    // Volatility S&P 500
    mappings.clear();
    mappings.insert(LinkTypeTwinkorswim, "VIX");
    m_predefined.insert("$VIX", PredefinedTicker(Qt::Key_V, mappings));

    // S&P 500 Fund
    m_predefined.insert("SPY", PredefinedTicker(Qt::Key_F));

    PredefinedTickers::const_iterator itEnd = m_predefined.end();

    for(PredefinedTickers::const_iterator it = m_predefined.begin();it != itEnd;++it)
    {
        QShortcut *s = new QShortcut(it.value().key, this, SLOT(slotLoadPredefinedTicker()));

        s->setProperty("predefined-ticker-key", it.key());

        menu_load->addAction(it.key() + '\t' + QKeySequence(it.value().key).toString(), s, SIGNAL(activated()));
    }

    // lock links
    new QShortcut(Qt::CTRL+Qt::Key_L, this, SLOT(slotLockLinks()));
    new QShortcut(Qt::Key_L, this, SLOT(slotLoadTicker()));
    new QShortcut(QKeySequence::Find, this, SLOT(slotOpenOrCloseSearchTicker()));

    // db updater
    new TickersDatabaseUpdater(this);

    // sectors
    if(Settings::instance()->restoreNeighborsAtStartup() && Settings::instance()->showNeighborsAtStartup())
        slotShowNeighbors(startupTicker);

    // watch for QWhatsThisClickedEvent
    qApp->installEventFilter(this);
}

THT::~THT()
{
    Settings::instance()->setNyseOnly(ui->checkNyse->isChecked(), Settings::NoSync);

    if(Settings::instance()->saveGeometry())
    {
        Settings::instance()->setWindowSize(size(), Settings::NoSync);
        Settings::instance()->setWindowPosition(pos(), Settings::NoSync);
    }

    Settings::instance()->setShowNeighborsAtStartup(m_sectors);

    delete ui;
}

void THT::setVisible(bool vis)
{
    if(!vis)
    {
        m_lastActiveWindow = qApp->activeWindow();

        if(m_sectors)
            m_sectors->hide();
    }

    QWidget::setVisible(vis);
}

void THT::closeEvent(QCloseEvent *e)
{
    if(Settings::instance()->hideToTray())
    {
        if(!Settings::instance()->trayNoticeSeen())
        {
            Settings::instance()->setTrayNoticeSeen(true);
            m_tray->showMessage(tr("Notice"), tr("THT will continue to run in a system tray"), QSystemTrayIcon::Information, 7000);
        }

        e->ignore();
        hide();
    }
    else
    {
        e->accept();
        qApp->quit();
    }
}

bool THT::eventFilter(QObject *o, QEvent *e)
{
    if(e->type() == QEvent::WhatsThisClicked)
    {
        QWhatsThisClickedEvent *ce = static_cast<QWhatsThisClickedEvent *>(e);

        if(ce)
        {
            QUrl url(ce->href());

            if(url.isValid())
                QDesktopServices::openUrl(url);
        }

        return true;
    }

    return QObject::eventFilter(o, e);
}

void THT::contextMenuEvent(QContextMenuEvent *event)
{
    QApplication::restoreOverrideCursor();

    event->accept();
    m_menu->exec(event->globalPos());
}

void THT::sendKey(int key, bool extended)
{
    KEYBDINPUT kbInput = {0};
    INPUT input[4] = {{0}, {0}, {0}, {0}};

    int nelem = 2;
    int index = 0;

    // do we need SHIFT key? Don't check all the cases, we don't need all of them
    bool shift = (key >= '!' && key <= '+');

    // send SHIFT down
    if(shift)
    {
        nelem += 2;

        kbInput.dwFlags = 0;
        kbInput.wVk = VK_SHIFT;
        kbInput.wScan = MapVirtualKey(VK_SHIFT, 0);

        input[index].type = INPUT_KEYBOARD;
        input[index].ki = kbInput;

        ++index;
    }

    // key down
    SHORT vkey = VkKeyScan(key);

    if(extended)
        kbInput.dwFlags = KEYEVENTF_EXTENDEDKEY;

    kbInput.wVk = vkey;
    kbInput.wScan = MapVirtualKey(vkey, 0);

    input[index].type = INPUT_KEYBOARD;
    input[index].ki = kbInput;
    ++index;

    // key up
    kbInput.dwFlags = KEYEVENTF_KEYUP;

    if(extended)
        kbInput.dwFlags |= KEYEVENTF_EXTENDEDKEY;

    input[index].type = INPUT_KEYBOARD;
    input[index].ki = kbInput;
    ++index;

    // SHIFT up
    if(shift)
    {
        kbInput.dwFlags = KEYEVENTF_KEYUP;
        kbInput.wVk = VK_SHIFT;
        kbInput.wScan = MapVirtualKey(VK_SHIFT, 0);

        input[index].type = INPUT_KEYBOARD;
        input[index].ki = kbInput;
    }

    // send both combinations
    SendInput(nelem, input, sizeof(INPUT));
}

void THT::sendString(const QString &ticker, LinkType type)
{
    if(ticker.isEmpty())
    {
        qDebug("Ticker is empty, nothing to send");
        return;
    }

    for(int i = 0;i < ticker.length();i++)
        sendKey(ticker.at(i).toAscii());

    // Fix for TOS@paperMoney
    if(type == LinkTypeTwinkorswim)
        Sleep(5);

    sendKey(VK_RETURN);
}

void THT::rebuildUi()
{
    bool doResize = true;

    // check the number of lists
    int nlists = Settings::instance()->numberOfLists();

    // create more
    if(nlists > m_lists.size())
    {
        while(nlists > m_lists.size())
        {
            List *list = new List(m_lists.size()+1, this);

            connect(list, SIGNAL(copyLeft(QString,ListItem::Priority)),
                    this, SLOT(slotCopyLeft(QString,ListItem::Priority)));

            connect(list, SIGNAL(copyRight(QString,ListItem::Priority)),
                    this, SLOT(slotCopyRight(QString,ListItem::Priority)));

            connect(list, SIGNAL(copyTo(QString,ListItem::Priority,int)),
                    this, SLOT(slotCopyTo(QString,ListItem::Priority,int)));

            connect(list, SIGNAL(loadTicker(QString)),
                    this, SLOT(slotLoadTicker(QString)));

            connect(list, SIGNAL(dropped(QString,ListItem::Priority,QPoint)),
                    this, SLOT(slotTickerDropped(QString,ListItem::Priority,QPoint)));

            connect(list, SIGNAL(showNeighbors(QString)),
                    this, SLOT(slotShowNeighbors(QString)));

            connect(list, SIGNAL(needRebuildFinvizMenu()),
                    this, SLOT(slotNeedRebuildFinvizMenu()));

            m_layout->addWidget(list, 0, m_lists.size());
            m_lists.append(list);
        }
    }
    // delete useless
    else if(nlists < m_lists.size())
    {
        while(nlists < m_lists.size())
        {
            Settings::instance()->removeTickers(m_lists.size());

            delete m_lists.last();
            m_lists.removeLast();
        }
    }
    else
        doResize = false;

    bool saveTickers = Settings::instance()->saveTickers();

    foreach(List *l, m_lists)
    {
        l->setSaveTickers(saveTickers);
    }

    if(doResize)
        QTimer::singleShot(0, this, SLOT(slotAdjustSize()));
}

THT::Link THT::checkWindow(HWND hwnd)
{
    Link link(hwnd);

    // try to determine type
    link.threadId = GetWindowThreadProcessId(hwnd, &link.processId);

    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, link.processId);

    if(!h)
    {
        qWarning("Cannot open process %ld", link.processId);
        return link;
    }

    TCHAR name[MAX_PATH];

    // get executable name
    if(!GetProcessImageFileName(h, name, sizeof(name)))
    {
        qWarning("Cannot get a process info %ld (%ld)", link.processId, GetLastError());
        CloseHandle(h);
        return link;
    }

    QString sname = QFileInfo(
#ifdef UNICODE
        QString::fromWCharArray(name)
#else
        QString::fromUtf8(name)
#endif
        ).fileName().toLower();

    qDebug("Process name of %d is \"%s\"", (int)hwnd, qPrintable(sname));

    QString cname;

    if(!GetClassName(link.hwnd, name, sizeof(name)))
        qWarning("Cannot get a class name for window %d (%ld)", (int)link.hwnd, GetLastError());
    else
        cname =
#ifdef UNICODE
            QString::fromWCharArray(name);
#else
            QString::fromUtf8(name);
#endif

    if(sname == "advancedget.exe" || sname == "winsig.exe")
        link.type = LinkTypeAdvancedGet;
    else if(sname == "graybox.exe")
    {
        link.type = LinkTypeGraybox;
        link.findSubControl = grayBoxFindSubControl;
    }
    else if(sname == "thinkorswim.exe")
        link.type = LinkTypeTwinkorswim;
    else if((sname == "mbtdes~1.exe" || sname == "mbtdesktop.exe") && cname == "MbtTearFrame")
    {
        link.type = LinkTypeMBTDesktop;
        link.findSubControl = mbtFindSubControl;
    }
    else if((sname == "mbtdes~1.exe" || sname == "mbtdesktoppro.exe") && cname == "MbtNavPro_FloatFrame")
    {
        link.type = LinkTypeMBTDesktopPro;
        link.findSubControl = mbtProFindSubControl;
        link.waitForCaption = false;
    }
    else
        link.type = LinkTypeOther;

    if(link.type != LinkTypeNotInitialized)
        qDebug("Window under cursor %d", (int)link.hwnd);

    CloseHandle(h);

    return link;
}

THT::Link THT::checkTargetWindow(const QPoint &p, bool allowThisWindow)
{
    POINT pnt;

    pnt.x = p.x();
    pnt.y = p.y();

    HWND hwnd = RealChildWindowFromPoint(GetDesktopWindow(), pnt);

    if(!hwnd)
    {
        qDebug("Cannot find window under cursor %d,%d", p.x(), p.y());
        return Link();
    }

    // this window
    if(!allowThisWindow && hwnd == winId())
    {
        qDebug("Ignoring ourselves");
        return Link();
    }

    // desktop window?
    bool isDesktop = false;
    TCHAR classname[256];

    if(!GetClassName(hwnd, classname, sizeof(classname)))
    {
        qDebug("Cannot get class name for window %d", (int)hwnd);
    }
    else
    {
        LONG style = GetWindowLong(hwnd, GWL_STYLE);

        isDesktop = !lstrcmp(classname, TEXT("Progman")) && !(style & WS_CAPTION) && !GetParent(hwnd);
    }

    // desktop
    if(hwnd == GetDesktopWindow() || isDesktop)
    {
        qDebug("Ignoring desktop window");
        return Link();
    }

    // already linked?
    QList<Link>::iterator itEnd = m_windows->end();

    for(QList<Link>::iterator it = m_windows->begin();it != itEnd;++it)
    {
        if((*it).hwnd == hwnd)
        {
            qDebug("Window %d is already linked", (int)hwnd);
            return Link();
        }
    }

    return Link(hwnd);
}

void THT::checkWindows()
{
    RECT rect;

    QList<Link>::iterator itEnd = m_windows->end();

    for(QList<Link>::iterator it = m_windows->begin();it != itEnd;)
    {
        // remove dead windows
        if(!GetWindowRect((*it).hwnd, &rect))
        {
            qDebug("Window id %d is not valid, removing (%ld)", (int)(*it).hwnd, GetLastError());
            it = m_windows->erase(it);
            itEnd = m_windows->end();
        }
        else
            ++it;
    }

    if(m_windows == &m_windowsDrop)
        return;

    // join window types to a status string
    int ag = 0, gb = 0, o = 0;

    for(QList<Link>::iterator it = m_windows->begin();it != itEnd;++it)
    {
        if((*it).type == LinkTypeAdvancedGet)
            ++ag;
        else if((*it).type == LinkTypeGraybox)
            ++gb;
        else
            ++o;
    }

    if(ag || gb || o)
    {
        ui->stackLinks->setCurrentIndex(1);

        ui->labelAG->setNum(ag);
        ui->labelGB->setNum(gb);
        ui->labelO->setNum(o);
    }
    else
        ui->stackLinks->setCurrentIndex(0); // "No links" warning
}

void THT::nextLoadableWindowIndex(int startFrom)
{
    m_currentWindow += startFrom;

    if(m_ticker.startsWith('$'))
    {
        while(m_currentWindow < m_windows->size() && m_windows->at(m_currentWindow).type != LinkTypeAdvancedGet)
            m_currentWindow++;
    }
}

void THT::loadNextWindow()
{
    nextLoadableWindowIndex(+1);

    if(m_currentWindow >= m_windows->size())
    {
        qDebug("Done for all windows");

        if(m_windows == &m_windowsDrop)
        {
            qDebug("Clearing drop list");
            m_windows->clear();
        }

        busy(false);
        activate();
        m_running = false;
    }
    else
        m_timerLoadToNextWindow->start();
}

void THT::busy(bool b)
{
    foreach(List *l, m_lists)
        l->setIgnoreInput(b);

    ui->stackBusy->setCurrentIndex(b);
}

void THT::loadTicker(const QString &ticker)
{
    qDebug("Load ticker \"%s\"", qPrintable(ticker));

    if(m_locked)
    {
        qDebug("Locked");
        return;
    }

    if(m_running)
    {
        qDebug("In progress, won't load a new ticker");
        return;
    }

    if(m_sectors && sender() != m_sectors)
        m_sectors->showTicker(ticker);

    checkWindows();

    if(m_windows->isEmpty())
    {
        qDebug("No windows configured");
        m_running = false;
        return;
    }

    m_ticker = ticker;
    m_currentWindow = 0;
    nextLoadableWindowIndex();

    if(m_currentWindow >= m_windows->size())
    {
        qDebug("Cannot find where to load the ticker");
        return;
    }

    m_running = true;
    m_startupTime = QDateTime::currentMSecsSinceEpoch();

    busy(true);

    m_lastActiveWindow = qApp->activeWindow();

    m_timerLoadToNextWindow->start();
}

void THT::startDelayedScreenshot(bool allowKbd)
{
    m_useKeyboardInRegion = allowKbd;
    m_wasVisible = isVisible();

    hide();

    QTimer::singleShot(m_wasVisible ? 50 : 0, this, SLOT(slotTakeScreenshotReal()));
}

void THT::activate()
{
    qDebug("Activating");

    if(m_sectors)
        raiseWindow(m_sectors);

    raiseWindow(this);

    if(m_sectors && m_lastActiveWindow == m_sectors)
        m_sectors->activateWindow();
    else
        activateWindow();

    m_lastActiveWindow = 0;
}

void THT::slotAdjustSize()
{
    qDebug("Resizing automatically");

    layout()->activate();
    resize(minimumSizeHint().width(), height());
}

void THT::slotCheckActive()
{
    if(m_windows->isEmpty())
    {
        qDebug("Window list is empty");
        m_running = false;
        return;
    }

    Link &link = (*m_windows)[m_currentWindow];

    WINDOWINFO pwi = {0};

    if(link.waitForCaption)
    {
        pwi.cbSize = sizeof(WINDOWINFO);
        GetWindowInfo(link.hwnd, &pwi);
    }
    else
    {
        pwi.dwWindowStatus = WS_ACTIVECAPTION; // nice hack
        Sleep(5);
    }

    if(GetForegroundWindow() == link.hwnd && pwi.dwWindowStatus == WS_ACTIVECAPTION)
    {
        qDebug("Found window, sending data");

        QString add;
        bool okToLoad = false;

        // set focus to the subcontrol
        if(link.findSubControl)
        {
            HWND sc = link.cachedSubControl ? link.cachedSubControl : link.findSubControl(link.hwnd);

            if(!sc)
                qWarning("Cannot find a subcontrol");
            else
            {
                qDebug("Found the target subcontrol");

                link.cachedSubControl = sc;

                if(setForeignFocus(sc, link.threadId))
                    okToLoad = true;
            }
        }
        else
            okToLoad = true;

        // real ticker to load
        QString ticker;

        // determine the real ticker from the private ticker
        if(m_ticker.startsWith(THT_PRIVATE_TICKER_PREFIX))
        {
            ticker = m_ticker.right(m_ticker.length() - qstrlen(THT_PRIVATE_TICKER_PREFIX));

            // find mapping [ticker] => [predefined ticker data]
            PredefinedTickers::const_iterator it = m_predefined.find(ticker);

            if(it != m_predefined.end())
            {
                // find mapping [link type] => [ticker to load]
                PredefinedTickerMappings::const_iterator itm = it.value().mappings.find(link.type);

                // real ticker
                if(itm != it.value().mappings.end())
                {
                    if(!itm.value().isEmpty())
                        ticker = itm.value();
                }

                okToLoad = true;
            }
            else
                qWarning("Private ticker \"%s\" doesn't have a mapping", qPrintable(m_ticker));
        }
        else
        {
            ticker = m_ticker;
            okToLoad = true;
        }

        // load ticker
        if(okToLoad)
        {
            if(link.type == LinkTypeAdvancedGet
                && ui->checkNyse->isChecked()
                && !m_ticker.startsWith(QChar('$')))
            add = "=N";

            sendString(ticker + add, link.type);
        }

        loadNextWindow();
    }
    else
    {
        if(QDateTime::currentMSecsSinceEpoch() - m_startupTime > THT_WINDOW_STARTUP_TIMEOUT)
        {
            qDebug("Failed to wait the window due to timeout");
            loadNextWindow();
        }
        else
        {
            qDebug("Cannot find window, continuing to search");
            m_timerLoadToNextWindow->start();
        }
    }
}

void THT::slotAbout()
{
    About about(this);
    about.exec();
}

void THT::slotAboutQt()
{
    QMessageBox::aboutQt(this);
}

void THT::slotQuit()
{
    qApp->quit();
}

void THT::slotOptions()
{
    bool oldDups = Settings::instance()->allowDuplicates();
    bool oldMini = Settings::instance()->miniTickerEntry();

    Options opt(this);

    if(opt.exec() == QDialog::Accepted)
    {
        opt.save();
        rebuildUi();

        // always on top?
        QList<QWidget *> widgets = QList<QWidget *>() << this << m_sectors;

        foreach(QWidget *w, widgets)
        {
            if(!w)
                continue;

            Qt::WindowFlags flags = w->windowFlags();

            if(Settings::instance()->onTop())
                flags |= Qt::WindowStaysOnTopHint;
            else
                flags &= ~Qt::WindowStaysOnTopHint;

            w->setWindowFlags(flags);
            w->show();
        }

        m_tray->setVisible(Settings::instance()->hideToTray());

        // delete duplicates
        if(oldDups && !Settings::instance()->allowDuplicates())
        {
            foreach(List *l, m_lists)
                l->removeDuplicates();
        }

        // reconfigure mini ticker entry
        if(oldMini != Settings::instance()->miniTickerEntry())
        {
            foreach(List *l, m_lists)
                l->reconfigureMiniTickerEntry();
        }

        // reset geometry
        if(!Settings::instance()->saveGeometry())
        {
            Settings::instance()->setWindowSize(QSize(), Settings::NoSync);
            Settings::instance()->setWindowPosition(QPoint(), Settings::NoSync);

            Settings::instance()->setNeighborsWindowSize(QSize(), Settings::NoSync);
            Settings::instance()->setNeighborsWindowPosition(QPoint(), Settings::Sync);
        }
    }
}

void THT::slotCopyLeft(const QString &ticker, ListItem::Priority p)
{
    qDebug("Copy ticker \"%s\" left", qPrintable(ticker));

    int index = m_lists.indexOf(qobject_cast<List *>(sender()));

    if(index < 0)
    {
        qWarning("Cannot find where to copy the ticker");
        return;
    }

    if(m_lists.size() < 2)
    {
        qWarning("Cannot copy from the single list");
        return;
    }

    if(!index)
        index = m_lists.size() - 1;
    else
        --index;

    m_lists[index]->addTicker(ticker, p);
}

void THT::slotCopyRight(const QString &ticker, ListItem::Priority p)
{
    qDebug("Copy ticker \"%s\"/%d right", qPrintable(ticker), p);

    int index = m_lists.indexOf(qobject_cast<List *>(sender()));

    if(index < 0)
    {
        qWarning("Cannot find where to copy the ticker");
        return;
    }

    if(m_lists.size() < 2)
    {
        qWarning("Cannot copy from the single list");
        return;
    }

    if(index == m_lists.size()-1)
        index = 0;
    else
        ++index;

    m_lists[index]->addTicker(ticker, p);
}

void THT::slotCopyTo(const QString &ticker, ListItem::Priority p, int index)
{
    qDebug("Copy ticker \"%s\" to list #%d", qPrintable(ticker), index);

    if(index < 0 || index >= m_lists.size())
    {
        qWarning("Cannot find where to copy the ticker");
        return;
    }

    m_lists[index]->addTicker(ticker, p);
}

void THT::slotLoadTicker(const QString &ticker)
{
    m_windows = &m_windowsLoad;
    loadTicker(ticker);
}

void THT::slotLoadTicker()
{
    if(Settings::instance()->miniTickerEntry())
    {
        // find where to set the focus
        const QWidget *focused = focusWidget();
        List *found = 0;

        foreach(List *l, m_lists)
        {
            if(l->isAncestorOf(focused))
            {
                found = l;
                break;
            }
        }

        if(!found && m_lists.size())
            found = m_lists.at(0);

        found->focusMiniTickerEntry();
    }
    else
    {
        TickerInput ti(this);

        if(ti.exec() != QDialog::Accepted)
            return;

        slotLoadTicker(ti.ticker());
    }
}

void THT::slotLoadToNextWindow()
{
    HWND window = m_windows->at(m_currentWindow).hwnd;

    qDebug("Trying window %d", (int)window);

    // window flags to set
    int flags = SW_SHOWNORMAL;

    if(IsZoomed(window))
        flags |= SW_SHOWMAXIMIZED;

    // try to switch to this window
    ShowWindow(window, flags);
    SetForegroundWindow(window);

    m_timerCheckActive->start();
}

void THT::slotTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch(reason)
    {
        case QSystemTrayIcon::Trigger:
        {
            if(!isVisible() || isMinimized())
                activate();
            else
                hide();
        }
        break;

        default: ;
    }
}

void THT::slotTakeScreenshotFromGlobal()
{
    startDelayedScreenshot(qApp->activeWindow());
}

void THT::slotTakeScreenshot()
{
    startDelayedScreenshot(true);
}

void THT::slotTakeScreenshotReal()
{
    RegionSelect selector(m_useKeyboardInRegion
                          ? RegionSelect::UseKeyboard
                          : RegionSelect::DontUseKeyboard);
    QPixmap px;

    // ignore screenshot
    if(selector.exec() != QDialog::Accepted)
    {
        if(m_wasVisible)
            activate();

        return;
    }

    px = selector.selection();

    if(px.isNull())
    {
        QMessageBox::critical(this, tr("Error"), tr("Cannot take screenshot"));

        if(m_wasVisible)
            activate();

        return;
    }

    if(m_wasVisible)
        activate();

    m_takeScreen->setDisabled();

    // save screenshot
    SaveScreenshot s(px, this);

    s.exec();

    m_takeScreen->setEnabled();
}

void THT::slotClearLists()
{
    foreach(List *l, m_lists)
        l->clear();
}

void THT::slotClearLinks()
{
    qDebug("Clear links");

    m_windows = &m_windowsLoad;

    if(m_windows->isEmpty())
        return;

    MessageBeep(MB_OK);

    m_windows->clear();

    checkWindows();
}

void THT::slotManageLinks()
{
    QList<QPoint> links;

    foreach(Link l, m_windowsLoad)
    {
        links.append(l.dropPoint);
    }

    LinkPointManager mgr(links, this);

    if(mgr.exec() == QDialog::Accepted && mgr.changed())
    {
        Settings::instance()->setLinks(mgr.links());
        rebuildLinkPoints();
    }
}

void THT::slotLoadLinks()
{
    QAction *a = qobject_cast<QAction *>(sender());

    if(!a)
        return;

    QList<QPoint> links = a->data().value<QList<QPoint> >();

    slotClearLinks();

    foreach(QPoint p, links)
    {
        slotTargetDropped(p);
    }
}

void THT::slotLockLinks()
{
    m_locked = !m_locked;

    ui->stackBusy->setCurrentIndex(m_locked);

    static QWidgetList labels = QWidgetList()
                                << ui->labelAG
                                << ui->labelAG_n
                                << ui->labelGB
                                << ui->labelGB_n
                                << ui->labelO
                                << ui->labelO_n;

    QColor color = m_locked ? Qt::red : palette().color(QPalette::WindowText);
    QPalette pal;

    foreach(QWidget *l, labels)
    {
        pal = l->palette();
        pal.setColor(QPalette::WindowText, color);
        l->setPalette(pal);
    }
}

void THT::slotTickerDropped(const QString &t, ListItem::Priority pr, const QPoint &p)
{
    m_windows = &m_windowsDrop;
    m_windows->clear();

    Link link = checkTargetWindow(p, true);

    if(!link.hwnd)
        return;

    // our window
    if(link.hwnd == winId())
    {
        int index = 0;

        foreach(List *l, m_lists)
        {
            if(l->contains(p))
            {
                qDebug("Dropped onto a list");
                slotCopyTo(t, pr, index);
                break;
            }

            index++;
        }

        return;
    }

    link = checkWindow(link.hwnd);

    if(link.type == LinkTypeNotInitialized)
        return;

    m_windows->append(link);

    loadTicker(t);
}

void THT::slotTargetDropped(const QPoint &p)
{
    m_windows = &m_windowsLoad;

    Link link = checkTargetWindow(p, false);

    if(!link.hwnd)
        return;

    link = checkWindow(link.hwnd);

    if(link.type == LinkTypeNotInitialized)
        return;

    link.dropPoint = p;

    // beep
    MessageBeep(MB_OK);

    m_windows->append(link);

    checkWindows();
}

// IPC message
void THT::slotMessageReceived(const QString &msg)
{
    qDebug("Got message \"%s\"", qPrintable(msg));

    if(msg == "wake up")
        activate();
}

void THT::slotLoadPredefinedTicker()
{
    QShortcut *s = qobject_cast<QShortcut *>(sender());

    if(!s)
        return;

    QString ticker = s->property("predefined-ticker-key").toString();

    if(ticker.isEmpty())
        return;

    slotLoadTicker(THT_PRIVATE_TICKER_PREFIX + ticker);
}

void THT::slotOpenOrCloseSearchTicker()
{
    bool done = false;

    foreach(List *l, m_lists)
    {
        if(l->searching())
        {
            l->stopSearching();
            done = true;
        }
    }

    if(done)
        return;

    foreach(List *l, m_lists)
    {
        if(l->hasFocus())
        {
            l->startSearching();
            break;
        }
    }
}

void THT::slotShowNeighbors(const QString &ticker)
{
    if(m_sectors)
    {
        m_sectors->showTicker(ticker);
        raiseWindow(m_sectors);
        m_sectors->activateWindow();
        return;
    }

    m_sectors = new TickerNeighbors(ticker, this);

    if(Settings::instance()->onTop())
        m_sectors->setWindowFlags(m_sectors->windowFlags() | Qt::WindowStaysOnTopHint);

    connect(m_sectors, SIGNAL(loadTicker(QString)), this, SLOT(slotLoadTicker(QString)));

    m_sectors->show();
}

void THT::slotNeedRebuildFinvizMenu()
{
    foreach(List *l, m_lists)
    {
        l->rebuildFinvizMenu();
    }
}

bool THT::setForeignFocus(HWND window, DWORD threadId)
{
    const DWORD currentThreadId = GetCurrentThreadId();

    if(!AttachThreadInput(threadId, currentThreadId, TRUE))
    {
        qWarning("Cannot attach to the thread %ld (%ld)", threadId, GetLastError());
        return false;
    }

    HWND hwnd = SetFocus(window);

    AttachThreadInput(threadId, currentThreadId, FALSE);

    if(!hwnd)
    {
        qWarning("Cannot set focus to the window %d (%ld)", (int)window, GetLastError());
        return false;
    }

    return true;
}

void THT::rebuildLinkPoints()
{
    qDebug("Rebuild link menu");

    QList<LinkPoint> linkpoints = Settings::instance()->links();

    if(!ui->pushLinks->menu())
    {
        QMenu *menu = new QMenu(ui->pushLinks);
        ui->pushLinks->setMenu(menu);
    }

    QMenu *menu = ui->pushLinks->menu();

    menu->clear();

    foreach(LinkPoint lp, linkpoints)
    {
        QAction *a = menu->addAction(lp.name, this, SLOT(slotLoadLinks()));
        a->setData(QVariant::fromValue(lp.points));
    }

    if(!linkpoints.isEmpty())
        menu->addSeparator();

    menu->addAction(QIcon(":/images/links-customize.png"), tr("Customize..."), this, SLOT(slotManageLinks()));
}

void THT::raiseWindow(QWidget *w)
{
    if(!w)
        return;

    w->show();
    w->setWindowState(w->windowState() & ~Qt::WindowMinimized);
    w->raise();
}

HWND THT::grayBoxFindSubControl(HWND parent)
{
    return FindWindowEx(parent, 0, TEXT("Edit"), 0);
}

HWND THT::mbtFindSubControl(HWND parent)
{
    HWND after = 0, r;
    TCHAR cname[MAX_PATH];
    bool final = false;

    const TCHAR *cedit = TEXT("Edit");
    const int cedit_len = lstrlen(cedit);

    const TCHAR *ctoolbar = TEXT("ToolbarWindow32");
    const int ctoolbar_len = lstrlen(ctoolbar);

    if(!GetClassName(parent, cname, sizeof(cname)))
    {
        qWarning("Failed to get a class name for window %d (%ld)", (int)parent, GetLastError());
        return 0;
    }
    else if(CompareString(LOCALE_USER_DEFAULT, 0, cname, lstrlen(cname), ctoolbar, ctoolbar_len) == CSTR_EQUAL)
        final = true;

    while((after = FindWindowEx(parent, after, 0, 0)))
    {
        if(!GetClassName(after, cname, sizeof(cname)))
        {
            qWarning("Failed to get a class name for window %d (%ld)", (int)after, GetLastError());
            continue;
        }

        if(final && CompareString(LOCALE_USER_DEFAULT, 0, cname, lstrlen(cname), cedit, cedit_len) == CSTR_EQUAL)
            return after;
        else if((r = mbtFindSubControl(after)))
            return r;
    }

    return 0;
}

HWND THT::mbtProFindSubControl(HWND parent)
{
    HWND after = 0;
    TCHAR cname[MAX_PATH];

    const TCHAR *ctoolbar = TEXT("BCGPToolBar:");
    const int ctoolbar_len = lstrlen(ctoolbar);

    while((after = FindWindowEx(parent, after, 0, 0)))
    {
        if(!GetClassName(after, cname, sizeof(cname)))
        {
            qWarning("Failed to get a class name for window %d (%ld)", (int)after, GetLastError());
            continue;
        }

        if(CompareString(LOCALE_USER_DEFAULT, 0, cname, ctoolbar_len, ctoolbar, ctoolbar_len) == CSTR_EQUAL)
        {
            after = FindWindowEx(after, 0, TEXT("ComboBox"), 0);
            break;
        }
    }

    return after;
}
