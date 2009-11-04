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

#ifndef GROUP_H
#define GROUP_H

#include <QtCore/QString>
#include <QtCore/QMetaType>

#include "Visibility.h"

typedef struct __pmgrp_t pmgrp_t;

namespace Aqpm
{

class AQPM_EXPORT Group
{
public:
    typedef QList<Group> List;

    Group(pmgrp_t *grp);
    Group();

    QString name() const;
    pmgrp_t *alpmGroup() const;
    bool isValid() const;

private:
    class Private;
    Private *d;
};

}

Q_DECLARE_METATYPE(Aqpm::Group)
Q_DECLARE_METATYPE(Aqpm::Group::List)

#endif // GROUP_H
