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

#include <QApplication>
#include <QEventLoop>
#include <QDebug>

#include "../Backend.h"
#include "../Database.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QEventLoop e;

    Aqpm::Backend::instance()->safeInitialization();
    Aqpm::Backend::instance()->setShouldHandleAuthorization(true);

    // Load twice
    Aqpm::Backend::instance()->reloadPacmanConfiguration();

    foreach(Aqpm::Database *db, Aqpm::Backend::instance()->availableDatabases()) {
        qDebug() << db->name();
    }

    qDebug() << "Now updating databases...";
    Aqpm::Backend::instance()->updateDatabase();

    e.connect(Aqpm::Backend::instance(), SIGNAL(operationFinished(bool)), &e, SLOT(quit()));
    e.connect(Aqpm::Backend::instance(), SIGNAL(errorOccurred(Aqpm::Globals::Errors, QVariantMap)),
              QCoreApplication::instance(), SLOT(quit()));
    e.exec();

    qDebug() << "Done";

    return 0;
}
