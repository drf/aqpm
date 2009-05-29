/***************************************************************************
 *   Copyright (C) 2009 by Dario Freddi                                    *
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

#include "QueueItem.h"

namespace Aqpm
{

QueueItem::QueueItem()
{
}

QueueItem::QueueItem(QString n, Action a)
            : name(n),
            action_id(a)
{
}

}

// Marshall the WicdConnectionInfo data into a D-BUS argument
QDBusArgument &operator<<(QDBusArgument &argument, const Aqpm::QueueItem &item)
{
    argument.beginStructure();
    argument << (int)(item.action_id) << item.name;
    argument.endStructure();
    return argument;
}

// Retrieve the WicdConnectionInfo data from the D-BUS argument
const QDBusArgument &operator>>(const QDBusArgument &argument, Aqpm::QueueItem &item)
{
    argument.beginStructure();
    argument >> item.action_id >> item.name;
    argument.endStructure();
    return argument;
}
