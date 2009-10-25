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

#ifndef BACKEND_P_H
#define BACKEND_P_H

#include "Backend.h"

#include "BackendThread.h"

namespace Aqpm {

class Backend::Private
{
public:
    Private()   : ready(false)
                , list_total(0)
                , list_xfered(0)
                , list_last(0)
                , averageRateTime() {}

    Backend *q;
    BackendThread *thread;
    ContainerThread *containerThread;
    //ContainerThread *downloaderThread;
    QMap<Backend::ActionType, QEvent::Type> events;
    bool ready;

    qint64 list_total;
    qint64 list_xfered;
    qint64 list_last;
    QDateTime averageRateTime;

    // privates
    QEvent::Type getEventTypeFor(ActionType event) const;

    // Q_PRIVATE_SLOTS
    void __k__backendReady();
    void __k__setUpSelf(BackendThread *t);
    void __k__streamError(int code, const QVariantMap &args);
    void __k__doStreamTransProgress(int event, const QString &pkgname, int percent,
                             int howmany, int remain);
    void __k__doStreamTransEvent(int event, const QVariantMap &args);
    void __k__doStreamTransQuestion(int event, const QVariantMap &args);
    void __k__computeDownloadProgress(qint64 downloaded, qint64 total, const QString &filename);
    void __k__totalOffsetReceived(int offset);
};

}

#endif
