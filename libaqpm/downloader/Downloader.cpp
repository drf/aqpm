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

#include "Downloader.h"

#include "aqpmdownloaderadaptor.h"

#include <config-aqpm.h>

#include <QtDBus/QDBusConnection>
#include <QTimer>
#include <QDebug>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkProxy>

namespace AqpmDownloader
{

class Downloader::Private
{
public:
    Private() {}

    QNetworkRequest createNetworkRequest(const QUrl &url);

    QTimer *timeout;
    QNetworkAccessManager *manager;
    QList<QNetworkReply*> replies;
};

QNetworkRequest Downloader::Private::createNetworkRequest(const QUrl &url)
{
    QNetworkRequest request;
    request.setUrl(url);
    request.setRawHeader("User-Agent", ("Aqpm/" + QString(AQPM_VERSION)).toUtf8());
    return request;
}

Downloader::Downloader(QObject *parent)
        : QObject(parent)
        , d(new Private())
{
    new AqpmdownloaderAdaptor(this);

    if (!QDBusConnection::systemBus().registerService("org.chakraproject.aqpmdownloader")) {
        qDebug() << "another downloader is already running";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    if (!QDBusConnection::systemBus().registerObject("/Downloader", this)) {
        qDebug() << "unable to register service interface to dbus";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    d->manager = new QNetworkAccessManager(this);
    d->timeout = new QTimer(this);
    connect(d->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
    connect(d->timeout, SIGNAL(timeout()), QCoreApplication::instance(), SLOT(quit()));
    d->timeout->setInterval(8000);
    d->timeout->start();
}

Downloader::~Downloader()
{
    d->manager->deleteLater();
    d->timeout->deleteLater();
}

int Downloader::checkHeader(const QString &url)
{
    d->timeout->stop();

    qDebug() << "About to check";
    QNetworkReply *reply = d->manager->head(d->createNetworkRequest(QUrl(url)));
    qDebug() << "Getting started";
    QEventLoop e;
    connect(reply, SIGNAL(finished()), &e, SLOT(quit()));
    return reply->header(QNetworkRequest::LastModifiedHeader).toDateTime().toTime_t();
}

void Downloader::download(const QString &url, const QString &to)
{
    d->timeout->stop();

    qDebug() << "About to get";
    QNetworkReply *reply = d->manager->get(d->createNetworkRequest(QUrl(url)));
    qDebug() << "Getting started";
    reply->setProperty("to_FileName_p", to);
    d->replies.append(reply);
    connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(progress(qint64,qint64)));
}

void Downloader::downloadFinished(QNetworkReply *reply)
{
    QFile file(reply->property("to_FileName_p").toString());

    file.open(QIODevice::ReadWrite);
    file.write(reply->readAll());

    emit finished(reply->url().toString());

    d->replies.removeOne(reply);

    reply->deleteLater();

    if (d->replies.isEmpty()) {
        d->timeout->start();
    }
}

void Downloader::abortDownload()
{
    foreach (QNetworkReply *r, d->replies) {
        r->abort();
        r->deleteLater();
    }

    d->replies.clear();
    d->timeout->start();
}

void Downloader::progress(qint64 done, qint64 total)
{
    emit downloadProgress(done, total);
}

}
