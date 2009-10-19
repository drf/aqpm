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

#include "AurBackend.h"

namespace Aqpm {

namespace Aur {

class AurBackendHelper
{
    public:
        AurBackendHelper() : q(0) {}
        ~AurBackendHelper() {
            delete q;
        }
        Backend *q;
};

Q_GLOBAL_STATIC(AurBackendHelper, s_globalAurBackend)

Backend *Backend::instance()
{
    if (!s_globalAurBackend()->q) {
        new Backend;
    }

    return s_globalAurBackend()->q;
}

Backend::Backend(QObject* parent)
        : QObject(parent)
{
    Q_ASSERT(!s_globalAurBackend()->q);
    s_globalAurBackend()->q = this;
}

Backend::~Backend()
{

}

}
}
