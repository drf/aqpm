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

#include "BackendThread.h"

#include "ConfigurationParser.h"
#include "ActionEvent.h"

#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QProcess>
#include <QFile>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <Auth>

#define PERFORM_RETURN(ty, val) \
        QVariantMap retmap; \
        retmap["retvalue"] = QVariant::fromValue(val); \
        emit actionPerformed(ty, retmap); \
        return val;

#define PERFORM_RETURN_VOID(ty) emit actionPerformed(ty, QVariantMap());

namespace Aqpm
{

void ContainerThread::run()
{
    BackendThread *t = new BackendThread();
    emit threadCreated(t);
    exec();
    t->deleteLater();
}

class BackendThread::Private
{
public:
    Private() : handleAuth(true) {}

    void waitForWorkerReady();
    bool initWorker(const QString &polkitAction);

    Q_DECLARE_PUBLIC(BackendThread)
    BackendThread *q_ptr;

    pmdb_t *db_local;
    pmdb_t *dbs_sync;

    QueueItem::List queue;
    QList<pmtransflag_t> flags;

    bool handleAuth;
};

void BackendThread::Private::waitForWorkerReady()
{
    if (!QDBusConnection::systemBus().interface()->isServiceRegistered("org.chakraproject.aqpmworker")) {
        usleep(20);
        qDebug() << "Waiting for interface to appear";
    }

    QDBusInterface i("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker", QDBusConnection::systemBus());

    qDebug() << "Got the interface";

    bool ready = false;

    while (!ready) {
        qDebug() << "Checking if interface is ready";

        QDBusReply<bool> reply = i.call("isWorkerReady");

        ready = reply.value();
    }

    qDebug() << "Ready, here we go";
}

bool BackendThread::Private::initWorker(const QString &polkitAction)
{
    Q_Q(BackendThread);

    QDBusMessage message;
    message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("isWorkerReady"));
    QDBusMessage reply = QDBusConnection::systemBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage
            && reply.arguments().size() == 1) {
        qDebug() << reply.arguments().first().toBool();
    } else if (reply.type() == QDBusMessage::MethodCallMessage) {
        qWarning() << "Message did not receive a reply (timeout by message bus)";
        return false;
    }

    if (handleAuth) {
        if (!PolkitQt::Auth::computeAndObtainAuth(polkitAction)) {
            qDebug() << "User unauthorized";
            return false;
        }
    }

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "dbQty", q, SIGNAL(dbQty(const QStringList&)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "dbStatusChanged", q, SIGNAL(dbStatusChanged(const QString&, int)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "streamDlProg", q, SIGNAL(streamDlProg(const QString&, int, int, int, int, int)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "streamTransProgress", q, SIGNAL(streamTransProgress(int, const QString&, int, int, int)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "streamTransEvent", q, SIGNAL(streamTransEvent(int, QVariantMap)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "streamTransQuestion", q, SIGNAL(streamTransQuestion(int, QVariantMap)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "errorOccurred", q, SIGNAL(errorOccurred(int, QVariantMap)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "logMessageStreamed", q, SIGNAL(logMessageStreamed(QString)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "workerResult", q, SLOT(workerResult(bool)));

    return true;
}

BackendThread::BackendThread(QObject *parent)
        : QObject(parent),
        d(new Private())
{
    d->q_ptr = this;
    qDebug() << "Handling libalpm in a separate Thread";
    connect(this, SIGNAL(operationFinished(bool)), SIGNAL(transactionReleased()));
}

BackendThread::~BackendThread()
{
    delete d;
}

void BackendThread::init()
{
    qDebug() << "Initializing";
    alpm_initialize();
    emit threadInitialized();
}

void BackendThread::customEvent(QEvent *event)
{
    qDebug() << "Received event of type" << event->type();

    if (event->type() == Backend::instance()->getEventTypeFor(Backend::UpdateDatabase)) {
        updateDatabase();
    } else if (event->type() == Backend::instance()->getEventTypeFor(Backend::ProcessQueue)) {
        processQueue();
    } else if (event->type() == Backend::instance()->getEventTypeFor(Backend::Initialization)) {
        init();
    } else if (event->type() == Backend::instance()->getEventTypeFor(Backend::SystemUpgrade)) {
        fullSystemUpgrade();
    } else if (event->type() == Backend::instance()->getEventTypeFor(Backend::PerformAction)) {
        ActionEvent *ae = dynamic_cast<ActionEvent*>(event);

        if (!ae) {
            qDebug() << "Someone just made some shit up";
            return;
        }

        switch (ae->actionType()) {
            case Backend::SetUpAlpm:
                setUpAlpm();
            case Backend::GetAvailableDatabases:
                getAvailableDatabases();
                break;
            default:
                qDebug() << "Implement me!!";
        }
    }
}

bool BackendThread::testLibrary()
{
    return !QFile::exists("/var/lib/pacman/db.lck");
}

bool BackendThread::isOnTransaction()
{
    return false;

}

bool BackendThread::reloadPacmanConfiguration()
{
    PacmanConf pdata;

    alpm_db_unregister_all();

    //pdata.HoldPkg = alpmListToStringList(alpm_option_get_holdpkgs());

    pdata.IgnorePkg = alpmListToStringList(alpm_option_get_ignorepkgs());

    pdata.IgnoreGrp = alpmListToStringList(alpm_option_get_ignoregrps());

    pdata.NoExtract = alpmListToStringList(alpm_option_get_noextracts());

    pdata.NoUpgrade = alpmListToStringList(alpm_option_get_noupgrades());

    /*foreach(const QString &str, pdata.HoldPkg) {
        alpm_option_remove_holdpkg(str.toAscii().data());
    }*/

    foreach(const QString &str, pdata.IgnorePkg) {
        alpm_option_remove_ignorepkg(str.toAscii().data());
    }

    foreach(const QString &str, pdata.IgnoreGrp) {
        alpm_option_remove_ignoregrp(str.toAscii().data());
    }

    foreach(const QString &str, pdata.NoExtract) {
        alpm_option_remove_noextract(str.toAscii().data());
    }

    foreach(const QString &str, pdata.NoUpgrade) {
        alpm_option_remove_noupgrade(str.toAscii().data());
    }

    alpm_option_remove_cachedir("/var/cache/pacman/pkg");

    setUpAlpm();

    return true;
}

void BackendThread::setUpAlpm()
{
    PacmanConf pdata;

    pdata = ConfigurationParser::instance()->getPacmanConf(true);

    alpm_option_set_root("/");
    alpm_option_set_dbpath("/var/lib/pacman");
    alpm_option_add_cachedir("/var/cache/pacman/pkg");

    d->db_local = alpm_db_register_local();

    if (pdata.logFile.isEmpty()) {
        alpm_option_set_logfile("/var/log/pacman.log");
    } else {
        alpm_option_set_logfile(pdata.logFile.toAscii().data());
    }

    /* Register our sync databases, kindly taken from pacdata */

    if (!pdata.loaded) {
        qDebug() << "Error Parsing Pacman Configuration!!";
    }

    for (int i = 0; i < pdata.syncdbs.size(); ++i) {
        if (pdata.serverAssoc.size() <= i) {
            qDebug() << "Could not find a matching repo for" << pdata.syncdbs.at(i);
            continue;
        }

        d->dbs_sync = alpm_db_register_sync(pdata.syncdbs.at(i).toAscii().data());

        if (alpm_db_setserver(d->dbs_sync, pdata.serverAssoc.at(i).toAscii().data()) == 0) {
            qDebug() << pdata.syncdbs.at(i) << "--->" << pdata.serverAssoc.at(i);
        } else {
            qDebug() << "Failed to add" << pdata.syncdbs.at(i) << "!!";
        }
    }

    /*foreach(const QString &str, pdata.HoldPkg) {
        alpm_option_add_holdpkg(str.toAscii().data());
    }*/

    foreach(const QString &str, pdata.IgnorePkg) {
        alpm_option_add_ignorepkg(str.toAscii().data());
    }

    foreach(const QString &str, pdata.IgnoreGrp) {
        alpm_option_add_ignoregrp(str.toAscii().data());
    }

    foreach(const QString &str, pdata.NoExtract) {
        alpm_option_add_noextract(str.toAscii().data());
    }

    foreach(const QString &str, pdata.NoUpgrade) {
        alpm_option_add_noupgrade(str.toAscii().data());
    }

    //alpm_option_set_usedelta(pdata.useDelta); Until a proper implementation is there
    alpm_option_set_usesyslog(pdata.useSysLog);

    PERFORM_RETURN_VOID(Backend::SetUpAlpm)
}

Database::List BackendThread::getAvailableDatabases()
{
    alpm_list_t *dbs = alpm_option_get_syncdbs();
    Database::List retlist;

    dbs = alpm_list_first(dbs);

    while (dbs != NULL) {
        retlist.append(Database((pmdb_t *) alpm_list_getdata(dbs)));
        dbs = alpm_list_next(dbs);
    }

    PERFORM_RETURN(Backend::GetAvailableDatabases, retlist);
}

Package::List BackendThread::getInstalledPackages()
{
    Package::List retlist;
    alpm_list_t *pkgs = alpm_list_first(alpm_db_get_pkgcache(d->db_local));

    while (pkgs) {
        retlist.append(Package((pmpkg_t *)alpm_list_getdata(pkgs)));
        pkgs = alpm_list_next(pkgs);
    }

    return retlist;
}

Package::List BackendThread::getUpgradeablePackages()
{
    alpm_list_t *syncpkgs = alpm_db_get_pkgcache(d->db_local);
    alpm_list_t *syncdbs = alpm_option_get_syncdbs();
    Package::List retlist;

    while (syncpkgs) {
        pmpkg_t *pkg = alpm_sync_newversion((pmpkg_t*)alpm_list_getdata(syncpkgs), syncdbs);

        if (pkg != NULL) {
            retlist.append(Package(pkg));
        }

        syncpkgs = alpm_list_next(syncpkgs);
    }

    return retlist;
}

Group::List BackendThread::getAvailableGroups()
{
    alpm_list_t *grps = NULL, *syncdbs;
    Group::List retlist;

    syncdbs = alpm_list_first(alpm_option_get_syncdbs());

    while (syncdbs != NULL) {
        grps = alpm_db_get_grpcache((pmdb_t *)alpm_list_getdata(syncdbs));
        grps = alpm_list_first(grps);

        while (grps != NULL) {
            retlist.append(Group((pmgrp_t *)alpm_list_getdata(grps)));
            grps = alpm_list_next(grps);
        }

        syncdbs = alpm_list_next(syncdbs);
    }

    return retlist;
}

Package::List BackendThread::getPackageDependencies(const Package &package)
{
    alpm_list_t *deps;
    Package::List retlist;

    deps = alpm_pkg_get_depends(package.alpmPackage());

    while (deps != NULL) {
        retlist.append(getPackage(alpm_dep_get_name((pmdepend_t *)alpm_list_getdata(deps)), QString()));
        deps = alpm_list_next(deps);
    }

    return retlist;
}

Package::List BackendThread::getDependenciesOnPackage(const Package &package)
{
    alpm_list_t *deps;
    Package::List retlist;

    deps = alpm_pkg_compute_requiredby(package.alpmPackage());

    while (deps != NULL) {
        retlist.append(getPackage((char *)alpm_list_getdata(deps), QString()));
        deps = alpm_list_next(deps);
    }

    return retlist;
}

bool BackendThread::isInstalled(const Package &package)
{
    pmpkg_t *localpackage = alpm_db_get_pkg(d->db_local, alpm_pkg_get_name(package.alpmPackage()));

    if (localpackage == NULL) {
        return false;
    }

    return true;
}

QStringList BackendThread::getPackageFiles(const Package &package)
{
    alpm_list_t *files;
    QStringList retlist;

    files = alpm_pkg_get_files(alpm_db_get_pkg(d->db_local, alpm_pkg_get_name(package.alpmPackage())));

    while (files != NULL) {
        retlist.append(QString((char*)alpm_list_getdata(files)).prepend(alpm_option_get_root()));
        files = alpm_list_next(files);
    }

    return retlist;
}

QStringList BackendThread::getProviders(const Package &package)
{
    alpm_list_t *provides;
    QStringList retlist;

    provides = alpm_pkg_get_provides(package.alpmPackage());

    while (provides != NULL) {
        retlist.append(QString((char *)alpm_list_getdata(provides)));
        provides = alpm_list_next(provides);
    }

    return retlist;
}

bool BackendThread::isProviderInstalled(const QString &provider)
{
    /* Here's what we need to do: iterate the installed
     * packages, and find if something between them provides
     * &provider
     */

    foreach (const Package &package, getInstalledPackages()) {
        QStringList prv(getProviders(package));

        for (int i = 0; i < prv.size(); ++i) {
            QStringList tmp(prv.at(i).split('='));
            if (!tmp.at(0).compare(provider)) {
                qDebug() << "Provider is installed and it is" << package.name();
                return true;
            }
        }
    }

    return false;
}

QString BackendThread::getAlpmVersion()
{
    return QString(alpm_version());
}

Database BackendThread::getPackageDatabase(const Package &package, bool checkver)
{
    Database db(alpm_pkg_get_db(package.alpmPackage()));

    if (checkver && (package.version() ==
                     getPackage(alpm_pkg_get_name(package.alpmPackage()), "local").version())) {
        return Database();
    }

    return db;
}

Package::List BackendThread::getPackagesFromGroup(const Group &group)
{
    Package::List retlist;
    alpm_list_t *pkgs = alpm_grp_get_pkgs(group.alpmGroup());

    while (pkgs != NULL) {
        retlist.append(Package((pmpkg_t *) alpm_list_getdata(pkgs)));
        pkgs = alpm_list_next(pkgs);
    }

    return retlist;
}

Package::List BackendThread::getPackagesFromDatabase(const Database &db)
{
    /* Since here local would be right the same of calling
     * getInstalledPackages(), here by local we mean packages that
     * don't belong to any other repo.
     */

    Package::List retlist;

    if (db.name() == "local") {
        alpm_list_t *pkglist = alpm_db_get_pkgcache(d->db_local);

        pkglist = alpm_list_first(pkglist);

        while (pkglist != NULL) {
            if (!getPackageDatabase(Package((pmpkg_t *) alpm_list_getdata(pkglist))).isValid()) {
                retlist.append((pmpkg_t *) alpm_list_getdata(pkglist));
            }

            pkglist = alpm_list_next(pkglist);
        }
    } else {
        alpm_list_t *pkgs = alpm_db_get_pkgcache(db.alpmDatabase());

        while (pkgs != NULL) {
            retlist.append((pmpkg_t *) alpm_list_getdata(pkgs));
            pkgs = alpm_list_next(pkgs);
        }
    }

    return retlist;
}

int BackendThread::countPackages(Globals::PackageStatus status)
{
    if (status == Globals::AllPackages) {
        int retvalue = 0;

        foreach (const Database &db, getAvailableDatabases()) {
            alpm_list_t *currentpkgs = alpm_db_get_pkgcache(db.alpmDatabase());
            retvalue += alpm_list_count(currentpkgs);
        }

        return retvalue;
    } else if (status == Globals::UpgradeablePackages) {
        return getUpgradeablePackages().count();
    } else if (status == Globals::InstalledPackages) {
        alpm_list_t *currentpkgs = alpm_db_get_pkgcache(d->db_local);

        return alpm_list_count(currentpkgs);
    } else
        return 0;
}

QStringList BackendThread::alpmListToStringList(alpm_list_t *list)
{
    QStringList retlist;

    list = alpm_list_first(list);

    while (list != NULL) {
        retlist.append((char *) alpm_list_getdata(list));
        list = alpm_list_next(list);
    }

    return retlist;
}

Package BackendThread::getPackage(const QString &name, const QString &repo)
{
    if (!repo.compare("local")) {
        return alpm_db_get_pkg(d->db_local, name.toAscii().data());
    }

    alpm_list_t *dbsync = alpm_list_first(alpm_option_get_syncdbs());

    while (dbsync != NULL) {
        pmdb_t *dbcrnt = (pmdb_t *)alpm_list_getdata(dbsync);

        if (!repo.compare(QString((char *)alpm_db_get_name(dbcrnt))) || repo.isEmpty()) {
            dbsync = alpm_list_first(dbsync);
            pmpkg_t *pack = alpm_db_get_pkg(dbcrnt, name.toAscii().data());
            if (pack != 0 || !repo.isEmpty()) {
                return Package(pack);
            }
        }

        dbsync = alpm_list_next(dbsync);
    }

    return 0;
}

Group BackendThread::getGroup(const QString &name)
{
    foreach (Group g, getAvailableGroups()) {
        if (g.name() == name) {
            return g;
        }
    }
}

Database BackendThread::getDatabase(const QString &name)
{
    foreach (Database d, getAvailableDatabases()) {
        if (d.name() == name) {
            return d;
        }
    }
}

Group::List BackendThread::getPackageGroups(const Package &package)
{
    alpm_list_t *list = alpm_pkg_get_groups(package.alpmPackage());
    Group::List retlist;
    Group::List groups = getAvailableGroups();

    while (list != NULL) {
        foreach (Group g, groups) {
            if (g.name() == (char*)alpm_list_getdata(list)) {
                retlist.append(g);
            }
        }
        list = alpm_list_next(list);
    }

    return retlist;
}

bool BackendThread::updateDatabase()
{
    emit transactionStarted();

    if (!d->initWorker("org.chakraproject.aqpm.updatedatabase")) {
        emit errorOccurred((int) Aqpm::Globals::WorkerInitializationFailed, QVariantMap());
        workerResult(false);
    }

    qDebug() << "Starting update";

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("updateDatabase"));
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);

    return true;
}

void BackendThread::fullSystemUpgrade()
{
    emit transactionStarted();

    QVariantList flags;

    foreach(const pmtransflag_t &ent, d->flags) {
        flags.append(ent);
    }

    if (!d->initWorker("org.chakraproject.aqpm.systemupgrade")) {
        emit errorOccurred((int) Aqpm::Globals::WorkerInitializationFailed, QVariantMap());
        workerResult(false);
    }

    qDebug() << "System upgrade started";

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("systemUpgrade"));
    QList<QVariant> argumentList;
    argumentList << qVariantFromValue(flags);
    message.setArguments(argumentList);
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);
}

void BackendThread::clearQueue()
{
    d->queue.clear();
}

void BackendThread::addItemToQueue(const QString &name, QueueItem::Action action)
{
    d->queue.append(QueueItem(name, action));
}

QueueItem::List BackendThread::queue()
{
    return d->queue;
}

void BackendThread::setFlags(const QList<pmtransflag_t> &flags)
{
    d->flags = flags;
}

void BackendThread::processQueue()
{
    emit transactionStarted();

    QVariantList flags;
    QVariantList packages;

    foreach(const pmtransflag_t &ent, d->flags) {
        flags.append(ent);
    }

    foreach(const QueueItem &ent, d->queue) {
        qDebug() << "Appending " << ent.name;
        packages.append(QVariant::fromValue(ent));
    }

    if (!d->initWorker("org.chakraproject.aqpm.processqueue")) {
        emit errorOccurred((int) Aqpm::Globals::WorkerInitializationFailed, QVariantMap());
        workerResult(false);
    }

    qDebug() << "Process queue started";

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("processQueue"));
    QList<QVariant> argumentList;
    argumentList << qVariantFromValue(packages);
    argumentList << qVariantFromValue(flags);
    message.setArguments(argumentList);
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);
}

void BackendThread::setShouldHandleAuthorization(bool should)
{
    d->handleAuth = should;
}

bool BackendThread::shouldHandleAuthorization()
{
    return d->handleAuth;
}

void BackendThread::workerResult(bool result)
{
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "dbQty", this, SIGNAL(dbQty(const QStringList&)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "dbStatusChanged", this, SIGNAL(dbStatusChanged(const QString&, int)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "streamDlProg", this, SIGNAL(streamTransDlProg(const QString&, int, int, int, int, int)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "streamTransProgress", this, SIGNAL(streamTransProgress(int, const QString&, int, int, int)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "streamTransEvent", this, SIGNAL(streamTransEvent(int, QVariantMap)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "streamTransQuestion", this, SIGNAL(streamTransQuestion(int, QVariantMap)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "errorOccurred", this, SIGNAL(errorOccurred(int, QVariantMap)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "logMessageStreamed", this, SIGNAL(logMessageStreamed(QString)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "workerResult", this, SLOT(workerResult(bool)));

    // After a worker operation ends, reload Alpm and clear the queue
    reloadPacmanConfiguration();
    clearQueue();

    emit transactionReleased();
    emit operationFinished(result);
}

void BackendThread::setAnswer(int answer)
{
    QDBusInterface iface("org.chakraproject.aqpmworker", "/Worker",
                         "org.chakraproject.aqpmworker", QDBusConnection::systemBus());

    iface.call("setAnswer", answer);
}

}
