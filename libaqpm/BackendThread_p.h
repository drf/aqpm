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

#ifndef BACKENDTHREAD_P_H
#define BACKENDTHREAD_P_H

#include <QThread>
#include <QEvent>

#include "Backend.h"

#include <alpm.h>

namespace Aqpm
{

class ConfigurationThread;

class BackendThread : public QObject
{
    Q_OBJECT

public:
    BackendThread(QObject *parent = 0);
    virtual ~BackendThread();

private:
    void init();

    void setUpAlpm();
    bool testLibrary();
    bool isOnTransaction();

    bool setAqpmRoot(const QString &root, bool applyToConfiguration);
    QString aqpmRoot();

    Database::List getAvailableDatabases();
    Database *getLocalDatabase();

    Group::List getAvailableGroups();

    Package::List getPackagesFromDatabase(Database *db);

    Package::List getPackagesFromGroup(Group *group);

    Package::List getUpgradeablePackages();

    Package::List getInstalledPackages();

    Package::List getPackageDependencies(Package *package);

    Group::List getPackageGroups(Package *package);

    Package::List getDependenciesOnPackage(Package *package);

    QStringList getPackageFiles(Package *package);

    int countPackages(int status);

    QStringList getProviders(Package *package);
    bool isProviderInstalled(const QString &provider);

    Database *getPackageDatabase(Package *package, bool checkver = false);

    Package *getPackage(const QString &name, const QString &repo);
    Group *getGroup(const QString &name);
    Database *getDatabase(const QString &name);

    Package *loadPackageFromLocalFile(const QString &path);

    bool isInstalled(Package *package);

    bool updateDatabase();
    void fullSystemUpgrade(bool downgrade);

    void retrieveAdditionalTargetsForQueue();

    bool reloadPacmanConfiguration(); // In case the user modifies it.

    QStringList alpmListToStringList(alpm_list_t *list);
    alpm_list_t *stringListToAlpmList(const QStringList &list);

    QString getAlpmVersion();

    void clearQueue();
    void addItemToQueue(const QString &name, QueueItem::Action action);
    void setFlags(Globals::TransactionFlags flags);

    void processQueue();
    void downloadQueue();
    QueueItem::List queue();

    Package::List searchPackages(const QStringList &targets, const Database::List &dbs = Database::List());
    Package::List searchFiles(const QString &filename);

    void interruptTransaction();

    void setShouldHandleAuthorization(bool should);
    bool shouldHandleAuthorization();

    void setAnswer(int answer);

Q_SIGNALS:
    void dbStatusChanged(const QString &repo, int action);
    void dbQty(const QStringList &db);

    void transactionStarted();
    void transactionReleased();

    void streamTotalOffset(int offset);

    void streamTransProgress(int event, const QString &pkgname, int percent,
                             int howmany, int remain);

    void streamTransEvent(int event, const QVariantMap &args);

    void streamTransQuestion(int event, const QVariantMap &args);

    void errorOccurred(int code, const QVariantMap &args);

    void logMessageStreamed(const QString &msg);

    void operationFinished(bool success);

    void actionPerformed(int type, QVariantMap result);

    void additionalTargetsRetrieved(const Aqpm::Targets &targets);

    void threadInitialized();

    void aboutToBeDeleted();

private Q_SLOTS:
    void workerResult(bool success);
    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void targetsRetrieved(const QVariantMap &targets);
    void slotErrorOccurred(int code, const QVariantMap &args);

protected:
    void customEvent(QEvent *event);

private:
    Package *packageFromCache(const QString &repo, pmpkg_t *pkg);
    Group *groupFromCache(pmgrp_t *group);

    class Private;
    Private *d;
};

class ContainerThread : public QThread
{
    Q_OBJECT

public:
    ContainerThread(QObject* parent = 0);
    virtual ~ContainerThread();

protected:
    void run();

Q_SIGNALS:
    void threadCreated(BackendThread *thread);
};

}

Q_DECLARE_METATYPE(QList<pmtransflag_t>)

#endif /* BACKENDTHREAD_P_H */
