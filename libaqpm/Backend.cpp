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

#include "Configuration.h"
#include "BackendThread.h"
#include "SynchronousLoop.h"
#include "ActionEvent.h"
#include "Downloader.h"

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

void Backend::Private::__k__backendReady()
{
    ready = true;
    emit q->backendReady();
}

void Backend::Private::__k__setUpSelf(BackendThread *t)
{
    thread = t;

    // Create also the thread for Downloader
    /*downloaderThread = new ContainerThread("Downloader");
    downloaderThread->start();*/
    // Start up downloader
    Downloader::instance();
    connect(Downloader::instance(), SIGNAL(downloadProgress(qint64,qint64,QString)),
            q, SLOT(__k__computeDownloadProgress(qint64,qint64,QString)));

    connect(thread, SIGNAL(dbQty(const QStringList&)),
            q, SIGNAL(dbQty(const QStringList&)), Qt::QueuedConnection);
    connect(thread, SIGNAL(dbStatusChanged(const QString&, int)),
            q, SIGNAL(dbStatusChanged(const QString&, int)), Qt::QueuedConnection);
    connect(thread, SIGNAL(transactionStarted()),
            q, SIGNAL(transactionStarted()));
    connect(thread, SIGNAL(transactionReleased()),
            q, SIGNAL(transactionReleased()));
    connect(thread, SIGNAL(operationFinished(bool)),
            q, SIGNAL(operationFinished(bool)));
    connect(thread, SIGNAL(threadInitialized()),
            q, SLOT(__k__backendReady()));
    connect(thread, SIGNAL(streamTotalOffset(int)),
            q, SLOT(__k__totalOffsetReceived(int)));
    connect(thread, SIGNAL(streamTransProgress(int, const QString&, int, int, int)),
            q, SLOT(__k__doStreamTransProgress(int, const QString&, int, int, int)));
    connect(thread, SIGNAL(streamTransEvent(int, QVariantMap)),
            q, SLOT(__k__doStreamTransEvent(int, QVariantMap)));
    connect(thread, SIGNAL(errorOccurred(int,QVariantMap)),
            q, SLOT(__k__streamError(int,QVariantMap)));
    connect(thread, SIGNAL(logMessageStreamed(QString)),
            q, SIGNAL(logMessageStreamed(QString)));
    connect(thread, SIGNAL(streamTransQuestion(int, QVariantMap)),
            q, SLOT(__k__doStreamTransQuestion(int, QVariantMap)));

    QCoreApplication::postEvent(thread, new QEvent(q->getEventTypeFor(Initialization)));
}

void Backend::Private::__k__streamError(int code, const QVariantMap &args)
{
    emit q->errorOccurred((Globals::Errors) code, args);
}

void Backend::Private::__k__doStreamTransProgress(int event, const QString &pkgname, int percent,
                           int howmany, int remain)
{
    emit q->streamTransProgress((Aqpm::Globals::TransactionProgress)event, pkgname, percent, howmany, remain);
}

void Backend::Private::__k__doStreamTransEvent(int event, const QVariantMap &args)
{
    if ((Aqpm::Globals::TransactionEvent) event == Aqpm::Globals::RetrieveStart) {
        if (averageRateTime.isNull()) {
            averageRateTime = QDateTime::currentDateTime();
        }
    }

    emit q->streamTransEvent((Aqpm::Globals::TransactionEvent) event, args);
}

void Backend::Private::__k__doStreamTransQuestion(int event, const QVariantMap &args)
{
    emit q->streamTransQuestion((Aqpm::Globals::TransactionQuestion) event, args);
}

void Backend::Private::__k__computeDownloadProgress(qint64 downloaded, qint64 total, const QString &filename)
{
    if (downloaded == total) {
        list_xfered += downloaded;
        return;
    }

    list_last = list_xfered + downloaded;

    // Update Rate
    int seconds = averageRateTime.secsTo(QDateTime::currentDateTime());

    int rate = 0;

    if (seconds != 0 && list_last != 0) {
        rate = (int)(list_last / seconds);
    }

    emit q->streamDlProg(filename, (int)downloaded, (int)total, (int)rate, (int)list_last, (int)list_total);
}

void Backend::Private::__k__totalOffsetReceived(int offset)
{
    list_total = offset;
    qDebug() << "total called, offset" << offset;
    /* if we get a 0 value, it means this list has finished downloading,
    * so clear out our list_xfered as well */
    if (offset == 0) {
        list_xfered = 0;
        list_last = 0;
        list_total = 0;
    }
}

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

    d->q = this;

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

    d->containerThread = new ContainerThread("Backend");
    connect(d->containerThread, SIGNAL(threadCreated(BackendThread*)), SLOT(__k__setUpSelf(BackendThread*)));
    d->containerThread->start();
}

Backend::~Backend()
{
    d->containerThread->quit();
    d->containerThread->deleteLater();
/*    d->downloaderThread->quit();
    d->downloaderThread->deleteLater();*/
    delete d;
}

void Backend::safeInitialization()
{
    if (!ready()) {
        QEventLoop e;
        connect(this, SIGNAL(backendReady()), &e, SLOT(quit()));
        e.exec();

        setUpAlpm();
    }
}

QString Backend::version()
{
    return AQPM_VERSION;
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

bool Backend::ready() const
{
    return d->ready;
}

bool Backend::setAqpmRoot(const QString& root, bool applyToConfiguration) const
{
    QVariantMap args;
    args["root"] = QVariant::fromValue(root);
    args["applyToConfiguration"] = QVariant::fromValue(applyToConfiguration);
    SynchronousLoop s(SetAqpmRoot, args);
    return s.result()["retvalue"].toBool();
}

QString Backend::aqpmRoot() const
{
    SynchronousLoop s(AqpmRoot, QVariantMap());
    return s.result()["retvalue"].toString();
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
    args["database"] = QVariant::fromValue(db);
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

void Backend::fullSystemUpgrade(Globals::TransactionFlags flags, bool downgrade)
{
    QVariantMap args;
    args["flags"] = QVariant::fromValue((int)flags);
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

void Backend::processQueue(Globals::TransactionFlags flags)
{
    QVariantMap args;
    args["flags"] = QVariant::fromValue((int)flags);
    SynchronousLoop s(SetFlags, args);
    QCoreApplication::postEvent(d->thread, new QEvent(getEventTypeFor(ProcessQueue)));
    qDebug() << "Thread is running";
}

void Backend::downloadQueue()
{
    SynchronousLoop s(DownloadQueue, QVariantMap());
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

BackendThread *Backend::getInnerThread()
{
    return d->thread;
}

void Backend::interruptTransaction()
{
    SynchronousLoop s(InterruptTransaction, QVariantMap());
}

}

#include "Backend.moc"
