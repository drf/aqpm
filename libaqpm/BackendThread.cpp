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

#include "BackendThread.h"

#include "Configuration.h"
#include "ActionEvent.h"
// Private headers
#include "ConfigurationThread.h"

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
    // Right before that, instantiate Configuration in the container thread
    Configuration::instance();

    BackendThread *t = new BackendThread();
    emit threadCreated(t);
    exec();
    t->deleteLater();
}

class BackendThread::Private
{
public:
    Private() : chroot(QString()), confChrooted(false), handleAuth(true) {}

    void waitForWorkerReady();
    bool initWorker(const QString &polkitAction);

    Q_DECLARE_PUBLIC(BackendThread)
    BackendThread *q_ptr;

    pmdb_t *db_local;
    pmdb_t *dbs_sync;

    QueueItem::List queue;
    Globals::TransactionFlags flags;

    QString chroot;
    bool confChrooted;

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

    if (!QDBusConnection::systemBus().interface()->isServiceRegistered("org.chakraproject.aqpmworker")) {
        qDebug() << "Requesting service start";
        QDBusConnection::systemBus().interface()->startService("org.chakraproject.aqpmworker");
    }

    QDBusMessage message;

    // Check if the worker needs setting up
    message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
                                             "/Worker",
                                             "org.chakraproject.aqpmworker",
                                             QLatin1String("isWorkerReady"));
    QDBusMessage reply = QDBusConnection::systemBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage
        && reply.arguments().size() == 1) {
        if (!reply.arguments().first().toBool()) {
            // Set the chroot in the worker if needed
            qDebug() << chroot;
            if (!chroot.isEmpty()) {
                message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
                                                         "/Worker",
                                                         "org.chakraproject.aqpmworker",
                                                         QLatin1String("setAqpmRoot"));
                message << chroot;
                message << confChrooted;
                QDBusConnection::systemBus().call(message);
            }

            // Then set up alpm in the worker
            message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
            "/Worker",
            "org.chakraproject.aqpmworker",
            QLatin1String("setUpAlpm"));
            QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);

            qDebug() << "Worker has been set up, let's move";
        }
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
    connect(QDBusConnection::systemBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            q, SLOT(serviceOwnerChanged(QString,QString,QString)));

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
    //qDebug() << "Received event of type" << event->type();

    if (event->type() == Backend::instance()->getEventTypeFor(Backend::UpdateDatabase)) {
        updateDatabase();
    } else if (event->type() == Backend::instance()->getEventTypeFor(Backend::ProcessQueue)) {
        processQueue();
    } else if (event->type() == Backend::instance()->getEventTypeFor(Backend::Initialization)) {
        init();
    } else if (event->type() == Backend::instance()->getEventTypeFor(Backend::SystemUpgrade)) {
        ActionEvent *ae = dynamic_cast<ActionEvent*>(event);
        fullSystemUpgrade(ae->args()["downgrade"].toBool());
    } else if (event->type() == Backend::instance()->getEventTypeFor(Backend::PerformAction)) {
        ActionEvent *ae = dynamic_cast<ActionEvent*>(event);

        if (!ae) {
            qDebug() << "Someone just made some shit up";
            return;
        }

        switch ((Backend::ActionType)ae->actionType()) {
            case Backend::SetUpAlpm:
                setUpAlpm();
            case Backend::GetAvailableDatabases:
                getAvailableDatabases();
                break;
            case Backend::GetLocalDatabase:
                getLocalDatabase();
                break;
            case Backend::GetAvailableGroups:
                getAvailableGroups();
                break;
            case Backend::GetPackagesFromDatabase:
                getPackagesFromDatabase(ae->args()["db"].value<Database>());
                break;
            case Backend::GetPackagesFromGroup:
                getPackagesFromGroup(ae->args()["group"].value<Group>());
                break;
            case Backend::GetUpgradeablePackages:
                getUpgradeablePackages();
                break;
            case Backend::GetInstalledPackages:
                getInstalledPackages();
                break;
            case Backend::GetPackageDependencies:
                getPackageDependencies(ae->args()["package"].value<Package>());
                break;
            case Backend::GetPackageGroups:
                getPackageGroups(ae->args()["package"].value<Package>());
                break;
            case Backend::GetDependenciesOnPackage:
                getDependenciesOnPackage(ae->args()["package"].value<Package>());
                break;
            case Backend::CountPackages:
                countPackages(ae->args()["status"].toInt());
                break;
            case Backend::GetProviders:
                getProviders(ae->args()["package"].value<Package>());
                break;
            case Backend::IsProviderInstalled:
                isProviderInstalled(ae->args()["provider"].toString());
                break;
            case Backend::GetPackageDatabase:
                getPackageDatabase(ae->args()["package"].value<Package>(), ae->args()["checkver"].toBool());
                break;
            case Backend::IsInstalled:
                isInstalled(ae->args()["package"].value<Package>());
                break;
            case Backend::GetAlpmVersion:
                getAlpmVersion();
                break;
            case Backend::ClearQueue:
                clearQueue();
                break;
            case Backend::AddItemToQueue:
                addItemToQueue(ae->args()["name"].toString(), ae->args()["action"].value<QueueItem::Action>());
                break;
            case Backend::GetQueue:
                queue();
                break;
            case Backend::DownloadQueue:
                downloadQueue();
                break;
            case Backend::SetShouldHandleAuthorization:
                setShouldHandleAuthorization(ae->args()["should"].toBool());
                break;
            case Backend::ShouldHandleAuthorization:
                shouldHandleAuthorization();
                break;
            case Backend::SetAnswer:
                setAnswer(ae->args()["answer"].toInt());
                break;
            case Backend::GetPackage:
                getPackage(ae->args()["name"].toString(), ae->args()["database"].toString());
                break;
            case Backend::GetGroup:
                getGroup(ae->args()["name"].toString());
                break;
            case Backend::GetDatabase:
                getDatabase(ae->args()["name"].toString());
                break;
            case Backend::TestLibrary:
                testLibrary();
                break;
            case Backend::IsOnTransaction:
                isOnTransaction();
                break;
            case Backend::SetFlags:
                setFlags((Globals::TransactionFlags)(ae->args()["flags"].toInt()));
                break;
            case Backend::ReloadPacmanConfiguration:
                reloadPacmanConfiguration();
                break;
            case Backend::InterruptTransaction:
                interruptTransaction();
                break;
            case Backend::SetAqpmRoot:
                setAqpmRoot(ae->args()["root"].toString(), ae->args()["applyToConfiguration"].toBool());
                break;
            case Backend::AqpmRoot:
                aqpmRoot();
            default:
                qDebug() << "Implement me!!";
                break;
        }
    }
}

bool BackendThread::testLibrary()
{
    PERFORM_RETURN(Backend::TestLibrary, !QFile::exists(d->chroot + "/var/lib/pacman/db.lck"));
}

bool BackendThread::isOnTransaction()
{
    PERFORM_RETURN(Backend::IsOnTransaction, false);
}

bool BackendThread::reloadPacmanConfiguration()
{
    alpm_db_unregister_all();

    foreach(const QString &str, alpmListToStringList(alpm_option_get_ignorepkgs())) {
        alpm_option_remove_ignorepkg(str.toAscii().data());
    }

    foreach(const QString &str, alpmListToStringList(alpm_option_get_ignoregrps())) {
        alpm_option_remove_ignoregrp(str.toAscii().data());
    }

    foreach(const QString &str, alpmListToStringList(alpm_option_get_noextracts())) {
        alpm_option_remove_noextract(str.toAscii().data());
    }

    foreach(const QString &str, alpmListToStringList(alpm_option_get_noupgrades())) {
        alpm_option_remove_noupgrade(str.toAscii().data());
    }

    alpm_option_remove_cachedir(QString(d->chroot + "/var/cache/pacman/pkg").toAscii().data());

    setUpAlpm();

    PERFORM_RETURN(Backend::ReloadPacmanConfiguration, true)
}

void BackendThread::setUpAlpm()
{
    alpm_option_set_root(QString(d->chroot + '/').toAscii().data());
    alpm_option_set_dbpath(QString(d->chroot + "/var/lib/pacman").toAscii().data());
    alpm_option_add_cachedir(QString(d->chroot + "/var/cache/pacman/pkg").toAscii().data());

    d->db_local = alpm_db_register_local();

    if (Configuration::instance()->value("options/LogFile").toString().isEmpty()) {
        alpm_option_set_logfile(QString(d->chroot + "/var/log/pacman.log").toAscii().data());
    } else {
        alpm_option_set_logfile(QString(d->chroot + Configuration::instance()->value("options/LogFile").toString()).toAscii().data());
    }

    /* Register our sync databases, kindly taken from pacdata */

    foreach (const QString &db, Configuration::instance()->databases()) {
        QString srvr = Configuration::instance()->getServerForDatabase(db);
        if (srvr.isEmpty()) {
            qDebug() << "Could not find a matching repo for" << db;
            continue;
        }

        d->dbs_sync = alpm_db_register_sync(db.toAscii().data());

        if (alpm_db_setserver(d->dbs_sync, srvr.toAscii().data()) == 0) {
            qDebug() << db << "--->" << srvr;
        } else {
            qDebug() << "Failed to add" << db << "!!";
        }
    }

    /*foreach(const QString &str, pdata.HoldPkg) {
        alpm_option_add_holdpkg(str.toAscii().data());
    }*/

    foreach(const QString &str, Configuration::instance()->value("options/IgnorePkg").toString().split(' ')) {
        alpm_option_add_ignorepkg(str.toAscii().data());
    }

    foreach(const QString &str, Configuration::instance()->value("options/IgnoreGroup").toString().split(' ')) {
        alpm_option_add_ignoregrp(str.toAscii().data());
    }

    foreach(const QString &str, Configuration::instance()->value("options/NoExtract").toString().split(' ')) {
        alpm_option_add_noextract(str.toAscii().data());
    }

    foreach(const QString &str, Configuration::instance()->value("options/NoUpgrade").toString().split(' ')) {
        alpm_option_add_noupgrade(str.toAscii().data());
    }

    //alpm_option_set_usedelta(pdata.useDelta); Until a proper implementation is there
    alpm_option_set_usesyslog(Configuration::instance()->value("options/UseSyslog").toInt());

    PERFORM_RETURN_VOID(Backend::SetUpAlpm)
}

bool BackendThread::setAqpmRoot(const QString& root, bool applyToConfiguration)
{
    if (d->handleAuth) {
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.setaqpmroot")) {
            qDebug() << "User unauthorized, root was not changed";
            PERFORM_RETURN(Backend::SetAqpmRoot, false)
        }
    } else {
        if (!PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.setaqpmroot", QCoreApplication::applicationPid(), true)) {
            qDebug() << "User unauthorized, root was not changed";
            PERFORM_RETURN(Backend::SetAqpmRoot, false)
        }
    }

    d->chroot = root;
    d->confChrooted = applyToConfiguration;
    if (d->confChrooted) {
        Configuration::instance()->getInnerThread()->setAqpmRoot(root);
        Configuration::instance()->reload();
    } else {
        Aqpm::Configuration::instance()->getInnerThread()->setAqpmRoot(QString());
        Aqpm::Configuration::instance()->reload();
    }

    PERFORM_RETURN(Backend::SetAqpmRoot, true)
}

QString BackendThread::aqpmRoot()
{
    PERFORM_RETURN(Backend::AqpmRoot, d->chroot);
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

Database BackendThread::getLocalDatabase()
{
    PERFORM_RETURN(Backend::GetLocalDatabase, Database(d->db_local))
}

Package::List BackendThread::getInstalledPackages()
{
    Package::List retlist;
    alpm_list_t *pkgs = alpm_list_first(alpm_db_get_pkgcache(d->db_local));

    while (pkgs) {
        retlist.append(Package((pmpkg_t *)alpm_list_getdata(pkgs)));
        pkgs = alpm_list_next(pkgs);
    }

    PERFORM_RETURN(Backend::GetInstalledPackages, retlist)
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

    PERFORM_RETURN(Backend::GetUpgradeablePackages, retlist)
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

    PERFORM_RETURN(Backend::GetAvailableGroups, retlist)
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

    PERFORM_RETURN(Backend::GetPackageDependencies, retlist)
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

    PERFORM_RETURN(Backend::GetDependenciesOnPackage, retlist)
}

bool BackendThread::isInstalled(const Package &package)
{
    pmpkg_t *localpackage = alpm_db_get_pkg(d->db_local, alpm_pkg_get_name(package.alpmPackage()));

    if (localpackage == NULL) {
        PERFORM_RETURN(Backend::IsInstalled, false)
    }

    PERFORM_RETURN(Backend::IsInstalled, true)
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

    PERFORM_RETURN(Backend::GetProviders, retlist)
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
                PERFORM_RETURN(Backend::IsProviderInstalled, true)
            }
        }
    }

    PERFORM_RETURN(Backend::IsProviderInstalled, false)
}

QString BackendThread::getAlpmVersion()
{
    PERFORM_RETURN(Backend::GetAlpmVersion, QString(alpm_version()))
}

Database BackendThread::getPackageDatabase(const Package &package, bool checkver)
{
    Database db(alpm_pkg_get_db(package.alpmPackage()));

    if (checkver && (package.version() ==
                     getPackage(alpm_pkg_get_name(package.alpmPackage()), "local").version())) {
        PERFORM_RETURN(Backend::GetPackageDatabase, Database())
    }

    PERFORM_RETURN(Backend::GetPackageDatabase, db)
}

Package::List BackendThread::getPackagesFromGroup(const Group &group)
{
    Package::List retlist;
    alpm_list_t *pkgs = alpm_grp_get_pkgs(group.alpmGroup());

    while (pkgs != NULL) {
        retlist.append(Package((pmpkg_t *) alpm_list_getdata(pkgs)));
        pkgs = alpm_list_next(pkgs);
    }

    PERFORM_RETURN(Backend::GetPackagesFromGroup, retlist)
}

Package::List BackendThread::getPackagesFromDatabase(const Database &db)
{
    /* Since here local would be right the same of calling
     * getInstalledPackages(), here by local we mean packages that
     * don't belong to any other repo.
     */

    Package::List retlist;

    if (db.alpmDatabase() == d->db_local) {
        qDebug() << "Getting local packages";

        foreach (const Package &pkg, getInstalledPackages()) {
            bool matched = false;
            foreach (const Database &db, getAvailableDatabases()) {
                if (alpm_db_get_pkg(db.alpmDatabase(), pkg.name().toAscii().data())) {
                    matched = true;
                    break;
                }
            }
            if (!matched) {
                retlist.append(pkg);
            }
        }
    } else {
        alpm_list_t *pkgs = alpm_db_get_pkgcache(db.alpmDatabase());

        while (pkgs != NULL) {
            retlist.append((pmpkg_t *) alpm_list_getdata(pkgs));
            pkgs = alpm_list_next(pkgs);
        }
    }

    PERFORM_RETURN(Backend::GetPackagesFromDatabase, retlist)
}

int BackendThread::countPackages(int st)
{
    Globals::PackageStatus status = (Globals::PackageStatus)st;

    if (status == Globals::AllPackages) {
        int retvalue = 0;

        foreach (const Database &db, getAvailableDatabases()) {
            alpm_list_t *currentpkgs = alpm_db_get_pkgcache(db.alpmDatabase());
            retvalue += alpm_list_count(currentpkgs);
        }

        PERFORM_RETURN(Backend::CountPackages, retvalue)
    } else if (status == Globals::UpgradeablePackages) {
        PERFORM_RETURN(Backend::CountPackages, getUpgradeablePackages().count())
    } else if (status == Globals::InstalledPackages) {
        alpm_list_t *currentpkgs = alpm_db_get_pkgcache(d->db_local);

        PERFORM_RETURN(Backend::CountPackages, alpm_list_count(currentpkgs))
    } else {
        PERFORM_RETURN(Backend::CountPackages, 0)
    }
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
    if (repo == "local") {
        PERFORM_RETURN(Backend::GetPackage, Package(alpm_db_get_pkg(d->db_local, name.toAscii().data())))
    }

    alpm_list_t *dbsync = alpm_list_first(alpm_option_get_syncdbs());

    while (dbsync != NULL) {
        pmdb_t *dbcrnt = (pmdb_t *)alpm_list_getdata(dbsync);

        if (!repo.compare(QString((char *)alpm_db_get_name(dbcrnt))) || repo.isEmpty()) {
            dbsync = alpm_list_first(dbsync);
            pmpkg_t *pack = alpm_db_get_pkg(dbcrnt, name.toAscii().data());
            if (pack != 0 || !repo.isEmpty()) {
                PERFORM_RETURN(Backend::GetPackage, Package(pack))
            }
        }

        dbsync = alpm_list_next(dbsync);
    }

    PERFORM_RETURN(Backend::GetPackage, Package())
}

Group BackendThread::getGroup(const QString &name)
{
    foreach (Group g, getAvailableGroups()) {
        if (g.name() == name) {
            PERFORM_RETURN(Backend::GetGroup, g)
        }
    }
}

Database BackendThread::getDatabase(const QString &name)
{
    foreach (Database d, getAvailableDatabases()) {
        if (d.name() == name) {
            PERFORM_RETURN(Backend::GetDatabase, d)
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

    PERFORM_RETURN(Backend::GetPackageGroups, retlist)
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

void BackendThread::fullSystemUpgrade(bool downgrade)
{
    emit transactionStarted();

    if (!d->initWorker("org.chakraproject.aqpm.systemupgrade")) {
        emit errorOccurred((int) Aqpm::Globals::WorkerInitializationFailed, QVariantMap());
        workerResult(false);
    }

    qDebug() << "System upgrade started";

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("systemUpgrade"));
    message << (int)d->flags;
    message << downgrade;
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);
}

void BackendThread::clearQueue()
{
    d->queue.clear();
    PERFORM_RETURN_VOID(Backend::ClearQueue)
}

void BackendThread::addItemToQueue(const QString &name, QueueItem::Action action)
{
    d->queue.append(QueueItem(name, action));
    PERFORM_RETURN_VOID(Backend::AddItemToQueue)
}

QueueItem::List BackendThread::queue()
{
    PERFORM_RETURN(Backend::GetQueue, d->queue)
}

void BackendThread::setFlags(Globals::TransactionFlags flags)
{
    d->flags = flags;
    PERFORM_RETURN_VOID(Backend::SetFlags)
}

void BackendThread::processQueue()
{
    emit transactionStarted();

    QVariantList packages;

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
    argumentList << (int)d->flags;
    message.setArguments(argumentList);
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);
}

void BackendThread::downloadQueue()
{
    emit transactionStarted();

    QVariantList packages;

    foreach(const QueueItem &ent, d->queue) {
        qDebug() << "Appending " << ent.name;
        packages.append(QVariant::fromValue(ent));
    }

    if (!d->initWorker("org.chakraproject.aqpm.downloadqueue")) {
        emit errorOccurred((int) Aqpm::Globals::WorkerInitializationFailed, QVariantMap());
        workerResult(false);
    }

    qDebug() << "Download queue started";

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("downloadQueue"));
    QList<QVariant> argumentList;
    argumentList << qVariantFromValue(packages);
    message.setArguments(argumentList);
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);
}

void BackendThread::setShouldHandleAuthorization(bool should)
{
    d->handleAuth = should;
    PERFORM_RETURN_VOID(Backend::SetShouldHandleAuthorization)
}

bool BackendThread::shouldHandleAuthorization()
{
    PERFORM_RETURN(Backend::ShouldHandleAuthorization, d->handleAuth)
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
    disconnect(QDBusConnection::systemBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
               this, SLOT(serviceOwnerChanged(QString,QString,QString)));

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

    PERFORM_RETURN_VOID(Backend::SetAnswer)
}

void BackendThread::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner)

    if (name != "org.chakraproject.aqpmworker" || !newOwner.isEmpty()) {
        // We don't give a fuck
        return;
    }

    qDebug() << "It looks like our worker got lost";

    // Ok, something got screwed. Report and flee
    emit errorOccurred((int) Aqpm::Globals::WorkerDisappeared, QVariantMap());
    workerResult(false);
}

void BackendThread::interruptTransaction()
{
    if (!QDBusConnection::systemBus().interface()->isServiceRegistered("org.chakraproject.aqpmworker")) {
        emit errorOccurred((int) Aqpm::Globals::TransactionInterruptedByUser, QVariantMap());
        emit operationFinished(false);
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("interruptTransaction"));
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);
}

}
