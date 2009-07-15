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
    Private() : tempfile(0) {}

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
    Q_ASSERT(!s_globalConfiguration()->q);
    s_globalConfiguration()->q = this;

    reload();
}

Configuration::~Configuration()
{
    delete d;
}

void Configuration::reload()
{
    qDebug() << "reloading";

    if (d->tempfile) {
        d->tempfile->close();
        d->tempfile->deleteLater();
    }

    d->tempfile = new QTemporaryFile(this);
    d->tempfile->open();

    QFile file("/etc/pacman.conf");

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "prcd!";
        emit configurationSaved(false);
        return;
    }

    QTextStream out(d->tempfile);
    QTextStream in(&file);

    // Strip out comments
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (!line.startsWith('#')) {
            out << line;
        }
    }

    file.close();
    d->tempfile->close();

    qDebug() << d->tempfile->fileName();
    exists("test", "test");
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

    d->tempfile->open();
    message << QString(d->tempfile->readAll());
    QDBusConnection::systemBus().call(message);
    qDebug() << QDBusConnection::systemBus().lastError();
    d->tempfile->close();
}

void Configuration::workerResult(bool result)
{
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmworker", "/Worker", "org.chakraproject.aqpmworker",
                                            "workerResult", this, SLOT(workerResult(bool)));

    qDebug() << "Got a result:" << result;

    if (result) {
        reload();
    }

    d->lastResult = result;

    emit configurationSaved(result);
}

void Configuration::setValue(const QString &key, const QString &val)
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings.setValue(key, val);
}

QVariant Configuration::value(const QString &key)
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat, this);
    return settings.value(key);
}

void Configuration::remove(const QString &key)
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings.remove(key);
}

bool Configuration::exists(const QString &key, const QString &val)
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat);
    qDebug() << "Checking" << key << val;
    bool result = settings.contains(key);
    qDebug() << "Done";

    if (!val.isEmpty() && result) {
        qDebug() << "Check";
        result = value(key).toString() == val;
    }

    qDebug() << "Done";

    return result;
}

void Configuration::setOrUnset(bool set, const QString &key, const QString &val)
{
    if (set) {
        setValue(key, val);
    } else {
        remove(key);
    }
}

}
