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

#include "Visibility.h"
#include "QueueItem.h"
#include "Globals.h"
#include "Package.h"
#include "Database.h"
#include "Group.h"

#include <QThread>
#include <QStringList>
#include <QEvent>
#include <QMetaType>

#include <alpm.h>

//typedef struct __alpm_list_t alpm_list_t;
//enum pmtransflag_t;

namespace Aqpm
{

class BackendThread;

/**
 * This class represent an item in the queue. To enqueue items in
 * Aqpm, you have to create an instance of this class and populate it.
 */


class AQPM_EXPORT Backend : public QObject
{
    Q_OBJECT

public:
    enum BackendEvent {
        UpdateDatabase = 1001,
        ProcessQueue = 1002,
        Initialization = 1003,
        SystemUpgrade = 1004
    };

    enum ActionType {
        GetAvailableDatabases,
        GetAvailableGroups,
        GetPackagesFromDatabase,
        GetPackagesFromGroup,
        GetUpgradeablePackages,
        GetInstalledPackages,
        GetPackageDependencies,
        GetPackageGroups,
        GetDependenciesOnPackage
    };

    static Backend *instance();
    static QString version();

    virtual ~Backend();

    QEvent::Type getEventTypeFor(BackendEvent event);

    bool testLibrary();
    bool isOnTransaction();

    Database::List getAvailableDatabases() const;

    Group::List getAvailableGroups();

    Package::List getPackagesFromDatabase(const Database &db);

    Package::List getPackagesFromGroup(const Group &group);

    Package::List getUpgradeablePackages();

    Package::List getInstalledPackages();

    Package::List getPackageDependencies(const Package &package);

    Group::List getPackageGroups(const Package &package);

    Package::List getDependenciesOnPackage(const Package &package);

    QStringList getPackageFiles(const Package &package);

    int countPackages(Globals::PackageStatus status);

    QStringList getProviders(const Package &package);
    bool isProviderInstalled(const QString &provider);

    Database getPackageDatabase(const Package &package, bool checkver = false) const;

    bool isInstalled(const Package &package);

    bool updateDatabase();
    void fullSystemUpgrade(const QList<pmtransflag_t> &flags);

    bool reloadPacmanConfiguration(); // In case the user modifies it.

    QStringList alpmListToStringList(alpm_list_t *list);

    QString getAlpmVersion();

    void clearQueue();
    void addItemToQueue(const QString &name, QueueItem::Action action);
    void processQueue(const QList<pmtransflag_t> &flags);
    QueueItem::List queue();

    void setShouldHandleAuthorization(bool should);
    bool shouldHandleAuthorization() const;

    void setAnswer(int answer);

    Package getPackage(const QString &name, const QString &repo) const;
    Group getGroup(const QString &name) const;
    Database getDatabase(const QString &name) const;

    BackendThread *getInnerThread();

public Q_SLOTS:
    void setUpAlpm();

Q_SIGNALS:
    void dbStatusChanged(const QString &repo, int action);
    void dbQty(const QStringList &db);

    void transactionStarted();
    void transactionReleased();

    void streamDlProg(const QString &c, int bytedone, int bytetotal, int speed,
                      int listdone, int listtotal);

    void streamTransProgress(Aqpm::Globals::TransactionProgress event, const QString &pkgname, int percent,
                             int howmany, int remain);

    void streamTransEvent(Aqpm::Globals::TransactionEvent event, const QVariantMap &args);

    void streamTransQuestion(Aqpm::Globals::TransactionQuestion event, const QVariantMap &args);

    void errorOccurred(Aqpm::Globals::Errors code, const QVariantMap &args);

    void logMessageStreamed(const QString &msg);

    void operationFinished(bool success);

    void backendReady();

    // Reserved for thread communication

    void startDbUpdate();
    void startQueue(QList<pmtransflag_t> flags);

private Q_SLOTS:
    void setUpSelf(BackendThread *t);
    void streamError(int code, const QVariantMap &args);
    void doStreamTransProgress(int event, const QString &pkgname, int percent,
                             int howmany, int remain);
    void doStreamTransEvent(int event, const QVariantMap &args);
    void doStreamTransQuestion(int event, const QVariantMap &args);

private:
    Backend();

    class Private;
    Private *d;
};

}

#endif /* BACKEND_H */
