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

#include "QVBoxLayout"
#include "dthememanager.h"
#include "list_view.h"
#include "process_item.h"
#include "process_manager.h"
#include <DDesktopServices>
#include <QApplication>
#include <QDebug>
#include <QList>
#include <QProcess>
#include <QStyleFactory>
#include <QToolTip>
#include <proc/sysinfo.h>
#include <signal.h>

DCORE_USE_NAMESPACE

using namespace Utils;

ProcessManager::ProcessManager(int tabIndex, QList<bool> columnHideFlags, int sortingIndex, bool sortingOrder)
{
    // Init widget.
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    processView = new ProcessView(columnHideFlags);
    connect(processView, &ListView::changeColumnVisible, this, &ProcessManager::changeColumnVisible);
    connect(processView, &ListView::changeSortingStatus, this, &ProcessManager::changeSortingStatus);
    
    layout->addWidget(processView);

    initTheme();
    
    connect(DThemeManager::instance(), &DThemeManager::themeChanged, this, &ProcessManager::changeTheme);
    
    // Set sort algorithms.
    QList<SortAlgorithm> *alorithms = new QList<SortAlgorithm>();
    alorithms->append(&ProcessItem::sortByName);
    alorithms->append(&ProcessItem::sortByCPU);
    alorithms->append(&ProcessItem::sortByMemory);
    alorithms->append(&ProcessItem::sortByDiskWrite);
    alorithms->append(&ProcessItem::sortByDiskRead);
    alorithms->append(&ProcessItem::sortByNetworkDownload);
    alorithms->append(&ProcessItem::sortByNetworkUpload);
    alorithms->append(&ProcessItem::sortByPid);
    processView->setColumnSortingAlgorithms(alorithms, sortingIndex, sortingOrder);
    processView->setSearchAlgorithm(&ProcessItem::search);
}

ProcessManager::~ProcessManager()
{
    delete processView;
}

void ProcessManager::initTheme()
{
}

void ProcessManager::changeTheme(QString )
{
    initTheme();
}

ProcessView* ProcessManager::getProcessView()
{
    return processView;
}

void ProcessManager::updateStatus(QList<ListItem*> items)
{
    processView->refreshItems(items);
}
