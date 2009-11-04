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

#include "main_builder.h"

void BuilderTest::newOutput(const QString &output)
{
    qDebug() << output;
}

void BuilderTest::buildFinished(bool result)
{
    if (!result) {
        qDebug() << "AqpmBuilder: Build failed";
    } else {
        qDebug() << "AqpmAbs: Build succeeded";
    }

    QCoreApplication::quit();
}

void BuilderTest::buildEnvironmentStatusChanged(const QString& package, Aqpm::Builder::Backend::Status status)
{
    qDebug() << "AqpmBuilder: Package " << package << " is now in status " << status;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (QCoreApplication::arguments().count() != 2) {
        printf("Usage: builder <buildenvironment>\n");
        return 0;
    }

    BuilderTest *test = new BuilderTest;

    if (!Aqpm::Builder::Backend::instance()->addBuildEnvironmentToQueue(QCoreApplication::arguments().at(1))) {
        qDebug() << "The build environment you supplied is not valid";
        return 1;
    }

    test->connect(Aqpm::Builder::Backend::instance(), SIGNAL(operationFinished(bool)), test, SLOT(buildFinished(bool)));
    test->connect(Aqpm::Builder::Backend::instance(), SIGNAL(operationProgress(QString)), test, SLOT(newOutput(QString)));
    test->connect(Aqpm::Builder::Backend::instance(), SIGNAL(buildEnvironmentStatusChanged(QString,Aqpm::Builder::Backend::Status)),
                  test, SLOT(buildEnvironmentStatusChanged(QString,Aqpm::Builder::Backend::Status)));
    qDebug() << "Makedepends are " << Aqpm::Builder::Backend::instance()->makeDependsForQueue();
    qDebug() << "Depends are " << Aqpm::Builder::Backend::instance()->dependsForQueue();
    qDebug() << "Now building packages...";
    Aqpm::Builder::Backend::instance()->processQueue();

    return app.exec();
}

#include "main_builder.moc"
