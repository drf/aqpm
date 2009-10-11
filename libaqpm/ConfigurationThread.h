/***************************************************************************
 *   Copyright (C) 2009 by Dario Freddi                                    *
 *   drf@kde.org                                                           *
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

#ifndef CONFIGURATIONTHREAD_H
#define CONFIGURATIONTHREAD_H

#include <QObject>
#include <QVariantMap>

#include "Configuration.h"

namespace Aqpm {

class ConfigurationThread : public QObject
{
    Q_OBJECT

public:
    ConfigurationThread();
    virtual ~ConfigurationThread();

    void setAqpmRoot(const QString &root);

private:
    bool saveConfiguration();
    void saveConfigurationAsync();

    void setValue(const QString &key, const QVariant &val);
    QVariant value(const QString &key);

    QStringList databases();
    QString getServerForDatabase(const QString &db);
    QStringList getServersForDatabase(const QString &db);

    QStringList getMirrorList(Configuration::MirrorType type);

    bool addMirrorToMirrorList(const QString &mirror, Configuration::MirrorType type);
    void addMirrorToMirrorListAsync(const QString &mirror, Configuration::MirrorType type);

    QStringList mirrors(bool thirdpartyonly = false);
    QStringList serversForMirror(const QString &mirror);
    QStringList databasesForMirror(const QString &mirror);

    void setDatabases(const QStringList &databases);
    void setDatabasesForMirror(const QStringList &database, const QString &mirror);
    void setServersForMirror(const QStringList &servers, const QString &mirror);

    void remove(const QString &key);
    bool exists(const QString &key, const QString &val = QString());

    void setOrUnset(bool set, const QString &key, const QString &val = QString());

private Q_SLOTS:
    void configuratorResult(bool result);
    void reload();

protected:
    void customEvent(QEvent *event);

Q_SIGNALS:
    void configurationSaved(bool result);
    void actionPerformed(int type, QVariantMap result);

private:
    class Private;
    Private *d;
};

}

#endif // CONFIGURATIONTHREAD_H
