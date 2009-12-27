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

#include "BackendThread_p.h"

namespace Aqpm
{

class BackendPrivate
{
    Q_DECLARE_PUBLIC(Backend)
    Backend * const q_ptr;

public:
    Private(Backend *b)
            : q_ptr(b)
            , ready(false)
            , list_total(0)
            , list_xfered(0)
            , list_last(0)
            , averageRateTime() {}

    BackendThread *thread;
    ContainerThread *containerThread;
    QMap<Backend::ActionType, QEvent::Type> events;
    bool ready;

    qint64 list_total;
    qint64 list_xfered;
    qint64 list_last;
    QDateTime averageRateTime;

    // privates
    QEvent::Type getEventTypeFor(Backend::ActionType event) const;
    void startUpDownloader();
    void shutdownDownloader();

    BackendThread *thread();
    void setUpAlpm();

private:
    // Q_PRIVATE_SLOTS
    void __k__backendReady();
    void __k__setUpSelf(BackendThread *t);
    void __k__streamError(int code, const QVariantMap &args);
    void __k__doStreamTransProgress(int event, const QString &pkgname, int percent,
                                    int howmany, int remain);
    void __k__doStreamTransEvent(int event, const QVariantMap &args);
    void __k__doStreamTransQuestion(int event, const QVariantMap &args);
    void __k__computeDownloadProgress(qlonglong downloaded, qlonglong total, const QString& filename);
    void __k__totalOffsetReceived(int offset);
    void __k__operationFinished(bool result);
};

}

#endif
