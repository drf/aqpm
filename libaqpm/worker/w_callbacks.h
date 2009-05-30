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

#ifndef W_CALLBACKS_H
#define W_CALLBACKS_H

#include <iostream>
#include <alpm.h>
#include <sys/time.h>
#include <sys/types.h>

#include <QObject>
#include <QVariantMap>

namespace AqpmWorker
{

class CallBacksPrivate;

class CallBacks : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(CallBacks)
    Q_DECLARE_PRIVATE(CallBacks)

public:

    static CallBacks *instance();

    CallBacks(QObject *parent = 0);
    virtual ~CallBacks();
    static float get_update_timediff(int first_call);
    void cb_trans_evt(pmtransevt_t event, void *data1, void *data2);
    void cb_trans_conv(pmtransconv_t event, void *data1, void *data2,
                       void *data3, int *response);
    void cb_trans_progress(pmtransprog_t event, const char *pkgname, int percent,
                           int howmany, int remain);
    void cb_dl_total(off_t total);
    void cb_log(pmloglevel_t level, char *fmt, va_list args);
    int cb_fetch(const char *url, const char *localpath, time_t mtimeold, time_t *mtimenew);

public Q_SLOTS:
    void setAnswer(int answer);

private Q_SLOTS:
    void computeDownloadProgress(qint64 downloaded, qint64 total);

signals:
    void streamTransEvent(int event, QVariantMap args);
    void streamTransQuestion(int event, void *data1, void *data2,
                             void *data3);
    void streamTransProgress(int event, const QString &pkgname, int percent,
                             int howmany, int remain);
    void streamDlProg(const QString &pkg, int package_done, int package_total,
                          int rate, int list_done, int list_total);
    void questionStreamed(const QString &msg);
    void logMsgStreamed(const QString &msg);

    // Internal use
    void answerReady();

private:
    CallBacksPrivate *d_ptr;
};

void cb_trans_evt(pmtransevt_t event, void *data1, void *data2);
void cb_trans_conv(pmtransconv_t event, void *data1, void *data2,
                   void *data3, int *response);
void cb_dl_total(off_t total);
void cb_trans_progress(pmtransprog_t event, const char *pkgname, int percent,
                       int howmany, int remain);
void cb_log(pmloglevel_t level, char *fmt, va_list args);
int cb_fetch(const char *url, const char *localpath, time_t mtimeold, time_t *mtimenew);
int pm_vasprintf(char **string, pmloglevel_t level, const char *format, va_list args);

}


#endif /*W_CALLBACKS_H*/
