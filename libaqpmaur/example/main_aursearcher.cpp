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
#include <QDebug>

#include "../AurBackend.h"
#include <QStringList>
#include <QDir>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (QCoreApplication::arguments().count() != 3) {
        printf("Usage: aursearcher (search|info|download) <package>\n<package> is a string for search, "
               "and a number (the package ID) for the other commands\n");
        return 0;
    }

    if (QCoreApplication::arguments().at(1) == "search") {
        Aqpm::Aur::Package::List retlist = Aqpm::Aur::Backend::instance()->searchSync(QCoreApplication::arguments().at(2));

        foreach (const Aqpm::Aur::Package &result, retlist) {
            printf("%s - %s (ID: %i)\n", result.name.toAscii().data(), result.version.toAscii().data(), result.id);
        }
    } else if (QCoreApplication::arguments().at(1) == "info") {
        Aqpm::Aur::Package package = Aqpm::Aur::Backend::instance()->infoSync(QCoreApplication::arguments().at(2).toInt());

        printf("Name: %s\nVersion: %s\nDescription: %s\nUrl: %s\nVotes: %i\n%s\n", package.name.toAscii().data(),
               package.version.toAscii().data(), package.description.toAscii().data(), package.url.toAscii().data(),
               package.votes, package.outOfDate ? "The package is out of date" : "");
    } else if (QCoreApplication::arguments().at(1) == "download") {
        Aqpm::Aur::Backend::instance()->prepareBuildEnvironmentSync(QCoreApplication::arguments().at(2).toInt(), QDir::currentPath());
    } else {
        printf("Usage: aursearcher (search|info|download) <package>\n<package> is a string for search, "
               "and a number (the package ID) for the other commands\n");
    }

    return 0;
}
