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

#ifndef BACKEND_H
#define BACKEND_H

#include <alpm.h>

#include "Visibility.h"

#include <QThread>
#include <QPointer>
#include <QTextStream>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QStringList>

namespace Aqpm
{

class AQPM_EXPORT QueueItem
{
public:
    enum Action {
        Sync = 1,
        Remove = 2,
        FromFile = 4,
        FullUpgrade = 8
    };

    QueueItem();
    QueueItem(QString n, Action a)
     : name(n),
     action_id(a)
     {};

    typedef QList<QueueItem*> List;

    QString name;
    Action action_id;
};

class TrCommitThread : public QThread
{
    Q_OBJECT

public:
    TrCommitThread(QueueItem::List item, pmtransflag_t flags, QObject *parent = 0);
    void run();

    bool isError() {
        return m_error;
    }

private:
    bool performCurrentTransaction();

signals:
    void error(int code);

private:
    bool m_error;
    pmtransflag_t m_flags;
    QueueItem::List m_packages;
};

class UpDbThread : public QThread
{
    Q_OBJECT

public:
    explicit UpDbThread(QObject *parent = 0);
    void run();

    bool isError() {
        return m_error;
    }

signals:
    void error(int code);
    void dbStatusChanged(const QString &repo, int action);
    void dbQty(const QStringList &db);

private:
    bool m_error;
};

class AQPM_EXPORT Backend : public QObject
{
    Q_OBJECT

public:

    enum ErrorCode {
        PrepareError = 1,
        CommitError = 2,
        InitTransactionError = 4,
        ReleaseTransactionError = 8,
        AddTargetError = 16,
        CreateSysUpgradeError = 32
    };

    enum DatabaseState {
        Checking = 1,
        Downloading = 2,
        Updating = 4,
        Updated = 8,
        Unchanged = 16,
        DatabaseError = 32
    };

    enum PackageStatus {
        AllPackages,
        InstalledPackages,
        UpgradeablePackages
    };

    static Backend *instance();

    Backend();
    virtual ~Backend();

    QMutex *backendMutex();
    QWaitCondition *backendWCond();

    void setUpAlpm();
    bool testLibrary();
    bool isOnTransaction();

    alpm_list_t *getAvailableRepos();
    QStringList getAvailableReposAsStringList();

    alpm_list_t *getAvailableGroups();
    QStringList getAvailableGroupsAsStringList();

    alpm_list_t *getPackagesFromRepo(const QString &reponame);
    QStringList getPackagesFromRepoAsStringList(const QString &reponame);

    alpm_list_t *getUpgradeablePackages();
    QStringList getUpgradeablePackagesAsStringList();

    alpm_list_t *getInstalledPackages();
    QStringList getInstalledPackagesAsStringList();

    QStringList getPackageDependencies(pmpkg_t *package);
    QStringList getPackageDependencies(const QString &name, const QString &repo);

    QStringList getDependenciesOnPackage(pmpkg_t *package);
    QStringList getDependenciesOnPackage(const QString &name, const QString &repo);

    QStringList getPackageFiles(pmpkg_t *package);
    QStringList getPackageFiles(const QString &name);

    int countPackages(PackageStatus status);

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
    void addItemToQueue(QueueItem *itm);
    void processQueue(QList<pmtransflag_t> flags);
    QueueItem::List queue();

Q_SIGNALS:
    void dbStatusChanged(const QString &repo, int action);
    void dbQty(const QStringList &db);

    void transactionStarted();
    void transactionReleased();

    void streamTransDlProg(char *c, int bytedone, int bytetotal, int speed,
                           int listdone, int listtotal, int speedtotal);

    void streamTransProgress(pmtransprog_t event, char *pkgname, int percent,
                             int howmany, int remain);

    void streamTransEvent(pmtransevt_t event, void *data1, void *data2);

    void errorOccurred(int code);

    void operationSuccessful();
    void operationFailed();

private Q_SLOTS:
    void computeTransactionResult();

private:
    class Private;
    Private *d;
};

}

#endif /* BACKEND_H */
