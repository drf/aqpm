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

#ifndef W_CALLBACKS_P_H
#define W_CALLBACKS_P_H

#include <iostream>
#include <alpm.h>
#include <sys/time.h>
#include <sys/types.h>

#include <QObject>
#include <QVariantMap>
#include <QEventLoop>

namespace AqpmWorker
{

class CallBacksPrivate;
class Worker;

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

    void setWorker(Worker *worker);

public Q_SLOTS:
    void setAnswer(int answer);

signals:
    void streamTransEvent(int event, QVariantMap args);
    void streamTransQuestion(int event, QVariantMap args);
    void streamTransProgress(int event, const QString &pkgname, int percent,
                             int howmany, int remain);
    void streamTotalOffset(int offset);
    void logMessageStreamed(const QString &msg);

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

class ReturnStringConditionalEventLoop : public QEventLoop
{
    Q_OBJECT

public:
    explicit ReturnStringConditionalEventLoop(const QString &str, QObject *parent = 0) : m_cond(str) {}
    virtual ~ReturnStringConditionalEventLoop() {}

    QString result() const;

public Q_SLOTS:
    void requestQuit(const QString &str, const QString &result);

private:
    QString m_cond;
    QString m_result;
};

}

Q_DECLARE_METATYPE(pmtransevt_t)
Q_DECLARE_METATYPE(void*)
Q_DECLARE_METATYPE(char*)
Q_DECLARE_METATYPE(off_t)
Q_DECLARE_METATYPE(pmtransprog_t)
Q_DECLARE_METATYPE(pmloglevel_t)

#endif /*W_CALLBACKS_P_H*/
