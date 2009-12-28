/*
    <one line to give the program's name and a brief idea of what it does.>
    Copyright (C) <year>  <name of author>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "AqpmBenchmark.h"
#include "../Backend.h"

void AqpmBenchmark::benchmark_Safeinit()
{
    QBENCHMARK_ONCE {
        Aqpm::Backend::instance()->safeInitialization();
    }
}

void AqpmBenchmark::benchmark_CacheAllPackages()
{
    QBENCHMARK_ONCE {
        foreach (Aqpm::Database *db, Aqpm::Backend::instance()->availableDatabases()) {
            db->packages();
        }
        // Now for the local db
        foreach(Aqpm::Package *pkg, Aqpm::Backend::instance()->localDatabase()->packages()) {
            pkg->groups();
        }
        // Now groups
        foreach(Aqpm::Group *group, Aqpm::Backend::instance()->availableGroups()) {
            group->packages();
        }
    }
}
void AqpmBenchmark::benchmark_RetrieveAllPackagesWhenCached()
{
    QBENCHMARK {
        foreach (Aqpm::Database *db, Aqpm::Backend::instance()->availableDatabases()) {
            db->packages();
        }
        // Now for the local db
        foreach(Aqpm::Package *pkg, Aqpm::Backend::instance()->localDatabase()->packages()) {
            pkg->groups();
        }
        // Now groups
        foreach(Aqpm::Group *group, Aqpm::Backend::instance()->availableGroups()) {
            group->packages();
        }
    }
}

void AqpmBenchmark::benchmark_GetPackage()
{
    QBENCHMARK {
//         Aqpm::Package *pkg = Aqpm::Backend::instance()->package("pacman", QString());
//         QVERIFY(pkg);
//         qDebug() << pkg->name() << pkg->desc() << pkg->database()->name();
    }
}

void AqpmBenchmark::benchmark_ReloadAndRecache()
{
    QBENCHMARK {
        Aqpm::Backend::instance()->reloadPacmanConfiguration();

        foreach (Aqpm::Database *db, Aqpm::Backend::instance()->availableDatabases()) {
            db->packages();
        }
        // Now for the local db
        foreach(Aqpm::Package *pkg, Aqpm::Backend::instance()->localDatabase()->packages()) {
            pkg->groups();
        }
        // Now groups
        foreach(Aqpm::Group *group, Aqpm::Backend::instance()->availableGroups()) {
            group->packages();
        }
    }
}

QTEST_MAIN(AqpmBenchmark)
#include "AqpmBenchmark.moc"
