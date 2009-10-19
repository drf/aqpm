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

#include "AurBackend.h"

#include <config-aqpm.h>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#ifndef KDE4_INTEGRATION
#include <QtNetwork/QNetworkAccessManager>
typedef QNetworkAccessManager AqpmNetworkAccessManager;
#else
#include <kio/accessmanager.h>
typedef KIO::AccessManager AqpmNetworkAccessManager;
#endif

namespace Aqpm {

namespace Aur {

class Backend::Private
{
    public:
        Private() : manager(new AqpmNetworkAccessManager) {}

        QNetworkRequest createNetworkRequest(const QString &type, const QString &arg) const;

        // Private slots
        void __k__replyFinished(QNetworkReply *reply);

        AqpmNetworkAccessManager *manager;
};

QNetworkRequest Backend::Private::createNetworkRequest(const QString& type, const QString& arg) const
{
    QNetworkRequest request;
    QUrl url(AQPM_AUR_REQUEST_URL + QString("type=%1&arg=%2").arg(type).arg(arg));
    request.setUrl(url);
    request.setRawHeader("User-Agent", ("Aqpm/" + QString(AQPM_VERSION)).toUtf8());
    return request;
}

class AurBackendHelper
{
    public:
        AurBackendHelper() : q(0) {}
        ~AurBackendHelper() {
            delete q;
        }
        Backend *q;
};

Q_GLOBAL_STATIC(AurBackendHelper, s_globalAurBackend)

Backend *Backend::instance()
{
    if (!s_globalAurBackend()->q) {
        new Backend;
    }

    return s_globalAurBackend()->q;
}

Backend::Backend(QObject* parent)
        : QObject(parent)
        , d(new Private)
{
    Q_ASSERT(!s_globalAurBackend()->q);
    s_globalAurBackend()->q = this;

    connect(d->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(__k__replyFinished(QNetworkReply*)));
}

Backend::~Backend()
{
    d->manager->deleteLater();
    delete d;
}

void Backend::search(const QString& subject)
{
    // Stream the request
    QNetworkReply *reply = d->manager->get(d->createNetworkRequest("search", subject));
    reply->setProperty("aqpm_AUR_Request_Type", "search");
}

void Backend::info(int id)
{

}

}
}

#include "AurBackend.moc"
