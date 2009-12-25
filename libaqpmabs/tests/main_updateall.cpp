// This is an example not a library
/***************************************************************************
 *   Copyright (C) 2009 by Dario Freddi                                    *
 *   drf@kde.org                                                           *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "main_updateall.h"

void AbsTest::newOutput(const QString &output)
{
    qDebug() << output;
}

void AbsTest::updateFinished(bool result)
{
    if (!result) {
        qDebug() << "AqpmAbs: Update failed";
        QCoreApplication::quit();
        return;
    } else {
        qDebug() << "AqpmAbs: Update succeeded";
    }

    qDebug() << "Now listing all packages...";

    qDebug() << Aqpm::Abs::Backend::instance()->allPackages();

    qDebug() << "Preparing environment for 9base in current dir...";

    bool res = Aqpm::Abs::Backend::instance()->prepareBuildEnvironment("9base", QDir::currentPath() + "/9base");

    if (!res) {
        qDebug() << "AqpmAbs: Setup failed";
    } else {
        qDebug() << "AqpmAbs: Setup succeeded";
    }

    QCoreApplication::quit();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Aqpm::Abs::Backend::instance()->setShouldHandleAuthorization(true);

    AbsTest *test = new AbsTest;

    test->connect(Aqpm::Abs::Backend::instance(), SIGNAL(operationFinished(bool)), test, SLOT(updateFinished(bool)));
    test->connect(Aqpm::Abs::Backend::instance(), SIGNAL(operationProgress(QString)), test, SLOT(newOutput(QString)));

    qDebug() << "Now updating abs...";
    Aqpm::Abs::Backend::instance()->updateAll();

    return app.exec();
}

#include "main_updateall.moc"
