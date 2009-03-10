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

#include "callbacks.h"
#include "ConfigurationParser.h"
#include "BackendThread.h"

#include <QMetaType>
#include <QDebug>
#include <QCoreApplication>

#include "misc/Singleton.h"

namespace Aqpm
{

class Backend::Private
{
public:

    Private() : mutex(new QMutex()), wCond(new QWaitCondition()) {};

    QMutex *mutex;
    QWaitCondition *wCond;

    BackendThread *thread;
    ContainerThread *containerThread;
    QMap<Backend::BackendEvents, QEvent::Type> events;
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

AQPM_GLOBAL_STATIC(BackendHelper, s_globalBackend)

Backend *Backend::instance()
{
    if (!s_globalBackend->q) {
        new Backend;
    }

    return s_globalBackend->q;
}

Backend::Backend()
        : QObject(),
        d(new Private())
{
    Q_ASSERT(!s_globalBackend->q);
    s_globalBackend->q = this;

    qDebug() << "Construction Backend singleton";

    qRegisterMetaType<pmtransevt_t>("pmtransevt_t");
    qRegisterMetaType<pmtransprog_t>("pmtransprog_t");

    d->events[Initialization] = (QEvent::Type)QEvent::registerEventType();
    d->events[UpdateDatabase] = (QEvent::Type)QEvent::registerEventType();
    d->events[ProcessQueue] = (QEvent::Type)QEvent::registerEventType();

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

void Backend::setUpSelf(BackendThread *t)
{
    d->thread = t;

    connect(d->thread, SIGNAL(dbQty(const QStringList&)),
            this, SIGNAL(dbQty(const QStringList&)), Qt::QueuedConnection);
    connect(d->thread, SIGNAL(dbStatusChanged(const QString&,int)),
            this, SIGNAL(dbStatusChanged(const QString&,int)), Qt::QueuedConnection);
    connect(d->thread, SIGNAL(transactionStarted()),
            this, SIGNAL(transactionStarted()));
    connect(d->thread, SIGNAL(transactionReleased()),
            this, SIGNAL(transactionReleased()));
    connect(d->thread, SIGNAL(errorOccurred(int)),
            this, SIGNAL(errorOccurred(int)));
    connect(d->thread, SIGNAL(operationFinished(bool)),
            this, SIGNAL(operationFinished(bool)));
    connect(d->thread, SIGNAL(threadInitialized()),
            this, SLOT(connectCallbacks()));

    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(Initialization)));
}

QEvent::Type Backend::getEventTypeFor(BackendEvents event)
{
    return d->events[event];
}

void Backend::connectCallbacks()
{
    connect(CallBacks::instance(), SIGNAL(streamTransDlProg(char*, int, int, int, int, int, int)),
            SIGNAL(streamTransDlProg(char*, int, int, int, int, int, int)));
    connect(CallBacks::instance(), SIGNAL(streamTransProgress(pmtransprog_t, char*, int, int, int)),
            SIGNAL(streamTransProgress(pmtransprog_t, char*, int, int, int)));
    connect(CallBacks::instance(), SIGNAL(streamTransEvent(pmtransevt_t, void*, void*)),
            SIGNAL(streamTransEvent(pmtransevt_t, void*, void*)));

    emit backendReady();
}

QMutex *Backend::backendMutex()
{
    return d->mutex;
}

QWaitCondition *Backend::backendWCond()
{
    return d->wCond;
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

alpm_list_t *Backend::getAvailableRepos()
{
    return d->thread->getAvailableRepos();
}

QStringList Backend::getAvailableReposAsStringList()
{
    return d->thread->getAvailableReposAsStringList();
}

alpm_list_t *Backend::getInstalledPackages()
{
    return d->thread->getInstalledPackages();
}

QStringList Backend::getInstalledPackagesAsStringList()
{
    return d->thread->getInstalledPackagesAsStringList();
}

alpm_list_t *Backend::getUpgradeablePackages()
{
    return d->thread->getUpgradeablePackages();
}

QStringList Backend::getUpgradeablePackagesAsStringList()
{
    return d->thread->getUpgradeablePackagesAsStringList();
}

alpm_list_t *Backend::getAvailableGroups()
{
    return d->thread->getAvailableGroups();
}

QStringList Backend::getAvailableGroupsAsStringList()
{
    return d->thread->getAvailableGroupsAsStringList();
}

QStringList Backend::getPackageDependencies(pmpkg_t *package)
{
    return d->thread->getPackageDependencies(package);
}

QStringList Backend::getPackageDependencies(const QString &name, const QString &repo)
{
    return d->thread->getPackageDependencies(name, repo);
}

QStringList Backend::getDependenciesOnPackage(pmpkg_t *package)
{
    return d->thread->getDependenciesOnPackage(package);
}

QStringList Backend::getDependenciesOnPackage(const QString &name, const QString &repo)
{
    return d->thread->getDependenciesOnPackage(name, repo);
}

bool Backend::isInstalled(pmpkg_t *pkg)
{
    return d->thread->isInstalled(pkg);
}

bool Backend::isInstalled(const QString &pkg)
{
    return d->thread->isInstalled(pkg);
}

QStringList Backend::getPackageFiles(pmpkg_t *package)
{
    return d->thread->getPackageFiles(package);
}

QStringList Backend::getPackageFiles(const QString &name)
{
    return d->thread->getPackageFiles(name);
}

QStringList Backend::getProviders(const QString &name, const QString &repo)
{
    return d->thread->getProviders(name, repo);
}

QStringList Backend::getProviders(pmpkg_t *pkg)
{
    return d->thread->getProviders(pkg);
}

bool Backend::isProviderInstalled(const QString &provider)
{
    return d->thread->isProviderInstalled(provider);
}

unsigned long Backend::getPackageSize(pmpkg_t *package)
{
    return d->thread->getPackageSize(package);
}

unsigned long Backend::getPackageSize(const QString &name, const QString &repo)
{
    return d->thread->getPackageSize(name, repo);
}

QString Backend::getAlpmVersion()
{
    return d->thread->getAlpmVersion();
}

QString Backend::getPackageVersion(pmpkg_t *package)
{
    return d->thread->getPackageVersion(package);
}

QString Backend::getPackageVersion(const QString &name, const QString &repo)
{
    return d->thread->getPackageVersion(name, repo);
}

QString Backend::getPackageRepo(const QString &name, bool checkver)
{
    return d->thread->getPackageRepo(name, checkver);
}

alpm_list_t *Backend::getPackagesFromGroup(const QString &groupname)
{
    return d->thread->getPackagesFromGroup(groupname);
}

QStringList Backend::getPackagesFromGroupAsStringList(const QString &groupname)
{
    return d->thread->getPackagesFromGroupAsStringList(groupname);
}

alpm_list_t *Backend::getPackagesFromRepo(const QString &reponame)
{
    return d->thread->getPackagesFromRepo(reponame);
}

QStringList Backend::getPackagesFromRepoAsStringList(const QString &reponame)
{
    return d->thread->getPackagesFromRepoAsStringList(reponame);
}

int Backend::countPackages(PackageStatus status)
{
    return d->thread->countPackages(status);
}

QStringList Backend::alpmListToStringList(alpm_list_t *list)
{
    return d->thread->alpmListToStringList(list);
}

pmpkg_t *Backend::getPackageFromName(const QString &name, const QString &repo)
{
    return d->thread->getPackageFromName(name, repo);
}

alpm_list_t *Backend::getPackageGroups(pmpkg_t *package)
{
    return d->thread->getPackageGroups(package);
}

QStringList Backend::getPackageGroupsAsStringList(pmpkg_t *package)
{
    return d->thread->getPackageGroupsAsStringList(package);
}

bool Backend::updateDatabase()
{
    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(UpdateDatabase)));
    qDebug() << "Thread is running";
    return true;
}

bool Backend::fullSystemUpgrade()
{
    return d->thread->fullSystemUpgrade();
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

void Backend::processQueue(QList<pmtransflag_t> flags)
{
    d->thread->setFlags(flags);
    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(ProcessQueue)));
    qDebug() << "Thread is running";
}

}

