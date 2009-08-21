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

#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusVariant>

class QNetworkReply;

namespace AqpmDownloader
{

class Downloader : public QObject, protected QDBusContext
{
    Q_OBJECT

public:
    Downloader(QObject *parent = 0);
    virtual ~Downloader();

public Q_SLOTS:
    int checkHeader(const QString &url);
    void download(const QString &url, const QString &to);
    void abortDownload();

private Q_SLOTS:
    void downloadFinished(QNetworkReply *reply);
    void progress(qint64 done, qint64 total);

Q_SIGNALS:
    void downloadProgress(int, int);
    void finished(const QString &url);

private:
    class Private;
    Private *d;
};

}

#endif /* WORKER_H */
