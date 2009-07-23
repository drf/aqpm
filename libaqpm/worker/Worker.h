/***************************************************************************
 *   Copyright (C) 2009 by Dario Freddi                                    *
 *   drf@chakra-project.org                                                *
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
    void processQueue(const QVariantList &packages, const QVariantList &flags);
    void updateDatabase();
    void systemUpgrade(const QVariantList &flags, bool downgrade);
    void setAnswer(int answer);
    void saveConfiguration(const QString &conf);
    void addMirror(const QString &mirror, int type);
    void performMaintenance(int type);

private:
    void setUpAlpm();
    void operationPerformed(bool result);
    bool addTransTarget(const QString &target);
    bool performTransaction();

Q_SIGNALS:
    void workerReady();
    void workerResult(bool);

    void dbStatusChanged(const QString &repo, int action);
    void dbQty(const QStringList &db);

    void streamTransProgress(int event, const QString &pkgname, int percent,
                             int howmany, int remain);

    void streamTransEvent(int event, const QVariantMap &args);

    void streamTransQuestion(int event, const QVariantMap &args);

    void streamDlProg(const QString &pkg, int package_done, int package_total,
                          int rate, int list_done, int list_total);

    void errorOccurred(int code, const QVariantMap &details);

    void messageStreamed(const QString &msg);

    void logMessageStreamed(const QString &msg);

    void streamAnswer(int answer);

private:
    class Private;
    Private *d;
};

}

#endif /* WORKER_H */
