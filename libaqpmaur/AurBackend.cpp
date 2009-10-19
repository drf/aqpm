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

#include "PackageLoops_p.h"

#include <config-aqpm.h>

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <QTemporaryFile>
#include <QDir>
#include <QProcess>

#include <qjson/parser.h>

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
        QNetworkRequest createDownloadRequest(const Package &package) const;
        Package packageFromMap(const QVariantMap &map) const;

        // Private slots
        void __k__replyFinished(QNetworkReply *reply);

        AqpmNetworkAccessManager *manager;
        Backend *q;
};

QNetworkRequest Backend::Private::createNetworkRequest(const QString& type, const QString& arg) const
{
    QNetworkRequest request;
    QUrl url(AQPM_AUR_REQUEST_URL + QString("type=%1&arg=%2").arg(type).arg(arg));
    request.setUrl(url);
    request.setRawHeader("User-Agent", ("Aqpm/" + QString(AQPM_VERSION)).toUtf8());
    return request;
}

QNetworkRequest Backend::Private::createDownloadRequest(const Aqpm::Aur::Package& package) const
{
    QNetworkRequest request;
    QUrl url(AQPM_AUR_BASE_URL + package.path);
    request.setUrl(url);
    request.setRawHeader("User-Agent", ("Aqpm/" + QString(AQPM_VERSION)).toUtf8());
    return request;
}

void Backend::Private::__k__replyFinished(QNetworkReply* reply)
{
    if (reply->property("aqpm_AUR_is_archive_Download").toBool()) {
        // We got an archive. Let's save and extract it where requested
        QTemporaryFile file;
        file.open();
        file.write(reply->readAll());
        file.flush();

        QString filepath = QDir::tempPath() + '/' + file.fileName();
        QProcess process;
        process.setWorkingDirectory(reply->property("aqpm_AUR_extract_path").toString());
        process.start(QString("tar -zxf %1").arg(filepath));
        process.waitForFinished();
        file.close();

        // Stream the success
        emit q->buildEnvironmentReady(reply->property("aqpm_AUR_ID").toInt(), reply->property("aqpm_AUR_pkg_name").toString());
        return;
    }

    // Parse it!
    QJson::Parser parser;
    bool ok;
    QVariantMap result = parser.parse(reply, &ok).toMap();

    if (!ok || result["type"].toString() != reply->property("aqpm_AUR_Request_Type").toString()) {
        // Shit.
        return;
    }

    if (reply->property("aqpm_AUR_Request_Type").toString() == "search") {
        Package::List retlist;

        foreach (const QVariant &ent, result["results"].toList()) {
            QVariantMap valuesMap = ent.toMap();
            retlist.append(packageFromMap(valuesMap));
        }

        emit q->searchCompleted(reply->property("aqpm_AUR_Subject").toString(), retlist);
    } else if (reply->property("aqpm_AUR_Request_Type").toString() == "info") {
        if (reply->property("aqpm_AUR_is_Download").toBool()) {
            // We are not really looking for info, we actually need to download the package we got.
            Package p = packageFromMap(result["results"].toMap());
            QNetworkReply *nreply = manager->get(createDownloadRequest(p));
            nreply->setProperty("aqpm_AUR_is_archive_Download", true);
            nreply->setProperty("aqpm_AUR_extract_path", reply->property("aqpm_AUR_extract_path").toString());
            nreply->setProperty("aqpm_AUR_ID", reply->property("aqpm_AUR_ID").toInt());
            nreply->setProperty("aqpm_AUR_pkg_name", p.name);
        } else {
            // A simple info request, just notify and pass by
            emit q->infoCompleted(reply->property("aqpm_AUR_ID").toInt(), packageFromMap(result["results"].toMap()));
        }
    }
}

Package Backend::Private::packageFromMap(const QVariantMap& map) const
{
    Package p;
    p.category = map["CategoryID"].toInt();
    p.description = map["Description"].toString();
    p.id = map["ID"].toInt();
    p.license = map["License"].toString();
    p.location = map["LocationID"].toInt();
    p.name = map["Name"].toString();
    p.outOfDate = map["OutOfDate"].toInt() == 0 ? true : false;
    p.path = map["URLPath"].toString();
    p.url = map["URL"].toString();
    p.version = map["Version"].toString();
    p.votes = map["NumVotes"].toInt();

    return p;
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

    d->q = this;
    connect(d->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(__k__replyFinished(QNetworkReply*)));
}

Backend::~Backend()
{
    d->manager->deleteLater();
    delete d;
}

void Backend::search(const QString& subject) const
{
    // Stream the request
    QNetworkReply *reply = d->manager->get(d->createNetworkRequest("search", subject));
    reply->setProperty("aqpm_AUR_Request_Type", "search");
    reply->setProperty("aqpm_AUR_Subject", subject);
}

void Backend::info(int id) const
{
    // Stream the request
    QNetworkReply *reply = d->manager->get(d->createNetworkRequest("info", QString::number(id)));
    reply->setProperty("aqpm_AUR_Request_Type", "info");
    reply->setProperty("aqpm_AUR_ID", id);
}

Package::List Backend::searchSync(const QString& subject) const
{
    PackageListConditionalEventLoop e(subject);
    connect(this, SIGNAL(searchCompleted(QString,Aqpm::Aur::Package::List)), &e, SLOT(requestQuit(QString,Aqpm::Aur::Package::List)));
    search(subject);
    e.exec();
    return e.packageList();
}

Package Backend::infoSync(int id) const
{
    PackageConditionalEventLoop e(id);
    connect(this, SIGNAL(infoCompleted(int,Aqpm::Aur::Package)), &e, SLOT(requestQuit(int,Aqpm::Aur::Package)));
    info(id);
    e.exec();
    return e.package();
}

void Backend::prepareBuildEnvironment(int id, const QString& envpath) const
{
    // First we have to do an info request to get the URL. We will add an additional property
    QNetworkReply *reply = d->manager->get(d->createNetworkRequest("info", QString::number(id)));
    reply->setProperty("aqpm_AUR_Request_Type", "info");
    reply->setProperty("aqpm_AUR_ID", id);
    reply->setProperty("aqpm_AUR_is_Download", true);
    reply->setProperty("aqpm_AUR_extract_path", envpath);
}

}
}

#include "AurBackend.moc"
