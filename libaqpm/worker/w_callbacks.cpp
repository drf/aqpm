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

#include "w_callbacks.h"

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <QWaitCondition>
#include <QDebug>
#include <QGlobalStatic>
#include <QDateTime>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkProxy>

#include "alpm.h"

#define UPDATE_SPEED_SEC 0.2f

namespace AqpmWorker
{

class CallBacksPrivate
{
public:
    CallBacksPrivate();

    QNetworkRequest createNetworkRequest(const QUrl &url);

    Q_DECLARE_PUBLIC(CallBacks)
    CallBacks *q_ptr;

    float rate_last;
    int xfered_last;
    float rate_total;
    int xfered_total;
    float list_total;
    float list_xfered;
    float last_file_xfered;
    int onDl;
    struct timeval initial_time;
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
 : onDl(1)
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
}

CallBacks::CallBacks(QObject *parent)
        : QObject(parent)
        , d_ptr(new CallBacksPrivate())
{
    Q_ASSERT(!s_globalCallbacks()->q);
    s_globalCallbacks()->q = this;
    d_ptr->q_ptr = this;
    qDebug() << "Constructing callbacks";
    answer = -1;
}

CallBacks::~CallBacks()
{
    delete d_ptr;
}

float CallBacks::get_update_timediff(int first_call)
{
    float retval = 0.0;
    static struct timeval last_time = {0, 0};

    /* on first call, simply set the last time and return */
    if (first_call) {
        gettimeofday(&last_time, NULL);
    } else {
        struct timeval this_time;
        float diff_sec, diff_usec;

        gettimeofday(&this_time, NULL);
        diff_sec = this_time.tv_sec - last_time.tv_sec;
        diff_usec = this_time.tv_usec - last_time.tv_usec;

        retval = diff_sec + (diff_usec / 1000000.0);

        /* return 0 and do not update last_time if interval was too short */
        if (retval < UPDATE_SPEED_SEC) {
            retval = 0.0;
        } else {
            last_time = this_time;
            /* printf("\nupdate retval: %f\n", retval); DEBUG*/
        }
    }

    return(retval);
}

void CallBacks::cb_trans_evt(pmtransevt_t event, void *data1, void *data2)
{
    emit streamTransEvent(event, data1, data2);
    qDebug() << "Alpm Thread Waiting.";
    qDebug() << "Alpm Thread awake.";
}

void CallBacks::cb_trans_conv(pmtransconv_t event, void *data1, void *data2,
                              void *data3, int *response)
{
    qDebug() << "Alpm is asking a question.";

    QString message;

    switch (event) {
    case PM_TRANS_CONV_INSTALL_IGNOREPKG:
        if (data2)
            /* TODO we take this route based on data2 being not null? WTF */
            message = QString(tr("%1 requires installing %2 from IgnorePkg/IgnoreGroup.\n Install anyway?")).arg(alpm_pkg_get_name((pmpkg_t *)data1)).
                      arg(alpm_pkg_get_name((pmpkg_t *)data2));
        else
            message = QString(tr("%1 is in IgnorePkg/IgnoreGroup.\n Install anyway?")).arg(alpm_pkg_get_name((pmpkg_t *)data1));

        break;
    case PM_TRANS_CONV_REMOVE_HOLDPKG:
        message = QString(tr("%1 is designated as a HoldPkg.\n Remove anyway?")).arg(alpm_pkg_get_name((pmpkg_t *)data1));
        break;
    case PM_TRANS_CONV_REPLACE_PKG:
        message = QString(tr("Replace %1 with %2/%3?")).arg(alpm_pkg_get_name((pmpkg_t *)data1)).
                  arg((char *)data3).arg(alpm_pkg_get_name((pmpkg_t *)data2));
        break;
    case PM_TRANS_CONV_CONFLICT_PKG:
        message = QString(tr("%1 conflicts with %2.\nRemove %3?")).arg((char *)data1).
                  arg((char *)data2).arg((char *)data2);
        break;
    case PM_TRANS_CONV_LOCAL_NEWER:
        message = QString(tr("%1-%2: local version is newer.\nUpgrade anyway?")).arg(alpm_pkg_get_name((pmpkg_t *)data1)).
                  arg(alpm_pkg_get_version((pmpkg_t *)data1));
        break;
    case PM_TRANS_CONV_CORRUPTED_PKG:
        message = QString(tr("File %1 is corrupted.\nDo you want to delete it?")).arg((char *)data1);
        break;
    }

    emit questionStreamed(message);

    qDebug() << "Question Streamed, Alpm Thread waiting.";

    qDebug() << "Alpm Thread awake, committing answer.";

    *response = answer;

    return;

}

int CallBacks::cb_fetch(const char *url, const char *localpath, time_t mtimeold, time_t *mtimenew)
{
    Q_D(CallBacks);

    QNetworkRequest request = d->createNetworkRequest(QUrl(url));
    QNetworkReply *reply = d->manager->head(request);

    // Let's check if the modification times collide. If so, there's no need to download the file

    while (!reply->header(QNetworkRequest::LastModifiedHeader).isValid()) {
        reply->waitForBytesWritten(200);
    }

    QDateTime dtime = reply->header(QNetworkRequest::LastModifiedHeader).toDateTime();
    *mtimenew = dtime.toTime_t();
    if (*mtimenew == mtimeold) {
        return 1;
    }

}

void CallBacks::cb_trans_progress(pmtransprog_t event, const char *pkgname, int percent,
                                  int howmany, int remain)
{
    Q_D(CallBacks);
    float timediff = 0.0;

    if (percent == 0) {
        gettimeofday(&d->initial_time, NULL);
        timediff = get_update_timediff(1);
    } else {
        timediff = get_update_timediff(0);

        if (timediff < UPDATE_SPEED_SEC)
            return;
    }

    if (percent > 0 && percent < 100 && !timediff)
        return;

    qDebug() << "Streaming trans progress";

    emit streamTransProgress((int)event, QString(pkgname), percent, howmany, remain);
}



void CallBacks::cb_log(pmloglevel_t level, char *fmt, va_list args)
{
    if (!strlen(fmt)) {
        return;
    }

    char *string = NULL;
    if (pm_vasprintf(&string, level, fmt, args) == -1)
        return;

    if (string != NULL) {
        QString msg = QString::fromLocal8Bit(string);
        emit logMsgStreamed(msg);
    }
}

/* Now the real suckness is coming... */

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
    qDebug() << "Question Event Triggered";
    CallBacks::instance()->cb_trans_conv(event, data1, data2, data3, response);
}

void cb_trans_evt(pmtransevt_t event, void *data1, void *data2)
{
    qDebug() << "Received Event Callback";
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
