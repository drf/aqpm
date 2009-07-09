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

#include "Group.h"

#include <alpm.h>

using namespace Aqpm;

class Group::Private
{
    public:
    Private(pmgrp_t *grp) : underlying(grp) {}

    pmgrp_t *underlying;
};

Group::Group(pmgrp_t *grp)
        : d(new Private(grp))
{
}

QString Group::name() const
{
    return alpm_grp_get_name(d->underlying);
}

pmgrp_t *Group::alpmGroup() const
{
    return d->underlying;
}
