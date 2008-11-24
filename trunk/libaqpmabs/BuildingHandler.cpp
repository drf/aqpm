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

#include "BuildingHandler.h"

#include "ABSHandler.h"

#include <QStringList>
#include <QPointer>
#include <QDir>
#include <QDebug>

namespace Aqpm {

class BuildingHandler::Private
{
    public:
        Private()
         : working_dir("/tmp/aqpm/build")
        {};

        QStringList built_packages;
        BuildItem::List queue;
        BuildItem::List::const_iterator iterator;
        QPointer<QProcess> process;
        QString working_dir;
};

BuildingHandler::BuildingHandler()
 : d(new Private())
{
}

BuildingHandler::~BuildingHandler()
{
}

void BuildingHandler::addItemToQueue(BuildItem *itm)
{
    d->queue.append(itm);
}

void BuildingHandler::clearQueue()
{
    qDeleteAll(d->queue);
    d->queue.clear();
}

QStringList BuildingHandler::getBuiltPackages()
{
    return d->built_packages;
}

void BuildingHandler::clearBuiltPackages()
{
    d->built_packages.clear();
}

void BuildingHandler::processQueue()
{
    emit buildingStarted();
    d->iterator = d->queue.constBegin();
    QDir dir(d->working_dir);
    dir.mkpath(d->working_dir);
    processNextItem();
}

void BuildingHandler::processNextItem()
{
    if (d->process) {
        d->process->deleteLater();
    }

    d->process = new QProcess();

    /* Let's build the path we need */

    if ((*d->iterator)->env_path.isEmpty()) {
        (*d->iterator)->env_path = ABSHandler::getABSPath((*d->iterator)->name);
    }

    QString path = d->working_dir;

    if (!path.endsWith(QChar('/'))) {
        path.append(QChar('/'));
    }

    path.append((*d->iterator)->name);

    QDir dir(path);
    dir.mkpath(path);

    QDir from((*d->iterator)->env_path);

    from.setFilter( QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks );

    QFileInfoList Plist = from.entryInfoList();

    for ( int i = 0; i < Plist.size(); ++i ) {
        QString dest(path);

        if (!dest.endsWith(QChar('/'))) {
            dest.append( QChar( '/' ) );
        }

        dest.append(Plist.at(i).fileName());

        qDebug() << "Copying " << Plist.at( i ).absoluteFilePath() << " to " << dest;

        if (!QFile::copy(Plist.at(i).absoluteFilePath(), dest)) {
            qDebug() << "Copy failed";
        }
    }

    d->process->setWorkingDirectory( path );

    connect( d->process, SIGNAL( readyReadStandardOutput() ), SLOT( processOutput() ) );
    connect( d->process, SIGNAL( readyReadStandardError() ), SLOT( processOutput() ) );
    connect( d->process, SIGNAL( finished( int, QProcess::ExitStatus ) ), SLOT( finishedBuildingAction( int, QProcess::ExitStatus ) ) );

    d->process->start( "makepkg" );
}

}
