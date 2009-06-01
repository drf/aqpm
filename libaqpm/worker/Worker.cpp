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

#include "Worker.h"

#include "aqpmworkeradaptor.h"

#include "../ConfigurationParser.h"
#include "w_callbacks.h"
#include "../Backend.h"

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
    Private() : ready(false) {}

    pmdb_t *db_local;
    pmdb_t *dbs_sync;

    bool ready;
};

Worker::Worker(QObject *parent)
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

    setUpAlpm();
}

Worker::~Worker()
{
}

bool Worker::isWorkerReady()
{
    return d->ready;
}

void Worker::setUpAlpm()
{
    Aqpm::PacmanConf pdata;

    pdata = Aqpm::ConfigurationParser::instance()->getPacmanConf(true);

    alpm_option_set_root("/");
    alpm_option_set_dbpath("/var/lib/pacman");
    alpm_option_add_cachedir("/var/cache/pacman/pkg");

    d->db_local = alpm_db_register_local();

    alpm_option_set_logcb(AqpmWorker::cb_log);
    alpm_option_set_fetchcb(AqpmWorker::cb_fetch);
    alpm_option_set_totaldlcb(AqpmWorker::cb_dl_total);

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

    /*foreach(const QString &str, pdata.HoldPkg) {
        alpm_option_add_(str.toAscii().data());
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

    d->ready = true;
    emit workerReady();
}

void Worker::updateDatabase()
{
    qDebug() << "Starting DB Update";

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.updatedatabase",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        QCoreApplication::instance()->quit();
        return;
    }

    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_ALLDEPS, AqpmWorker::cb_trans_evt,
                        AqpmWorker::cb_trans_conv, AqpmWorker::cb_trans_progress) == -1) {
        operationPerformed(false);
        qDebug() << "Error!";
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
        }
    }

    qDebug() << "Database Update Performed";

    operationPerformed(true);

    QCoreApplication::instance()->quit();
}

void Worker::processQueue(const QVariantList &packages, const QVariantList &flags)
{
    qDebug() << "Starting Queue Processing";

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.processqueue",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        QCoreApplication::instance()->quit();
        return;
    }

    pmtransflag_t alpmflags;

    if (flags.isEmpty()) {
        alpmflags = PM_TRANS_FLAG_ALLDEPS;
    } else {
        alpmflags = (pmtransflag_t)flags.at(0).toUInt();

        for (int i = 1; i < flags.count(); ++i) {
            alpmflags = (pmtransflag_t)((pmtransflag_t)alpmflags | (pmtransflag_t)flags.at(i).toUInt());
        }
    }

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
                return;
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
                return;
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
                return;
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

void Worker::systemUpgrade()
{
    qDebug() << "Starting System Upgrade";

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.systemupgrade",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        QCoreApplication::instance()->quit();
        return;
    }

    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_ALLDEPS,
                        AqpmWorker::cb_trans_evt, AqpmWorker::cb_trans_conv,
                        AqpmWorker::cb_trans_progress) == -1) {
        QVariantMap args;
        args["ErrorString"] = QString(alpm_strerrorlast());
        emit errorOccurred(Aqpm::Globals::InitTransactionError, args);
        operationPerformed(false);
        return;
    }

    if (alpm_trans_sysupgrade() == -1) {
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
    QTimer::singleShot(2000, QCoreApplication::instance(), SLOT(quit()));
}

bool Worker::performTransaction()
{
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
            emit errorOccurred(Aqpm::Globals::PrepareError | Aqpm::Globals::UnsatisfiedDependencies, args);
            break;
        case PM_ERR_CONFLICTING_DEPS:
            for(i = data; i; i = alpm_list_next(i)) {
                pmconflict_t *conflict = (pmconflict_t*) alpm_list_getdata(i);
                innerdata[alpm_conflict_get_package1(conflict)] = alpm_conflict_get_package2(conflict);
            }
            args["ConflictingDeps"] = innerdata;
            emit errorOccurred(Aqpm::Globals::PrepareError | Aqpm::Globals::ConflictingDependencies, args);
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
                    args["ConflictingTargets"] = innerdata;
                    emit errorOccurred(Aqpm::Globals::CommitError | Aqpm::Globals::FileConflictTarget, args);
                    break;
                case PM_FILECONFLICT_FILESYSTEM:
                    innerdata[alpm_fileconflict_get_target(conflict)] = alpm_fileconflict_get_file(conflict);
                    args["ConflictingFiles"] = innerdata;
                    emit errorOccurred(Aqpm::Globals::CommitError | Aqpm::Globals::FileConflictFilesystem, args);
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
            emit errorOccurred(Aqpm::Globals::CommitError | Aqpm::Globals::CorruptedFile, args);
            break;
        default:
            args["ErrorString"] = alpm_strerrorlast();
            emit errorOccurred(Aqpm::Globals::CommitError, args);
            break;
        }
        qDebug() << "Could not commit transaction";
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
            emit errorOccurred(Aqpm::Globals::AddTargetError | Aqpm::Globals::DuplicateTarget, args);
        } else if (pm_errno == PM_ERR_PKG_IGNORED) {
            QVariantMap args;
            args["PackageName"] = target;
            emit errorOccurred(Aqpm::Globals::AddTargetError | Aqpm::Globals::PackageIgnored, args);
        } else if (pm_errno == PM_ERR_PKG_NOT_FOUND) {
            QVariantMap args;
            args["PackageName"] = target;
            emit errorOccurred(Aqpm::Globals::AddTargetError | Aqpm::Globals::PackageNotFound, args);
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

}
