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

#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <iostream>
#include <alpm.h>
#include <sys/time.h>

#include <QObject>

namespace Aqpm {

class CallBacks : public QObject
{
    /* I know this sucks, but it seems the only way. We have to declare
     * callback functions as C functions, aka outside a class. But we
     * obviously have some hard times with intercommunication. So...
     * here's the nastiest thing ever.
     */

    Q_OBJECT

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
    void cb_dl_progress(const char *filename, off_t file_xfered, off_t file_total);
    void cb_log(pmloglevel_t level, char *fmt, va_list args);

signals:
    void streamTransEvent(pmtransevt_t event, void *data1, void *data2);
    void streamTransQuestion(pmtransconv_t event, void *data1, void *data2,
                             void *data3);
    void streamTransProgress(pmtransprog_t event, char *pkgname, int percent,
                             int howmany, int remain);
    void streamTransDlProg(char *filename, int file_x, int file_t, int spd_f,
                           int list_x, int list_t, int spd_l);
    void questionStreamed(const QString &msg);
    void streamTransDlProg(const QString &filename, int singlePercent, int singleSpeed,
                           int totalPercent, int totalSpeed);
    void logMsgStreamed(const QString &msg);

public:
    int answer;

private:
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

void cb_trans_evt(pmtransevt_t event, void *data1, void *data2);
void cb_trans_conv(pmtransconv_t event, void *data1, void *data2,
                   void *data3, int *response);
void cb_trans_progress(pmtransprog_t event, const char *pkgname, int percent,
                       int howmany, int remain);
void cb_dl_total(off_t total);
/* callback to handle display of download progress */
void cb_dl_progress(const char *filename, off_t file_xfered, off_t file_total);
void cb_log(pmloglevel_t level, char *fmt, va_list args);
int pm_vasprintf(char **string, pmloglevel_t level, const char *format, va_list args);

}


#endif /*CALLBACKS_H*/
