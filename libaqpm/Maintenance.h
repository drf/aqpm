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

#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include <QtCore/QObject>

namespace Aqpm {

class Maintenance : public QObject
{
    Q_OBJECT

public:
    enum Action {
        CleanUnusedDatabases = 1,
        CleanCache = 2,
        EmptyCache = 4,
        OptimizeDatabases = 8,
        CleanABS = 16,
        RemoveUnusedPackages = 32
    };

    static Maintenance *instance();

    virtual ~Maintenance();

    void performAction(Action type);

Q_SIGNALS:
    void actionPerformed(bool result);
    void actionOutput(const QString &output);

private:
    Maintenance();

    class Private;
    Private *d;

    Q_PRIVATE_SLOT(d, void __k__workerResult(bool result))
};

}

#endif // MAINTENANCE_H
