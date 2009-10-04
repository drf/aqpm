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

#ifndef QUEUEITEM_H
#define QUEUEITEM_H

#include "Visibility.h"

#include <QtCore/QMetaType>
#include <QtDBus/QDBusArgument>

namespace Aqpm
{

class AQPM_EXPORT QueueItem
{
public:
    /**
     * Defines the action that will be done on the item.
     */
    enum Action {
        /** Sync operation (install/upgrade) */
        Sync = 1,
        /** Remove operation */
        Remove = 2,
        /** From file operation */
        FromFile = 4,
        /**
         * When adding an item with this token, all others will be ignored
         * and the queue populated with all upgradeable packages
         */
        FullUpgrade = 8
    };

    QueueItem();
    /**
     * Constructs a queue item with the needed data
     *
     * @param n The name of the package
     * @param a The action that should be done on it
     */
    QueueItem(QString n, Action a);

    /**
     * A list of QueueItem, convenience TypeDef
     */
    typedef QList<QueueItem> List;

    QString name;
    uint action_id;
};

}

Q_DECLARE_METATYPE(Aqpm::QueueItem)
Q_DECLARE_METATYPE(Aqpm::QueueItem::List)
Q_DECLARE_METATYPE(Aqpm::QueueItem::Action)

QDBusArgument &operator<<(QDBusArgument &argument, const Aqpm::QueueItem &item);
const QDBusArgument &operator>>(const QDBusArgument &argument, Aqpm::QueueItem &item);

#endif /* QUEUEITEM_H */
