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

#include <iostream>

#include "Worker.h"

#include <QCoreApplication>
#include <QStringList>

#include <alpm.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("chakra");
    QCoreApplication::setOrganizationDomain("chakra-project.org");
    QCoreApplication::setApplicationName("aqpmworker");
    QCoreApplication::setApplicationVersion("0.2");

    QStringList arguments = app.arguments();

    AqpmWorker::Worker *w = new AqpmWorker::Worker(0);

    app.exec();
}
