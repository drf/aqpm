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

#ifndef WORKER_P_H
#define WORKER_P_H

#include <QtCore/QObject>
#include <QDBusContext>
#include "misc/TemporizedApplication_p.h"

#include <QProcess>

namespace Aqpm {

namespace AbsWorker {

class Worker : public QObject, protected QDBusContext, private TemporizedApplication
{
    Q_OBJECT

    public:
        explicit Worker(bool temporized, QObject* parent = 0);
        virtual ~Worker();

    public Q_SLOTS:
        void update(const QStringList &targets, bool tarball);
        void updateAll(bool tarball);
        bool prepareBuildEnvironment(const QString &from, const QString &to) const;
        void slotOutputReady();
        void slotAbsUpdated(int,QProcess::ExitStatus);

    Q_SIGNALS:
        void absUpdated(bool);
        void newOutput(QString);

    private:
        QProcess *m_process;
};

}

}

#endif // WORKER_P_H
