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

#include "Downloader_p.h"

#include "aqpmdownloaderadaptor.h"
#include "Loops_p.h"

#include <config-aqpm.h>

#include <QtDBus/QDBusConnection>
#include <QTimer>
#include <QDebug>

#ifndef KDE4_INTEGRATION
#include <QtNetwork/QNetworkAccessManager>
typedef QNetworkAccessManager AqpmNetworkAccessManager;
#else
#include <kio/accessmanager.h>
typedef KIO::AccessManager AqpmNetworkAccessManager;
#endif

#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkProxy>

namespace Aqpm
{

class Downloader::Private
{
public:
    Private() : error(false) {}

    QNetworkRequest createNetworkRequest(const QUrl &url);

    AqpmNetworkAccessManager *manager;
    QList<QNetworkReply*> replies;
    bool error;
    QTime busControlTime;
};

QNetworkRequest Downloader::Private::createNetworkRequest(const QUrl &url)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", ("Aqpm/" + QString(AQPM_VERSION)).toUtf8());
    return request;
}

class DownloaderHelper
{
public:
    DownloaderHelper() : q(0) {}
    ~DownloaderHelper() {
        delete q;
    }
    Downloader *q;
};

Q_GLOBAL_STATIC(DownloaderHelper, s_globalDownloader)

Downloader *Downloader::instance()
{
    if (!s_globalDownloader()->q) {
        new Downloader;
    }

    return s_globalDownloader()->q;
}

void Downloader::destroyInstance()
{
    if (s_globalDownloader()->q) {
        s_globalDownloader()->q->deleteLater();
        s_globalDownloader()->q = 0;
    }
}

bool Downloader::hasInstance()
{
    return s_globalDownloader()->q;
}

Downloader::Downloader(QObject *parent)
        : QObject(parent)
        , d(new Private())
{
    Q_ASSERT(!s_globalDownloader()->q);
    s_globalDownloader()->q = this;

    new AqpmdownloaderAdaptor(this);

    if (!QDBusConnection::systemBus().registerService("org.chakraproject.aqpmdownloader")) {
        qDebug() << "unable to register the downloader service";
        d->error = true;
    }

    if (!QDBusConnection::systemBus().registerObject("/Downloader", this)) {
        qDebug() << "unable to register service interface to dbus";
        d->error = true;
    }

    d->manager = new AqpmNetworkAccessManager(this);
    connect(d->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
}

Downloader::~Downloader()
{
    QDBusConnection::systemBus().unregisterService("org.chakraproject.aqpmdownloader");
    d->manager->deleteLater();
}

bool Downloader::hasError() const
{
    return d->error;
}

int Downloader::checkHeader(const QString &url)
{
    qDebug() << "About to check";
    QNetworkReply *reply = d->manager->head(d->createNetworkRequest(QUrl(url)));
    reply->setProperty("is_Header_Check", true);
    qDebug() << "Head check done";
    QEventLoop e;
    connect(reply, SIGNAL(finished()), &e, SLOT(quit()));
    e.exec();
    return reply->header(QNetworkRequest::LastModifiedHeader).toDateTime().toTime_t();
}

void Downloader::download(const QString& url)
{
    qDebug() << "About to get " << url;
    QNetworkReply *reply = d->manager->get(d->createNetworkRequest(QUrl(url)));
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(progress(qint64, qint64)));
    qDebug() << "Getting started";
    d->replies.append(reply);
}

void Downloader::downloadFinished(QNetworkReply *reply)
{
    if (reply->property("is_Header_Check").toBool()) {
        // First of all notify that the head check has taken place
        emit headCheckFinished(reply);

        return;
    }
    qDebug() << "Download finished";
    QTemporaryFile file;
    file.setAutoRemove(false);

    file.open();
    file.write(reply->readAll());
    file.flush();
    file.close();

    emit finished(reply->url().toString(), file.fileName());

    // Remove the reply from the list and delete it
    d->replies.removeOne(reply);
    reply->deleteLater();
}

void Downloader::abortDownload()
{
    foreach(QNetworkReply *r, d->replies) {
        // Emit a finished signal with empty path
        emit finished(r->url().toString(), QString());
        r->abort();
        r->deleteLater();
    }

    d->replies.clear();
}

void Downloader::progress(qint64 done, qint64 total)
{
    // Don't you fucking hog the bus! Update maximum each 0.250 seconds
    if (d->busControlTime.msecsTo(QTime::currentTime()) > 250 || !d->busControlTime.isValid()) {
        emit downloadProgress(done, total,
                              qobject_cast<QNetworkReply*>(sender())->url().toString().split('/').last());
        d->busControlTime = QTime::currentTime();
    }
}

}
