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

#ifndef AURBACKEND_H
#define AURBACKEND_H

#include <QtCore/QObject>

class QNetworkReply;
namespace Aqpm {

namespace Aur {

class Package
{
    public:
        typedef QList<Package> List;
};

class Backend : public QObject
{
    Q_OBJECT

public:
    static Backend *instance();

    virtual ~Backend();

    void search(const QString &subject);
    void info(int id);

Q_SIGNALS:
    void searchCompleted(const QString &searchSubject, const Package::List &results);
    void infoCompleted(int id, const Package &result);

private:
    Backend(QObject* parent = 0);

    class Private;
    Private *d;

    Q_PRIVATE_SLOT(d, void __k__replyFinished(QNetworkReply *reply))
};

}
}

#endif // AURBACKEND_H
