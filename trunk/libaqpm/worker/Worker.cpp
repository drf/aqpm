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
#include <QDebug>

#include <Context>
#include <Auth>

namespace AqpmWorker {

class Worker::Private
{
    public:
        Private() : ready(false) {};

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
        QTimer::singleShot(0, this, SLOT(quit()));
        return;
    }

    if (!QDBusConnection::systemBus().registerObject("/Worker", this)) {
        qDebug() << "unable to register service interface to dbus";
        QTimer::singleShot(0, this, SLOT(quit()));
        return;
    }

    alpm_initialize();

    connect(AqpmWorker::CallBacks::instance(), SIGNAL(streamTransDlProg(const QString&, int, int, int, int, int, int)),
            SIGNAL(streamTransDlProg(const QString&, int, int, int, int, int, int)));
    connect(AqpmWorker::CallBacks::instance(), SIGNAL(streamTransProgress(int, const QString&, int, int, int)),
            SIGNAL(streamTransProgress(int, const QString&, int, int, int)));
    connect(AqpmWorker::CallBacks::instance(), SIGNAL(streamTransEvent(int, void*, void*)),
            SIGNAL(streamTransEvent(int, void*, void*)));

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

    alpm_option_set_dlcb(AqpmWorker::cb_dl_progress);
    alpm_option_set_totaldlcb(AqpmWorker::cb_dl_total);
    alpm_option_set_logcb(AqpmWorker::cb_log);

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

    d->ready = true;
    emit workerReady();
}

void Worker::updateDatabase()
{
    qDebug() << "Starting DB Update";

    PolKitResult result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.updatedatabase",
                                      message().service(),
                                      true);
    if (result == POLKIT_RESULT_YES) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        QCoreApplication::instance()->quit();
        return;
    }

    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_ALLDEPS, AqpmWorker::cb_trans_evt,
                        AqpmWorker::cb_trans_conv, AqpmWorker::cb_trans_progress) == -1) {
        emit workerResult(false);
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

        emit dbStatusChanged(curdbname, Aqpm::Backend::Checking);

        r = alpm_db_update(0, dbcrnt);

        if (r == 1) {
            emit dbStatusChanged(curdbname, Aqpm::Backend::Unchanged);
        } else if (r < 0) {
            emit dbStatusChanged(curdbname, Aqpm::Backend::DatabaseError);
        } else {
            emit dbStatusChanged(curdbname, Aqpm::Backend::Updated);
        }

        syncdbs = alpm_list_next(syncdbs);
    }

    if (alpm_trans_release() == -1) {
        if (alpm_trans_interrupt() == -1) {
            emit workerResult(false);
        }
    }

    qDebug() << "Database Update Performed";

    emit workerResult(true);

    QCoreApplication::instance()->quit();
}

void Worker::processQueue(QVariantList packages, QVariantList flags)
{
    qDebug() << "Starting Queue Processing";

    PolKitResult result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.processqueue",
                                                message().service(),
                                                true);
    if (result == POLKIT_RESULT_YES) {
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
    bool sysupgrade = false;
    bool remove = false;
    bool file = false;

    Aqpm::QueueItem::List queue;

    foreach (QVariant ent, packages) {
        queue.append(ent.value<Aqpm::QueueItem*>());
    }

    foreach(Aqpm::QueueItem *itm, queue) {
        switch (itm->action_id) {
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
            case Aqpm::QueueItem::FullUpgrade:
                qDebug() << "Upgrade action";
                sysupgrade = true;
                break;
        }
    }

    if (remove) {
        if (alpm_trans_init(PM_TRANS_TYPE_REMOVE, alpmflags,
                cb_trans_evt,
                cb_trans_conv,
                cb_trans_progress) == -1) {
            emit errorOccurred(Aqpm::Backend::InitTransactionError);
            emit workerResult(false);
            return;
        }

        qDebug() << "Starting Package Removal";

        foreach(Aqpm::QueueItem *itm, queue) {
            if (itm->action_id != Aqpm::QueueItem::Remove) {
                return;
            }

            int res = alpm_trans_addtarget(qstrdup(itm->name.toUtf8()));

            if (res == -1) {
                emit errorOccurred(Aqpm::Backend::AddTargetError);
                emit workerResult(false);
                return;
            }
        }

        alpm_list_t *data = NULL;

        qDebug() << "Preparing...";
        if (alpm_trans_prepare(&data) == -1) {
            qDebug() << "Could not prepare transaction";
            emit errorOccurred(Aqpm::Backend::PrepareError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }

        qDebug() << "Committing...";
        if (alpm_trans_commit(&data) == -1) {
            qDebug() << "Could not commit transaction";
            emit errorOccurred(Aqpm::Backend::CommitError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }
        qDebug() << "Done";

        if (data) {
            FREELIST(data);
        }

        alpm_trans_release();
    }

    if (sync) {
        if (alpm_trans_init(PM_TRANS_TYPE_SYNC, alpmflags,
                cb_trans_evt,
                cb_trans_conv,
                cb_trans_progress) == -1) {
            emit errorOccurred(Aqpm::Backend::InitTransactionError);
            emit workerResult(false);
            return;
        }

        qDebug() << "Starting Package Syncing";

        foreach(Aqpm::QueueItem *itm, queue) {
            if (itm->action_id != Aqpm::QueueItem::Sync) {
                return;
            }

            qDebug() << "Adding " << itm->name;
            int res = alpm_trans_addtarget(qstrdup(itm->name.toUtf8()));

            if (res == -1) {
                emit errorOccurred(Aqpm::Backend::AddTargetError);
                emit workerResult(false);
                return;
            }
        }

        alpm_list_t *data = NULL;

        qDebug() << "Preparing...";
        if (alpm_trans_prepare(&data) == -1) {
            qDebug() << "Could not prepare transaction";
            emit errorOccurred(Aqpm::Backend::PrepareError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }

        qDebug() << "Committing...";
        if (alpm_trans_commit(&data) == -1) {
            qDebug() << "Could not commit transaction";
            emit errorOccurred(Aqpm::Backend::CommitError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }
        qDebug() << "Done";

        if (data) {
            FREELIST(data);
        }

        alpm_trans_release();
    }

    if (sysupgrade) {
        if (alpm_trans_init(PM_TRANS_TYPE_SYNC, alpmflags,
                cb_trans_evt,
                cb_trans_conv,
                cb_trans_progress) == -1) {
            emit errorOccurred(Aqpm::Backend::InitTransactionError);
            emit workerResult(false);
            return;
        }

        if (alpm_trans_sysupgrade() == -1) {
            qDebug() << "Creating a sysupgrade transaction failed!!";
            emit errorOccurred(Aqpm::Backend::CreateSysUpgradeError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }

        alpm_list_t *data = NULL;

        qDebug() << "Preparing...";
        if (alpm_trans_prepare(&data) == -1) {
            qDebug() << "Could not prepare transaction";
            emit errorOccurred(Aqpm::Backend::PrepareError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }

        qDebug() << "Committing...";
        if (alpm_trans_commit(&data) == -1) {
            qDebug() << "Could not commit transaction";
            emit errorOccurred(Aqpm::Backend::CommitError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }
        qDebug() << "Done";

        if (data) {
            FREELIST(data);
        }

        alpm_trans_release();
    }

    if (file) {
        if (alpm_trans_init(PM_TRANS_TYPE_UPGRADE, alpmflags,
                cb_trans_evt,
                cb_trans_conv,
                cb_trans_progress) == -1) {
            emit errorOccurred(Aqpm::Backend::InitTransactionError);
            emit workerResult(false);
            return;
        }

        qDebug() << "Starting Package Syncing";

        foreach(Aqpm::QueueItem *itm, queue) {
            if (itm->action_id != Aqpm::QueueItem::FromFile) {
                return;
            }

            int res = alpm_trans_addtarget(qstrdup(itm->name.toUtf8()));

            if (res == -1) {
                emit errorOccurred(Aqpm::Backend::AddTargetError);
                emit workerResult(false);
                return;
            }
        }

        alpm_list_t *data = NULL;

        qDebug() << "Preparing...";
        if (alpm_trans_prepare(&data) == -1) {
            qDebug() << "Could not prepare transaction";
            emit errorOccurred(Aqpm::Backend::PrepareError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }

        qDebug() << "Committing...";
        if (alpm_trans_commit(&data) == -1) {
            qDebug() << "Could not commit transaction";
            emit errorOccurred(Aqpm::Backend::CommitError);
            alpm_trans_release();
            emit workerResult(false);
            return;
        }
        qDebug() << "Done";

        if (data) {
            FREELIST(data);
        }

        alpm_trans_release();
    }

    emit workerResult(true);
}

}
