/***************************************************************************
 *   Copyright (C) 2009 by Dario Freddi                                    *
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

#ifndef GLOBALSH
#define GLOBALSH

#include <QFlags>

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
    };

    enum TransactionProgress {
        InvalidProgress = 0,
        AddRoutineStart = 1,
        ConflictsRoutineStart = 2,
        UpgradeRoutineStart = 3,
        RemoveRoutineStart = 4
    };

    enum ErrorCode {
        PrepareError = 1,
        CommitError = 2,
        InitTransactionError = 4,
        ReleaseTransactionError = 8,
        AddTargetError = 16,
        CreateSysUpgradeError = 32,
        DuplicateTarget = 64,
        PackageIgnored = 128,
        PackageNotFound = 256,
        UnsatisfiedDependencies = 512,
        ConflictingDependencies = 1024,
        FileConflictTarget = 2048,
        FileConflictFilesystem = 4096,
        CorruptedFile = 8192
    };

    Q_DECLARE_FLAGS(Errors, ErrorCode)

    enum DatabaseState {
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
}

#endif // GLOBALSH
