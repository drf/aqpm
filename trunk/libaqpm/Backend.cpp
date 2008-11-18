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

    alpm_initialize();

    qRegisterMetaType<pmtransevt_t>("pmtransevt_t");
    qRegisterMetaType<pmtransprog_t>("pmtransprog_t");

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

void Backend::initAlpm()
{
    PacmanConf pdata;

    pdata = ConfigurationParser::instance()->getPacmanConf(true);

    alpm_option_set_root("/");
    alpm_option_set_dbpath("/var/lib/pacman");
    alpm_option_add_cachedir("/var/cache/pacman/pkg");

    d->db_local = alpm_db_register_local();

    alpm_option_set_dlcb(cb_dl_progress);
    alpm_option_set_totaldlcb(cb_dl_total);

    if (pdata.logFile.isEmpty())
        alpm_option_set_logfile("/var/log/pacman.log");
    else
        alpm_option_set_logfile(pdata.logFile.toAscii().data());


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
}

QStringList Backend::getUpgradeablePackages()
{
    alpm_list_t *syncpkgs = NULL;
    QStringList retlist;
    retlist.clear();

    if (alpm_sync_sysupgrade(d->db_local, alpm_option_get_syncdbs(), &syncpkgs) == -1) {
        return retlist;
    }

    syncpkgs = alpm_list_first(syncpkgs);

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

void Backend::computeTransactionResult()
{
    if (d->trThread) {
        bool error = d->trThread->isError();
        d->trThread->deleteLater();

        if (error) {
            emit operationFailed();
        } else {
            emit operationSuccessful();
        }
    }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

TrCommitThread::TrCommitThread(pmtranstype_t type, const QStringList &packages, QObject *parent)
        : QThread(parent),
        m_error(false),
        m_type(type),
        m_packages(packages)
{
}

void TrCommitThread::run()
{
    pmtransflag_t flags;

    if (m_type == PM_TRANS_TYPE_SYNC) {
        flags = (pmtransflag_t)((pmtransflag_t)PM_TRANS_FLAG_ALLDEPS |
                                (pmtransflag_t)PM_TRANS_FLAG_FORCE);
    } else {
        flags = (pmtransflag_t)((pmtransflag_t)PM_TRANS_FLAG_NODEPS |
                                (pmtransflag_t)PM_TRANS_FLAG_FORCE);
    }

    if (alpm_trans_init(m_type, flags,
                        cb_trans_evt, cb_trans_conv, cb_trans_progress) == -1) {
        emit error(tr("Could not initialize Alpm Transaction"));
        m_error = true;
        return;
    }

    foreach(const QString &package, m_packages) {
        int res = alpm_trans_addtarget(package.toAscii().data());

        if (res == -1) {
            emit error(tr("Could not add %1 to transaction").arg(package));
            m_error = true;
            return;
        }
    }

    alpm_list_t *data = NULL;

    if (alpm_trans_prepare(&data) == -1) {
        qDebug() << "Could not prepare transaction";
        alpm_trans_release();
        emit error(tr("Could not prepare transaction"));
        m_error = true;
        return;

    }

    if (alpm_trans_commit(&data) == -1) {
        qDebug() << "Could not commit transaction";
        alpm_trans_release();
        emit error(tr("Could not commit transaction"));
        m_error = true;
        return;
    }

    if (data) {
        FREELIST(data);
    }

    alpm_trans_release();
}

//

UpDbThread::UpDbThread(pmdb_t *db, QObject *parent)
        : QThread(parent),
        m_db(db),
        m_error(false)
{
    connect(this, SIGNAL(finished()), SLOT(deleteLater()));
}

void UpDbThread::run()
{
    if (alpm_trans_init(PM_TRANS_TYPE_SYNC, PM_TRANS_FLAG_ALLDEPS, cb_trans_evt, cb_trans_conv,
                        cb_trans_progress) == -1) {
        emit error(tr("Could not initialize Alpm Transaction"));
        m_error = true;
        return;
    }

    alpm_db_update(0, m_db);

    if (alpm_trans_release() == -1) {
        if (alpm_trans_interrupt() == -1) {
            m_error = true;
        }
    }
}

