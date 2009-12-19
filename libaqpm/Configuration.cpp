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

#include "Configuration.h"

#include "ConfigurationThread_p.h"
#include "Loops_p.h"

namespace Aqpm
{

class Configuration::Private
{
public:
    Private() : thread(new ConfigurationThread()) {}

    ConfigurationThread *thread;
    QEvent::Type type;
};

class ConfigurationHelper
{
public:
    ConfigurationHelper() : q(0) {}
    ~ConfigurationHelper() {
        delete q;
    }
    Configuration *q;
};

Q_GLOBAL_STATIC(ConfigurationHelper, s_globalConfiguration)

Configuration *Configuration::instance()
{
    if (!s_globalConfiguration()->q) {
        new Configuration;
    }

    return s_globalConfiguration()->q;
}

Configuration::Configuration()
        : QObject(0)
        , d(new Private())
{
    Q_ASSERT(!s_globalConfiguration()->q);
    s_globalConfiguration()->q = this;

    d->type = (QEvent::Type)QEvent::registerEventType();

    connect(d->thread, SIGNAL(configurationSaved(bool)), this, SIGNAL(configurationSaved(bool)));
}

Configuration::~Configuration()
{
    d->thread->deleteLater();
    delete d;
}

QEvent::Type Configuration::eventType()
{
    return d->type;
}

void Configuration::reload()
{
    SynchronousLoop s(Reload, QVariantMap());
}

bool Configuration::saveConfiguration() const
{
    SynchronousLoop s(SaveConfiguration, QVariantMap());
    return s.result()["retvalue"].toBool();
}

void Configuration::saveConfigurationAsync()
{
    SynchronousLoop s(SaveConfigurationAsync, QVariantMap());
}

void Configuration::setValue(const QString &key, const QVariant &val)
{
    QVariantMap args;
    args["key"] = QVariant::fromValue(key);
    args["val"] = val;
    SynchronousLoop s(SetValue, args);
}

QVariant Configuration::value(const QString &key) const
{
    QVariantMap args;
    args["key"] = QVariant::fromValue(key);
    SynchronousLoop s(Value, args);
    return s.result()["retvalue"];
}

QStringList Configuration::databases() const
{
    SynchronousLoop s(Databases, QVariantMap());
    return s.result()["retvalue"].toStringList();
}

QString Configuration::getServerForDatabase(const QString &db) const
{
    QVariantMap args;
    args["db"] = QVariant::fromValue(db);
    SynchronousLoop s(GetServerForDatabase, args);
    return s.result()["retvalue"].toString();
}

QStringList Configuration::getServersForDatabase(const QString &db) const
{
    QVariantMap args;
    args["db"] = QVariant::fromValue(db);
    SynchronousLoop s(GetServersForDatabase, args);
    return s.result()["retvalue"].toStringList();
}

QStringList Configuration::getMirrorList(MirrorType type) const
{
    QVariantMap args;
    args["type"] = QVariant::fromValue((int)type);
    SynchronousLoop s(GetMirrorList, args);
    return s.result()["retvalue"].toStringList();
}

bool Configuration::setMirrorList(const QStringList &mirrorlist, MirrorType type) const
{
    QVariantMap args;
    QString list;
    foreach (const QString &ent, mirrorlist) {
        list.append("Server = ");
        list.append(ent);
        list.append('\n');
    }
    args["type"] = QVariant::fromValue((int)type);
    args["mirror"] = QVariant::fromValue(list);
    SynchronousLoop s(SetMirrorList, args);
    return s.result()["retvalue"].toBool();
}

void Configuration::setMirrorListAsync(const QStringList &mirrorlist, MirrorType type)
{
    QVariantMap args;
    QString list;
    foreach (const QString &ent, mirrorlist) {
        list.append("Server = ");
        list.append(ent);
        list.append('\n');
    }
    args["type"] = QVariant::fromValue((int)type);
    args["mirror"] = QVariant::fromValue(list);
    SynchronousLoop s(SetMirrorListAsync, args);
}

void Configuration::remove(const QString &key)
{
    QVariantMap args;
    args["key"] = QVariant::fromValue(key);
    SynchronousLoop s(Remove, args);
}

bool Configuration::exists(const QString &key, const QString &val) const
{
    QVariantMap args;
    args["key"] = QVariant::fromValue(key);
    args["val"] = QVariant::fromValue(val);
    SynchronousLoop s(Exists, args);
    return s.result()["retvalue"].toBool();
}

void Configuration::setOrUnset(bool set, const QString &key, const QString &val)
{
    QVariantMap args;
    args["set"] = QVariant::fromValue(set);
    args["key"] = QVariant::fromValue(key);
    args["val"] = QVariant::fromValue(val);
    SynchronousLoop s(SetOrUnset, args);
}

void Configuration::setDatabases(const QStringList &databases)
{
    QVariantMap args;
    args["databases"] = QVariant::fromValue(databases);
    SynchronousLoop s(SetDatabases, args);
}

void Configuration::setDatabasesForMirror(const QStringList &databases, const QString &mirror)
{
    QVariantMap args;
    args["databases"] = QVariant::fromValue(databases);
    args["mirror"] = QVariant::fromValue(mirror);
    SynchronousLoop s(SetDatabasesForMirror, args);
}

QStringList Configuration::serversForMirror(const QString &mirror) const
{
    QVariantMap args;
    args["mirror"] = QVariant::fromValue(mirror);
    SynchronousLoop s(ServersForMirror, args);
    return s.result()["retvalue"].toStringList();
}

void Configuration::setServersForMirror(const QStringList &servers, const QString &mirror)
{
    QVariantMap args;
    args["servers"] = QVariant::fromValue(servers);
    args["mirror"] = QVariant::fromValue(mirror);
    SynchronousLoop s(SetServersForMirror, args);
}

QStringList Configuration::mirrors(bool thirdpartyonly) const
{
    QVariantMap args;
    args["thirdpartyonly"] = QVariant::fromValue(thirdpartyonly);
    SynchronousLoop s(Mirrors, args);
    return s.result()["retvalue"].toStringList();
}

QStringList Configuration::databasesForMirror(const QString &mirror) const
{
    QVariantMap args;
    args["mirror"] = QVariant::fromValue(mirror);
    SynchronousLoop s(DatabasesForMirror, args);
    return s.result()["retvalue"].toStringList();
}

ConfigurationThread *Configuration::getInnerThread()
{
    return d->thread;
}

}
