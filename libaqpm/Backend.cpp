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

#include "Backend.h"

#include "ConfigurationParser.h"
#include "BackendThread.h"
#include "SynchronousLoop.h"

#include <QMetaType>
#include <QDebug>
#include <QCoreApplication>
#include <QDBusMetaType>
#include <QGlobalStatic>

namespace Aqpm
{

class Backend::Private
{
public:
    Private() {}

    BackendThread *thread;
    ContainerThread *containerThread;
    QMap<Backend::BackendEvent, QEvent::Type> events;
};

class BackendHelper
{
public:
    BackendHelper() : q(0) {}
    ~BackendHelper() {
        delete q;
    }
    Backend *q;
};

Q_GLOBAL_STATIC(BackendHelper, s_globalBackend)

Backend *Backend::instance()
{
    if (!s_globalBackend()->q) {
        new Backend;
    }

    return s_globalBackend()->q;
}

Backend::Backend()
        : QObject(),
        d(new Private())
{
    Q_ASSERT(!s_globalBackend()->q);
    s_globalBackend()->q = this;

    qRegisterMetaType<QueueItem>();
    qDBusRegisterMetaType<QueueItem>();
    qRegisterMetaType<Package>();
    qRegisterMetaType<Group>();
    qRegisterMetaType<Database>();
    qRegisterMetaType<Package::List>();
    qRegisterMetaType<Group::List>();
    qRegisterMetaType<Database::List>();

    qDebug() << "Constructing Backend singleton";

    d->events[Initialization] = (QEvent::Type)QEvent::registerEventType();
    d->events[UpdateDatabase] = (QEvent::Type)QEvent::registerEventType();
    d->events[ProcessQueue] = (QEvent::Type)QEvent::registerEventType();
    d->events[SystemUpgrade] = (QEvent::Type)QEvent::registerEventType();
    d->events[PerformAction] = (QEvent::Type)QEvent::registerEventType();

    qDebug() << d->events;

    d->containerThread = new ContainerThread();
    connect(d->containerThread, SIGNAL(threadCreated(BackendThread*)), SLOT(setUpSelf(BackendThread*)));
    d->containerThread->start();
}

Backend::~Backend()
{
    d->containerThread->quit();
    d->containerThread->deleteLater();
    delete d;
}

QString Backend::version()
{
    //TODO
    return "1.0.0";
}

void Backend::setUpSelf(BackendThread *t)
{
    d->thread = t;

    connect(d->thread, SIGNAL(dbQty(const QStringList&)),
            this, SIGNAL(dbQty(const QStringList&)), Qt::QueuedConnection);
    connect(d->thread, SIGNAL(dbStatusChanged(const QString&, int)),
            this, SIGNAL(dbStatusChanged(const QString&, int)), Qt::QueuedConnection);
    connect(d->thread, SIGNAL(transactionStarted()),
            this, SIGNAL(transactionStarted()));
    connect(d->thread, SIGNAL(transactionReleased()),
            this, SIGNAL(transactionReleased()));
    connect(d->thread, SIGNAL(operationFinished(bool)),
            this, SIGNAL(operationFinished(bool)));
    connect(d->thread, SIGNAL(threadInitialized()),
            this, SIGNAL(backendReady()));
    connect(d->thread, SIGNAL(streamDlProg(const QString&, int, int, int, int, int)),
            this, SIGNAL(streamDlProg(const QString&, int, int, int, int, int)));
    connect(d->thread, SIGNAL(streamTransProgress(int, const QString&, int, int, int)),
            this, SLOT(doStreamTransProgress(int, const QString&, int, int, int)));
    connect(d->thread, SIGNAL(streamTransEvent(int, QVariantMap)),
            this, SLOT(doStreamTransEvent(int, QVariantMap)));
    connect(d->thread, SIGNAL(errorOccurred(int,QVariantMap)),
            this, SLOT(streamError(int,QVariantMap)));
    connect(d->thread, SIGNAL(logMessageStreamed(QString)),
            this, SIGNAL(logMessageStreamed(QString)));
    connect(d->thread, SIGNAL(streamTransQuestion(int, QVariantMap)),
            this, SLOT(doStreamTransQuestion(int, QVariantMap)));

    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(Initialization)));
}

QEvent::Type Backend::getEventTypeFor(BackendEvent event)
{
    return d->events[event];
}

bool Backend::testLibrary()
{
    return d->thread->testLibrary();
}

bool Backend::isOnTransaction()
{
    return d->thread->isOnTransaction();
}

bool Backend::reloadPacmanConfiguration()
{
    return d->thread->reloadPacmanConfiguration();
}

void Backend::setUpAlpm()
{
    d->thread->setUpAlpm();
}

Database::List Backend::getAvailableDatabases() const
{
    SynchronousLoop s(GetAvailableDatabases, QVariantMap());
    return s.result()["retvalue"].value<Database::List>();
}

Package::List Backend::getInstalledPackages()
{
    return d->thread->getInstalledPackages();
}

Package::List Backend::getUpgradeablePackages()
{
    return d->thread->getUpgradeablePackages();
}

Group::List Backend::getAvailableGroups()
{
    return d->thread->getAvailableGroups();
}

Package::List Backend::getPackageDependencies(const Package &package)
{
    return d->thread->getPackageDependencies(package);
}

Package::List Backend::getDependenciesOnPackage(const Package &package)
{
    return d->thread->getDependenciesOnPackage(package);
}

bool Backend::isInstalled(const Package &package)
{
    return d->thread->isInstalled(package);
}

QStringList Backend::getPackageFiles(const Package &package)
{
    return d->thread->getPackageFiles(package);
}

QStringList Backend::getProviders(const Package &package)
{
    return d->thread->getProviders(package);
}

bool Backend::isProviderInstalled(const QString &provider)
{
    return d->thread->isProviderInstalled(provider);
}

QString Backend::getAlpmVersion()
{
    return d->thread->getAlpmVersion();
}

Database Backend::getPackageDatabase(const Package &package, bool checkver) const
{
    return d->thread->getPackageDatabase(package, checkver);
}

Package::List Backend::getPackagesFromGroup(const Group &group)
{
    return d->thread->getPackagesFromGroup(group);
}

Package::List Backend::getPackagesFromDatabase(const Database &db)
{
    return d->thread->getPackagesFromDatabase(db);
}

int Backend::countPackages(Globals::PackageStatus status)
{
    return d->thread->countPackages(status);
}

QStringList Backend::alpmListToStringList(alpm_list_t *list)
{
    return d->thread->alpmListToStringList(list);
}

Group::List Backend::getPackageGroups(const Package &package)
{
    return d->thread->getPackageGroups(package);
}

Package Backend::getPackage(const QString &name, const QString &db) const
{
    return d->thread->getPackage(name, db);
}

bool Backend::updateDatabase()
{
    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(UpdateDatabase)));
    qDebug() << "Thread is running";
    return true;
}

void Backend::fullSystemUpgrade(const QList<pmtransflag_t> &flags)
{
    d->thread->setFlags(flags);
    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(SystemUpgrade)));
    qDebug() << "Thread is running";
}

void Backend::clearQueue()
{
    d->thread->clearQueue();
}

void Backend::addItemToQueue(const QString &name, QueueItem::Action action)
{
    d->thread->addItemToQueue(name, action);
}

QueueItem::List Backend::queue()
{
    return d->thread->queue();
}

void Backend::processQueue(const QList<pmtransflag_t> &flags)
{
    d->thread->setFlags(flags);
    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(ProcessQueue)));
    qDebug() << "Thread is running";
}

void Backend::setShouldHandleAuthorization(bool should)
{
    d->thread->setShouldHandleAuthorization(should);
}

bool Backend::shouldHandleAuthorization() const
{
    return d->thread->shouldHandleAuthorization();
}

void Backend::setAnswer(int answer)
{
    d->thread->setAnswer(answer);
}

void Backend::streamError(int code, const QVariantMap &args)
{
    emit errorOccurred((Globals::Errors) code, args);
}

void Backend::doStreamTransProgress(int event, const QString &pkgname, int percent,
                           int howmany, int remain)
{
    emit streamTransProgress((Aqpm::Globals::TransactionProgress)event, pkgname, percent, howmany, remain);
}

void Backend::doStreamTransEvent(int event, const QVariantMap &args)
{
    emit streamTransEvent((Aqpm::Globals::TransactionEvent) event, args);
}

void Backend::doStreamTransQuestion(int event, const QVariantMap &args)
{
    emit streamTransQuestion((Aqpm::Globals::TransactionQuestion) event, args);
}

BackendThread *Backend::getInnerThread()
{
    return d->thread;
}

}
