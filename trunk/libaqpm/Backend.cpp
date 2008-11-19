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

#include "Backend.h"

#include "callbacks.h"
#include "ConfigurationParser.h"

#include <QMetaType>
#include <QDebug>

#include "misc/Singleton.h"

namespace Aqpm
{

class Backend::Private
{
public:

    Private() : mutex(new QMutex()), wCond(new QWaitCondition()) {};

    pmdb_t *db_local;
    pmdb_t *dbs_sync;

    QPointer<UpDbThread> upThread;
    QPointer<TrCommitThread> trThread;

    QMutex *mutex;
    QWaitCondition *wCond;

    QueueItem::List queue;
};

class BackendHelper
{
public:
    BackendHelper() : q(0) {}
    ~BackendHelper() {
        delete q;
    }
    Backend *q;
};

AQPM_GLOBAL_STATIC(BackendHelper, s_globalBackend)

Backend *Backend::instance()
{
    if (!s_globalBackend->q) {
        new Backend;
    }

    return s_globalBackend->q;
}

Backend::Backend()
        : QObject(),
        d(new Private())
{
    Q_ASSERT(!s_globalBackend->q);
    s_globalBackend->q = this;

    qDebug() << "Construction Backend singleton";

    alpm_initialize();

    qRegisterMetaType<pmtransevt_t>("pmtransevt_t");
    qRegisterMetaType<pmtransprog_t>("pmtransprog_t");

    CallBacks::instance();

    connect(CallBacks::instance(), SIGNAL(streamTransDlProg(char*, int, int, int, int, int, int)),
            SIGNAL(streamTransDlProg(char*, int, int, int, int, int, int)));
    connect(CallBacks::instance(), SIGNAL(streamTransProgress(pmtransprog_t, char*, int, int, int)),
            SIGNAL(streamTransProgress(pmtransprog_t, char*, int, int, int)));
    connect(CallBacks::instance(), SIGNAL(streamTransEvent(pmtransevt_t, void*, void*)),
            SIGNAL(streamTransEvent(pmtransevt_t, void*, void*)));
}

Backend::~Backend()
{
    delete d;
}

QMutex *Backend::backendMutex()
{
    return d->mutex;
}

QWaitCondition *Backend::backendWCond()
{
    return d->wCond;
}

bool Backend::testLibrary()
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

bool Backend::isOnTransaction()
{
    // I know, it sucks
    if (!d->trThread && !d->upThread) {
        return false;
    } else {
        return true;
    }

}

bool Backend::reloadPacmanConfiguration()
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

void Backend::setUpAlpm()
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

alpm_list_t *Backend::getAvailableRepos()
{
    return alpm_option_get_syncdbs();
}

QStringList Backend::getAvailableReposAsStringList()
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

alpm_list_t *Backend::getInstalledPackages()
{
    return alpm_db_get_pkgcache(d->db_local);
}

QStringList Backend::getInstalledPackagesAsStringList()
{
    QStringList retlist;
    alpm_list_t *pkgs = alpm_list_first(getInstalledPackages());

    while (pkgs) {
        retlist.append(QString(alpm_pkg_get_name((pmpkg_t *)alpm_list_getdata(pkgs))));
        pkgs = alpm_list_next(pkgs);
    }

    return retlist;
}

alpm_list_t *Backend::getUpgradeablePackages()
{
    alpm_list_t *syncpkgs = NULL;
    QStringList retlist;
    retlist.clear();

    if (alpm_trans_sysupgrade() == -1) {
        return NULL;
    }

    return alpm_trans_get_pkgs();
}

QStringList Backend::getUpgradeablePackagesAsStringList()
{
    alpm_list_t *syncpkgs = NULL;
    QStringList retlist;
    retlist.clear();

    if (alpm_trans_sysupgrade()) {
        return retlist;
    }

    syncpkgs = alpm_list_first(alpm_trans_get_pkgs());

    if (!syncpkgs) {
        return retlist;
    } else {
        qDebug() << "Upgradeable packages:";
        while (syncpkgs != NULL) {
            /* To Alpm Devs : LOL. Call three functions to get a fucking
             * name of a package? Please.
             */
            QString tmp(alpm_pkg_get_name(alpm_sync_get_pkg((pmsyncpkg_t *) alpm_list_getdata(syncpkgs))));
            qDebug() << tmp;
            retlist.append(tmp);
            syncpkgs = alpm_list_next(syncpkgs);
        }
        return retlist;
    }
}

alpm_list_t *Backend::getAvailableGroups()
{
    alpm_list_t *grps = NULL, *syncdbs;

    syncdbs = alpm_list_first(alpm_option_get_syncdbs());

    while (syncdbs != NULL) {
        grps = alpm_list_join(grps, alpm_db_get_grpcache((pmdb_t *)alpm_list_getdata(syncdbs)));

        syncdbs = alpm_list_next(syncdbs);
    }

    return grps;
}

QStringList Backend::getAvailableGroupsAsStringList()
{
    alpm_list_t *grps = NULL, *syncdbs;
    QStringList retlist;
    retlist.clear();

    syncdbs = alpm_list_first(alpm_option_get_syncdbs());

    while (syncdbs != NULL) {
        grps = alpm_db_get_grpcache((pmdb_t *)alpm_list_getdata(syncdbs));
        grps = alpm_list_first(grps);

        while (grps != NULL) {
            retlist.append(alpm_grp_get_name((pmgrp_t *)alpm_list_getdata(grps)));
            grps = alpm_list_next(grps);
        }

        syncdbs = alpm_list_next(syncdbs);
    }

    return retlist;
}

QStringList Backend::getPackageDependencies(pmpkg_t *package)
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

QStringList Backend::getPackageDependencies(const QString &name, const QString &repo)
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

QStringList Backend::getDependenciesOnPackage(pmpkg_t *package)
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

QStringList Backend::getDependenciesOnPackage(const QString &name, const QString &repo)
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

bool Backend::isInstalled(pmpkg_t *pkg)
{
    pmpkg_t *localpackage = alpm_db_get_pkg(d->db_local, alpm_pkg_get_name(pkg));
    if (localpackage == NULL)
        return false;

    return true;
}

bool Backend::isInstalled(const QString &pkg)
{
    pmpkg_t *localpackage = alpm_db_get_pkg(d->db_local, pkg.toAscii().data());
    if (localpackage == NULL)
        return false;

    return true;
}

QStringList Backend::getPackageFiles(pmpkg_t *package)
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

QStringList Backend::getPackageFiles(const QString &name)
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

QStringList Backend::getProviders(const QString &name, const QString &repo)
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

QStringList Backend::getProviders(pmpkg_t *pkg)
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

bool Backend::isProviderInstalled(const QString &provider)
{
    /* Here's what we need to do: iterate the installed
     * packages, and find if something between them provides
     * &provider
     */

    alpm_list_t *localpack = alpm_db_get_pkgcache(d->db_local);

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

unsigned long Backend::getPackageSize(pmpkg_t *package)
{
    return alpm_pkg_get_size(package);
}

unsigned long Backend::getPackageSize(const QString &name, const QString &repo)
{
    return getPackageSize(getPackageFromName(name, repo));
}

QString Backend::getAlpmVersion()
{
    return QString(alpm_version());
}

QString Backend::getPackageVersion(pmpkg_t *package)
{
    return alpm_pkg_get_version(package);
}

QString Backend::getPackageVersion(const QString &name, const QString &repo)
{
    return getPackageVersion(getPackageFromName(name, repo));
}

QString Backend::getPackageRepo(const QString &name, bool checkver)
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

alpm_list_t *Backend::getPackagesFromGroup(const QString &groupname)
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

QStringList Backend::getPackagesFromGroupAsStringList(const QString &groupname)
{
    QStringList retlist;
    alpm_list_t *pkgs = alpm_list_first(getPackagesFromGroup(groupname));

    while (pkgs) {
        retlist.append(QString(alpm_pkg_get_name((pmpkg_t *)alpm_list_getdata(pkgs))));
        pkgs = alpm_list_next(pkgs);
    }

    return retlist;
}

alpm_list_t *Backend::getPackagesFromRepo(const QString &reponame)
{
    /* Since here local would be right the same of calling
     * getInstalledPackages(), here by local we mean packages that
     * don't belong to any other repo.
     */

    alpm_list_t *retlist;

    retlist = NULL;

    if (reponame == "local") {
        alpm_list_t *pkglist = alpm_db_get_pkgcache(d->db_local);

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
                retlist = alpm_db_get_pkgcache((pmdb_t *) alpm_list_getdata(syncdbs));
                break;
            }

            syncdbs = alpm_list_next(syncdbs);
        }

        syncdbs = alpm_list_first(syncdbs);
    }

    return retlist;
}

QStringList Backend::getPackagesFromRepoAsStringList(const QString &reponame)
{
    QStringList retlist;
    alpm_list_t *pkgs = alpm_list_first(getPackagesFromRepo(reponame));

    while (pkgs) {
        retlist.append(QString(alpm_pkg_get_name((pmpkg_t *)alpm_list_getdata(pkgs))));
        pkgs = alpm_list_next(pkgs);
    }

    return retlist;
}

int Backend::countPackages(PackageStatus status)
{
    if (status == AllPackages) {
        int retvalue = 0;

        alpm_list_t *databases = getAvailableRepos();

        databases = alpm_list_first(databases);

        while (databases != NULL) {
            pmdb_t *dbcrnt = (pmdb_t *)alpm_list_getdata(databases);

            alpm_list_t *currentpkgs = alpm_db_get_pkgcache(dbcrnt);

            retvalue += alpm_list_count(currentpkgs);

            databases = alpm_list_next(databases);
        }

        return retvalue;
    } else if (status == UpgradeablePackages) {
        return getUpgradeablePackagesAsStringList().count();
    } else if (status == InstalledPackages) {
        alpm_list_t *currentpkgs = alpm_db_get_pkgcache(d->db_local);

        return alpm_list_count(currentpkgs);
    } else
        return 0;
}

QStringList Backend::alpmListToStringList(alpm_list_t *list)
{
    QStringList retlist;

    list = alpm_list_first(list);

    while (list != NULL) {
        retlist.append((char *) alpm_list_getdata(list));
        list = alpm_list_next(list);
    }

    return retlist;
}

pmpkg_t *Backend::getPackageFromName(const QString &name, const QString &repo)
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

bool Backend::updateDatabase()
{
    d->upThread = new UpDbThread(this);

    connect(d->upThread, SIGNAL(dbQty(const QStringList&)), SIGNAL(dbQty(const QStringList&)));
    connect(d->upThread, SIGNAL(dbStatusChanged(const QString&, int)),
            SIGNAL(dbStatusChanged(const QString&, int)));
    connect(d->upThread, SIGNAL(error(int)), SIGNAL(errorOccurred(int)));
    connect(d->upThread, SIGNAL(finished()), SIGNAL(transactionReleased()));

    emit transactionStarted();

    qDebug() << "Starting DB Update";

    d->upThread->start();
}

bool Backend::fullSystemUpgrade()
{
    clearQueue();
    addItemToQueue(new QueueItem(QString(), QueueItem::FullUpgrade));

    return true;
}

void Backend::computeTransactionResult()
{
    if (d->trThread) {
        clearQueue();
        bool error = d->trThread->isError();
        d->trThread->deleteLater();

        if (error) {
            emit operationFailed();
        } else {
            emit operationSuccessful();
        }
    }
}

void Backend::clearQueue()
{
    qDeleteAll(d->queue);
    d->queue.clear();
}

void Backend::addItemToQueue(QueueItem *itm)
{
    d->queue.append(itm);
}

QueueItem::List Backend::queue()
{
    return d->queue;
}

void Backend::processQueue(QList<pmtransflag_t> flags)
{
    pmtransflag_t alpmflags;

    if (flags.isEmpty()) {
        alpmflags = PM_TRANS_FLAG_ALLDEPS;
    } else {
        alpmflags = flags.at(0);

        for (int i = 1; i < flags.count(); ++i) {
            alpmflags = (pmtransflag_t)((pmtransflag_t)alpmflags | (pmtransflag_t)flags.at(i));
        }
    }

    qDebug() << alpmflags;

    d->trThread = new TrCommitThread(d->queue, alpmflags, this);

    connect(d->trThread, SIGNAL(finished()), SLOT(computeTransactionResult()));

    d->trThread->start();
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

TrCommitThread::TrCommitThread(QueueItem::List item, pmtransflag_t flags, QObject *parent)
        : QThread(parent),
        m_error(false),
        m_flags(flags),
        m_packages(item)
{
}

void TrCommitThread::run()
{
    bool sync = false;
    bool sysupgrade = false;
    bool remove = false;
    bool file = false;

    foreach(QueueItem *itm, m_packages) {
        switch (itm->action_id) {
        case QueueItem::Sync:
            qDebug() << "Sync action";
            sync = true;
            break;
        case QueueItem::FromFile:
            qDebug() << "From File action";
            file = true;
            break;
        case QueueItem::Remove:
            qDebug() << "Remove action";
            remove = true;
            break;
        case QueueItem::FullUpgrade:
            qDebug() << "Upgrade action";
            sysupgrade = true;
            break;
        }
    }

    if (remove) {
        if (alpm_trans_init(PM_TRANS_TYPE_REMOVE, m_flags,
                            cb_trans_evt,
                            cb_trans_conv,
                            cb_trans_progress) == -1) {
            emit error(Backend::InitTransactionError);
            m_error = true;
            return;
        }

        qDebug() << "Starting Package Removal";

        foreach(QueueItem *itm, m_packages) {
            if (itm->action_id != QueueItem::Remove) {
                return;
            }

            int res = alpm_trans_addtarget(qstrdup(itm->name.toUtf8()));

            if (res == -1) {
                emit error(Backend::AddTargetError);
                m_error = true;
                return;
            }
        }

        if (!performCurrentTransaction()) {
            return;
        }
    }

    if (sync) {
        if (alpm_trans_init(PM_TRANS_TYPE_SYNC, m_flags,
                            cb_trans_evt,
                            cb_trans_conv,
                            cb_trans_progress) == -1) {
            emit error(Backend::InitTransactionError);
            m_error = true;
            return;
        }

        qDebug() << "Starting Package Syncing";

        foreach(QueueItem *itm, m_packages) {
            if (itm->action_id != QueueItem::Sync) {
                return;
            }

            qDebug() << "Adding " << itm->name;
            int res = alpm_trans_addtarget(qstrdup(itm->name.toUtf8()));

            if (res == -1) {
                emit error(Backend::AddTargetError);
                m_error = true;
                return;
            }
        }

        if (!performCurrentTransaction()) {
            return;
        }
    }

    if (sysupgrade) {
        if (alpm_trans_init(PM_TRANS_TYPE_SYNC, m_flags,
                            cb_trans_evt,
                            cb_trans_conv,
                            cb_trans_progress) == -1) {
            emit error(Backend::InitTransactionError);
            m_error = true;
            return;
        }

        if (alpm_trans_sysupgrade() == -1) {
            qDebug() << "Creating a sysupgrade transaction failed!!";
            emit error(Backend::CreateSysUpgradeError);
            alpm_trans_release();
            m_error = true;
            return;
        }

        if (!performCurrentTransaction()) {
            return;
        }
    }

    if (file) {
        if (alpm_trans_init(PM_TRANS_TYPE_UPGRADE, m_flags,
                            cb_trans_evt,
                            cb_trans_conv,
                            cb_trans_progress) == -1) {
            emit error(Backend::InitTransactionError);
            m_error = true;
            return;
        }

        qDebug() << "Starting Package Syncing";

        foreach(QueueItem *itm, m_packages) {
            if (itm->action_id != QueueItem::FromFile) {
                return;
            }

            int res = alpm_trans_addtarget(qstrdup(itm->name.toUtf8()));

            if (res == -1) {
                emit error(Backend::AddTargetError);
                m_error = true;
                return;
            }
        }

        if (!performCurrentTransaction()) {
            return;
        }
    }
}

bool TrCommitThread::performCurrentTransaction()
{
    alpm_list_t *data = NULL;

    qDebug() << "Preparing...";
    if (alpm_trans_prepare(&data) == -1) {
        qDebug() << "Could not prepare transaction";
        emit error(Backend::PrepareError);
        m_error = true;
        alpm_trans_release();
        return false;

    }

    qDebug() << "Committing...";
    if (alpm_trans_commit(&data) == -1) {
        qDebug() << "Could not commit transaction";
        emit error(Backend::CommitError);
        m_error = true;
        alpm_trans_release();
        return false;
    }
    qDebug() << "Done";

    if (data) {
        FREELIST(data);
    }

    alpm_trans_release();

    return true;
}

//

UpDbThread::UpDbThread(QObject *parent)
        : QThread(parent),
        m_error(false)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater()));
}

void UpDbThread::run()
{
    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_ALLDEPS, cb_trans_evt, cb_trans_conv,
                        cb_trans_progress) == -1) {
        emit error(Backend::InitTransactionError);
        qDebug() << "Error!";
        m_error = true;
        return;
    }

    int r;
    alpm_list_t *syncdbs;

    syncdbs = alpm_list_first(alpm_option_get_syncdbs());

    QStringList list;

    while (syncdbs != NULL) {
        pmdb_t *dbcrnt = (pmdb_t *)alpm_list_getdata(syncdbs);

        list.append(QString((char *)alpm_db_get_name(dbcrnt)));
        syncdbs = alpm_list_next(syncdbs);
    }

    qDebug() << "Found " << list;

    emit dbQty(list);

    syncdbs = alpm_list_first(alpm_option_get_syncdbs());

    while (syncdbs != NULL) {
        qDebug() << "Updating DB";
        pmdb_t *dbcrnt = (pmdb_t *)alpm_list_getdata(syncdbs);
        QString curdbname((char *)alpm_db_get_name(dbcrnt));

        emit dbStatusChanged(curdbname, Backend::Checking);

        r = alpm_db_update(0, dbcrnt);

        if (r > 0) {
            emit dbStatusChanged(curdbname, Backend::DatabaseError);
        } else if (r < 0) {
            emit dbStatusChanged(curdbname, Backend::Unchanged);
        } else {
            emit dbStatusChanged(curdbname, Backend::Updated);
        }

        syncdbs = alpm_list_next(syncdbs);
    }

    if (alpm_trans_release() == -1) {
        if (alpm_trans_interrupt() == -1) {
            m_error = true;
            emit error(Backend::ReleaseTransactionError);
        }
    }

    qDebug() << "Database Update Performed";
}

}

