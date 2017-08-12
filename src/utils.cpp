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

#include "hashqstring.h"
#include "utils.h"
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFontMetrics>
#include <QIcon>
#include <QLayout>
#include <QPainter>
#include <QPixmap>
#include <QStandardPaths>
#include <QString>
#include <QWidget>
#include <QtMath>
#include <QtX11Extras/QX11Info>
#include <X11/extensions/shape.h>
#include <fstream>
#include <qdiriterator.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <time.h>
#include <unordered_set>

namespace Utils {
    int getStatusBarMaxWidth()
    {
        int offset = 171;
        QString swapTitle = QString("%1 (%2)").arg(QObject::tr("Swap")).arg(QObject::tr("Not enabled"));

        return std::max(400, getRenderSize(9, swapTitle).width() + offset);
    }

    QSize getRenderSize(int fontSize, QString string)
    {
        QFont font;
        font.setPointSize(fontSize);
        QFontMetrics fm(font);

        int width = 0;
        int height = 0;
        foreach (auto line, string.split("\n")) {
            int lineWidth = fm.width(line);
            int lineHeight = fm.height();

            if (lineWidth > width) {
                width = lineWidth;
            }
            height += lineHeight;
        }

        return QSize(width, height);
    }

    QString formatBandwidth(double v)
    {
        static const char* orders[] = { "KB/s", "MB/s", "GB/s", "TB/s" };

        return formatUnitSize(v, orders, sizeof(orders)/sizeof(orders[0]));
    }

    QString formatByteCount(double v)
    {
        static const char* orders[] = { "B", "KB", "MB", "GB", "TB" };

        return formatUnitSize(v, orders, sizeof(orders)/sizeof(orders[0]));
    }

    QString formatUnitSize(double v, const char** orders, int nb_orders)
    {
        int order = 0;
        while (v >= 1024 && order + 1 < nb_orders) {
            order++;
            v  = v/1024;
        }
        char buffer1[30];
        snprintf(buffer1, sizeof(buffer1), "%.1lf %s", v, orders[order]);

        return QString(buffer1);
    }

    QString formatMillisecond(int millisecond)
    {
        if (millisecond / 1000 < 3600) {
            // At least need return 1 seconds.
            return QDateTime::fromTime_t(std::max(1, millisecond / 1000)).toUTC().toString("mm:ss");
        } else {
            return QDateTime::fromTime_t(millisecond / 1000).toUTC().toString("hh:mm:ss");
        }
    }

    QString getImagePath(QString imageName)
    {
        QDir dir(qApp->applicationDirPath());
        dir.cdUp();

        return QDir(dir.filePath("image")).filePath(imageName);
    }

    QString getQrcPath(QString imageName)
    {
        return QString(":/image/%1").arg(imageName);
    }

    QString getQssPath(QString qssName)
    {
        return QString(":/qss/%1").arg(qssName);
    }

    bool fileExists(QString path)
    {
        QFileInfo check_file(path);

        // check if file exists and if yes: Is it really a file and no directory?
        return check_file.exists() && check_file.isFile();
    }

    double calculateCPUPercentage(const proc_t* before, const proc_t* after, const unsigned long long &prevCpuTime, const unsigned long long &cpuTime)
    {
        double totalCpuTime = cpuTime - prevCpuTime;
        unsigned long long processcpuTime = ((after->utime + after->stime) - (before->utime + before->stime));

        return (processcpuTime / totalCpuTime) * 100.0;
    }

    qreal easeInOut(qreal x)
    {
        return (1 - qCos(M_PI * x)) / 2;
    }

    qreal easeInQuad(qreal x)
    {
        return qPow(x, 2);
    }

    qreal easeOutQuad(qreal x)
    {
        return -1 * qPow(x - 1, 2) + 1;
    }

    qreal easeInQuint(qreal x)
    {
        return qPow(x, 5);
    }

    qreal easeOutQuint(qreal x)
    {
        return qPow(x - 1, 5) + 1;
    }

    /**
     * @brief getTotalCpuTime Read the data from /proc/stat and get the total time the cpu has been busy
     * @return The total cpu time
     */
    unsigned long long getTotalCpuTime(unsigned long long &workTime)
    {
        FILE* file = fopen("/proc/stat", "r");
        if (file == NULL) {
            perror("Could not open stat file");
            return 0;
        }

        char buffer[1024];
        unsigned long long user = 0, nice = 0, system = 0, idle = 0;
        // added between Linux 2.5.41 and 2.6.33, see man proc(5)
        unsigned long long iowait = 0, irq = 0, softirq = 0, steal = 0, guest = 0, guestnice = 0;

        char* ret = fgets(buffer, sizeof(buffer) - 1, file);
        if (ret == NULL) {
            perror("Could not read stat file");
            fclose(file);
            return 0;
        }
        fclose(file);

        sscanf(buffer,
               "cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal, &guest, &guestnice);

        workTime = user + nice + system;

        // sum everything up (except guest and guestnice since they are already included
        // in user and nice, see http://unix.stackexchange.com/q/178045/20626)
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }

    void addLayoutWidget(QLayout *layout, QWidget *widget)
    {
        layout->addWidget(widget);
        widget->show();
    }

    void applyQss(QWidget *widget, QString qssName)
    {
        QFile file(getQssPath(qssName));
        file.open(QFile::ReadOnly);
        QTextStream filetext(&file);
        QString stylesheet = filetext.readAll();
        widget->setStyleSheet(stylesheet);
        file.close();
    }

    void blurRect(DWindowManager *windowManager, int widgetId, QRectF rect)
    {
        QVector<uint32_t> data;

        data << rect.x() << rect.y() << rect.width() << rect.height() << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
        windowManager->setWindowBlur(widgetId, data);
    }

    void blurRects(DWindowManager *windowManager, int widgetId, QList<QRectF> rects)
    {
        QVector<uint32_t> data;
        foreach (auto rect, rects) {
            data << rect.x() << rect.y() << rect.width() << rect.height() << RECTANGLE_RADIUS << RECTANGLE_RADIUS;
        }
        windowManager->setWindowBlur(widgetId, data);
    }

    void clearBlur(DWindowManager *windowManager, int widgetId)
    {
        QVector<uint32_t> data;
        data << 0 << 0 << 0 << 0 << 0 << 0;
        windowManager->setWindowBlur(widgetId, data);
    }

    void drawLoadingRing(QPainter &painter,
                         int centerX,
                         int centerY,
                         int radius,
                         int penWidth,
                         int loadingAngle,
                         int rotationAngle,
                         QString foregroundColor,
                         double foregroundOpacity,
                         QString backgroundColor,
                         double backgroundOpacity,
                         double percent)
    {
        drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle, rotationAngle, backgroundColor, backgroundOpacity);
        drawRing(painter, centerX, centerY, radius, penWidth, loadingAngle * percent, rotationAngle, foregroundColor, foregroundOpacity);
    }

    void drawRing(QPainter &painter, int centerX, int centerY, int radius, int penWidth, int loadingAngle, int rotationAngle, QString color, double opacity)
    {
        QRect drawingRect;

        drawingRect.setX(centerX - radius + penWidth);
        drawingRect.setY(centerY - radius + penWidth);
        drawingRect.setWidth(radius * 2 - penWidth * 2);
        drawingRect.setHeight(radius * 2 - penWidth * 2);

        painter.setOpacity(opacity);
        painter.setRenderHint(QPainter::Antialiasing);

        QPen pen(QBrush(QColor(color)), penWidth);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);

        int arcLengthApproximation = penWidth + penWidth / 3;
        painter.drawArc(drawingRect, 90 * 16 - arcLengthApproximation + rotationAngle * 16, -loadingAngle * 16);
    }

    void drawTooltipBackground(QPainter &painter, QRect rect, qreal opacity)
    {
        painter.setOpacity(opacity);
        QPainterPath path;
        path.addRoundedRect(QRectF(rect), RECTANGLE_RADIUS, RECTANGLE_RADIUS);
        painter.fillPath(path, QColor("#F5F5F5"));

        QPen pen(QColor("#000000"));
        painter.setOpacity(0.04);
        pen.setWidth(1);
        painter.setPen(pen);
        painter.drawPath(path);
    }

    void drawTooltipText(QPainter &painter, QString text, QString textColor, int textSize, QRectF rect)
    {
        setFontSize(painter, textSize);
        painter.setOpacity(1);
        painter.setPen(QPen(QColor(textColor)));
        painter.drawText(rect, Qt::AlignCenter, text);
    }

    void getNetworkBandWidth(unsigned long long int &receiveBytes, unsigned long long int &sendBytes)
    {
        char *buf;
        static int bufsize;
        FILE *devfd;

        buf = (char *) calloc(255, 1);
        bufsize = 255;
        devfd = fopen("/proc/net/dev", "r");

        // Ignore the first two lines of the file.
        fgets(buf, bufsize, devfd);
        fgets(buf, bufsize, devfd);

        receiveBytes = 0;
        sendBytes = 0;

        while (fgets(buf, bufsize, devfd)) {
            unsigned long long int rBytes, sBytes;
            char *line = strdup(buf);

            char *dev;
            dev = strtok(line, ":");

            // Filter lo (virtual network device).
            if (QString::fromStdString(dev).trimmed() != "lo") {
                sscanf(buf + strlen(dev) + 2, "%llu %*d %*d %*d %*d %*d %*d %*d %llu", &rBytes, &sBytes);

                receiveBytes += rBytes;
                sendBytes += sBytes;
            }

            free(line);
        }

        fclose(devfd);
        free(buf);
    }

    void passInputEvent(int wid)
    {
        XRectangle* reponseArea = new XRectangle;
        reponseArea->x = 0;
        reponseArea->y = 0;
        reponseArea->width = 0;
        reponseArea->height = 0;

        XShapeCombineRectangles(QX11Info::display(), wid, ShapeInput, 0, 0, reponseArea ,1 ,ShapeSet, YXBanded);

        delete reponseArea;
    }

    void removeChildren(QWidget *widget)
    {
        qDeleteAll(widget->children());
    }

    void removeLayoutChild(QLayout *layout, int index)
    {
        QLayoutItem *item = layout->itemAt(index);
        if (item != 0) {
            QWidget *widget = item->widget();
            if (widget != NULL) {
                widget->hide();
                widget->setParent(NULL);
                layout->removeWidget(widget);
            }
        }
    }

    void setFontSize(QPainter &painter, int textSize)
    {
        QFont font = painter.font() ;
        font.setPointSize(textSize);
        painter.setFont(font);
    }

    /**
     * @brief explode Explode a string based on
     * @param s The string to explode
     * @param c The seperator to use
     * @return A vector of the exploded string
     */
    const std::vector<std::string> explode(const std::string& s, const char& c)
    {
        std::string buff{""};
        std::vector<std::string> v;

        for (auto n:s) {
            if (n != c) {
                buff+=n;
            } else if (n == c && buff != "") {
                v.push_back(buff); buff = "";
            }
        }
        if (buff != "") {
            v.push_back(buff);
        }

        return v;
    }
}
