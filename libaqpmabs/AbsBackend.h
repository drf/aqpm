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

#ifndef ABSBACKEND_H
#define ABSBACKEND_H

#include "Visibility.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>

namespace Aqpm
{

namespace Abs
{

class AQPM_EXPORT Backend : public QObject
{
    Q_OBJECT

public:
    static Backend *instance();

    void update(const QStringList &targets, bool tarball = false);
    void updateAll(bool tarball = false);
    /**
     * Prepares
     */
    bool prepareBuildEnvironment(const QString &package, const QString &path, bool privileged = false) const;

    bool shouldHandleAuthorization() const;
    void setShouldHandleAuthorization(bool should);

    QString path(const QString &package) const;

    QStringList allPackages() const;

Q_SIGNALS:
    void operationFinished(bool success);
    void operationProgress(const QString &progress);

private:
    Backend(QObject* parent = 0);

    class Private;
    Private * const d;

    Q_PRIVATE_SLOT(d, void __k__operationFinished(bool result))
    Q_PRIVATE_SLOT(d, void __k__newOutput(const QString &output))
};

}

}

#endif // ABSBACKEND_H
