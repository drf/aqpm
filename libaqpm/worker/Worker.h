/***************************************************************************
 *   Copyright (C) 2008 by Dario Freddi                                    *
 *   drf54321@yahoo.it                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QVariantList>
#include <QtDBus/QDBusContext>

#include <alpm.h>

namespace AqpmWorker
{

class Worker : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    Worker(QObject *parent = 0);
    virtual ~Worker();

    bool isWorkerReady();

public Q_SLOTS:
    void processQueue(QVariantList packages, QVariantList flags);
    void updateDatabase();
    void setAnswer(int answer);

private:
    void setUpAlpm();
    void operationPerformed(bool result);

Q_SIGNALS:
    void workerReady();
    void workerResult(bool);

    void dbStatusChanged(const QString &repo, int action);
    void dbQty(const QStringList &db);

    void streamTransProgress(int event, const QString &pkgname, int percent,
                             int howmany, int remain);

    void streamTransEvent(int event, QVariantMap args);

    void streamTransQuestion(int event, QVariantMap args);

    void streamDlProg(const QString &pkg, int package_done, int package_total,
                          int rate, int list_done, int list_total);

    void errorOccurred(int code);

    void streamAnswer(int answer);

private:
    class Private;
    Private *d;
};

}

#endif /* WORKER_H */
