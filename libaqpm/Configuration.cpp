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
#include <QTimer>
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
    QString retrieveServerFromPacmanConf(const QString &db) const;
    QString convertPacmanConfToAqpmConf() const;

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

    if (QFile::exists("/etc/aqpm.conf")) {
        QFile file("/etc/aqpm.conf");

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
    } else {
        d->convertPacmanConfToAqpmConf();
    }


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
    if (Backend::instance()->shouldHandleAuthorization()) {
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.saveconfiguration")) {
            qDebug() << "User unauthorized";
            configuratorResult(false);
            return;
        }
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
    QDBusConnection::systemBus().call(message);
    qDebug() << QDBusConnection::systemBus().lastError();
    d->tempfile->close();
}

void Configuration::configuratorResult(bool result)
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
    return settings.value("options/DbOrder").toStringList();
}

QString Configuration::getServerForDatabase(const QString &db)
{
    return getMirrorsForDatabase(db).first();
}

QStringList Configuration::getMirrorsForDatabase(const QString &db)
{
    QSettings settings(d->tempfile->fileName(), QSettings::IniFormat, this);
    QStringList retlist;

    settings.beginGroup("mirrors");

    qDebug() << "Checking " << db;

    if (db == "core" || db == "extra" || db == "community" || db == "testing") {
        settings.beginGroup("arch");
    } else if (db == "kdemod-core" || db == "kdemod-extragear" || db == "kdemod-unstable" ||
               db == "kdemod-legacy" || db == "kdemod-testing" || db == "kdemod-playground") {
        settings.beginGroup("kdemod");
    } else {
        foreach (const QString &mirror, settings.childGroups()) {
            if (settings.value(mirror + "/Databases").toStringList().contains(db)) {
                settings.beginGroup(mirror);
            }
        }
    }

    qDebug() << "ok, keys are " << settings.childKeys();

    for (int i = 1; i <= settings.childKeys().size(); ++i) {
        QString retstr = settings.value(QString("Mirror%1").arg(i)).toString();
        retstr.replace("$repo", db);
        retstr.replace("$arch", d->arch);
        retlist.append(retstr);
    }

    return retlist;
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
    if (Backend::instance()->shouldHandleAuthorization()) {
        if (!PolkitQt::Auth::computeAndObtainAuth("org.chakraproject.aqpm.addmirror")) {
            qDebug() << "User unauthorized";
            configuratorResult(false);
            return;
        }
    }

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmconfigurator", "/Configurator", "org.chakraproject.aqpmconfigurator",
                                         "configuratorResult", this, SLOT(configuratorResult(bool)));

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmconfigurator",
                                                          "/Configurator",
                                                          "org.chakraproject.aqpmconfigurator",
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

QString Configuration::Private::retrieveServerFromPacmanConf(const QString &db) const
{
    QFile pfile("/etc/pacman.conf");
    pfile.open(QFile::ReadOnly | QFile::Text);
    QTextStream in(&pfile);

    QString retstr;

    // Find out the db
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith('[' + db)) {
            // Let's go to the next line
            QString nextLine = "#";

            while (retstr.isEmpty() && !in.atEnd()) {
                while ((nextLine.startsWith('#') || nextLine.isEmpty()) && !in.atEnd()) {
                    nextLine = in.readLine();
                }
                // Cool, let's see if it's valid
                if (!nextLine.startsWith("Server") && !nextLine.startsWith("Include")) {
                    retstr.clear();
                } else if (nextLine.startsWith("Server")) {
                    nextLine.remove(' ');
                    nextLine.remove('=');
                    nextLine.remove("Server", Qt::CaseSensitive);
                    retstr = nextLine;
                } else {
                    nextLine.remove(' ');
                    nextLine.remove('=');
                    nextLine.remove("Include", Qt::CaseSensitive);

                    // Now let's hit the include file
                    QFile file(nextLine);

                    file.open(QIODevice::ReadOnly);

                    QTextStream incin(&file);

                    while (!incin.atEnd()) {
                        QString incLine = incin.readLine();
                        if (incLine.startsWith("Server")) {
                            incLine.remove(' ');
                            incLine.remove('=');
                            incLine.remove("Server", Qt::CaseSensitive);
                            file.close();
                            retstr = incLine;
                            break;
                        }
                    }
                }
            }
        }
    }

    pfile.close();

    return retstr;
}

QString Configuration::Private::convertPacmanConfToAqpmConf() const
{
    QSettings settings("/etc/pacman.conf", QSettings::IniFormat);
    QSettings writeSettings(tempfile->fileName(), QSettings::IniFormat);

    QStringList databases = settings.childGroups();
    databases.removeOne("options");

    bool kdemodset = false;
    bool coreset = false;

    writeSettings.beginGroup("mirrors");

    foreach (const QString &db, databases) {
        if (db == "core" || db == "extra" || db == "community" || db == "testing") {
            if (coreset) {
                continue;
            }
            writeSettings.beginGroup("arch");
            QString server = retrieveServerFromPacmanConf(db);
            server.replace(db, "$repo");
            writeSettings.setValue("Mirror1", server);
            writeSettings.endGroup();
            coreset = true;
        } else if (db == "kdemod-core" || db == "kdemod-extragear" || db == "kdemod-unstable" ||
                   db == "kdemod-legacy" || db == "kdemod-testing" || db == "kdemod-playground") {
            if (kdemodset) {
                continue;
            }
            writeSettings.beginGroup("kdemod");
            QString server = retrieveServerFromPacmanConf(db);
            server.replace(db, "$repo");
            writeSettings.setValue("Mirror1", server);
            writeSettings.endGroup();
            kdemodset = true;
        } else {
            // Create a custom mirror section
            writeSettings.beginGroup(db);
            QString server = retrieveServerFromPacmanConf(db);
            server.replace(db, "$repo");
            writeSettings.setValue("Databases", QStringList() << db);
            writeSettings.setValue("Mirror1", server);
            writeSettings.endGroup();
        }
    }

    writeSettings.endGroup();
    writeSettings.beginGroup("options");
    settings.beginGroup("options");

    foreach (const QString &key, settings.childKeys()) {
        writeSettings.setValue(key, settings.value(key));
    }

    writeSettings.setValue("DbOrder", databases);

    writeSettings.endGroup();
    writeSettings.sync();
}

}
