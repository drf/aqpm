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

#include "Worker.h"

#include "aqpmworkeradaptor.h"

#include "../Configuration.h"
#include "../Maintenance.h"
#include "w_callbacks.h"
#include "../Globals.h"
#include "../QueueItem.h"

#include <QtDBus/QDBusConnection>
#include <QTimer>
#include <QDebug>

#include <Context>
#include <Auth>

namespace AqpmWorker
{

class Worker::Private
{
public:
    Private() : timeout(new QTimer()), ready(false), chroot(QString()) {}

    pmdb_t *db_local;
    pmdb_t *dbs_sync;
    QTimer *timeout;
    QProcess *proc;

    bool ready;
    QString chroot;
};

Worker::Worker(bool temporized, QObject *parent)
        : QObject(parent)
        , d(new Private())
{
    new AqpmworkerAdaptor(this);

    if (!QDBusConnection::systemBus().registerService("org.chakraproject.aqpmworker")) {
        qDebug() << "another helper is already running";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    if (!QDBusConnection::systemBus().registerObject("/Worker", this)) {
        qDebug() << "unable to register service interface to dbus";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    alpm_initialize();

    connect(AqpmWorker::CallBacks::instance(), SIGNAL(streamDlProg(const QString&, int, int, int, int, int)),
            this, SIGNAL(streamDlProg(const QString&, int, int, int, int, int)));
    connect(AqpmWorker::CallBacks::instance(), SIGNAL(streamTransProgress(int, const QString&, int, int, int)),
            this, SIGNAL(streamTransProgress(int, const QString&, int, int, int)));
    connect(AqpmWorker::CallBacks::instance(), SIGNAL(streamTransQuestion(int,QVariantMap)),
            this, SIGNAL(streamTransQuestion(int,QVariantMap)));
    connect(AqpmWorker::CallBacks::instance(), SIGNAL(streamTransEvent(int,QVariantMap)),
            this, SIGNAL(streamTransEvent(int,QVariantMap)));
    connect(AqpmWorker::CallBacks::instance(), SIGNAL(logMessageStreamed(QString)),
            this, SIGNAL(logMessageStreamed(QString)));
    connect(this, SIGNAL(streamAnswer(int)),
            AqpmWorker::CallBacks::instance(), SLOT(setAnswer(int)));

    setIsTemporized(temporized);
    setTimeout(3000);
    startTemporizing();
}

Worker::~Worker()
{
}

void Worker::setAqpmRoot(const QString& root, bool toConfiguration)
{
    stopTemporizing();
    d->chroot = root;
    startTemporizing();
}

bool Worker::isWorkerReady()
{
    return d->ready;
}

void Worker::setUpAlpm()
{
    stopTemporizing();

    alpm_option_set_root(QString(d->chroot + '/').toAscii().data());
    alpm_option_set_dbpath(QString(d->chroot + "/var/lib/pacman").toAscii().data());
    alpm_option_add_cachedir(QString(d->chroot + "/var/cache/pacman/pkg").toAscii().data());

    alpm_option_set_fetchcb(cb_fetch);
    alpm_option_set_totaldlcb(cb_dl_total);
    alpm_option_set_logcb(cb_log);

    d->db_local = alpm_db_register_local();

    if (Aqpm::Configuration::instance()->value("options/LogFile").toString().isEmpty()) {
        alpm_option_set_logfile(QString(d->chroot + "/var/log/pacman.log").toAscii().data());
    } else {
        alpm_option_set_logfile(QString(d->chroot +
                                        Aqpm::Configuration::instance()->value("options/LogFile").toString()).toAscii().data());
    }

    /* Register our sync databases, kindly taken from pacdata */

    foreach (const QString &db, Aqpm::Configuration::instance()->databases()) {
        QString srvr = Aqpm::Configuration::instance()->getServerForDatabase(db);
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

    foreach(const QString &str, Aqpm::Configuration::instance()->value("options/IgnorePkg").toStringList()) {
        alpm_option_add_ignorepkg(str.toAscii().data());
    }

    foreach(const QString &str, Aqpm::Configuration::instance()->value("options/IgnoreGroup").toStringList()) {
        alpm_option_add_ignoregrp(str.toAscii().data());
    }

    foreach(const QString &str, Aqpm::Configuration::instance()->value("options/NoExtract").toStringList()) {
        alpm_option_add_noextract(str.toAscii().data());
    }

    foreach(const QString &str, Aqpm::Configuration::instance()->value("options/NoUpgrade").toStringList()) {
        alpm_option_add_noupgrade(str.toAscii().data());
    }

    //alpm_option_set_usedelta(pdata.useDelta); Until a proper implementation is there
    alpm_option_set_usesyslog(Aqpm::Configuration::instance()->value("options/UseSyslog").toInt());

    qDebug() << "Yeah";

    d->ready = true;
    emit workerReady();

    qDebug() << "Yeah";

    startTemporizing();
}

void Worker::updateDatabase()
{
    stopTemporizing();

    qDebug() << "Starting DB Update";

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.updatedatabase",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit errorOccurred((int) Aqpm::Globals::AuthorizationNotGranted, QVariantMap());
        operationPerformed(false);
        return;
    }

    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_ALLDEPS, AqpmWorker::cb_trans_evt,
                        AqpmWorker::cb_trans_conv, AqpmWorker::cb_trans_progress) == -1) {
        QVariantMap args;
        args["ErrorString"] = QString(alpm_strerrorlast());
        emit errorOccurred(Aqpm::Globals::InitTransactionError, args);
        operationPerformed(false);
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

        emit dbStatusChanged(curdbname, Aqpm::Globals::Checking);

        r = alpm_db_update(0, dbcrnt);

        if (r == 1) {
            emit dbStatusChanged(curdbname, Aqpm::Globals::Unchanged);
        } else if (r < 0) {
            emit dbStatusChanged(curdbname, Aqpm::Globals::DatabaseError);
        } else {
            emit dbStatusChanged(curdbname, Aqpm::Globals::Updated);
        }

        syncdbs = alpm_list_next(syncdbs);
    }

    if (alpm_trans_release() == -1) {
        if (alpm_trans_interrupt() == -1) {
            operationPerformed(false);
            return;
        }
    }

    qDebug() << "Database Update Performed";

    operationPerformed(true);
}

void Worker::processQueue(const QVariantList &packages, int flags)
{
    stopTemporizing();

    qDebug() << "Starting Queue Processing";

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.processqueue",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit errorOccurred((int) Aqpm::Globals::AuthorizationNotGranted, QVariantMap());
        operationPerformed(false);
        return;
    }

    Aqpm::Globals::TransactionFlags gflags;

    if (flags == 0) {
        gflags = Aqpm::Globals::AllDeps;
    } else {
        gflags = (Aqpm::Globals::TransactionFlags)flags;
    }

    pmtransflag_t alpmflags = processFlags(gflags);

    qDebug() << alpmflags;

    bool sync = false;
    bool remove = false;
    bool file = false;

    Aqpm::QueueItem::List queue;

    qDebug() << "Appending packages";

    foreach(const QVariant &ent, packages) {
        qDebug() << ent.typeName();
        Aqpm::QueueItem item;
        ent.value<QDBusArgument>() >> item;
        queue.append(item);
    }

    qDebug() << "Packages appended, starting evaluation";

    foreach(const Aqpm::QueueItem &itm, queue) {
        switch (itm.action_id) {
        case Aqpm::QueueItem::Sync:
            qDebug() << "Sync action";
            sync = true;
            break;
        case Aqpm::QueueItem::FromFile:
            qDebug() << "From File action";
            file = true;
            break;
        case Aqpm::QueueItem::Remove:
            qDebug() << "Remove action";
            remove = true;
            break;
        default:
            qDebug() << "What is that?";
            break;
        }
    }

    if (remove) {
        if (alpm_trans_init(PM_TRANS_TYPE_REMOVE, alpmflags,
                            AqpmWorker::cb_trans_evt, AqpmWorker::cb_trans_conv,
                            AqpmWorker::cb_trans_progress) == -1) {
            QVariantMap args;
            args["ErrorString"] = QString(alpm_strerrorlast());
            emit errorOccurred(Aqpm::Globals::InitTransactionError, args);
            operationPerformed(false);
            return;
        }

        qDebug() << "Starting Package Removal";

        foreach(const Aqpm::QueueItem &itm, queue) {
            if (itm.action_id != Aqpm::QueueItem::Remove) {
                continue;
            }

            if (!addTransTarget(itm.name)) {
                return;
            }
        }

        if (!performTransaction()) {
            return;
        }

    }

    if (sync) {
        if (alpm_trans_init(PM_TRANS_TYPE_SYNC, alpmflags,
                            AqpmWorker::cb_trans_evt, AqpmWorker::cb_trans_conv,
                            AqpmWorker::cb_trans_progress) == -1) {
            QVariantMap args;
            args["ErrorString"] = QString(alpm_strerrorlast());
            emit errorOccurred(Aqpm::Globals::InitTransactionError, args);
            operationPerformed(false);
            return;
        }

        qDebug() << "Starting Package Syncing";

        foreach(const Aqpm::QueueItem &itm, queue) {
            if (itm.action_id != Aqpm::QueueItem::Sync) {
                continue;
            }

            qDebug() << "Adding " << itm.name;

            if (!addTransTarget(itm.name)) {
                return;
            }
        }

        if (!performTransaction()) {
            return;
        }
    }

    if (file) {
        if (alpm_trans_init(PM_TRANS_TYPE_UPGRADE, alpmflags,
                            AqpmWorker::cb_trans_evt, AqpmWorker::cb_trans_conv,
                            AqpmWorker::cb_trans_progress) == -1) {
            QVariantMap args;
            args["ErrorString"] = QString(alpm_strerrorlast());
            emit errorOccurred(Aqpm::Globals::InitTransactionError, args);
            operationPerformed(false);
            return;
        }

        qDebug() << "Starting Package Syncing";

        foreach(const Aqpm::QueueItem &itm, queue) {
            if (itm.action_id != Aqpm::QueueItem::FromFile) {
                continue;
            }

            if (!addTransTarget(itm.name)) {
                return;
            }
        }

        if (!performTransaction()) {
            return;
        }
    }

    operationPerformed(true);
}

void Worker::downloadQueue(const QVariantList &packages)
{
    stopTemporizing();

    qDebug() << "Starting Queue Download";

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.downloadqueue",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit errorOccurred((int) Aqpm::Globals::AuthorizationNotGranted, QVariantMap());
        operationPerformed(false);
        return;
    }

    pmtransflag_t alpmflags;

    alpmflags = PM_TRANS_FLAG_ALLDEPS;
    alpmflags = (pmtransflag_t)((pmtransflag_t)alpmflags | (pmtransflag_t)PM_TRANS_FLAG_DOWNLOADONLY);

    Aqpm::QueueItem::List queue;

    qDebug() << "Appending packages";

    foreach(const QVariant &ent, packages) {
        qDebug() << ent.typeName();
        Aqpm::QueueItem item;
        ent.value<QDBusArgument>() >> item;
        queue.append(item);
    }

    qDebug() << "Packages appended, starting evaluation";

    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, alpmflags,
                        AqpmWorker::cb_trans_evt, AqpmWorker::cb_trans_conv,
                        AqpmWorker::cb_trans_progress) == -1) {
        QVariantMap args;
        args["ErrorString"] = QString(alpm_strerrorlast());
        emit errorOccurred(Aqpm::Globals::InitTransactionError, args);
        operationPerformed(false);
        return;
    }

    qDebug() << "Starting Package Syncing";

    foreach (const Aqpm::QueueItem &itm, queue) {
        if (itm.action_id == Aqpm::QueueItem::FullUpgrade) {
            if (alpm_trans_sysupgrade(0) == -1) {
                qDebug() << "Creating a sysupgrade transaction failed!!";
                QVariantMap args;
                args["ErrorString"] = QString(alpm_strerrorlast());
                emit errorOccurred(Aqpm::Globals::CreateSysUpgradeError, args);
                alpm_trans_release();
                operationPerformed(false);
                return;
            }

            if (!performTransaction()) {
                return;
            }

            operationPerformed(true);
            return;
        }
    }

    foreach (const Aqpm::QueueItem &itm, queue) {
        if (itm.action_id != Aqpm::QueueItem::Sync) {
            continue;
        }

        qDebug() << "Adding " << itm.name;

        if (!addTransTarget(itm.name)) {
            return;
        }
    }

    if (!performTransaction()) {
        return;
    }

    operationPerformed(true);
}

void Worker::systemUpgrade(int flags, bool downgrade)
{
    stopTemporizing();

    qDebug() << "Starting System Upgrade";

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.systemupgrade",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit errorOccurred((int) Aqpm::Globals::AuthorizationNotGranted, QVariantMap());
        operationPerformed(false);
        return;
    }

    Aqpm::Globals::TransactionFlags gflags;

    if (flags == 0) {
        gflags = Aqpm::Globals::AllDeps;
    } else {
        gflags = (Aqpm::Globals::TransactionFlags)flags;
    }

    pmtransflag_t alpmflags = processFlags(gflags);

    qDebug() << alpmflags;

    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, alpmflags,
                        AqpmWorker::cb_trans_evt, AqpmWorker::cb_trans_conv,
                        AqpmWorker::cb_trans_progress) == -1) {
        QVariantMap args;
        args["ErrorString"] = QString(alpm_strerrorlast());
        emit errorOccurred(Aqpm::Globals::InitTransactionError, args);
        operationPerformed(false);
        return;
    }

    int dwng = downgrade ? 1 : 0;

    if (alpm_trans_sysupgrade(downgrade) == -1) {
        qDebug() << "Creating a sysupgrade transaction failed!!";
        QVariantMap args;
        args["ErrorString"] = QString(alpm_strerrorlast());
        emit errorOccurred(Aqpm::Globals::CreateSysUpgradeError, args);
        alpm_trans_release();
        operationPerformed(false);
        return;
    }

    if (!performTransaction()) {
        return;
    }

    operationPerformed(true);
}

void Worker::setAnswer(int answer)
{
    emit streamAnswer(answer);
}

void Worker::operationPerformed(bool result)
{
    emit workerResult(result);
    startTemporizing();
}

bool Worker::performTransaction()
{
    stopTemporizing();

    alpm_list_t *data = NULL;

    qDebug() << "Preparing...";

    alpm_list_t *i;
    QVariantMap args;
    QVariantMap innerdata;
    QStringList files;

    if (alpm_trans_prepare(&data) == -1) {
        switch(pm_errno) {
        case PM_ERR_UNSATISFIED_DEPS:
            for (i = data; i; i = alpm_list_next(i)) {
                pmdepmissing_t *miss = (pmdepmissing_t*) alpm_list_getdata(i);
                pmdepend_t *dep = (pmdepend_t*) alpm_miss_get_dep(miss);
                char *depstring = alpm_dep_compute_string(dep);
                innerdata[alpm_miss_get_target(miss)] = QString(depstring);
                //free(depstring);
            }
            args["UnsatisfiedDeps"] = innerdata;
            emit errorOccurred(Aqpm::Globals::UnsatisfiedDependencies, args);
            break;
        case PM_ERR_CONFLICTING_DEPS:
            for(i = data; i; i = alpm_list_next(i)) {
                pmconflict_t *conflict = (pmconflict_t*) alpm_list_getdata(i);
                innerdata[QString(alpm_conflict_get_package1(conflict))] =
                        QVariant::fromValue(QString(alpm_conflict_get_package2(conflict)));
            }
            args["ConflictingDeps"] = QVariant::fromValue(innerdata);
            emit errorOccurred(Aqpm::Globals::ConflictingDependencies, args);
            break;
        default:
            args["ErrorString"] = alpm_strerrorlast();
            emit errorOccurred(Aqpm::Globals::PrepareError, args);
            break;
        }
        qDebug() << "Could not prepare transaction";
        alpm_trans_release();
        operationPerformed(false);
        return false;
    }

    qDebug() << "Committing...";
    if (alpm_trans_commit(&data) == -1) {
        switch(pm_errno) {
        case PM_ERR_FILE_CONFLICTS:
            for(i = data; i; i = alpm_list_next(i)) {
                pmfileconflict_t *conflict = (pmfileconflict_t*) alpm_list_getdata(i);

                switch(alpm_fileconflict_get_type(conflict)) {
                case PM_FILECONFLICT_TARGET:
                    innerdata[alpm_fileconflict_get_file(conflict)] =
                            QStringList() << alpm_fileconflict_get_target(conflict)
                            << alpm_fileconflict_get_ctarget(conflict);
                    qDebug() << innerdata;
                    args["ConflictingTargets"] = innerdata;
                    emit errorOccurred(Aqpm::Globals::FileConflictTarget, args);
                    break;
                case PM_FILECONFLICT_FILESYSTEM:
                    innerdata[alpm_fileconflict_get_target(conflict)] = alpm_fileconflict_get_file(conflict);
                    args["ConflictingFiles"] = innerdata;
                    emit errorOccurred(Aqpm::Globals::FileConflictFilesystem, args);
                    break;
                default:
                    args["ErrorString"] = alpm_strerrorlast();
                    emit errorOccurred(Aqpm::Globals::CommitError, args);
                    break;
                }
            }
            break;
        case PM_ERR_PKG_INVALID:
        case PM_ERR_DLT_INVALID:
            for(i = data; i; i = alpm_list_next(i)) {
                files << QString((char*) alpm_list_getdata(i));
            }
            args["Filenames"] = files;
            emit errorOccurred(Aqpm::Globals::CorruptedFile, args);
            break;
        default:
            args["ErrorString"] = alpm_strerrorlast();
            emit errorOccurred(Aqpm::Globals::CommitError, args);
            break;
        }
        qDebug() << "Could not commit transaction" << alpm_strerrorlast();
        alpm_trans_release();
        operationPerformed(false);
        return false;
    }
    qDebug() << "Done";

    if (data) {
        FREELIST(data);
    }

    alpm_trans_release();

    return true;
}

bool Worker::addTransTarget(const QString &target)
{
    int res = alpm_trans_addtarget(qstrdup(target.toUtf8()));

    if (res == -1) {
        if (pm_errno == PM_ERR_TRANS_DUP_TARGET) {
            QVariantMap args;
            args["PackageName"] = target;
            emit errorOccurred(Aqpm::Globals::DuplicateTarget, args);
        } else if (pm_errno == PM_ERR_PKG_IGNORED) {
            QVariantMap args;
            args["PackageName"] = target;
            emit errorOccurred(Aqpm::Globals::PackageIgnored, args);
        } else if (pm_errno == PM_ERR_PKG_NOT_FOUND) {
            QVariantMap args;
            args["PackageName"] = target;
            emit errorOccurred(Aqpm::Globals::PackageNotFound, args);
        } else {
            QVariantMap args;
            args["ErrorString"] = alpm_strerrorlast();
            emit errorOccurred(Aqpm::Globals::AddTargetError, args);
        }

        operationPerformed(false);
        return false;
    }

    return true;
}

void Worker::performMaintenance(int type)
{
    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.performmaintenance",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit errorOccurred((int) Aqpm::Globals::AuthorizationNotGranted, QVariantMap());
        operationPerformed(false);
        return;
    }

    stopTemporizing();

    switch ((Aqpm::Maintenance::Action)type) {
    case Aqpm::Maintenance::CleanCache:
    case Aqpm::Maintenance::CleanUnusedDatabases:
        // TODO: This should not work this way
        if (!QProcess::execute("pacman -Sc --noconfirm")) {
            operationPerformed(true);
        } else {
            operationPerformed(false);
        }
        return;
    case Aqpm::Maintenance::EmptyCache:
        // TODO: This should not work this way
        if (!QProcess::execute("pacman -Scc --noconfirm")) {
            operationPerformed(true);
        } else {
            operationPerformed(false);
        }
        return;
    case Aqpm::Maintenance::OptimizeDatabases:
        d->proc = new QProcess(this);
        connect(d->proc, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished(int,QProcess::ExitStatus)));
        connect(d->proc, SIGNAL(readyReadStandardError()), this, SLOT(errorOutput()));
        connect(d->proc, SIGNAL(readyReadStandardOutput()), this, SLOT(standardOutput()));
        d->proc->start("pacman-optimize");
        break;
    default:
        operationPerformed(false);
        break;
    }
}

void Worker::processFinished(int exitCode, QProcess::ExitStatus)
{
    d->proc->deleteLater();

    if (exitCode) {
        operationPerformed(false);
    } else {
        QProcess::execute("sync");
        operationPerformed(true);
    }
}

void Worker::errorOutput()
{
    d->proc->setReadChannel(QProcess::StandardError);
    emit messageStreamed(QString::fromLocal8Bit(d->proc->readLine()));
}

void Worker::standardOutput()
{
    d->proc->setReadChannel(QProcess::StandardOutput);
    emit messageStreamed(QString::fromLocal8Bit(d->proc->readLine()));
}

void Worker::interruptTransaction()
{
    if (QDBusConnection::systemBus().interface()->isServiceRegistered("org.chakraproject.aqpmdownloader")) {
        QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmdownloader",
                                                              "/Downloader",
                                                              "org.chakraproject.aqpmdownloader",
                                                              QLatin1String("abortDownload"));
        QDBusConnection::systemBus().call(message, QDBus::NoBlock);
    }

    if (alpm_trans_interrupt() == 0) {
        alpm_trans_release();
        emit errorOccurred((int) Aqpm::Globals::TransactionInterruptedByUser, QVariantMap());
        emit operationPerformed(false);
        startTemporizing();
    } else {
    }
}

pmtransflag_t Worker::processFlags(Aqpm::Globals::TransactionFlags flags)
{
    pmtransflag_t retflags = (pmtransflag_t)0;

    if (flags & Aqpm::Globals::NoDeps) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_NODEPS);
    }
    if (flags & Aqpm::Globals::Force) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_FORCE);
    }
    if (flags & Aqpm::Globals::NoSave) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_NOSAVE);
    }
    if (flags & Aqpm::Globals::Cascade) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_CASCADE);
    }
    if (flags & Aqpm::Globals::Recurse) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_RECURSE);
    }
    if (flags & Aqpm::Globals::DbOnly) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_DBONLY);
    }
    if (flags & Aqpm::Globals::AllDeps) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_ALLDEPS);
    }
    if (flags & Aqpm::Globals::DownloadOnly) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_DOWNLOADONLY);
    }
    if (flags & Aqpm::Globals::NoScriptlet) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_NOSCRIPTLET);
    }
    if (flags & Aqpm::Globals::NoConflicts) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_NOCONFLICTS);
    }
    if (flags & Aqpm::Globals::Needed) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_NEEDED);
    }
    if (flags & Aqpm::Globals::AllExplicit) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_ALLEXPLICIT);
    }
    if (flags & Aqpm::Globals::UnNeeded) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_UNNEEDED);
    }
    if (flags & Aqpm::Globals::RecurseAll) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_RECURSEALL);
    }
    if (flags & Aqpm::Globals::NoLock) {
        retflags = (pmtransflag_t)((pmtransflag_t)retflags | (pmtransflag_t)PM_TRANS_FLAG_NOLOCK);
    }

    return retflags;
}

}
