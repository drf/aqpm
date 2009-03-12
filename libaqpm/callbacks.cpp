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

#include "callbacks.h"

#include "Backend.h"

#include <iostream>
#include <string>
#include <unistd.h>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QDebug>
#include <QGlobalStatic>

#include "alpm.h"

#define UPDATE_SPEED_SEC 0.2f

namespace Aqpm
{

class CallBacks::Private
{
public:
    Private() : onDl(1) {};

    float rate_last;
    int xfered_last;
    float rate_total;
    int xfered_total;
    float list_total;
    float list_xfered;
    float last_file_xfered;
    int onDl;
    struct timeval initial_time;
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

CallBacks::CallBacks(QObject *parent)
        : QObject(parent),
        d(new Private())
{
    Q_ASSERT(!s_globalCallbacks()->q);
    s_globalCallbacks()->q = this;
    qDebug() << "Constructing callbacks";
    answer = -1;
}

CallBacks::~CallBacks()
{
    delete d;
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
    QMutexLocker lock(Backend::instance()->backendMutex());
    emit streamTransEvent(event, data1, data2);
    qDebug() << "Alpm Thread Waiting.";
    Backend::instance()->backendWCond()->wait(Backend::instance()->backendMutex());
    qDebug() << "Alpm Thread awake.";
}

void CallBacks::cb_trans_conv(pmtransconv_t event, void *data1, void *data2,
                              void *data3, int *response)
{
    qDebug() << "Alpm is asking a question.";

    QMutexLocker lock(Backend::instance()->backendMutex());
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

    Backend::instance()->backendWCond()->wait(Backend::instance()->backendMutex());

    qDebug() << "Alpm Thread awake, committing answer.";

    *response = answer;

    return;

}

void CallBacks::cb_trans_progress(pmtransprog_t event, const char *pkgname, int percent,
                                  int howmany, int remain)
{
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

    emit streamTransProgress(event, (char *)pkgname, percent, howmany, remain);
}

void CallBacks::cb_dl_total(off_t total)
{
    d->list_total = total;
    qDebug() << "total called, offset" << total;
    /* if we get a 0 value, it means this list has finished downloading,
     * so clear out our list_xfered as well */
    if (total == 0) {
        d->list_xfered = 0;
    }
}

/* callback to handle display of download progress */
void CallBacks::cb_dl_progress(const char *filename, off_t file_xfered, off_t file_total)
{
    off_t xfered, total;
    float rate = 0.0, timediff = 0.0;

    /* only use TotalDownload if enabled and we have a callback value */
    if (d->list_total) {
        xfered = d->list_xfered + file_xfered;
        total = d->list_total;
    } else {
        xfered = file_xfered;
        total = file_total;
    }

    /* this is basically a switch on xfered: 0, total, and
     * anything else */
    if (file_xfered == 0) {
        /* set default starting values, ensure we only call this once
         * if TotalDownload is enabled */
        if (d->list_xfered == 0) {
            gettimeofday(&d->initial_time, NULL);
            d->xfered_last = (off_t)0;
            d->rate_last = 0.0;
            timediff = get_update_timediff(1);
        }
    } else if (file_xfered == file_total) {
        /* compute final values */
        struct timeval current_time;
        float diff_sec, diff_usec;

        gettimeofday(&current_time, NULL);
        diff_sec = current_time.tv_sec - d->initial_time.tv_sec;
        diff_usec = current_time.tv_usec - d->initial_time.tv_usec;
        timediff = diff_sec + (diff_usec / 1000000.0);
        rate = xfered / (timediff * 1024.0);
    } else {
        /* compute current average values */
        timediff = get_update_timediff(0);

        if (timediff < UPDATE_SPEED_SEC) {
            /* return if the calling interval was too short */
            return;
        }
        rate = (xfered - d->xfered_last) / (timediff * 1024.0);
        /* average rate to reduce jumpiness */
        rate = (rate + 2 * d->rate_last) / 3;
        d->rate_last = rate;
        d->xfered_last = xfered;
    }

    if (d->last_file_xfered > file_xfered) {
        d->last_file_xfered = 0;
    }

    d->list_xfered += file_xfered - d->last_file_xfered;
    d->last_file_xfered = file_xfered;

    qDebug() << "Emitting progress" << file_xfered << file_total;
    emit streamTransDlProg((char *)filename, (int)file_xfered, (int)file_total, (int)rate,
                           d->list_xfered, d->list_total, (int)rate);
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

void cb_dl_total(off_t total)
{
    CallBacks::instance()->cb_dl_total(total);
}

void cb_dl_progress(const char *filename, off_t file_xfered, off_t file_total)
{
    CallBacks::instance()->cb_dl_progress(filename, file_xfered, file_total);
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
