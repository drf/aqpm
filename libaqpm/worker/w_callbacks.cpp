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

#include "w_callbacks.h"

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
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkProxy>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include "alpm.h"

namespace AqpmWorker
{

class CallBacksPrivate
{
public:
    CallBacksPrivate();
    void init();

    QNetworkRequest createNetworkRequest(const QUrl &url);

    Q_DECLARE_PUBLIC(CallBacks)
    CallBacks *q_ptr;

    int answer;
    qint64 list_total;
    qint64 list_xfered;
    qint64 list_last;
    QDateTime averageRateTime;
    QTime busControlTime;
    QString currentFile;
    QNetworkAccessManager *manager;
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
        , averageRateTime()
        , list_xfered(0)
        , list_last(0)
        , list_total(0)
{
}

void CallBacksPrivate::init()
{
    Q_Q(CallBacks);
    manager = new QNetworkAccessManager(q);
    manager->setProxy(QNetworkProxy());
}

QNetworkRequest CallBacksPrivate::createNetworkRequest(const QUrl &url)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", "Aqpm 1.0");
    return request;
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

void CallBacks::cb_trans_evt(pmtransevt_t event, void *data1, void *data2)
{
    Q_D(CallBacks);

    QVariantMap args;
    QString logmsg;

    switch ( event ) {
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
                 .arg(args["PackageVersion"].toString());
        alpm_logaction( logmsg.toUtf8().data() );
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
                 .arg(args["PackageVersion"].toString());
        alpm_logaction( logmsg.toUtf8().data() );
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
                 .arg(args["OldVersion"].toString()).arg(args["NewVersion"].toString() );
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
        if (d->averageRateTime.isNull()) {
            d->averageRateTime = QDateTime::currentDateTime();
        }
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
    Aqpm::Globals::TransactionQuestion question;

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
    case PM_TRANS_CONV_REMOVE_PKGS:
        {
            alpm_list_t *unresolved = (alpm_list_t *) data1;
            alpm_list_t *i;
            QStringList pkgs;
            for (i = unresolved; i; i = i->next) {
                pkgs.append((char *)alpm_pkg_get_name((pmpkg_t*) (i->data)));
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

int CallBacks::cb_fetch(const char *url, const char *localpath, time_t mtimeold, time_t *mtimenew)
{
    Q_D(CallBacks);

    qDebug() << "fetching: " << url << localpath;

    QNetworkReply *reply;
    QEventLoop re;
    reply = d->manager->head(d->createNetworkRequest(QUrl(url)));
    connect(reply, SIGNAL(finished()), &re, SLOT(quit()));
    re.exec();

    d->currentFile = QString(url).split('/').at(QString(url).split('/').length() - 1);

    qDebug() << "The file is " << d->currentFile;

    // Let's check if the modification times collide. If so, there's no need to download the file

    QDateTime dtime = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
    qDebug() << "Modified on:" << dtime;
    time_t newtime = dtime.toTime_t();
    mtimenew = &newtime;
    reply->deleteLater();

    qDebug() << "Comparing";

    if (*mtimenew == mtimeold) {
        return 1;
    }

    qDebug() << "Compare successful";

    // If we got here, it's time to download the file.
    // We should handle the request synchronously, so get ready for some loops.

    QEventLoop e;

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmdownloader", "/Downloader", "org.chakraproject.aqpmdownloader",
                                         "finished", &e, SLOT(quit()));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmdownloader", "/Downloader", "org.chakraproject.aqpmdownloader",
                                         "downloadProgress", this, SLOT(computeDownloadProgress(int,int)));

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmdownloader",
              "/Downloader",
              "org.chakraproject.aqpmdownloader",
              QLatin1String("download"));
    QList<QVariant> argumentList;

    message << QString(url);
    message << QString(localpath + d->currentFile);
    QDBusMessage mreply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);

    if (mreply.type() == QDBusMessage::ErrorMessage) {
        // Damn this, there was an error
        return 1;
    }

    e.exec();

    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmdownloader", "/Downloader",
                                            "org.chakraproject.aqpmdownloader",
                                            "downloadProgress", this, SLOT(computeDownloadProgress(qint64,qint64)));

    QCoreApplication::processEvents();
    return 0;
}

void CallBacks::computeDownloadProgress(int downloaded, int total)
{
    Q_D(CallBacks);

    if (downloaded == total) {
        d->list_xfered += downloaded;
        return;
    }

    d->list_last = d->list_xfered + downloaded;

    // Update Rate
    int seconds = d->averageRateTime.secsTo(QDateTime::currentDateTime());

    int rate = 0;

    if (seconds != 0 && d->list_last != 0) {
        rate = (int)(d->list_last / seconds);
    }

    // Don't you fucking hog the bus! Update maximum each 0.250 seconds
    if (d->busControlTime.msecsTo(QTime::currentTime()) > 250 || !d->busControlTime.isValid()) {
        emit streamDlProg(d->currentFile, (int)downloaded, (int)total, (int)rate, (int)d->list_last, (int)d->list_total);
        d->busControlTime = QTime::currentTime();
    }
}

void CallBacks::cb_dl_total(off_t total)
{
    Q_D(CallBacks);

    d->list_total = total;
    qDebug() << "total called, offset" << total;
    /* if we get a 0 value, it means this list has finished downloading,
     * so clear out our list_xfered as well */
    if (total == 0) {
        d->list_xfered = 0;
        d->list_last = 0;
        d->list_total = 0;
    }
}

void CallBacks::cb_trans_progress(pmtransprog_t event, const char *pkgname, int percent,
                                  int howmany, int remain)
{
    Q_D(CallBacks);

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

int cb_fetch(const char *url, const char *localpath, time_t mtimeold, time_t *mtimenew)
{
    return CallBacks::instance()->cb_fetch(url, localpath, mtimeold, mtimenew);
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
