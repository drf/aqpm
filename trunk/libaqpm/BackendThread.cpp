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

#include "callbacks.h"
#include "ConfigurationParser.h"

#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QProcess>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <Auth>

namespace Aqpm {

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

    Private()
     : handleAuth(true)
    {};

    void waitForWorkerReady();

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

BackendThread::BackendThread(QObject *parent)
 : QObject(parent),
 d(new Private())
{
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
    CallBacks::instance();
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
    }
}

bool BackendThread::testLibrary()
{
    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_ALLDEPS,
                        cb_trans_evt,
                        cb_trans_conv,
                        cb_trans_progress) == -1) {
        return false;
    }

    if (alpm_trans_release() == -1) {
        return false;
    }

    return true;
}

bool BackendThread::isOnTransaction()
{
    return false;

}

bool BackendThread::reloadPacmanConfiguration()
{
    PacmanConf pdata;

    alpm_db_unregister(d->db_local);
    alpm_db_unregister_all();

    pdata.HoldPkg = alpmListToStringList(alpm_option_get_holdpkgs());

    pdata.IgnorePkg = alpmListToStringList(alpm_option_get_ignorepkgs());

    pdata.IgnoreGrp = alpmListToStringList(alpm_option_get_ignoregrps());

    pdata.NoExtract = alpmListToStringList(alpm_option_get_noextracts());

    pdata.NoUpgrade = alpmListToStringList(alpm_option_get_noupgrades());

    foreach(const QString &str, pdata.HoldPkg) {
        alpm_option_remove_holdpkg(str.toAscii().data());
    }

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

    alpm_option_set_dlcb(cb_dl_progress);
    alpm_option_set_totaldlcb(cb_dl_total);
    alpm_option_set_logcb(cb_log);

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

    if (!pdata.xferCommand.isEmpty()) {
        qDebug() << "XFerCommand is:" << pdata.xferCommand;
        alpm_option_set_xfercommand(pdata.xferCommand.toAscii().data());
    }

    alpm_option_set_nopassiveftp(pdata.noPassiveFTP);

    foreach(const QString &str, pdata.HoldPkg) {
        alpm_option_add_holdpkg(str.toAscii().data());
    }

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
}

alpm_list_t *BackendThread::getAvailableRepos()
{
    return alpm_option_get_syncdbs();
}

QStringList BackendThread::getAvailableReposAsStringList()
{
    alpm_list_t *dbs = alpm_option_get_syncdbs();
    QStringList retlist;

    retlist.clear();
    dbs = alpm_list_first(dbs);

    while (dbs != NULL) {
        retlist.append(alpm_db_get_name((pmdb_t *) alpm_list_getdata(dbs)));
        dbs = alpm_list_next(dbs);
    }

    return retlist;
}

alpm_list_t *BackendThread::getInstalledPackages()
{
    return alpm_db_getpkgcache(d->db_local);
}

QStringList BackendThread::getInstalledPackagesAsStringList()
{
    QStringList retlist;
    alpm_list_t *pkgs = alpm_list_first(getInstalledPackages());

    while (pkgs) {
        retlist.append(QString(alpm_pkg_get_name((pmpkg_t *)alpm_list_getdata(pkgs))));
        pkgs = alpm_list_next(pkgs);
    }

    return retlist;
}

alpm_list_t *BackendThread::getUpgradeablePackages()
{
    alpm_list_t *syncpkgs = NULL;
    QStringList retlist;
    alpm_list_t *syncdbs;
    retlist.clear();

    syncdbs = alpm_list_first( alpm_option_get_syncdbs() );

    if ( alpm_sync_sysupgrade( d->db_local, syncdbs, &syncpkgs ) == -1 ) {
        return NULL;
    }

    return alpm_list_first( syncpkgs );
}

QStringList BackendThread::getUpgradeablePackagesAsStringList()
{
    alpm_list_t *syncpkgs = NULL;
    QStringList retlist;
    alpm_list_t *syncdbs;
    retlist.clear();

    syncdbs = alpm_list_first( alpm_option_get_syncdbs() );

    if ( alpm_sync_sysupgrade( d->db_local, syncdbs, &syncpkgs ) == -1 )
        return retlist;

    syncpkgs = alpm_list_first( syncpkgs );

    if ( !syncpkgs ) {
        return retlist;
    } else {
        qDebug() << "Upgradeable packages:";
        while ( syncpkgs != NULL ) {
            /* To Alpm Devs : LOL. Call three functions to get a fucking
             * name of a package? Please.
             */
            QString tmp( alpm_pkg_get_name( alpm_sync_get_pkg(( pmsyncpkg_t * ) alpm_list_getdata( syncpkgs ) ) ) );
            qDebug() << tmp;
            retlist.append( tmp );
            syncpkgs = alpm_list_next( syncpkgs );
        }
        return retlist;
    }
}

alpm_list_t *BackendThread::getAvailableGroups()
{
    alpm_list_t *grps = NULL, *syncdbs;

    syncdbs = alpm_list_first(alpm_option_get_syncdbs());

    while (syncdbs != NULL) {
        grps = alpm_list_join(grps, alpm_db_getgrpcache((pmdb_t *)alpm_list_getdata(syncdbs)));

        syncdbs = alpm_list_next(syncdbs);
    }

    return grps;
}

QStringList BackendThread::getAvailableGroupsAsStringList()
{
    alpm_list_t *grps = NULL, *syncdbs;
    QStringList retlist;
    retlist.clear();

    syncdbs = alpm_list_first(alpm_option_get_syncdbs());

    while (syncdbs != NULL) {
        grps = alpm_db_getgrpcache((pmdb_t *)alpm_list_getdata(syncdbs));
        grps = alpm_list_first(grps);

        while (grps != NULL) {
            retlist.append(alpm_grp_get_name((pmgrp_t *)alpm_list_getdata(grps)));
            grps = alpm_list_next(grps);
        }

        syncdbs = alpm_list_next(syncdbs);
    }

    return retlist;
}

QStringList BackendThread::getPackageDependencies(pmpkg_t *package)
{
    alpm_list_t *deps;
    QStringList retlist;

    deps = alpm_pkg_get_depends(package);

    while (deps != NULL) {
        retlist.append(QString(alpm_dep_get_name((pmdepend_t *)alpm_list_getdata(deps))));
        deps = alpm_list_next(deps);
    }

    return retlist;
}

QStringList BackendThread::getPackageDependencies(const QString &name, const QString &repo)
{
    alpm_list_t *deps;
    QStringList retlist;

    deps = alpm_pkg_get_depends(getPackageFromName(name, repo));

    while (deps != NULL) {
        retlist.append(QString(alpm_dep_get_name((pmdepend_t *)alpm_list_getdata(deps))));
        deps = alpm_list_next(deps);
    }

    return retlist;
}

QStringList BackendThread::getDependenciesOnPackage(pmpkg_t *package)
{
    if (package == NULL)
        return QStringList();

    alpm_list_t *deps;
    QStringList retlist;

    deps = alpm_pkg_compute_requiredby(package);

    while (deps != NULL) {
        retlist.append(QString((char *)alpm_list_getdata(deps)));
        deps = alpm_list_next(deps);
    }

    return retlist;
}

QStringList BackendThread::getDependenciesOnPackage(const QString &name, const QString &repo)
{
    alpm_list_t *deps;
    QStringList retlist;

    deps = alpm_pkg_compute_requiredby(getPackageFromName(name, repo));

    while (deps != NULL) {
        retlist.append(QString((char *)alpm_list_getdata(deps)));
        deps = alpm_list_next(deps);
    }

    return retlist;
}

bool BackendThread::isInstalled(pmpkg_t *pkg)
{
    pmpkg_t *localpackage = alpm_db_get_pkg(d->db_local, alpm_pkg_get_name(pkg));
    if (localpackage == NULL)
        return false;

    return true;
}

bool BackendThread::isInstalled(const QString &pkg)
{
    pmpkg_t *localpackage = alpm_db_get_pkg(d->db_local, pkg.toAscii().data());
    if (localpackage == NULL)
        return false;

    return true;
}

QStringList BackendThread::getPackageFiles(pmpkg_t *package)
{
    alpm_list_t *files;
    QStringList retlist;

    files = alpm_pkg_get_files(alpm_db_get_pkg(d->db_local, alpm_pkg_get_name(package)));

    while (files != NULL) {
        retlist.append(QString((char*)alpm_list_getdata(files)).prepend(alpm_option_get_root()));
        files = alpm_list_next(files);
    }

    return retlist;
}

QStringList BackendThread::getPackageFiles(const QString &name)
{
    alpm_list_t *files;
    QStringList retlist;

    files = alpm_pkg_get_files(alpm_db_get_pkg(d->db_local, name.toAscii().data()));

    while (files != NULL) {
        retlist.append(QString((char*)alpm_list_getdata(files)).prepend(alpm_option_get_root()));
        files = alpm_list_next(files);
    }

    return retlist;
}

QStringList BackendThread::getProviders(const QString &name, const QString &repo)
{
    alpm_list_t *provides;
    QStringList retlist;

    provides = alpm_pkg_get_provides(getPackageFromName(name, repo));

    while (provides != NULL) {
        retlist.append(QString((char *)alpm_list_getdata(provides)));
        provides = alpm_list_next(provides);
    }

    return retlist;
}

QStringList BackendThread::getProviders(pmpkg_t *pkg)
{
    alpm_list_t *provides;
    QStringList retlist;

    provides = alpm_pkg_get_provides(pkg);

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

    alpm_list_t *localpack = alpm_db_getpkgcache(d->db_local);

    while (localpack != NULL) {
        QStringList prv(getProviders(QString(alpm_pkg_get_name(
                                                 (pmpkg_t *)alpm_list_getdata(localpack))), QString("local")));

        for (int i = 0; i < prv.size(); ++i) {
            QStringList tmp(prv.at(i).split('='));
            if (!tmp.at(0).compare(provider)) {
                qDebug() << "Provider is installed and it is" << alpm_pkg_get_name(
                    (pmpkg_t *)alpm_list_getdata(localpack));
                return true;
            }
        }

        localpack = alpm_list_next(localpack);
    }

    return false;
}

unsigned long BackendThread::getPackageSize(pmpkg_t *package)
{
    return alpm_pkg_get_size(package);
}

unsigned long BackendThread::getPackageSize(const QString &name, const QString &repo)
{
    return getPackageSize(getPackageFromName(name, repo));
}

QString BackendThread::getAlpmVersion()
{
    return QString(alpm_version());
}

QString BackendThread::getPackageVersion(pmpkg_t *package)
{
    return alpm_pkg_get_version(package);
}

QString BackendThread::getPackageVersion(const QString &name, const QString &repo)
{
    return getPackageVersion(getPackageFromName(name, repo));
}

QString BackendThread::getPackageRepo(const QString &name, bool checkver)
{
    alpm_list_t *syncdbs = alpm_option_get_syncdbs();
    for (alpm_list_t *i = syncdbs; i; i = alpm_list_next(i)) {
        pmdb_t *db = (pmdb_t *)alpm_list_getdata(i);
        pmpkg_t *pkg;
        pkg = alpm_db_get_pkg(db, name.toAscii().data());
        if (pkg == NULL)
            continue;

        if (checkver && (alpm_pkg_get_version(pkg) == getPackageVersion(alpm_pkg_get_name(pkg), "local")))
            continue;

        return alpm_db_get_name(db);
    }

    return QString();
}

alpm_list_t *BackendThread::getPackagesFromGroup(const QString &groupname)
{
    alpm_list_t *grps = alpm_list_first(getAvailableGroups());
    pmgrp_t *group = NULL;

    while (grps) {
        if (alpm_grp_get_name((pmgrp_t *)alpm_list_getdata(grps)) == groupname) {
            group = (pmgrp_t *)alpm_list_getdata(grps);
            break;
        }

        grps = alpm_list_next(grps);
    }

    if (group == NULL) {
        return NULL;
    }

    return alpm_grp_get_pkgs(group);
}

QStringList BackendThread::getPackagesFromGroupAsStringList(const QString &groupname)
{
    QStringList retlist;
    alpm_list_t *pkgs = alpm_list_first(getPackagesFromGroup(groupname));

    while (pkgs) {
        retlist.append(QString(alpm_pkg_get_name((pmpkg_t *)alpm_list_getdata(pkgs))));
        pkgs = alpm_list_next(pkgs);
    }

    return retlist;
}

alpm_list_t *BackendThread::getPackagesFromRepo(const QString &reponame)
{
    /* Since here local would be right the same of calling
     * getInstalledPackages(), here by local we mean packages that
     * don't belong to any other repo.
     */

    alpm_list_t *retlist;

    retlist = NULL;

    if (reponame == "local") {
        alpm_list_t *pkglist = alpm_db_getpkgcache(d->db_local);

        pkglist = alpm_list_first(pkglist);

        while (pkglist != NULL) {
            if (getPackageRepo(alpm_pkg_get_name((pmpkg_t *) alpm_list_getdata(pkglist))).isEmpty())
                retlist = alpm_list_add(retlist, alpm_list_getdata(pkglist));

            pkglist = alpm_list_next(pkglist);
        }
    } else {
        alpm_list_t *syncdbs = alpm_option_get_syncdbs();

        syncdbs = alpm_list_first(syncdbs);

        while (syncdbs != NULL) {
            if (!reponame.compare(alpm_db_get_name((pmdb_t *) alpm_list_getdata(syncdbs)))) {
                retlist = alpm_db_getpkgcache((pmdb_t *) alpm_list_getdata(syncdbs));
                break;
            }

            syncdbs = alpm_list_next(syncdbs);
        }

        syncdbs = alpm_list_first(syncdbs);
    }

    return retlist;
}

QStringList BackendThread::getPackagesFromRepoAsStringList(const QString &reponame)
{
    QStringList retlist;
    alpm_list_t *pkgs = alpm_list_first(getPackagesFromRepo(reponame));

    while (pkgs) {
        retlist.append(QString(alpm_pkg_get_name((pmpkg_t *)alpm_list_getdata(pkgs))));
        pkgs = alpm_list_next(pkgs);
    }

    return retlist;
}

int BackendThread::countPackages(Backend::PackageStatus status)
{
    if (status == Backend::AllPackages) {
        int retvalue = 0;

        alpm_list_t *databases = getAvailableRepos();

        databases = alpm_list_first(databases);

        while (databases != NULL) {
            pmdb_t *dbcrnt = (pmdb_t *)alpm_list_getdata(databases);

            alpm_list_t *currentpkgs = alpm_db_getpkgcache(dbcrnt);

            retvalue += alpm_list_count(currentpkgs);

            databases = alpm_list_next(databases);
        }

        return retvalue;
    } else if (status == Backend::UpgradeablePackages) {
        return getUpgradeablePackagesAsStringList().count();
    } else if (status == Backend::InstalledPackages) {
        alpm_list_t *currentpkgs = alpm_db_getpkgcache(d->db_local);

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

pmpkg_t *BackendThread::getPackageFromName(const QString &name, const QString &repo)
{
    if (!repo.compare("local"))
        return alpm_db_get_pkg(d->db_local, name.toAscii().data());

    alpm_list_t *dbsync = alpm_list_first(alpm_option_get_syncdbs());

    while (dbsync != NULL) {
        pmdb_t *dbcrnt = (pmdb_t *)alpm_list_getdata(dbsync);

        if (!repo.compare(QString((char *)alpm_db_get_name(dbcrnt)))) {
            dbsync = alpm_list_first(dbsync);
            return alpm_db_get_pkg(dbcrnt, name.toAscii().data());
        }

        dbsync = alpm_list_next(dbsync);
    }

    return NULL;
}

alpm_list_t *BackendThread::getPackageGroups(pmpkg_t *package)
{
    return alpm_pkg_get_groups(package);
}

QStringList BackendThread::getPackageGroupsAsStringList(pmpkg_t *package)
{
    alpm_list_t *list = alpm_pkg_get_groups(package);
    QStringList retlist;

    while (list != NULL) {
        retlist.append((char *)alpm_list_getdata(list));
        list = alpm_list_next(list);
    }

    return retlist;
}

bool BackendThread::updateDatabase()
{
    emit transactionStarted();

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

    if (d->handleAuth) {
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.updatedatabase")) {
            qDebug() << "User unauthorized";
            return false;
        }
    }

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "dbQty", this, SIGNAL(dbQty(const QStringList&)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "dbStatusChanged", this, SIGNAL(dbStatusChanged(const QString&, int)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "workerResult", this, SLOT(workerResult(bool)));

    qDebug() << "Starting update";

    message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
                                             "/Worker",
                                             "org.chakraproject.aqpmworker",
                                             QLatin1String("updateDatabase"));
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);

    return true;
}

bool BackendThread::fullSystemUpgrade()
{
    clearQueue();
    d->queue.append(QueueItem(QString(), QueueItem::FullUpgrade));

    return true;
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

void BackendThread::setFlags(QList<pmtransflag_t> flags)
{
    d->flags = flags;
}

void BackendThread::processQueue()
{
    QVariantList flags;
    QVariantList packages;

    foreach (pmtransflag_t ent, d->flags) {
        flags.append(ent);
    }

    foreach (QueueItem ent, d->queue) {
        qDebug() << "Appending " << ent.name;
        packages.append(QVariant::fromValue(ent));
    }

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
        return;
    }

    if (d->handleAuth) {
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.processqueue")) {
            qDebug() << "User unauthorized";
            return;
        }
    }

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
            "dbQty", this, SIGNAL(dbQty(const QStringList&)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
            "dbStatusChanged", this, SIGNAL(dbStatusChanged(const QString&, int)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
            "workerResult", this, SLOT(workerResult(bool)));

    qDebug() << "Starting update";

    message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
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

bool BackendThread::shouldHandleAuthorization() const
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
            "workerResult", this, SLOT(workerResult(bool)));

    emit operationFinished(result);
}

}
