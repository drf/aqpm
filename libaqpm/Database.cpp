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

#include "Database.h"

#include <alpm.h>

using namespace Aqpm;

class Database::Private
{
public:
    Private(pmdb_t *db);

    pmdb_t *underlying;

};

Database::Private::Private(pmdb_t *db)
        : underlying(db)
{
}

Database::Database()
        : d(new Private(NULL))
{
}

Database::Database(pmdb_t *db)
        : d(new Private(db))
{
}

QString Database::name() const
{
    return alpm_db_get_name(d->underlying);
}

QString Database::path() const
{
    return alpm_db_get_url(d->underlying);
}

pmdb_t *Database::alpmDatabase() const
{
    return d->underlying;
}

bool Database::isValid() const
{
    return d->underlying != NULL;
}
