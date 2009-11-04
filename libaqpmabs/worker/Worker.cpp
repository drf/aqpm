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
#include <polkit-qt/Auth>

namespace Aqpm
{

namespace AbsWorker
{

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

void Worker::update(const QStringList &targets, bool tarball)
{
    stopTemporizing();

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.updateabs",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit absUpdated(false);
        startTemporizing();
        return;
    }

    m_process = new QProcess(this);
    connect(m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(slotOutputReady()));
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotAbsUpdated(int, QProcess::ExitStatus)));

    if (tarball) {
        m_process->start("abs", QStringList() << targets << "--tarball");
    } else {
        m_process->start("abs", targets);
    }
}

void Worker::updateAll(bool tarball)
{
    stopTemporizing();

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.updateabs",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit absUpdated(false);
        startTemporizing();
        return;
    }

    m_process = new QProcess(this);
    connect(m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(slotOutputReady()));
    connect(m_process, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(slotAbsUpdated(int, QProcess::ExitStatus)));

    if (tarball) {
        m_process->start("abs", QStringList() << "--tarball");
    } else {
        m_process->start("abs");
    }
}

bool Worker::prepareBuildEnvironment(const QString &from, const QString &to) const
{
    stopTemporizing();

    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.preparebuildenvironment",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        startTemporizing();
        return false;
    }

    QDir absPDir(from);
    absPDir.setFilter(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    QFileInfoList Plist = absPDir.entryInfoList();

    for (int i = 0; i < Plist.size(); ++i) {
        QString dest(to);
        if (!dest.endsWith(QChar('/'))) {
            dest.append(QChar('/'));
        }
        dest.append(Plist.at(i).fileName());

        qDebug() << "Copying " << Plist.at(i).absoluteFilePath() << " to " << dest;

        if (!QFile::copy(Plist.at(i).absoluteFilePath(), dest)) {
            startTemporizing();
            return false;
        }
    }

    startTemporizing();
    return true;
}

void Worker::slotAbsUpdated(int code, QProcess::ExitStatus)
{
    if (code == 0) {
        emit absUpdated(true);
    } else {
        emit absUpdated(false);
    }

    m_process->deleteLater();

    startTemporizing();
}

void Worker::slotOutputReady()
{
    emit newOutput(QString::fromLocal8Bit(m_process->readLine(1024)));
}

}

}

#include "Worker_p.moc"
