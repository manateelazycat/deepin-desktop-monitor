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

    // Init window size.
    QRect screenRect = qApp->primaryScreen()->geometry();
    this->setFixedSize(screenRect.width(), screenRect.height());
    
    // Init.
    initTheme();

    layout = new QHBoxLayout();
    layout->setContentsMargins(100, 0, 0, 0);

    this->setLayout(layout);

    statusMonitor = new StatusMonitor();

    statusMonitor->updateStatus();

    layout->addWidget(statusMonitor, 0, Qt::AlignLeft);
}

MainWindow::~MainWindow()
{
    // We don't need clean pointers because application has exit here.
}

bool MainWindow::eventFilter(QObject *, QEvent *)
{
    return false;
}

void MainWindow::initTheme()
{
}

void MainWindow::paintEvent(QPaintEvent *)
{
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
