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
#include "status_monitor.h"
#include "utils.h"
#include <QDebug>
#include <QPainter>
#include <proc/sysinfo.h>
#include <thread>
#include <unistd.h>

using namespace Utils;

StatusMonitor::StatusMonitor()
{
    // Init size.
    setFixedWidth(Utils::getStatusBarMaxWidth());

    // Init attributes.
    processCpuPercents = new QMap<int, double>();
    
    totalCpuTime = 0;
    workCpuTime = 0;
    prevTotalCpuTime = 0;
    prevWorkCpuTime = 0;
    currentUsername = qgetenv("USER");

    prevTotalRecvBytes = 0;
    prevTotalSentBytes = 0;

    updateSeconds = updateDuration / 1000.0;

    // Init widgets.
    layout = new QVBoxLayout(this);

    cpuMonitor = new CpuMonitor();
    memoryMonitor = new MemoryMonitor();
    networkMonitor = new NetworkMonitor();

    layout->addWidget(cpuMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(memoryMonitor, 0, Qt::AlignHCenter);
    layout->addWidget(networkMonitor, 0, Qt::AlignHCenter);

    connect(this, &StatusMonitor::updateMemoryStatus, memoryMonitor, &MemoryMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateCpuStatus, cpuMonitor, &CpuMonitor::updateStatus, Qt::QueuedConnection);
    connect(this, &StatusMonitor::updateNetworkStatus, networkMonitor, &NetworkMonitor::updateStatus, Qt::QueuedConnection);

    // Start timer.
    updateStatusTimer = new QTimer(this);
    connect(updateStatusTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    updateStatusTimer->start(updateDuration);
}

StatusMonitor::~StatusMonitor()
{
}

void StatusMonitor::updateStatus()
{
    // Read the list of open processes information.
    PROCTAB* proc = openproc(
        PROC_FILLMEM |          // memory status: read information from /proc/#pid/statm
        PROC_FILLSTAT |         // cpu status: read information from /proc/#pid/stat
        PROC_FILLUSR            // user status: resolve user ids to names via /etc/passwd
        );
    static proc_t proc_info;
    memset(&proc_info, 0, sizeof(proc_t));

    StoredProcType processes;
    while (readproc(proc, &proc_info) != NULL) {
        processes[proc_info.tid] = proc_info;
    }
    closeproc(proc);

    // Fill in CPU.
    prevWorkCpuTime = workCpuTime;
    prevTotalCpuTime = totalCpuTime;
    totalCpuTime = getTotalCpuTime(workCpuTime);

    processCpuPercents->clear();
    if (prevProcesses.size()>0) {
        // we have previous proc info
        for (auto &newItr:processes) {
            for (auto &prevItr:prevProcesses) {
                if (newItr.first == prevItr.first) {
                    // PID matches, calculate the cpu
                    (*processCpuPercents)[newItr.second.tid] = calculateCPUPercentage(&prevItr.second, &newItr.second, prevTotalCpuTime, totalCpuTime);

                    break;
                }
            }
        }
    }

    // Read memory information.
    meminfo();

    // Update memory status.
    if (kb_swap_total > 0.0)  {
        updateMemoryStatus((kb_main_total - kb_main_available) * 1024, kb_main_total * 1024, kb_swap_used * 1024, kb_swap_total * 1024);
    } else {
        updateMemoryStatus((kb_main_total - kb_main_available) * 1024, kb_main_total * 1024, 0, 0);
    }

    // Update cpu status.
    if (prevWorkCpuTime != 0 && prevTotalCpuTime != 0) {
        updateCpuStatus((workCpuTime - prevWorkCpuTime) * 100.0 / (totalCpuTime - prevTotalCpuTime));
    } else {
        updateCpuStatus(0);
    }

    // Update network status.
    if (prevTotalRecvBytes == 0) {
        prevTotalRecvBytes = totalRecvBytes;
        prevTotalSentBytes = totalSentBytes;

        Utils::getNetworkBandWidth(totalRecvBytes, totalSentBytes);
        updateNetworkStatus(totalRecvBytes, totalSentBytes, 0, 0);
    } else {
        prevTotalRecvBytes = totalRecvBytes;
        prevTotalSentBytes = totalSentBytes;

        Utils::getNetworkBandWidth(totalRecvBytes, totalSentBytes);
        updateNetworkStatus(totalRecvBytes,
                            totalSentBytes,
                            ((totalRecvBytes - prevTotalRecvBytes) / 1024.0) / updateSeconds,
                            ((totalSentBytes - prevTotalSentBytes) / 1024.0) / updateSeconds);
    }

    // Keep processes we've read for cpu calculations next cycle.
    prevProcesses = processes;
}
