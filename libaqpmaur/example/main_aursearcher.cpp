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

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if (QCoreApplication::arguments().count() < 2) {
        qFatal("You need to specify a name to search for");
    }

    Aqpm::Aur::Package::List retlist = Aqpm::Aur::Backend::instance()->searchSync(QCoreApplication::arguments().at(1));

    foreach (const Aqpm::Aur::Package &result, retlist) {
        printf("%s - %s\n", result.name.toAscii().data(), result.version.toAscii().data());
    }

    return 0;
}
