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

#ifndef BACKENDTHREAD_H
#define BACKENDTHREAD_H

#include <QThread>
#include <QEvent>

#include "Backend.h"

#include <alpm.h>

namespace Aqpm
{

class BackendThread : public QObject
{
    Q_OBJECT

public:
    BackendThread(QObject *parent = 0);
    virtual ~BackendThread();

    void init();

private:
    void setUpAlpm();
    bool testLibrary();
    bool isOnTransaction();

    Database::List getAvailableDatabases();
    Database getLocalDatabase();

    Group::List getAvailableGroups();

    Package::List getPackagesFromDatabase(const Database &db);

    Package::List getPackagesFromGroup(const Group &group);

    Package::List getUpgradeablePackages();

    Package::List getInstalledPackages();

    Package::List getPackageDependencies(const Package &package);

    Group::List getPackageGroups(const Package &package);

    Package::List getDependenciesOnPackage(const Package &package);

    QStringList getPackageFiles(const Package &package);

    int countPackages(int status);

    QStringList getProviders(const Package &package);
    bool isProviderInstalled(const QString &provider);

    Database getPackageDatabase(const Package &package, bool checkver = false);

    Package getPackage(const QString &name, const QString &repo);
    Group getGroup(const QString &name);
    Database getDatabase(const QString &name);

    bool isInstalled(const Package &package);

    bool updateDatabase();
    void fullSystemUpgrade();

    bool reloadPacmanConfiguration(); // In case the user modifies it.

    QStringList alpmListToStringList(alpm_list_t *list);

    QString getAlpmVersion();

    void clearQueue();
    void addItemToQueue(const QString &name, QueueItem::Action action);
    void setFlags(const QList<pmtransflag_t> &flags);
    void processQueue();
    QueueItem::List queue();

    void setShouldHandleAuthorization(bool should);
    bool shouldHandleAuthorization();

    void setAnswer(int answer);

Q_SIGNALS:
    void dbStatusChanged(const QString &repo, int action);
    void dbQty(const QStringList &db);

    void transactionStarted();
    void transactionReleased();

    void streamDlProg(const QString &c, int bytedone, int bytetotal, int speed,
                      int listdone, int listtotal);

    void streamTransProgress(int event, const QString &pkgname, int percent,
                             int howmany, int remain);

    void streamTransEvent(int event, const QVariantMap &args);

    void streamTransQuestion(int event, const QVariantMap &args);

    void errorOccurred(int code, const QVariantMap &args);

    void logMessageStreamed(const QString &msg);

    void operationFinished(bool success);

    void actionPerformed(int type, QVariantMap result);

    void threadInitialized();

private Q_SLOTS:
    void workerResult(bool success);
    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);

protected:
    void customEvent(QEvent *event);

private:
    class Private;
    Private *d;
};

class ContainerThread : public QThread
{
    Q_OBJECT

public:
    ContainerThread() {}

protected:
    void run();

Q_SIGNALS:
    void threadCreated(BackendThread *thread);
};

}

Q_DECLARE_METATYPE(QList<pmtransflag_t>)

#endif /* BACKENDTHREAD_H */
