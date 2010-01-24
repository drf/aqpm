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

#include "ConfigurationThread_p.h"

#include "Backend.h"

#include "ActionEvent_p.h"

#include <config-aqpm.h>

#include <QSettings>
#include <QFile>
#include <QTemporaryFile>
#include <QEventLoop>
#include <QTextStream>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QPointer>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusConnectionInterface>

#include <polkit-qt/Auth>

#define PERFORM_RETURN(ty, val) \
    QVariantMap retmap; \
    retmap["retvalue"] = QVariant::fromValue(val); \
    emit actionPerformed((int)ty, retmap); \
    return val;

#define PERFORM_RETURN_VOID(ty) emit actionPerformed((int)ty, QVariantMap()); return;

namespace Aqpm
{

class ConfigurationThread::Private
{
public:
    Private();
    QString convertPacmanConfToAqpmConf() const;

    QPointer<QTemporaryFile> tempfile;
    bool lastResult;

    QString chroot;
};

ConfigurationThread::Private::Private()
        : tempfile(0), chroot(QString())
{
}

ConfigurationThread::ConfigurationThread()
        : QObject(0)
        , d(new Private())
{
    reload();
}

ConfigurationThread::~ConfigurationThread()
{
    delete d;
}

void ConfigurationThread::setAqpmRoot(const QString& root)
{
    d->chroot = root;
}

void ConfigurationThread::customEvent(QEvent *event)
{
    if (event->type() == Configuration::instance()->eventType()) {
        ActionEvent *ae = dynamic_cast<ActionEvent*>(event);

        if (!ae) {
            qDebug() << "Someone just made some shit up";
            return;
        }

        switch ((Configuration::ActionType)ae->actionType()) {
        case Configuration::SaveConfiguration:
            saveConfiguration();
            break;
        case Configuration::SaveConfigurationAsync:
            saveConfigurationAsync();
            break;
        case Configuration::SetValue:
            setValue(ae->args()["key"].toString(), ae->args()["val"]);
            break;
        case Configuration::Value:
            value(ae->args()["key"].toString());
            break;
        case Configuration::Databases:
            databases();
            break;
        case Configuration::GetServerForDatabase:
            getServerForDatabase(ae->args()["db"].toString());
            break;
        case Configuration::GetServersForDatabase:
            getServersForDatabase(ae->args()["db"].toString());
            break;
        case Configuration::GetMirrorList:
            getMirrorList((Configuration::MirrorType)(ae->args()["type"].toInt()));
            break;
        case Configuration::SetMirrorList:
            setMirrorList(ae->args()["mirror"].toString(),
                          (Configuration::MirrorType)(ae->args()["type"].toInt()));
            break;
        case Configuration::SetMirrorListAsync:
            setMirrorListAsync(ae->args()["mirror"].toString(),
                               (Configuration::MirrorType)(ae->args()["type"].toInt()));
            break;
        case Configuration::Mirrors:
            mirrors(ae->args()["thirdpartyonly"].toBool());
            break;
        case Configuration::ServersForMirror:
            serversForMirror(ae->args()["mirror"].toString());
            break;
        case Configuration::DatabasesForMirror:
            databasesForMirror(ae->args()["mirror"].toString());
            break;
        case Configuration::SetDatabases:
            setDatabases(ae->args()["databases"].toStringList());
            break;
        case Configuration::SetDatabasesForMirror:
            setDatabasesForMirror(ae->args()["databases"].toStringList(), ae->args()["mirror"].toString());
            break;
        case Configuration::SetServersForMirror:
            setServersForMirror(ae->args()["servers"].toStringList(), ae->args()["mirror"].toString());
            break;
        case Configuration::Remove:
            remove(ae->args()["key"].toString());
            break;
        case Configuration::Exists:
            exists(ae->args()["key"].toString(), ae->args()["val"].toString());
            break;
        case Configuration::SetOrUnset:
            setOrUnset(ae->args()["set"].toBool(), ae->args()["key"].toString(), ae->args()["val"].toString());
            break;
        case Configuration::Reload:
            reload();
            break;
        default:
            qDebug() << "Implement me!!";
            break;
        }
    }
}

void ConfigurationThread::reload()
{
    qDebug() << "reloading";

    if (!QFile::exists(d->chroot + AQPM_CONFIGURATION_FILE)) {
        qDebug() << "Loading...";

        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.convertconfiguration")) {
            qDebug() << "User unauthorized";
            configuratorResult(false);
            PERFORM_RETURN_VOID(Configuration::Reload)
        }

        qDebug() << "Kewl";

        QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmconfigurator",
                               "/Configurator",
                               "org.chakraproject.aqpmconfigurator",
                               QLatin1String("pacmanConfToAqpmConf"));

        message << true;
        message << QString(d->chroot + AQPM_CONFIGURATION_FILE);
        QDBusConnection::systemBus().call(message, QDBus::Block);
        qDebug() << QDBusConnection::systemBus().lastError();
    }

    if (d->tempfile) {
        d->tempfile->close();
        d->tempfile->deleteLater();
    }

    d->tempfile = new QTemporaryFile(this);
    d->tempfile->open();

    QFile file(d->chroot + AQPM_CONFIGURATION_FILE);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "prcd!";
        emit configurationSaved(false);
        PERFORM_RETURN_VOID(Configuration::Reload)
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

    PERFORM_RETURN_VOID(Configuration::Reload)
}

bool ConfigurationThread::saveConfiguration()
{
    QEventLoop e;

    connect(this, SIGNAL(configurationSaved(bool)), &e, SLOT(quit()));

    saveConfigurationAsync();
    e.exec();

    PERFORM_RETURN(Configuration::SaveConfiguration, d->lastResult)
}

void ConfigurationThread::saveConfigurationAsync()
{
    if (Backend::instance()->shouldHandleAuthorization()) {
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.saveconfiguration")) {
            qDebug() << "User unauthorized";
            configuratorResult(false);
            PERFORM_RETURN_VOID(Configuration::SaveConfigurationAsync)
        }
    }

    if (!QDBusConnection::systemBus().interface()->isServiceRegistered("org.chakraproject.aqpmconfigurator")) {
        qDebug() << "Requesting service start";
        QDBusConnection::systemBus().interface()->startService("org.chakraproject.aqpmconfigurator");
    }

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmconfigurator", "/Configurator",
                                         "org.chakraproject.aqpmconfigurator",
                                         "configuratorResult", this, SLOT(configuratorResult(bool)));

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmconfigurator",
                           "/Configurator",
                           "org.chakraproject.aqpmconfigurator",
                           QLatin1String("saveConfiguration"));

    d->tempfile->open();
    message << QString(d->tempfile->readAll());
    message << QString(d->chroot + AQPM_CONFIGURATION_FILE);
    QDBusConnection::systemBus().call(message);
    d->tempfile->close();

    PERFORM_RETURN_VOID(Configuration::SaveConfigurationAsync)
}

void ConfigurationThread::configuratorResult(bool result)
{
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmconfigurator", "/Configurator",
                                            "org.chakraproject.aqpmconfigurator",
                                            "configuratorResult", this, SLOT(configuratorResult(bool)));

    qDebug() << "Got a result:" << result;

    if (result) {
        reload();
    }

    d->lastResult = result;

    emit configurationSaved(result);
}

void ConfigurationThread::setValue(const QString &key, const QVariant &val)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings->setValue(key, val);
    settings->deleteLater();

    PERFORM_RETURN_VOID(Configuration::SetValue)
}

QVariant ConfigurationThread::value(const QString &key)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    QVariant ret = settings->value(key);
    settings->deleteLater();

    PERFORM_RETURN(Configuration::Value, ret)
}

QStringList ConfigurationThread::databases()
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    QStringList dbsreg = settings->childGroups();
    QStringList retlist = settings->value("options/DbOrder").toStringList();
    settings->deleteLater();
    PERFORM_RETURN(Configuration::Databases, retlist)
}

QString ConfigurationThread::getServerForDatabase(const QString &db)
{
    PERFORM_RETURN(Configuration::GetServerForDatabase, getServersForDatabase(db).first());
}

QStringList ConfigurationThread::getServersForDatabase(const QString &db)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    QStringList retlist;
    QStringList rawlist;

    settings->beginGroup("mirrors");

    qDebug() << "Checking " << db;

    if (db == "core" || db == "extra" || db == "community" || db == "testing") {
        rawlist = getMirrorList(Aqpm::Configuration::ArchMirror);
    } else if (db == "kdemod-core" || db == "kdemod-extragear" || db == "kdemod-unstable" ||
               db == "kdemod-legacy" || db == "kdemod-testing" || db == "kdemod-playground" ||
               db == "kdemod-tools" || db == "chakra") {
        rawlist = getMirrorList(Aqpm::Configuration::KdemodMirror);
    } else {
        foreach(const QString &mirror, settings->childGroups()) {
            if (settings->value(mirror + "/Databases").toStringList().contains(db)) {
                settings->beginGroup(mirror);
            }
        }

        qDebug() << "ok, keys are " << settings->childKeys();

        for (int i = 1; i <= settings->childKeys().size(); ++i) {
            QString retstr = settings->value(QString("Server%1").arg(i)).toString();
            retstr.replace("$repo", db);
            retstr.replace("$arch", value("options/Arch").toString());
            retlist.append(retstr);
        }
    }

    foreach (const QString &ent, rawlist) {
        QString rent = ent;
        rent.replace("$repo", db);
        rent.replace("$arch", value("options/Arch").toString());
        retlist.append(rent);
    }

    PERFORM_RETURN(Configuration::GetServersForDatabase, retlist);
}

QStringList ConfigurationThread::getMirrorList(Configuration::MirrorType type)
{
    QFile file;

    if (type == Configuration::ArchMirror) {
        file.setFileName("/etc/pacman.d/mirrorlist");
    } else if (type == Configuration::KdemodMirror) {
        if (QFile::exists("/etc/pacman.d/kdemodmirrorlist")) {
            file.setFileName("/etc/pacman.d/kdemodmirrorlist");
        } else {
            PERFORM_RETURN(Configuration::GetMirrorList, QStringList());
        }
    }

    QStringList retlist;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        PERFORM_RETURN(Configuration::GetMirrorList, QStringList());
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

    PERFORM_RETURN(Configuration::GetMirrorList, retlist);

}

bool ConfigurationThread::setMirrorList(const QString &mirrorlist, Configuration::MirrorType type)
{
    QEventLoop e;

    connect(this, SIGNAL(configurationSaved(bool)), &e, SLOT(quit()));

    setMirrorListAsync(mirrorlist, type);
    e.exec();

    PERFORM_RETURN(Configuration::SetMirrorList, d->lastResult);
}

void ConfigurationThread::setMirrorListAsync(const QString &mirrorlist, Configuration::MirrorType type)
{
    if (Backend::instance()->shouldHandleAuthorization()) {
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.setmirrorlist")) {
            qDebug() << "User unauthorized";
            configuratorResult(false);
            PERFORM_RETURN_VOID(Configuration::SetMirrorListAsync);
        }
    }

    if (!QDBusConnection::systemBus().interface()->isServiceRegistered("org.chakraproject.aqpmconfigurator")) {
        qDebug() << "Requesting service start";
        QDBusConnection::systemBus().interface()->startService("org.chakraproject.aqpmconfigurator");
    }

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmconfigurator", "/Configurator",
                                         "org.chakraproject.aqpmconfigurator",
                                         "configuratorResult", this, SLOT(configuratorResult(bool)));

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmconfigurator",
                           "/Configurator",
                           "org.chakraproject.aqpmconfigurator",
                           QLatin1String("setMirrorList"));

    message << mirrorlist;
    message << (int)type;
    QDBusConnection::systemBus().call(message, QDBus::NoBlock);

    PERFORM_RETURN_VOID(Configuration::SetMirrorListAsync);
}

void ConfigurationThread::remove(const QString &key)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings->remove(key);
    settings->deleteLater();

    PERFORM_RETURN_VOID(Configuration::Remove);
}

bool ConfigurationThread::exists(const QString &key, const QString &val)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    bool result = settings->contains(key);

    if (!val.isEmpty() && result) {
        qDebug() << "Check";
        result = value(key).toString() == val;
    }

    settings->deleteLater();

    PERFORM_RETURN(Configuration::Exists, result);
}

void ConfigurationThread::setOrUnset(bool set, const QString &key, const QString &val)
{
    if (set) {
        setValue(key, val);
    } else {
        remove(key);
    }

    PERFORM_RETURN_VOID(Configuration::SetOrUnset);
}

void ConfigurationThread::setDatabases(const QStringList &databases)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings->setValue("options/DbOrder", databases);
    settings->deleteLater();

    PERFORM_RETURN_VOID(Configuration::SetDatabases);
}

void ConfigurationThread::setDatabasesForMirror(const QStringList &databases, const QString &mirror)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings->setValue("mirrors/" + mirror + "/Databases", databases);
    settings->deleteLater();

    PERFORM_RETURN_VOID(Configuration::SetDatabasesForMirror);
}

QStringList ConfigurationThread::serversForMirror(const QString &mirror)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    QStringList retlist;
    settings->beginGroup("mirrors");
    settings->beginGroup(mirror);
    foreach(const QString &key, settings->childKeys()) {
        if (key.startsWith(QLatin1String("Server"))) {
            retlist.append(settings->value(key).toString());
        }
    }

    settings->endGroup();
    settings->endGroup();
    settings->deleteLater();

    PERFORM_RETURN(Configuration::ServersForMirror, retlist);
}

void ConfigurationThread::setServersForMirror(const QStringList &servers, const QString &mirror)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings->beginGroup("mirrors");
    settings->beginGroup(mirror);
    foreach(const QString &key, settings->childKeys()) {
        if (key.startsWith(QLatin1String("Server"))) {
            settings->remove(key);
        }
    }

    for (int i = 1; i <= servers.size(); ++i) {
        settings->setValue(QString("Server%1").arg(i), servers.at(i - 1));
    }

    settings->endGroup();
    settings->endGroup();
    settings->deleteLater();

    PERFORM_RETURN_VOID(Configuration::SetServersForMirror);
}

QStringList ConfigurationThread::mirrors(bool thirdpartyonly)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    settings->beginGroup("mirrors");
    QStringList retlist = settings->childGroups();
    settings->endGroup();

    if (thirdpartyonly) {
        retlist.removeOne("arch");
        retlist.removeOne("kdemod");
    }

    settings->deleteLater();
    PERFORM_RETURN(Configuration::Mirrors, retlist);
}

QStringList ConfigurationThread::databasesForMirror(const QString &mirror)
{
    QSettings *settings = new QSettings(d->tempfile->fileName(), QSettings::IniFormat, this);
    QStringList retlist = settings->value("mirrors/" + mirror + "/Databases").toStringList();
    settings->deleteLater();
    PERFORM_RETURN(Configuration::DatabasesForMirror, retlist);
}

}
