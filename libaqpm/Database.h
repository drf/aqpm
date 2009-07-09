/***************************************************************************
 *   Copyright (C) 2009 by Dario Freddi                                    *
 *   drf54321@gmail.com                                                    *
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

#ifndef DATABASE_H
#define DATABASE_H

#include <QList>

#include "Package.h"
#include "Group.h"

typedef struct __pmdb_t pmdb_t;

namespace Aqpm {

class Database
{
public:

    typedef QList<Database> List;

    Database();
    Database(pmdb_t *db);

    QString name() const;
    QString path() const;
    QStringList servers() const;
    pmdb_t *alpmDatabase() const;
    bool isValid() const;

private:
    class Private;
    Private *d;
};

}

#endif // DATABASE_H
