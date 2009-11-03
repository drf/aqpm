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

#include "Worker_p.h"

#include "aqpmabsworkeradaptor.h"
#include <QDBusConnection>

namespace Aqpm {

namespace AbsWorker {

Worker::Worker(bool temporized, QObject *parent)
        : QObject(parent)
{
    new AqpmabsworkerAdaptor(this);

    if (!QDBusConnection::systemBus().registerService("org.chakraproject.aqpmabsworker")) {
        qDebug() << "another helper is already running";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    if (!QDBusConnection::systemBus().registerObject("/Worker", this)) {
        qDebug() << "unable to register service interface to dbus";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    setIsTemporized(temporized);
    setTimeout(3000);
    startTemporizing();
}

Worker::~Worker()
{
}

void Worker::update(const QStringList &targets)
{
}

void Worker::updateAll()
{
}

bool Worker::prepareBuildEnvironment(const QString &from, const QString &to) const
{
}

}

}

#include "Worker_p.moc"
