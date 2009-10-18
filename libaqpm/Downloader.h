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

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>
#include <QtDBus/QDBusContext>
#include <QtDBus/QDBusVariant>

class QNetworkReply;

namespace Aqpm
{

class Downloader : public QObject
{
    Q_OBJECT

public:
    /**
    * @return The current instance of the Backend
    */
    static Downloader *instance();
    virtual ~Downloader();

public Q_SLOTS:
    int checkHeader(const QString &url);
    QString download(const QString &url) const;
    void abortDownload();

private Q_SLOTS:
    void downloadFinished(QNetworkReply *reply);
    void progress(qint64 done, qint64 total);

Q_SIGNALS:
    void downloadProgress(int, int);
    void finished(const QString &url);
    void headCheckFinished(QNetworkReply *reply);

private:
    Downloader(QObject *parent = 0);

    class Private;
    Private *d;
};

}

#endif /* DOWNLOADER_H */
