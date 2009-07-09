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

#ifndef PACKAGE_H
#define PACKAGE_H

#include "Visibility.h"

#include <QMetaType>
#include <QDBusArgument>
#include <QString>
#include <QDateTime>

typedef struct __pmpkg_t pmpkg_t;

namespace Aqpm {

class AQPM_EXPORT Package
{
public:

    typedef QList<Package> List;

    Package(pmpkg_t *pkg);
    Package();

    QString name() const;
    QString filename() const;
    QString version() const;
    QString desc() const;
    QString url() const;
    QDateTime installdate() const;
    QDateTime builddate() const;
    QString packager() const;
    QString md5sum() const;
    QString arch() const;
    qint32 size();
    qint32 isize();
    pmpkg_t *alpmPackage() const;

private:
    class Private;
    Private *d;
};

}

Q_DECLARE_METATYPE(Aqpm::Package)


#endif // PACKAGE_H
