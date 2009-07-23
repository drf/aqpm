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

#include "Backend.h"

#include <config-aqpm.h>

#include "ConfigurationParser.h"
#include "BackendThread.h"
#include "SynchronousLoop.h"
#include "ActionEvent.h"

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
    QMap<Backend::ActionType, QEvent::Type> events;
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
    qRegisterMetaType<QueueItem::List>();
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
    return AQPM_VERSION;
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

QEvent::Type Backend::getEventTypeFor(ActionType event)
{
    return d->events[event];
}

void Backend::performAsyncAction(ActionType type, const QVariantMap &args)
{
    QCoreApplication::postEvent(d->thread, new ActionEvent(getEventTypeFor(Backend::PerformAction), type, args));
}

bool Backend::testLibrary()
{
    SynchronousLoop s(TestLibrary, QVariantMap());
    return s.result()["retvalue"].toBool();
}

bool Backend::isOnTransaction()
{
    SynchronousLoop s(IsOnTransaction, QVariantMap());
    return s.result()["retvalue"].toBool();
}

bool Backend::reloadPacmanConfiguration()
{
    SynchronousLoop s(ReloadPacmanConfiguration, QVariantMap());
    return s.result()["retvalue"].toBool();
}

void Backend::setUpAlpm()
{
    SynchronousLoop s(SetUpAlpm, QVariantMap());
}

Database::List Backend::getAvailableDatabases() const
{
    SynchronousLoop s(GetAvailableDatabases, QVariantMap());
    return s.result()["retvalue"].value<Database::List>();
}

Database Backend::getLocalDatabase() const
{
    SynchronousLoop s(GetLocalDatabase, QVariantMap());
    return s.result()["retvalue"].value<Database>();
}

Package::List Backend::getInstalledPackages()
{
    SynchronousLoop s(GetInstalledPackages, QVariantMap());
    return s.result()["retvalue"].value<Package::List>();
}

Package::List Backend::getUpgradeablePackages()
{
    SynchronousLoop s(GetUpgradeablePackages, QVariantMap());
    return s.result()["retvalue"].value<Package::List>();
}

Group::List Backend::getAvailableGroups()
{
    SynchronousLoop s(GetAvailableGroups, QVariantMap());
    return s.result()["retvalue"].value<Group::List>();
}

Package::List Backend::getPackageDependencies(const Package &package)
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(package);
    SynchronousLoop s(GetPackageDependencies, args);
    return s.result()["retvalue"].value<Package::List>();
}

Package::List Backend::getDependenciesOnPackage(const Package &package)
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(package);
    SynchronousLoop s(GetDependenciesOnPackage, args);
    return s.result()["retvalue"].value<Package::List>();
}

bool Backend::isInstalled(const Package &package)
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(package);
    SynchronousLoop s(IsInstalled, args);
    return s.result()["retvalue"].toBool();
}

QStringList Backend::getPackageFiles(const Package &package)
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(package);
    SynchronousLoop s(GetPackageFiles, args);
    return s.result()["retvalue"].toStringList();
}

QStringList Backend::getProviders(const Package &package)
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(package);
    SynchronousLoop s(GetProviders, args);
    return s.result()["retvalue"].toStringList();
}

bool Backend::isProviderInstalled(const QString &provider)
{
    QVariantMap args;
    args["provider"] = QVariant::fromValue(provider);
    SynchronousLoop s(IsProviderInstalled, args);
    return s.result()["retvalue"].toBool();
}

QString Backend::getAlpmVersion()
{
    SynchronousLoop s(GetAlpmVersion, QVariantMap());
    return s.result()["retvalue"].toString();
}

Database Backend::getPackageDatabase(const Package &package, bool checkver) const
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(package);
    args["checkver"] = QVariant::fromValue(checkver);
    SynchronousLoop s(GetPackageDatabase, args);
    return s.result()["retvalue"].value<Database>();
}

Package::List Backend::getPackagesFromGroup(const Group &group)
{
    QVariantMap args;
    args["group"] = QVariant::fromValue(group);
    SynchronousLoop s(GetPackagesFromGroup, args);
    return s.result()["retvalue"].value<Package::List>();
}

Package::List Backend::getPackagesFromDatabase(const Database &db)
{
    QVariantMap args;
    args["db"] = QVariant::fromValue(db);
    SynchronousLoop s(GetPackagesFromDatabase, args);
    return s.result()["retvalue"].value<Package::List>();
}

int Backend::countPackages(Globals::PackageStatus status)
{
    QVariantMap args;
    args["status"] = QVariant::fromValue((int)status);
    SynchronousLoop s(CountPackages, args);
    return s.result()["retvalue"].toInt();
}

QStringList Backend::alpmListToStringList(alpm_list_t *list)
{
    QVariantMap args;
    args["list"] = QVariant::fromValue(list);
    SynchronousLoop s(AlpmListToStringList, args);
    return s.result()["retvalue"].toStringList();
}

Group::List Backend::getPackageGroups(const Package &package)
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(package);
    SynchronousLoop s(GetPackageGroups, args);
    return s.result()["retvalue"].value<Group::List>();
}

Package Backend::getPackage(const QString &name, const QString &db) const
{
    QVariantMap args;
    args["name"] = QVariant::fromValue(name);
    args["db"] = QVariant::fromValue(db);
    SynchronousLoop s(GetPackage, args);
    return s.result()["retvalue"].value<Package>();
}

Group Backend::getGroup(const QString &name) const
{
    QVariantMap args;
    args["name"] = QVariant::fromValue(name);
    SynchronousLoop s(GetGroup, args);
    return s.result()["retvalue"].value<Group>();
}

Database Backend::getDatabase(const QString &name) const
{
    QVariantMap args;
    args["name"] = QVariant::fromValue(name);
    SynchronousLoop s(GetDatabase, args);
    return s.result()["retvalue"].value<Database>();
}

bool Backend::updateDatabase()
{
    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(UpdateDatabase)));
    qDebug() << "Thread is running";
    return true;
}

void Backend::fullSystemUpgrade(const QList<pmtransflag_t> &flags, bool downgrade)
{
    QVariantMap args;
    args["flags"] = QVariant::fromValue(flags);
    args["downgrade"] = downgrade;
    SynchronousLoop s(SetFlags, args);
    QCoreApplication::postEvent(d->thread,
                                new ActionEvent(getEventTypeFor(SystemUpgrade), SystemUpgrade, args));
    qDebug() << "Thread is running";
}

void Backend::clearQueue()
{
    SynchronousLoop s(ClearQueue, QVariantMap());
}

void Backend::addItemToQueue(const QString &name, QueueItem::Action action)
{
    QVariantMap args;
    args["name"] = QVariant::fromValue(name);
    args["action"] = QVariant::fromValue(action);
    SynchronousLoop s(AddItemToQueue, args);
}

QueueItem::List Backend::queue() const
{
    SynchronousLoop s(GetQueue, QVariantMap());
    return s.result()["retvalue"].value<QueueItem::List>();
}

void Backend::processQueue(const QList<pmtransflag_t> &flags)
{
    QVariantMap args;
    args["flags"] = QVariant::fromValue(flags);
    SynchronousLoop s(SetFlags, args);
    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(ProcessQueue)));
    qDebug() << "Thread is running";
}

void Backend::setShouldHandleAuthorization(bool should)
{
    QVariantMap args;
    args["should"] = QVariant::fromValue(should);
    SynchronousLoop s(SetShouldHandleAuthorization, args);
}

bool Backend::shouldHandleAuthorization() const
{
    SynchronousLoop s(ShouldHandleAuthorization, QVariantMap());
    return s.result()["retvalue"].toBool();
}

void Backend::setAnswer(int answer)
{
    QVariantMap args;
    args["answer"] = QVariant::fromValue(answer);
    SynchronousLoop s(SetAnswer, args);
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
