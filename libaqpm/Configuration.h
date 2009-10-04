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

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <QtCore/QObject>
#include <QtCore/QEvent>

namespace Aqpm
{

class ConfigurationThread;

class Configuration : public QObject
{
    Q_OBJECT

public:
    enum MirrorType {
        ArchMirror = 1,
        KdemodMirror = 2
    };

    enum ActionType {
        SaveConfiguration,
        SaveConfigurationAsync,
        SetValue,
        Value,
        Databases,
        GetServerForDatabase,
        GetServersForDatabase,
        GetMirrorList,
        AddMirrorToMirrorList,
        AddMirrorToMirrorListAsync,
        Mirrors,
        ServersForMirror,
        DatabasesForMirror,
        SetDatabases,
        SetDatabasesForMirror,
        SetServersForMirror,
        Remove,
        Exists,
        SetOrUnset,
        Reload
    };

    static Configuration *instance();

    virtual ~Configuration();

    QEvent::Type eventType();

    bool saveConfiguration() const;
    void saveConfigurationAsync();

    void setValue(const QString &key, const QVariant &val);
    QVariant value(const QString &key) const;

    QStringList databases() const;
    QString getServerForDatabase(const QString &db) const;
    QStringList getServersForDatabase(const QString &db) const;

    QStringList getMirrorList(MirrorType type) const;

    bool addMirrorToMirrorList(const QString &mirror, MirrorType type) const;
    void addMirrorToMirrorListAsync(const QString &mirror, MirrorType type);

    QStringList mirrors(bool thirdpartyonly = false) const;
    QStringList serversForMirror(const QString &mirror) const;
    QStringList databasesForMirror(const QString &mirror) const;

    void setDatabases(const QStringList &databases);
    void setDatabasesForMirror(const QStringList &database, const QString &mirror);
    void setServersForMirror(const QStringList &servers, const QString &mirror);

    void remove(const QString &key);
    bool exists(const QString &key, const QString &val = QString()) const;

    void setOrUnset(bool set, const QString &key, const QString &val = QString());

    ConfigurationThread *getInnerThread();

public Q_SLOTS:
    void reload();

Q_SIGNALS:
    void configurationSaved(bool result);

private:
    Configuration();

    class Private;
    Private *d;
};

}

#endif // CONFIGURATION_H
