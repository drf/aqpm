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
#include "Backend_p.h"

#include <config-aqpm.h>

#include "Configuration.h"
#include "BackendThread_p.h"
#include "Loops_p.h"
#include "ActionEvent_p.h"
#include "Downloader_p.h"

#include <QMetaType>
#include <QDebug>
#include <QCoreApplication>
#include <QDBusMetaType>
#include <QGlobalStatic>

namespace Aqpm
{
void BackendPrivate::startUpDownloader()
{
    Q_Q(Backend);

    // Start up Downloader
    if (!Downloader::hasInstance()) {
        q->connect(Downloader::instance(), SIGNAL(downloadProgress(qlonglong, qlonglong, QString)),
                   q, SLOT(__k__computeDownloadProgress(qlonglong, qlonglong, QString)));
    }
}

void BackendPrivate::shutdownDownloader()
{
    if (Downloader::hasInstance()) {
        Downloader::destroyInstance();
    }
}

void BackendPrivate::__k__backendReady()
{
    Q_Q(Backend);

    ready = true;
    emit q->backendReady();
}

void BackendPrivate::__k__setUpSelf(BackendThread *t)
{
    Q_Q(Backend);

    thread = t;

    q->connect(thread, SIGNAL(dbQty(const QStringList&)),
            q, SIGNAL(dbQty(const QStringList&)));
    q->connect(thread, SIGNAL(dbStatusChanged(const QString&, int)),
            q, SIGNAL(dbStatusChanged(const QString&, int)));
    q->connect(thread, SIGNAL(transactionStarted()),
            q, SIGNAL(transactionStarted()));
    q->connect(thread, SIGNAL(transactionReleased()),
            q, SIGNAL(transactionReleased()));
    q->connect(thread, SIGNAL(operationFinished(bool)),
            q, SLOT(__k__operationFinished(bool)));
    q->connect(thread, SIGNAL(threadInitialized()),
            q, SLOT(__k__backendReady()));
    q->connect(thread, SIGNAL(streamTotalOffset(int)),
            q, SLOT(__k__totalOffsetReceived(int)));
    q->connect(thread, SIGNAL(streamTransProgress(int, const QString&, int, int, int)),
            q, SLOT(__k__doStreamTransProgress(int, const QString&, int, int, int)));
    q->connect(thread, SIGNAL(streamTransEvent(int, QVariantMap)),
            q, SLOT(__k__doStreamTransEvent(int, QVariantMap)));
    q->connect(thread, SIGNAL(errorOccurred(int, QVariantMap)),
            q, SLOT(__k__streamError(int, QVariantMap)));
    q->connect(thread, SIGNAL(logMessageStreamed(QString)),
            q, SIGNAL(logMessageStreamed(QString)));
    q->connect(thread, SIGNAL(streamTransQuestion(int, QVariantMap)),
            q, SLOT(__k__doStreamTransQuestion(int, QVariantMap)));
    q->connect(thread, SIGNAL(additionalTargetsRetrieved(Aqpm::Targets)),
            q, SIGNAL(additionalTargetsRetrieved(Aqpm::Targets)));

    QCoreApplication::postEvent(thread, new QEvent(getEventTypeFor(Backend::Initialization)));
}

void BackendPrivate::__k__streamError(int code, const QVariantMap &args)
{
    Q_Q(Backend);

    emit q->errorOccurred((Globals::Errors) code, args);
}

void BackendPrivate::__k__doStreamTransProgress(int event, const QString &pkgname, int percent,
        int howmany, int remain)
{
    Q_Q(Backend);

    emit q->streamTransProgress((Aqpm::Globals::TransactionProgress)event, pkgname, percent, howmany, remain);
}

void BackendPrivate::__k__doStreamTransEvent(int event, const QVariantMap &args)
{
    Q_Q(Backend);

    if ((Aqpm::Globals::TransactionEvent) event == Aqpm::Globals::RetrieveStart) {
        if (averageRateTime.isNull()) {
            averageRateTime = QDateTime::currentDateTime();
        }
    }

    emit q->streamTransEvent((Aqpm::Globals::TransactionEvent) event, args);
}

void BackendPrivate::__k__doStreamTransQuestion(int event, const QVariantMap &args)
{
    Q_Q(Backend);

    emit q->streamTransQuestion((Aqpm::Globals::TransactionQuestion) event, args);
}

void BackendPrivate::__k__computeDownloadProgress(qlonglong downloaded, qlonglong total, const QString &filename)
{
    Q_Q(Backend);

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

void BackendPrivate::__k__totalOffsetReceived(int offset)
{
    Q_Q(Backend);

    list_total = offset;
    qDebug() << "total called, offset" << offset;
    /* if we get a 0 value, it means this list has finished downloading,
    * so clear out our list_xfered as well */
    if (offset == 0) {
        list_xfered = 0;
        list_last = 0;
        list_total = 0;
    } else {
        emit q->streamTotalDownloadSize(offset);
    }
}

void BackendPrivate::__k__operationFinished(bool result)
{
    Q_Q(Backend);

    shutdownDownloader();
    emit q->operationFinished(result);
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
        d(new BackendPrivate(this))
{
    Q_ASSERT(!s_globalBackend()->q);
    s_globalBackend()->q = this;

    qRegisterMetaType<QueueItem>();
    qDBusRegisterMetaType<QueueItem>();
    qRegisterMetaType<QueueItem::List>();
    qDBusRegisterMetaType<QueueItem::List>();
    qRegisterMetaType<Package*>();
    qRegisterMetaType<Group*>();
    qRegisterMetaType<Database*>();
    qRegisterMetaType<Package::List>();
    qRegisterMetaType<Group::List>();
    qRegisterMetaType<Database::List>();
    qRegisterMetaType<Targets>();

    qDebug() << "Constructing Backend singleton";

    d->events[Initialization] = (QEvent::Type)QEvent::registerEventType();
    d->events[UpdateDatabase] = (QEvent::Type)QEvent::registerEventType();
    d->events[ProcessQueue] = (QEvent::Type)QEvent::registerEventType();
    d->events[SystemUpgrade] = (QEvent::Type)QEvent::registerEventType();
    d->events[DownloadQueue] = (QEvent::Type)QEvent::registerEventType();
    d->events[RetrieveAdditionalTargetsForQueue] = (QEvent::Type)QEvent::registerEventType();
    d->events[PerformAction] = (QEvent::Type)QEvent::registerEventType();

    d->containerThread = new ContainerThread();
    connect(d->containerThread, SIGNAL(threadCreated(BackendThread*)), SLOT(__k__setUpSelf(BackendThread*)));
    d->containerThread->start();
}

Backend::~Backend()
{
    delete d;
}

void Backend::safeInitialization()
{
    if (!ready()) {
        QEventLoop e;
        connect(this, SIGNAL(backendReady()), &e, SLOT(quit()));
        e.exec();

        d->setUpAlpm();
    }
}

QString Backend::version()
{
    return AQPM_VERSION;
}

QEvent::Type BackendPrivate::getEventTypeFor(Backend::ActionType event) const
{
    return events[event];
}

void Backend::performAsyncAction(ActionType type, const QVariantMap &args)
{
    QCoreApplication::postEvent(d->thread, new ActionEvent(d->getEventTypeFor(Backend::PerformAction), type, args));
}

bool Backend::testLibrary() const
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

bool Backend::isOnTransaction() const
{
    SynchronousLoop s(IsOnTransaction, QVariantMap());
    return s.result()["retvalue"].toBool();
}

bool Backend::reloadPacmanConfiguration() const
{
    SynchronousLoop s(ReloadPacmanConfiguration, QVariantMap());
    return s.result()["retvalue"].toBool();
}

void BackendPrivate::setUpAlpm()
{
    SynchronousLoop s(Backend::SetUpAlpm, QVariantMap());
}

Database::List Backend::availableDatabases() const
{
    SynchronousLoop s(GetAvailableDatabases, QVariantMap());
    return s.result()["retvalue"].value<Database::List>();
}

Database *Backend::localDatabase()
{
    SynchronousLoop s(GetLocalDatabase, QVariantMap());
    return s.result()["retvalue"].value<Database*>();
}

Package::List Backend::installedPackages() const
{
    SynchronousLoop s(GetInstalledPackages, QVariantMap());
    return s.result()["retvalue"].value<Package::List>();
}

Package::List Backend::upgradeablePackages() const
{
    SynchronousLoop s(GetUpgradeablePackages, QVariantMap());
    return s.result()["retvalue"].value<Package::List>();
}

Group::List Backend::availableGroups() const
{
    SynchronousLoop s(GetAvailableGroups, QVariantMap());
    return s.result()["retvalue"].value<Group::List>();
}

bool Backend::isProviderInstalled(const QString &provider) const
{
    QVariantMap args;
    args["provider"] = QVariant::fromValue(provider);
    SynchronousLoop s(IsProviderInstalled, args);
    return s.result()["retvalue"].toBool();
}

QString Backend::alpmVersion() const
{
    SynchronousLoop s(GetAlpmVersion, QVariantMap());
    return s.result()["retvalue"].toString();
}

int Backend::countPackages(Globals::PackageStatus status) const
{
    QVariantMap args;
    args["status"] = QVariant::fromValue((int)status);
    SynchronousLoop s(CountPackages, args);
    return s.result()["retvalue"].toInt();
}

Package *Backend::package(const QString &name, const QString &db)
{
    QVariantMap args;
    args["name"] = QVariant::fromValue(name);
    args["database"] = QVariant::fromValue(db);
    SynchronousLoop s(GetPackage, args);
    return s.result()["retvalue"].value<Package*>();
}

Group *Backend::group(const QString &name)
{
    QVariantMap args;
    args["name"] = QVariant::fromValue(name);
    SynchronousLoop s(GetGroup, args);
    return s.result()["retvalue"].value<Group*>();
}

Database *Backend::database(const QString &name)
{
    QVariantMap args;
    args["name"] = QVariant::fromValue(name);
    SynchronousLoop s(GetDatabase, args);
    return s.result()["retvalue"].value<Database*>();
}

Package *Backend::loadPackageFromLocalFile(const QString& path)
{
    QVariantMap args;
    args["path"] = QVariant::fromValue(path);
    SynchronousLoop s(LoadPackageFromLocalFile, args);
    return s.result()["retvalue"].value<Package*>();
}

void Backend::updateDatabase()
{
    d->startUpDownloader();
    QCoreApplication::postEvent(d->thread, new QEvent(d->getEventTypeFor(UpdateDatabase)));
    qDebug() << "Thread is running";
}

void Backend::fullSystemUpgrade(Globals::TransactionFlags flags, bool downgrade)
{
    d->startUpDownloader();
    QVariantMap args;
    args["flags"] = QVariant::fromValue((int)flags);
    args["downgrade"] = downgrade;
    SynchronousLoop s(SetFlags, args);
    QCoreApplication::postEvent(d->thread,
                                new ActionEvent(d->getEventTypeFor(SystemUpgrade), SystemUpgrade, args));
    qDebug() << "Thread is running";
}

void Backend::clearQueue()
{
    SynchronousLoop s(ClearQueue, QVariantMap());
}

void Backend::addItemToQueue(Package *package, QueueItem::Action action)
{
    if (action == QueueItem::Sync) {
        addItemToQueue(QString("%1/%2").arg(package->database()->name()).arg(package->name()), action);
    } else {
        addItemToQueue(package->name(), action);
    }
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
    d->startUpDownloader();
    QVariantMap args;
    args["flags"] = QVariant::fromValue((int)flags);
    SynchronousLoop s(SetFlags, args);
    QCoreApplication::postEvent(d->thread, new QEvent(d->getEventTypeFor(ProcessQueue)));
    qDebug() << "Thread is running";
}

void Backend::retrieveAdditionalTargetsForQueue(Globals::TransactionFlags flags)
{
    QVariantMap args;
    args["flags"] = QVariant::fromValue((int)flags);
    SynchronousLoop s(SetFlags, args);
    QCoreApplication::postEvent(d->thread, new QEvent(d->getEventTypeFor(RetrieveAdditionalTargetsForQueue)));
}

void Backend::downloadQueue()
{
    d->startUpDownloader();
    QCoreApplication::postEvent(d->thread, new QEvent(d->getEventTypeFor(DownloadQueue)));
    qDebug() << "Thread is running";
}

Package::List Backend::searchFiles(const QString& filename) const
{
    QVariantMap args;
    args["filename"] = QVariant::fromValue(filename);
    SynchronousLoop s(SearchFiles, args);
    return s.result()["retvalue"].value<Package::List>();
}

Package::List Backend::searchPackages(const QStringList& targets, const Aqpm::Database::List& dbs) const
{
    QVariantMap args;
    args["targets"] = QVariant::fromValue(targets);
    args["dbs"] = QVariant::fromValue(dbs);
    SynchronousLoop s(SearchPackages, args);
    return s.result()["retvalue"].value<Package::List>();
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

void Backend::interruptTransaction()
{
    SynchronousLoop s(InterruptTransaction, QVariantMap());
}

}

#include "Backend.moc"
