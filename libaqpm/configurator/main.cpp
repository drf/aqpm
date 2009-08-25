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

#include "Configurator.h"

#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include <config-aqpm.h>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("chakra");
    QCoreApplication::setOrganizationDomain("chakra-project.org");
    QCoreApplication::setApplicationName("aqpmconfigurator");
    QCoreApplication::setApplicationVersion(AQPM_VERSION);

    QStringList arguments = app.arguments();

    bool tmprz = true;

    if (arguments.contains("--no-timeout")) {
        tmprz = false;
    }

    AqpmConfigurator::Configurator dwn(tmprz);

    app.exec();
}
