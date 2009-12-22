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

#ifndef BACKEND_H
#define BACKEND_H

#include "Visibility.h"
#include "QueueItem.h"
#include "Globals.h"
#include "Package.h"
#include "Database.h"
#include "Group.h"

#include <QtCore/QThread>
#include <QtCore/QStringList>
#include <QtCore/QEvent>
#include <QtCore/QMetaType>

namespace Aqpm
{

typedef QHash< QString, Aqpm::QueueItem::Action > Targets;

class BackendThread;

/**
 * \brief The main and only entry point for performing operations with Aqpm
 *
 * Backend encapsulates all the needed logic to perform each and every operation with Aqpm.
 * It can work both in synchronous and asynchronous mode, and it provides access to all the most
 * common functionalities in Alpm. 90% of the features are covered in Aqpm, although some very
 * advanced and uncommon ones are not present, and you will need them only if you're attempting to
 * develop a full-fledged frontend.
 * \par
 * The class is implemented as a singleton and it spawns an additional thread where Alpm will be jailed.
 * Since alpm was not designed to support threads, Aqpm implements a queue to avoid accidental concurrent access
 * to alpm methods. You are free to use Aqpm in multithreaded environments without having to worry.
 * \par
 * Aqpm \b _NEEDS_ to work as standard, non-privileged user. Failure in doing so might lead to unsecure behavior.
 * Aqpm uses privilege escalation with PolicyKit to perform privileged actions. Everything is done for you, even
 * if you can choose to manage the authentication part by yourself, in case you want more tight integration with
 * your GUI
 *
 * \note All the methods in this class, unless noted otherwise, are thread safe
 *
 * \author Dario Freddi
 */
class AQPM_EXPORT Backend : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Backend)

public:

    /**
     * Defines the action type in a synchonous operation. There is a matching entry for
     * every function available
     */
    enum ActionType {
        GetAvailableDatabases,
        GetAvailableGroups,
        GetPackagesFromDatabase,
        GetLocalDatabase,
        GetPackagesFromGroup,
        GetUpgradeablePackages,
        GetInstalledPackages,
        GetPackageDependencies,
        GetPackageGroups,
        GetDependenciesOnPackage,
        CountPackages,
        GetProviders,
        IsProviderInstalled,
        GetPackageDatabase,
        IsInstalled,
        GetAlpmVersion,
        ClearQueue,
        AddItemToQueue,
        GetQueue,
        SetShouldHandleAuthorization,
        ShouldHandleAuthorization,
        SetAnswer,
        GetPackage,
        GetGroup,
        GetDatabase,
        SetUpAlpm,
        TestLibrary,
        IsOnTransaction,
        SetFlags,
        ReloadPacmanConfiguration,
        UpdateDatabase,
        ProcessQueue,
        DownloadQueue,
        Initialization,
        SystemUpgrade,
        PerformAction,
        InterruptTransaction,
        Ready,
        SetAqpmRoot,
        AqpmRoot,
        RetrieveAdditionalTargetsForQueue
    };

    /**
     * \return The current instance of the Backend
     */
    static Backend *instance();
    /**
     * \return Aqpm's version
     */
    static QString version();

    /**
     * \return \c true if the backend is ready to be used, \c false if it's still in the initialization phase
     */
    bool ready() const;

    /**
     * This method initializes Aqpm safely, if needed, and sets up Aqpm. Once this method has returned, Aqpm
     * is ready to be used. setUpAlpm() is already called by this function, you don't need to call it again.
     * If the backend had been already initialized elsewhere, this function will simply return immediately.
     * It is strongly advised to always use this function to initialize Aqpm.
     *
     * \note this function will work only if it will be the VERY FIRST CALL to Backend::instance(). Failure in
     * doing so might lead to undefined behavior.
     */
    void safeInitialization();

    /**
     * Default destructor
     */
    virtual ~Backend();

    /**
     * Performs a test on the library to check if Alpm and Aqpm are ready to be used
     *
     * \return \c true if the library is ready, \c false if there was a problem while testing it
     */
    bool testLibrary() const;
    bool isOnTransaction() const;

    /**
     * Attempts to change the root directory of package management to \c root. This has the effect
     * of "chrooting" Aqpm to a new root, letting you also use the configuration present in the new root
     * \note This method is mapped to the org.chakraproject.aqpm.setaqpmroot action
     *
     * \param root The new root, without the ending slash (e.g.: "/media/myotherdisk")
     * \param applyToConfiguration Whether to use the configuration found in \c root or the standard config file
     * \returns Whether the change was successful or not
     */
    bool setAqpmRoot(const QString &root, bool applyToConfiguration) const;
    /**
     * \returns The current root directory used by aqpm, without the ending slash
     */
    QString aqpmRoot() const;

    /**
     * Performs an action asynchronously. You are not advised to use this method as it is right now,
     * since it's still a work in progress
     */
    void performAsyncAction(ActionType type, const QVariantMap &args);

    /**
     * Gets a list of all available remote databases on the system. This list does not include
     * the local database.
     *
     * \return every sync database registered
     */
    Database::List getAvailableDatabases() const;
    /**
     * Returns the local database, which carries all installed packages
     *
     * \return the local database
     */
    Database getLocalDatabase() const;

    Group::List getAvailableGroups() const;

    Package::List getPackagesFromDatabase(const Database &db) const;

    Package::List getPackagesFromGroup(const Group &group) const;

    Package::List getUpgradeablePackages() const;

    Package::List getInstalledPackages() const;

    Package::List getPackageDependencies(const Package &package) const;

    Group::List getPackageGroups(const Package &package) const;

    Package::List getDependenciesOnPackage(const Package &package) const;

    int countPackages(Globals::PackageStatus status) const;

    QStringList getProviders(const Package &package) const;
    bool isProviderInstalled(const QString &provider) const;

    Database getPackageDatabase(const Package &package, bool checkver = false) const;

    bool isInstalled(const Package &package) const;

    /**
     * \brief Starts an update operation.
     * This function starts up the process, eventually requiring authentication.
     * Please note that the operation is completely asynchronous: this method will
     * return immediately. You can monitor the transaction throughout Backend's signals
     */
    void updateDatabase();
    /**
     * \brief Starts a system upgrade operation
     *
     * Starts up a system upgrade operation (equal to pacman -Su), eventually requiring authentication.
     * Please note that the operation is completely asynchronous: this method will
     * return immediately. You can monitor the transaction throughout Backend's signals
     *
     * \param flags the flags to be used in this transaction
     * \param downgrade whether the upgrade will be able to downgrade packages
     */
    void fullSystemUpgrade(Globals::TransactionFlags flags, bool downgrade = false);

    bool reloadPacmanConfiguration() const; // In case the user modifies it.

    QString getAlpmVersion() const;

    /**
     * Clears the current queue
     */
    void clearQueue();
    /**
     * Adds an item to the current queue. Please note that items should be added just once.
     * They can be both in the format "package" or "database/package".
     *
     * \param name the name of the item
     * \param action the type of action to be done on the item
     *
     * \see processQueue
     */
    void addItemToQueue(const QString &name, QueueItem::Action action);

    /**
     * \brief Starts processing the current queue
     *
     * Starts up a sync operation (equal to pacman -S \<targets\>), eventually requiring authentication.
     * Please note that the operation is completely asynchronous: this method will
     * return immediately. You can monitor the transaction throughout Backend's signals.
     * \par
     * All the items in the current queue will be processed, and when the transaction will be
     * over, the queue will get cleared, even if there were errors
     *
     * \param flags the flags to be used in this transaction
     *
     * \see addItemToQueue
     */
    void processQueue(Globals::TransactionFlags flags);
    /**
     * \brief Starts downloading the current queue
     *
     * Starts up a sync download-only operation (equal to pacman -Sw \<targets\>),
     * eventually requiring authentication.
     * Please note that the operation is completely asynchronous: this method will
     * return immediately. You can monitor the transaction throughout Backend's signals.
     * \par
     * All the items in the current queue will be processed, and when the transaction will be
     * over, the queue will get cleared, even if there were errors
     *
     * \param flags the flags to be used in this transaction
     *
     * \see addItemToQueue
     */
    void downloadQueue();

    void retrieveAdditionalTargetsForQueue(Globals::TransactionFlags flags);

    QueueItem::List queue() const;

    void interruptTransaction();

    void setShouldHandleAuthorization(bool should);
    bool shouldHandleAuthorization() const;

    void setAnswer(int answer);

    Package getPackage(const QString &name, const QString &repo) const;
    Group getGroup(const QString &name) const;
    Database getDatabase(const QString &name) const;

    BackendThread *getInnerThread();

public Q_SLOTS:
    void setUpAlpm();

Q_SIGNALS:
    void dbStatusChanged(const QString &repo, int action);
    void dbQty(const QStringList &db);

    void transactionStarted();
    void transactionReleased();

    void streamTotalDownloadSize(qint64 size);

    void streamDlProg(const QString &c, int bytedone, int bytetotal, int speed,
                      int listdone, int listtotal);

    void streamTransProgress(Aqpm::Globals::TransactionProgress event, const QString &pkgname, int percent,
                             int howmany, int remain);

    void streamTransEvent(Aqpm::Globals::TransactionEvent event, const QVariantMap &args);

    void streamTransQuestion(Aqpm::Globals::TransactionQuestion event, const QVariantMap &args);

    void errorOccurred(Aqpm::Globals::Errors code, const QVariantMap &args);

    void logMessageStreamed(const QString &msg);

    void additionalTargetsRetrieved(const Aqpm::Targets &targets);

    void operationFinished(bool success);

    void backendReady();

private:
    Backend();

    class Private;
    Private * const d;

    friend class BackendThread;
    friend class SynchronousLoop;

    Q_PRIVATE_SLOT(d, void __k__backendReady())
    Q_PRIVATE_SLOT(d, void __k__setUpSelf(BackendThread *t))
    Q_PRIVATE_SLOT(d, void __k__streamError(int code, const QVariantMap &args))
    Q_PRIVATE_SLOT(d, void __k__doStreamTransProgress(int event, const QString &pkgname, int percent,
                   int howmany, int remain))
    Q_PRIVATE_SLOT(d, void __k__doStreamTransEvent(int event, const QVariantMap &args))
    Q_PRIVATE_SLOT(d, void __k__doStreamTransQuestion(int event, const QVariantMap &args))
    Q_PRIVATE_SLOT(d, void __k__computeDownloadProgress(qlonglong downloaded, qlonglong total, const QString &filename))
    Q_PRIVATE_SLOT(d, void __k__totalOffsetReceived(int offset))
    Q_PRIVATE_SLOT(d, void __k__operationFinished(bool result))
};

}

Q_DECLARE_METATYPE(Aqpm::Targets)

#endif /* BACKEND_H */
