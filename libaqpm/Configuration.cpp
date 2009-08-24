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
    Private();

    QTemporaryFile *tempfile;
    QString arch;
    bool lastResult;
};

Configuration::Private::Private()
         : tempfile(0)
{
    QProcess proc;
    proc.start("arch");
    proc.waitForFinished(20000);

    arch = QString(proc.readAllStandardOutput()).remove('\n').remove(' ');
}

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

    QString fname;

    if (QFile::exists("/etc/aqpm.conf")) {
        fname = "/etc/aqpm.conf";
    } else {
        fname = "/etc/pacman.conf";
    }

    QFile file(fname);

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
            out << '\n';
        }
    }

    file.close();
    d->tempfile->close();
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

QStringList Configuration::databases()
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat, this);
    QStringList dbsreg = settings.childGroups();
    QStringList ordereddbs = settings.value("options/dborder").toStringList();

    if (dbsreg.size() != ordereddbs.size()) {
        foreach(const QString &db, dbsreg) {
            if (!ordereddbs.contains(db)) {
                ordereddbs.append(db);
            }
        }
        foreach(const QString &db, ordereddbs) {
            if (!dbsreg.contains(db)) {
                ordereddbs.removeOne(db);
            }
        }
    }

    ordereddbs.removeOne("options");
    return ordereddbs;
}

QString Configuration::getServerForDatabase(const QString &db)
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat, this);
    QString retstr = settings.value(db + "/Server").toString();
    retstr.replace("$repo", db);
    retstr.replace("$arch", d->arch);
    return retstr;
}

QStringList Configuration::getMirrorList(MirrorType type) const
{
    QFile file;

    if (type == ArchMirror) {
        file.setFileName("/etc/pacman.d/mirrorlist");
    } else if (type == KdemodMirror) {
        if (QFile::exists("/etc/pacman.d/kdemodmirrorlist")) {
            file.setFileName("/etc/pacman.d/kdemodmirrorlist");
        } else {
            return QStringList();
        }
    }

    QStringList retlist;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringList();
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();

        if (line.startsWith('#')) {
            line.remove(0, 1);
        }

        if (!line.contains("server", Qt::CaseInsensitive)) {
            continue;
        }

        QStringList list(line.split('=', QString::SkipEmptyParts));
        if (list.count() >= 1) {
            QString serverN(list.at(1));

            serverN.remove(QChar(' '), Qt::CaseInsensitive);

            retlist.append(serverN);
        }
    }

    file.close();

    return retlist;

}

bool Configuration::addMirror(const QString &mirror, MirrorType type)
{
    QEventLoop e;

    connect(this, SIGNAL(configurationSaved(bool)), &e, SLOT(quit()));

    addMirrorAsync(mirror, type);
    e.exec();

    return d->lastResult;
}

void Configuration::addMirrorAsync(const QString &mirror, MirrorType type)
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
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.addmirror")) {
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
              QLatin1String("addMirror"));

    message << mirror;
    message << (int)type;
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);
}

void Configuration::remove(const QString &key)
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings.remove(key);
}

bool Configuration::exists(const QString &key, const QString &val)
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat);
    bool result = settings.contains(key);

    if (!val.isEmpty() && result) {
        qDebug() << "Check";
        result = value(key).toString() == val;
    }

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
