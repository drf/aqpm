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

#include "Backend.h"

#include <QSettings>
#include <QFile>
#include <QTemporaryFile>
#include <QEventLoop>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <Auth>

namespace Aqpm
{

class Configuration::Private
{
public:
    Private() {}

    QSettings *settings;
    QTemporaryFile *tempfile;
    bool lastResult;
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
}

Configuration::~Configuration()
{
}

void Configuration::reload()
{
    if (d->settings) {
        d->settings->deleteLater();
    }

    if (d->tempfile) {
        d->tempfile->close();
        d->tempfile->deleteLater();
    }

    d->tempfile = new QTemporaryFile(this);
    d->tempfile->open();

    QFile file("/tmp/pacman.conf.aqpm.tmp");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit configurationSaved(false);
        return;
    }

    QTextStream out(d->tempfile);
    out << file.readAll();

    d->settings = new QSettings(QDir::tempPath() + '/' + d->tempfile->fileName(), QSettings::IniFormat, this);
}

bool Configuration::saveConfiguration()
{
    QEventLoop e;

    connect(this, SIGNAL(configurationSaved(bool)), &e, SLOT(quit()));

    saveConfigurationAsync();
    e.exec();

    return d->lastResult;
}

void Configuration::saveConfigurationAsync()
{
    d->settings->sync();

    QDBusMessage message;
    message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("isWorkerReady"));
    QDBusMessage reply = QDBusConnection::systemBus().call(message);
    if (reply.type() == QDBusMessage::ReplyMessage
            && reply.arguments().size() == 1) {
        qDebug() << reply.arguments().first().toBool();
    } else if (reply.type() == QDBusMessage::MethodCallMessage) {
        qWarning() << "Message did not receive a reply (timeout by message bus)";
        workerResult(false);
        return;
    }

    if (Backend::instance()->shouldHandleAuthorization()) {
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.saveconfiguration")) {
            qDebug() << "User unauthorized";
            workerResult(false);
            return;
        }
    }

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                         "workerResult", this, SLOT(workerResult(bool)));

    message = QDBusMessage::createMethodCall("org.chakraproject.aqpmworker",
              "/Worker",
              "org.chakraproject.aqpmworker",
              QLatin1String("saveConfiguration"));
    QList<QVariant> argumentList;

    argumentList << d->tempfile->readAll();
    message.setArguments(argumentList);
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);
}

void Configuration::workerResult(bool result)
{
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "workerResult", this, SLOT(workerResult(bool)));

    if (result) {
        reload();
    }

    d->lastResult = result;

    emit configurationSaved(result);
}

void Configuration::setValue(const QString &key, const QString &value)
{
    d->settings->setValue(key, value);
}

QVariant Configuration::value(const QString &key) const
{
    return d->settings->value(key);
}

}
