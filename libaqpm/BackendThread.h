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

#ifndef BACKENDTHREAD_H
#define BACKENDTHREAD_H

#include <QThread>
#include <QEvent>

#include "Backend.h"

#include <alpm.h>

namespace Aqpm {

class BackendThread : public QObject
{
    Q_OBJECT

    public:
        BackendThread(QObject *parent = 0);
        virtual ~BackendThread();

        void init();

    public Q_SLOTS:
        void setUpAlpm();
        bool testLibrary();
        bool isOnTransaction();

        alpm_list_t *getAvailableRepos();
        QStringList getAvailableReposAsStringList();

        alpm_list_t *getAvailableGroups();
        QStringList getAvailableGroupsAsStringList();

        alpm_list_t *getPackagesFromRepo(const QString &reponame);
        QStringList getPackagesFromRepoAsStringList(const QString &reponame);

        alpm_list_t *getPackagesFromGroup(const QString &groupname);
        QStringList getPackagesFromGroupAsStringList(const QString &groupname);

        alpm_list_t *getUpgradeablePackages();
        QStringList getUpgradeablePackagesAsStringList();

        alpm_list_t *getInstalledPackages();
        QStringList getInstalledPackagesAsStringList();

        QStringList getPackageDependencies(pmpkg_t *package);
        QStringList getPackageDependencies(const QString &name, const QString &repo);

        alpm_list_t *getPackageGroups(pmpkg_t *package);
        QStringList getPackageGroupsAsStringList(pmpkg_t *package);

        QStringList getDependenciesOnPackage(pmpkg_t *package);
        QStringList getDependenciesOnPackage(const QString &name, const QString &repo);

        QStringList getPackageFiles(pmpkg_t *package);
        QStringList getPackageFiles(const QString &name);

        int countPackages(Backend::PackageStatus status);

        QStringList getProviders(const QString &name, const QString &repo);
        QStringList getProviders(pmpkg_t *pkg);
        bool isProviderInstalled(const QString &provider);

        unsigned long getPackageSize(const QString &name, const QString &repo);
        unsigned long getPackageSize(pmpkg_t *package);

        QString getPackageVersion(const QString &name, const QString &repo);
        QString getPackageVersion(pmpkg_t *package);

        QString getPackageRepo(const QString &name, bool checkver = false);

        bool isInstalled(pmpkg_t *pkg);
        bool isInstalled(const QString &pkg);

        bool updateDatabase();
        bool fullSystemUpgrade();

        bool reloadPacmanConfiguration(); // In case the user modifies it.

        pmpkg_t *getPackageFromName(const QString &name, const QString &repo);

        QStringList alpmListToStringList(alpm_list_t *list);

        QString getAlpmVersion();

        void clearQueue();
        void addItemToQueue(const QString &name, QueueItem::Action action);
        void setFlags(QList<pmtransflag_t> flags);
        void processQueue();
        QueueItem::List queue();

        void setShouldHandleAuthorization(bool should);
        bool shouldHandleAuthorization() const;

    Q_SIGNALS:
        void dbStatusChanged(const QString &repo, int action);
        void dbQty(const QStringList &db);

        void transactionStarted();
        void transactionReleased();

        void streamTransDlProg(const QString &c, int bytedone, int bytetotal, int speed,
                               int listdone, int listtotal, int speedtotal);

        void streamTransProgress(int event, const QString &pkgname, int percent,
                                 int howmany, int remain);

        void streamTransEvent(int event, void *data1, void *data2);

        void errorOccurred(int code);

        void operationFinished(bool success);

        void threadInitialized();

    private Q_SLOTS:
        void workerResult(bool success);

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
        ContainerThread() {};

    protected:
        void run();

    Q_SIGNALS:
        void threadCreated(BackendThread *thread);
};

}

#endif /* BACKENDTHREAD_H */
