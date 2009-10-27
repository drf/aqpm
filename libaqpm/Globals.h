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

#ifndef GLOBALSH
#define GLOBALSH

#include <QtCore/QFlags>

namespace Aqpm
{

class Globals {
public:

    enum TransactionEvent {
        InvalidEvent = 0,
        CheckDepsStart = 1,
        CheckDepsDone = 2,
        FileConflictsStart = 3,
        FileConflictsDone = 4,
        ResolveDepsStart = 5,
        ResolveDepsDone = 6,
        InterConflictsStart = 7,
        InterConflictsDone = 8,
        AddStart = 9,
        AddDone = 10,
        RemoveStart = 11,
        RemoveDone = 12,
        UpgradeStart = 13,
        UpgradeDone = 14,
        IntegrityStart = 16,
        IntegrityDone = 17,
        DeltaIntegrityStart = 18,
        DeltaIntegrityDone = 19,
        DeltaPatchesStart = 20,
        DeltaPatchesDone = 21,
        DeltaPatchStart = 22,
        DeltaPatchDone = 23,
        DeltaPatchFailed = 24,
        ScriptletInfo = 25,
        PrintUri = 26,
        RetrieveStart = 27
    };

    enum TransactionQuestion {
        InvalidQuestion = 0,
        IgnorePackage = 1,
        ReplacePackage = 2,
        PackageConflict = 3,
        CorruptedPackage = 4,
        NewerLocalPackage = 5,
        UnresolvedDependencies = 6
    };

    enum TransactionProgress {
        InvalidProgress = 0,
        AddRoutineStart = 1,
        ConflictsRoutineStart = 2,
        UpgradeRoutineStart = 3,
        RemoveRoutineStart = 4
    };

    enum ErrorCode {
        UnknownError = 0,
        PrepareError = 1,
        CommitError = 2,
        InitTransactionError = 3,
        ReleaseTransactionError = 4,
        AddTargetError = 5,
        CreateSysUpgradeError = 6,
        DuplicateTarget = 7,
        PackageIgnored = 8,
        PackageNotFound = 9,
        UnsatisfiedDependencies = 10,
        ConflictingDependencies = 11,
        FileConflictTarget = 12,
        FileConflictFilesystem = 13,
        CorruptedFile = 14,
        WorkerInitializationFailed = 15,
        AuthorizationNotGranted = 16,
        WorkerDisappeared = 17,
        TransactionInterruptedByUser = 18
    };

    Q_DECLARE_FLAGS(Errors, ErrorCode);

    enum TransactionFlag {
        NoDeps = 0x01,
        Force = 0x02,
        NoSave = 0x04,
        /* 0x08 flag can go here */
        Cascade = 0x10,
        Recurse = 0x20,
        DbOnly = 0x40,
        /* 0x80 flag can go here */
        AllDeps = 0x100,
        DownloadOnly = 0x200,
        NoScriptlet = 0x400,
        NoConflicts = 0x800,
        /* 0x1000 flag can go here */
        Needed = 0x2000,
        AllExplicit = 0x4000,
        UnNeeded = 0x8000,
        RecurseAll = 0x10000,
        NoLock = 0x20000
    };

    Q_DECLARE_FLAGS(TransactionFlags, TransactionFlag);

    enum DatabaseState {
        UnknownDatabaseState = 0,
        Checking = 1,
        Downloading = 2,
        Updating = 4,
        Updated = 8,
        Unchanged = 16,
        DatabaseError = 32
    };

    enum PackageStatus {
        AllPackages,
        InstalledPackages,
        UpgradeablePackages
    };

};

Q_DECLARE_OPERATORS_FOR_FLAGS(Globals::TransactionFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(Globals::Errors)

}

#endif // GLOBALSH
