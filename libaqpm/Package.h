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

#include <QtCore/QMetaType>
#include <QtCore/QString>
#include <QtCore/QDateTime>
#include "Group.h"
#include <QtCore/QStringList>

typedef struct __pmpkg_t pmpkg_t;

namespace Aqpm
{
class Database;

class AQPM_EXPORT Package
{
public:
    virtual ~Package();

    typedef QList<Package*> List;

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
    bool hasScriptlet() const;
    QDateTime buildDate() const;
    QDateTime installDate() const;
    qint32 size() const;
    qint32 isize() const;
    QStringList files() const;

    Package::List dependsOn() const;
    Package::List requiredBy() const;
    Group::List groups() const;
    QStringList providers() const;
    Database *database(bool checkver = false);
    bool isInstalled();

    pmpkg_t *alpmPackage() const;
    bool isValid() const;

    QString retrieveChangelog() const;
    QString retrieveLoggedActions() const;

private:
    explicit Package(pmpkg_t *pkg);

    class Private;
    Private *d;

    friend class BackendThread;
};

}

Q_DECLARE_METATYPE(Aqpm::Package*)
Q_DECLARE_METATYPE(Aqpm::Package::List)

#endif // PACKAGE_H
