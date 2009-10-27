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

#ifndef DOWNLOADER_P_H
#define DOWNLOADER_P_H

#include <QEventLoop>
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
    static bool hasInstance();
    static void destroyInstance();

    virtual ~Downloader();

    bool hasError() const;

public Q_SLOTS:
    int checkHeader(const QString &url);
    void download(const QString &url);
    void abortDownload();

Q_SIGNALS:
    void downloadProgress(qlonglong, qlonglong, QString);
    void finished(const QString &url, const QString &filename);
    void headCheckFinished(QNetworkReply *reply);

private Q_SLOTS:
    void downloadFinished(QNetworkReply *reply);
    void progress(qint64 done, qint64 total);

private:
    Downloader(QObject *parent = 0);

    class Private;
    Private *d;
};

}

#endif /* DOWNLOADER_P_H */
