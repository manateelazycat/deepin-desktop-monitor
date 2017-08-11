/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2011 ~ 2017 Deepin, Inc.
 *               2011 ~ 2017 Wang Yong
 *
 * Author:     Wang Yong <wangyong@deepin.com>
 * Maintainer: Wang Yong <wangyong@deepin.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "constant.h"
#include "dthememanager.h"
#include "dwindowmanagerhelper.h"
#include "main_window.h"
#include <DTitlebar>
#include <QApplication>
#include <QDebug>
#include <QScreen>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QStyleFactory>
#include <iostream>
#include <signal.h>
#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <QtX11Extras/QX11Info>

using namespace std;

MainWindow::MainWindow(QWidget *parent) : QWidget(parent)
{
    installEventFilter(this);   // add event filter

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowTransparentForInput);
    setParent(0); // Create TopLevel-Widget
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_TranslucentBackground, true);

    settings = new Settings(this);
    settings->init();

    // Init window size.
    QRect screenRect = qApp->primaryScreen()->geometry();
    this->setFixedSize(screenRect.width(), screenRect.height());
    
    // Init.
    initTheme();

    layout = new QHBoxLayout();

    this->setLayout(layout);

    int tabIndex = 1;

    processManager = new ProcessManager(tabIndex, getColumnHideFlags(), getSortingIndex(), getSortingOrder());
    processManager->getProcessView()->installEventFilter(this);
    statusMonitor = new StatusMonitor(tabIndex);

    connect(processManager, &ProcessManager::activeTab, this, &MainWindow::switchTab);
    connect(processManager, &ProcessManager::changeColumnVisible, this, &MainWindow::recordVisibleColumn);
    connect(processManager, &ProcessManager::changeSortingStatus, this, &MainWindow::recordSortingStatus);

    connect(statusMonitor, &StatusMonitor::updateProcessStatus, processManager, &ProcessManager::updateStatus, Qt::QueuedConnection);

    statusMonitor->updateStatus();

    layout->addWidget(statusMonitor);
    layout->addWidget(processManager);

    killPid = -1;

    killProcessDialog = new DDialog(QString(tr("End application")), QString(tr("Ending this application may cause data loss.\nAre you sure to continue?")), this);
    killProcessDialog->setWindowFlags(killProcessDialog->windowFlags() | Qt::WindowStaysOnTopHint);
    killProcessDialog->setIcon(QIcon(Utils::getQrcPath("deepin-desktop-monitor.svg")));
    killProcessDialog->addButton(QString(tr("Cancel")), false, DDialog::ButtonNormal);
    killProcessDialog->addButton(QString(tr("End application")), true, DDialog::ButtonNormal);
    connect(killProcessDialog, &DDialog::buttonClicked, this, &MainWindow::dialogButtonClicked);
}

MainWindow::~MainWindow()
{
    // We don't need clean pointers because application has exit here.
}

QList<bool> MainWindow::getColumnHideFlags()
{
    QString processColumns = settings->getOption("process_columns").toString();

    QList<bool> toggleHideFlags;
    toggleHideFlags << true;
    toggleHideFlags << true;
    toggleHideFlags << true;
    toggleHideFlags << true;
    toggleHideFlags << true;
    toggleHideFlags << true;
    toggleHideFlags << true;
    toggleHideFlags << true;

    return toggleHideFlags;
}

bool MainWindow::eventFilter(QObject *, QEvent *event)
{
    return false;
}

int MainWindow::getSortingIndex()
{
    QString sortingName = settings->getOption("process_sorting_column").toString();

    QList<QString> columnNames = {
        "name", "cpu", "memory", "disk_write", "disk_read", "download", "upload", "pid"
    };

    return columnNames.indexOf(sortingName);
}

bool MainWindow::getSortingOrder()
{
    return settings->getOption("process_sorting_order").toBool();
}

void MainWindow::initTheme()
{
    QString theme = settings->getOption("theme_style").toString();
    DThemeManager::instance()->setTheme(theme);
}

void MainWindow::paintEvent(QPaintEvent *)
{
}

void MainWindow::dialogButtonClicked(int index, QString)
{
    if (index == 1) {
        if (killPid != -1) {
            if (kill(killPid, SIGKILL) != 0) {
                cout << "Kill failed." << endl;
            }

            killPid = -1;
        }
    }
}

void MainWindow::popupKillConfirmDialog(int pid)
{
    killPid = pid;
    killProcessDialog->show();
}

void MainWindow::recordSortingStatus(int index, bool sortingOrder)
{
    QList<QString> columnNames = {
        "name", "cpu", "memory", "disk_write", "disk_read", "download", "upload", "pid"
    };

    settings->setOption("process_sorting_column", columnNames[index]);
    settings->setOption("process_sorting_order", sortingOrder);
}

void MainWindow::recordVisibleColumn(int, bool, QList<bool> columnVisibles)
{
    QList<QString> visibleColumns;
    visibleColumns << "name";


    if (columnVisibles[1]) {
        visibleColumns << "cpu";
    }

    if (columnVisibles[2]) {
        visibleColumns << "memory";
    }

    if (columnVisibles[3]) {
        visibleColumns << "disk_write";
    }

    if (columnVisibles[4]) {
        visibleColumns << "disk_read";
    }

    if (columnVisibles[5]) {
        visibleColumns << "download";
    }

    if (columnVisibles[6]) {
        visibleColumns << "upload";
    }

    if (columnVisibles[7]) {
        visibleColumns << "pid";
    }

    QString processColumns = "";
    for (int i = 0; i < visibleColumns.length(); i++) {
        if (i != visibleColumns.length() - 1) {
            processColumns += QString("%1,").arg(visibleColumns[i]);
        } else {
            processColumns += visibleColumns[i];
        }
    }

    settings->setOption("process_columns", processColumns);
}

void MainWindow::switchTab(int index)
{
    if (index == 0) {
        statusMonitor->switchToOnlyGui();
    } else if (index == 1) {
        statusMonitor->switchToOnlyMe();
    } else {
        statusMonitor->switchToAllProcess();
    }

    settings->setOption("process_tab_index", index);
}

void MainWindow::registerDesktop()
{
    xcb_ewmh_connection_t m_ewmh_connection;
    xcb_intern_atom_cookie_t *cookie = xcb_ewmh_init_atoms(QX11Info::connection(), &m_ewmh_connection);
    xcb_ewmh_init_atoms_replies(&m_ewmh_connection, cookie, NULL);

    xcb_atom_t atoms[1];
    atoms[0] = m_ewmh_connection._NET_WM_WINDOW_TYPE_DESKTOP;
    xcb_ewmh_set_wm_window_type(&m_ewmh_connection, winId(), 1, atoms);

    show();
    lower();
}
