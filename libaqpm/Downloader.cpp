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
    Private() {}

    QNetworkRequest createNetworkRequest(const QUrl &url);

    AqpmNetworkAccessManager *manager;
    QList<QNetworkReply*> replies;
};

StringConditionalEventLoop::StringConditionalEventLoop(const QString &str, QObject *parent)
        : QEventLoop(parent)
        , m_cond(str)
{
}

StringConditionalEventLoop::~StringConditionalEventLoop()
{
}

void StringConditionalEventLoop::requestQuit(const QString &str)
{
    if (str == m_cond) {
        quit();
    }
}

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

Downloader::Downloader(QObject *parent)
        : QObject(parent)
        , d(new Private())
{
    Q_ASSERT(!s_globalDownloader()->q);
    s_globalDownloader()->q = this;

    new AqpmdownloaderAdaptor(this);

    if (!QDBusConnection::systemBus().registerObject("/Downloader", this)) {
        qDebug() << "unable to register service interface to dbus";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    d->manager = new AqpmNetworkAccessManager(this);
    connect(d->manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(downloadFinished(QNetworkReply*)));
}

Downloader::~Downloader()
{
    d->manager->deleteLater();
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

QString Downloader::download(const QString& url) const
{
    qDebug() << "About to get " << url;
    QNetworkReply *reply = d->manager->get(d->createNetworkRequest(QUrl(url)));
    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progress(qint64,qint64)));
    qDebug() << "Getting started";
    d->replies.append(reply);

    StringConditionalEventLoop e(reply->url().toString());
    connect(this, SIGNAL(finished(QString)), &e, SLOT(requestQuit(QString)));
    e.exec();

    qDebug() << "Getting done";

    QString retstring = reply->property("aqpm_Temporary_Download_Location").toString();
    reply->deleteLater();
    return retstring;
}

void Downloader::downloadFinished(QNetworkReply *reply)
{
    if (reply->property("is_Header_Check").toBool()) {
        // First of all notify that the head check has taken place
        emit headCheckFinished(reply);

        return;
    }

    QTemporaryFile file;
    file.setAutoRemove(false);

    file.open();
    file.write(reply->readAll());
    file.flush();
    file.close();

    // Set the filename as the property for the reply
    reply->setProperty("aqpm_Temporary_Download_Location", file.fileName());

    emit finished(reply->url().toString());

    d->replies.removeOne(reply);

    // The reply should be deleted by download Function
}

void Downloader::abortDownload()
{
    foreach (QNetworkReply *r, d->replies) {
        r->abort();
        r->deleteLater();
    }

    d->replies.clear();
}

void Downloader::progress(qint64 done, qint64 total)
{
#ifndef KDE4_INTEGRATION
    emit downloadProgress(done, total, qobject_cast<QNetworkReply*>(sender())->url().toString().split('/').last());
#else
    qDebug() << done << total;
    emit downloadProgress(done, total, qobject_cast<QNetworkReply*>(sender())->url().toString().split('/').last());
#endif
}

}
