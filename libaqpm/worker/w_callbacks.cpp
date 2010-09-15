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

#include "w_callbacks_p.h"

#include "../Globals.h"

#include <unistd.h>
#include <sys/types.h>
#include <QDebug>
#include <QGlobalStatic>
#include <QDateTime>
#include <QEventLoop>
#include <QFile>
#include <QTimer>
#include <QStringList>
#include <QCoreApplication>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include "alpm.h"
#include "Worker_p.h"

namespace AqpmWorker
{

void ReturnStringConditionalEventLoop::requestQuit(const QString &str, const QString &result)
{
    qDebug() << "Quit request";
    if (str == m_cond) {
        m_result = result;
        quit();
    }
}

QString ReturnStringConditionalEventLoop::result() const
{
    return m_result;
}

class CallBacksPrivate
{
public:
    CallBacksPrivate();
    void init();

    Q_DECLARE_PUBLIC(CallBacks)
    CallBacks *q_ptr;

    int answer;
    QTime busControlTime;
    QString currentFile;

    Worker *worker;
};

class CallBacksHelper
{
public:
    CallBacksHelper() : q(0) {}
    ~CallBacksHelper() {
        delete q;
    }
    CallBacks *q;
};

Q_GLOBAL_STATIC(CallBacksHelper, s_globalCallbacks)

CallBacks *CallBacks::instance()
{
    if (!s_globalCallbacks()->q) {
        new CallBacks;
    }

    return s_globalCallbacks()->q;
}

CallBacksPrivate::CallBacksPrivate()
        : answer(-1)
{
}

void CallBacksPrivate::init()
{
}

CallBacks::CallBacks(QObject *parent)
        : QObject(parent)
        , d_ptr(new CallBacksPrivate())
{
    Q_ASSERT(!s_globalCallbacks()->q);
    s_globalCallbacks()->q = this;
    d_ptr->q_ptr = this;
    qDebug() << "Constructing callbacks";
    d_ptr->init();
}

CallBacks::~CallBacks()
{
    delete d_ptr;
}


void CallBacks::setWorker(Worker *w)
{
    Q_D(CallBacks);

    d->worker = w;
}

void CallBacks::cb_trans_evt(pmtransevt_t event, void *data1, void *data2)
{
    QVariantMap args;
    QString logmsg;

    switch (event) {
    case PM_TRANS_EVT_CHECKDEPS_START:
        emit streamTransEvent((int) Aqpm::Globals::CheckDepsStart, QVariantMap());
        break;
    case PM_TRANS_EVT_FILECONFLICTS_START:
        emit streamTransEvent((int) Aqpm::Globals::FileConflictsStart, QVariantMap());
        break;
    case PM_TRANS_EVT_RESOLVEDEPS_START:
        emit streamTransEvent((int) Aqpm::Globals::ResolveDepsStart, QVariantMap());
        break;
    case PM_TRANS_EVT_INTERCONFLICTS_START:
        emit streamTransEvent((int) Aqpm::Globals::InterConflictsStart, QVariantMap());
        break;
    case PM_TRANS_EVT_ADD_START:
        args["PackageName"] = alpm_pkg_get_name((pmpkg_t*)data1);
        emit streamTransEvent((int) Aqpm::Globals::AddStart, args);
        break;
    case PM_TRANS_EVT_ADD_DONE:
        args["PackageName"] = alpm_pkg_get_name((pmpkg_t*)data1);
        args["PackageVersion"] = alpm_pkg_get_version((pmpkg_t*)data1);
        emit streamTransEvent((int) Aqpm::Globals::AddDone, args);
        logmsg = QString("%1 (%2) installed successfully").arg(args["PackageName"].toString())
                 .arg(args["PackageVersion"].toString()) + '\n';
        alpm_logaction(logmsg.toUtf8().data());
        break;
    case PM_TRANS_EVT_REMOVE_START:
        args["PackageName"] = alpm_pkg_get_name((pmpkg_t*)data1);
        emit streamTransEvent((int) Aqpm::Globals::RemoveStart, args);
        break;
    case PM_TRANS_EVT_REMOVE_DONE:
        args["PackageName"] = alpm_pkg_get_name((pmpkg_t*)data1);
        args["PackageVersion"] = alpm_pkg_get_version((pmpkg_t*)data1);
        emit streamTransEvent((int) Aqpm::Globals::RemoveDone, args);
        logmsg = QString("%1 (%2) removed successfully").arg(args["PackageName"].toString())
                 .arg(args["PackageVersion"].toString()) + '\n';
        alpm_logaction(logmsg.toUtf8().data());
        break;
    case PM_TRANS_EVT_UPGRADE_START:
        args["PackageName"] = alpm_pkg_get_name((pmpkg_t*)data1);
        emit streamTransEvent((int) Aqpm::Globals::UpgradeStart, args);
        break;
    case PM_TRANS_EVT_UPGRADE_DONE:
        args["PackageName"] = alpm_pkg_get_name((pmpkg_t*)data1);
        args["OldVersion"] = QString((char*)alpm_pkg_get_version((pmpkg_t*)data2));
        args["NewVersion"] = QString((char*)alpm_pkg_get_version((pmpkg_t*)data1));
        emit streamTransEvent((int) Aqpm::Globals::UpgradeDone, args);
        logmsg = QString("Upgraded %1 successfully (%2 -> %3)").arg(args["PackageName"].toString())
                 .arg(args["OldVersion"].toString()).arg(args["NewVersion"].toString()) + '\n';
        alpm_logaction(logmsg.toUtf8().data());
        break;
    case PM_TRANS_EVT_INTEGRITY_START:
        emit streamTransEvent((int) Aqpm::Globals::IntegrityStart, QVariantMap());
        break;
    case PM_TRANS_EVT_DELTA_INTEGRITY_START:
        emit streamTransEvent((int) Aqpm::Globals::DeltaIntegrityStart, QVariantMap());
        break;
    case PM_TRANS_EVT_DELTA_PATCHES_START:
        emit streamTransEvent((int) Aqpm::Globals::DeltaPatchesStart, QVariantMap());
        break;
    case PM_TRANS_EVT_DELTA_PATCH_START:
        args["From"] = QString((char*)data1);
        args["To"] = QString((char*)data2);
        emit streamTransEvent((int) Aqpm::Globals::DeltaPatchStart, args);
        break;
    case PM_TRANS_EVT_DELTA_PATCH_DONE:
        emit streamTransEvent((int) Aqpm::Globals::DeltaPatchDone, QVariantMap());
        break;
    case PM_TRANS_EVT_DELTA_PATCH_FAILED:
        emit streamTransEvent((int) Aqpm::Globals::DeltaPatchFailed, QVariantMap());
        break;
    case PM_TRANS_EVT_SCRIPTLET_INFO:
        args["Text"] = QString((char*)data1);
        emit streamTransEvent((int) Aqpm::Globals::ScriptletInfo, args);
        alpm_logaction((char*)data1);
        break;
    case PM_TRANS_EVT_RETRIEVE_START:
        args["Repo"] = QString((char*)data1);
        emit streamTransEvent((int) Aqpm::Globals::RetrieveStart, args);
        break;
    case PM_TRANS_EVT_FILECONFLICTS_DONE:
        emit streamTransEvent((int) Aqpm::Globals::FileConflictsDone, QVariantMap());
        break;
    case PM_TRANS_EVT_CHECKDEPS_DONE:
        emit streamTransEvent((int) Aqpm::Globals::CheckDepsDone, QVariantMap());
        break;
    case PM_TRANS_EVT_RESOLVEDEPS_DONE:
        emit streamTransEvent((int) Aqpm::Globals::ResolveDepsDone, QVariantMap());
        break;
    case PM_TRANS_EVT_INTERCONFLICTS_DONE:
        emit streamTransEvent((int) Aqpm::Globals::InterConflictsDone, QVariantMap());
        break;
    case PM_TRANS_EVT_INTEGRITY_DONE:
        emit streamTransEvent((int) Aqpm::Globals::IntegrityDone, QVariantMap());
        break;
    case PM_TRANS_EVT_DELTA_INTEGRITY_DONE:
        emit streamTransEvent((int) Aqpm::Globals::DeltaIntegrityDone, QVariantMap());
        break;
    case PM_TRANS_EVT_DELTA_PATCHES_DONE:
        emit streamTransEvent((int) Aqpm::Globals::DeltaPatchesDone, QVariantMap());
        break;
    default:
        break;
    }

    QCoreApplication::processEvents();
}

void CallBacks::setAnswer(int answer)
{
    Q_D(CallBacks);

    d->answer = answer;
    emit answerReady();
}

void CallBacks::cb_trans_conv(pmtransconv_t event, void *data1, void *data2,
                              void *data3, int *response)
{
    Q_D(CallBacks);

    qDebug() << "Alpm is asking a question.";

    QVariantMap args;
    Aqpm::Globals::TransactionQuestion question = Aqpm::Globals::InvalidQuestion;

    switch (event) {
    case PM_TRANS_CONV_INSTALL_IGNOREPKG:
        if (data2) {
            args["FirstPackage"] = alpm_pkg_get_name((pmpkg_t *)data1);
            args["SecondPackage"] = alpm_pkg_get_name((pmpkg_t *)data2);
            question = Aqpm::Globals::IgnorePackage;
            /* TODO we take this route based on data2 being not null? WTF */
        } else {
            args["FirstPackage"] = alpm_pkg_get_name((pmpkg_t *)data1);
            question = Aqpm::Globals::IgnorePackage;
        }
        break;
    case PM_TRANS_CONV_REPLACE_PKG:
        args["OldPackage"] = alpm_pkg_get_name((pmpkg_t *)data1);
        args["NewPackage"] = alpm_pkg_get_name((pmpkg_t *)data2);
        args["NewPackageRepo"] = QString((char *)data3);
        question = Aqpm::Globals::ReplacePackage;
        break;
    case PM_TRANS_CONV_CONFLICT_PKG:
        args["NewPackage"] = QString((char *)data1);
        args["OldPackage"] = QString((char *)data2);
        question = Aqpm::Globals::PackageConflict;
        break;
    case PM_TRANS_CONV_REMOVE_PKGS: {
        alpm_list_t *unresolved = (alpm_list_t *) data1;
        alpm_list_t *i;
        QStringList pkgs;
        for (i = unresolved; i; i = i->next) {
            pkgs.append((char *)alpm_pkg_get_name((pmpkg_t*)(i->data)));
        }
        args["Packages"] = pkgs;
        question = Aqpm::Globals::UnresolvedDependencies;
    }
    break;
    case PM_TRANS_CONV_LOCAL_NEWER:
        args["PackageName"] = alpm_pkg_get_name((pmpkg_t *)data1);
        args["PackageVersion"] = alpm_pkg_get_version((pmpkg_t *)data1);
        question = Aqpm::Globals::NewerLocalPackage;
        break;
    case PM_TRANS_CONV_CORRUPTED_PKG:
        args["Filename"] = QString((char *)data1);
        question = Aqpm::Globals::CorruptedPackage;
        break;
    default:
        qDebug() << event;
        break;
    }

    QEventLoop e;

    connect(this, SIGNAL(answerReady()), &e, SLOT(quit()));

    emit streamTransQuestion((int) question, args);
    qDebug() << "Question Streamed, entering in custom event loop, waiting for answer";

    e.exec();

    qDebug() << "Looks like answer is there, and it's " << d->answer;

    *response = d->answer;

    return;

}

int CallBacks::cb_fetch(const char *url, const char *localpath, int force)
{
    Q_D(CallBacks);

    qDebug() << "fetching: " << url << localpath;

    QDBusMessage message = QDBusMessage::createMethodCall(d->worker->dbusService(),
                           "/Downloader",
                           "org.chakraproject.aqpmdownloader",
                           QLatin1String("checkHeader"));

    message << QString(url);
    QDBusMessage mreply = d->worker->dbusConnection().call(message, QDBus::Block);

    if (mreply.type() == QDBusMessage::ErrorMessage || mreply.arguments().count() < 1) {
        // Damn this, there was an error
        qDebug() << "Error in the DBus reply of the header check!";
        qDebug() << mreply.errorMessage();
        qDebug() << d->worker->dbusService();
        qDebug() << d->worker->dbusConnection().lastError();
        return 1;
    }

    d->currentFile = QString(url).split('/').at(QString(url).split('/').length() - 1);

    qDebug() << "The file is " << d->currentFile;

    // Let's check if the modification times collide. If so, there's no need to download the file
/*
    time_t newtime = mreply.arguments().first().toInt();
    qDebug() << "Header says" << newtime;
    qDebug() << "Old file says" << mtimeold;
    mtimenew = new time_t;
    *mtimenew = newtime;

    qDebug() << "Comparing";

    if (*mtimenew == mtimeold) {
        return 1;
    }

    qDebug() << "Compare successful";
*/
    // If we got here, it's time to download the file.

    ReturnStringConditionalEventLoop e(url);

    qDebug() << d->worker->dbusConnection().connect(d->worker->dbusService(),
            "/Downloader",
            "org.chakraproject.aqpmdownloader", "finished", &e, SLOT(requestQuit(QString, QString)));

    QDBusMessage qmessage = QDBusMessage::createMethodCall(d->worker->dbusService(),
                            "/Downloader",
                            "org.chakraproject.aqpmdownloader",
                            QLatin1String("download"));

    qmessage << QString(url);
    d->worker->dbusConnection().asyncCall(qmessage);

    e.exec();

    QString downloadedFile = e.result();
    qDebug() << "Moving downloaded file " << downloadedFile << " to " << localpath + d->currentFile;
    QFile::copy(downloadedFile, localpath + d->currentFile);
    // cleanup
    QFile::remove(downloadedFile);

    return 0;
}

void CallBacks::cb_dl_total(off_t total)
{
    qDebug() << "total called, offset" << total;
    emit streamTotalOffset(total);
}

void CallBacks::cb_trans_progress(pmtransprog_t event, const char *pkgname, int percent,
                                  int howmany, int remain)
{
    Aqpm::Globals::TransactionProgress evt;

    if (percent > 0 && percent < 100) {
        return;
    }

    switch (event) {
    case PM_TRANS_PROGRESS_ADD_START:
        evt = Aqpm::Globals::AddRoutineStart;
        break;
    case PM_TRANS_PROGRESS_CONFLICTS_START:
        evt = Aqpm::Globals::ConflictsRoutineStart;
        break;
    case PM_TRANS_PROGRESS_REMOVE_START:
        evt = Aqpm::Globals::RemoveRoutineStart;
        break;
    case PM_TRANS_PROGRESS_UPGRADE_START:
        evt = Aqpm::Globals::UpgradeRoutineStart;
        break;
    }

    qDebug() << "Streaming trans progress";

    emit streamTransProgress((int)evt, pkgname, percent, howmany, remain);
}

void CallBacks::cb_log(pmloglevel_t level, char *fmt, va_list args)
{
    if (!strlen(fmt)) {
        return;
    }

    char *string = NULL;
    if (pm_vasprintf(&string, level, fmt, args) == -1) {
        return;
    }

    if (string != NULL) {
        QString msg = QString::fromLocal8Bit(string);
        emit logMessageStreamed(msg);

        if (level == PM_LOG_ERROR) {
            alpm_logaction(string);
        }
    }
}

void cb_dl_total(off_t total)
{
    CallBacks::instance()->cb_dl_total(total);
}

int cb_fetch(const char *url, const char *localpath, int force)
{
    return CallBacks::instance()->cb_fetch(url, localpath, force);
}

void cb_trans_progress(pmtransprog_t event, const char *pkgname, int percent,
                       int howmany, int remain)
{
    CallBacks::instance()->cb_trans_progress(event, pkgname, percent, howmany, remain);
}

void cb_trans_conv(pmtransconv_t event, void *data1, void *data2,
                   void *data3, int *response)
{
    CallBacks::instance()->cb_trans_conv(event, data1, data2, data3, response);
}

void cb_trans_evt(pmtransevt_t event, void *data1, void *data2)
{
    CallBacks::instance()->cb_trans_evt(event, data1, data2);
}

void cb_log(pmloglevel_t level, char *fmt, va_list args)
{
    CallBacks::instance()->cb_log(level, fmt, args);
}

int pm_vasprintf(char **string, pmloglevel_t level, const char *format, va_list args)
{
    int ret = 0;
    char *msg = NULL;

    /* if current logmask does not overlap with level, do not print msg */

    /* print the message using va_arg list */
    ret = vasprintf(&msg, format, args);

    /* print a prefix to the message */
    switch (level) {
    case PM_LOG_DEBUG:
        return -1;
        break;
    case PM_LOG_ERROR:
        asprintf(string, "error: %s", msg);
        break;
    case PM_LOG_WARNING:
        asprintf(string, "warning: %s", msg);
        break;
    case PM_LOG_FUNCTION:
        return -1;
        break;
    default:
        break;
    }

    free(msg);

    return(ret);
}

}

#include "w_callbacks_p.moc"
