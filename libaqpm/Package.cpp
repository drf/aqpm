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

#include "Package.h"

#include <alpm.h>

using namespace Aqpm;

class Package::Private {
    public:

    Private(pmpkg_t *pkg);

    pmpkg_t *underlying;
};

Package::Private::Private(pmpkg_t *pkg)
{
    underlying = pkg;
}

Package::Package(pmpkg_t *pkg)
        : d(new Private(pkg))
{
}

Package::Package()
        : d(new Private(NULL))
{
}



QString Package::arch() const
{
    return alpm_pkg_get_arch(d->underlying);
}

QDateTime Package::builddate() const
{
    return QDateTime::fromTime_t(alpm_pkg_get_builddate(d->underlying));
}

QDateTime Package::installdate() const
{
    return QDateTime::fromTime_t(alpm_pkg_get_installdate(d->underlying));
}

QString Package::desc() const
{
    return alpm_pkg_get_desc(d->underlying);
}

QString Package::filename() const
{
    return alpm_pkg_get_filename(d->underlying);
}

qint32 Package::isize()
{
    return alpm_pkg_get_isize(d->underlying);
}

QString Package::md5sum() const
{
    return alpm_pkg_get_md5sum(d->underlying);
}

QString Package::name() const
{
    return alpm_pkg_get_name(d->underlying);
}

QString Package::packager() const
{
    return alpm_pkg_get_packager(d->underlying);
}

qint32 Package::size()
{
    return alpm_pkg_get_size(d->underlying);
}

QString Package::url() const
{
    return alpm_pkg_get_url(d->underlying);
}

QString Package::version() const
{
    return alpm_pkg_get_version(d->underlying);
}

pmpkg_t *Package::alpmPackage() const
{
    return d->underlying;
}

bool Package::operator==(const Package &pkg)
{
    return pkg.alpmPackage() == d->underlying;
}
