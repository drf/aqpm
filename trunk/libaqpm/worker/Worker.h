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

#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QVariantList>

#include <alpm.h>

namespace AqpmWorker {

class Worker : public QObject
{
    Q_OBJECT

    public:
        Worker(QObject *parent = 0);
        virtual ~Worker();

    public Q_SLOTS:
        void processQueue(QVariantList packages, QVariantList flags);
        void updateDatabase();

    private:
        void setUpAlpm();

    Q_SIGNALS:
        void workerReady();
        void workerFailure();
        void workerSuccess();

        void dbStatusChanged(const QString &repo, int action);
        void dbQty(const QStringList &db);

        void streamTransDlProg(char *c, int bytedone, int bytetotal, int speed,
                int listdone, int listtotal, int speedtotal);

        void streamTransProgress(pmtransprog_t event, char *pkgname, int percent,
                int howmany, int remain);

        void streamTransEvent(pmtransevt_t event, void *data1, void *data2);

    private:
        class Private;
        Private *d;
};

}

#endif /* WORKER_H */
