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

#include "Maintenance.h"

namespace Aqpm
{

class Maintenance::Private
{
public:
    Private() {}
};

class MaintenanceHelper
{
public:
    MaintenanceHelper() : q(0) {}
    ~MaintenanceHelper() {
        delete q;
    }
    Maintenance *q;
};

Q_GLOBAL_STATIC(MaintenanceHelper, s_globalMaintenance)

Maintenance *Maintenance::instance()
{
    if (!s_globalMaintenance()->q) {
        new Maintenance;
    }

    return s_globalMaintenance()->q;
}

Maintenance::Maintenance()
        : QObject(0)
        , d(new Private())
{
    Q_ASSERT(!s_globalMaintenance()->q);
    s_globalMaintenance()->q = this;
}

Maintenance::~Maintenance()
{
    delete d;
}

}
