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

#include "Configurator.h"

#include "aqpmconfiguratoradaptor.h"

#include <config-aqpm.h>

#include <QtDBus/QDBusConnection>
#include <QTimer>
#include <QTemporaryFile>
#include <QDebug>

#include "../Globals.h"
#include "../Configuration.h"

#include <Auth>

namespace AqpmConfigurator
{

class Configurator::Private
{
public:
    Private() {}

    QString retrieveServerFromPacmanConf(const QString &db) const;
};

Configurator::Configurator(bool temporize, QObject *parent)
        : QObject(parent)
        , d(new Private())
{
    new AqpmconfiguratorAdaptor(this);

    if (!QDBusConnection::systemBus().registerService("org.chakraproject.aqpmconfigurator")) {
        qDebug() << "another configurator is already running";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    if (!QDBusConnection::systemBus().registerObject("/Configurator", this)) {
        qDebug() << "unable to register service interface to dbus";
        QTimer::singleShot(0, QCoreApplication::instance(), SLOT(quit()));
        return;
    }

    setIsTemporized(temporize);
    setTimeout(2000);
    startTemporizing();
}

Configurator::~Configurator()
{
}

void Configurator::saveConfiguration(const QString &conf)
{
    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.saveconfiguration",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit errorOccurred((int) Aqpm::Globals::AuthorizationNotGranted, QVariantMap());
        operationPerformed(false);
        return;
    }

    stopTemporizing();

    qDebug() << "About to write:" << conf;

    QFile::remove(AQPM_CONFIGURATION_FILE);
    QFile file(AQPM_CONFIGURATION_FILE);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        operationPerformed(false);
        return;
    }

    QTextStream out(&file);
    out << conf;

    file.flush();
    file.close();

    operationPerformed(true);
}

void Configurator::addMirror(const QString &mirror, int type)
{
    PolkitQt::Auth::Result result;
    result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.addmirror",
             message().service(),
             true);
    if (result == PolkitQt::Auth::Yes) {
        qDebug() << message().service() << QString(" authorized");
    } else {
        qDebug() << QString("Not authorized");
        emit errorOccurred((int) Aqpm::Globals::AuthorizationNotGranted, QVariantMap());
        operationPerformed(false);
        return;
    }

    stopTemporizing();

    QString toInsert("Server=");
    toInsert.append(mirror);

    QFile file;

    switch ((Aqpm::Configuration::MirrorType)type) {
    case Aqpm::Configuration::ArchMirror:
        file.setFileName("/etc/pacman.d/mirrorlist");
        break;
    case Aqpm::Configuration::KdemodMirror:
        file.setFileName("/etc/pacman.d/kdemodmirrorlist");
        break;
    }

    if (!file.open(QIODevice::Append | QIODevice::Text)) {
        operationPerformed(false);
        return;
    }

    file.write(toInsert.toUtf8().data(), toInsert.length());
    file.write("\n", 1);

    file.flush();
    file.close();

    operationPerformed(true);
}

QString Configurator::Private::retrieveServerFromPacmanConf(const QString &db) const
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

QString Configurator::pacmanConfToAqpmConf(bool writeconf)
{
    qDebug() << "IN da call";

    if (writeconf) {
        PolkitQt::Auth::Result result;
        result = PolkitQt::Auth::isCallerAuthorized("org.chakraproject.aqpm.convertconfiguration",
                                                    message().service(),
                                                    true);
        if (result == PolkitQt::Auth::Yes) {
            qDebug() << message().service() << QString(" authorized");
        } else {
            qDebug() << QString("Not authorized");
            emit errorOccurred((int) Aqpm::Globals::AuthorizationNotGranted, QVariantMap());
            operationPerformed(false);
            return QString();
        }
    }

    QTemporaryFile *tmpconf = new QTemporaryFile(this);
    tmpconf->open();
    QTemporaryFile *tmppacmanconf = new QTemporaryFile(this);
    tmppacmanconf->open();

    // Strip comments out of pacman.conf first
    QFile file("/etc/pacman.conf");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream out(tmppacmanconf);
    QByteArray toWrite;

    while (!file.atEnd()) {
        QString line = file.readLine();
        if (!line.startsWith(QChar('#'))) {
            toWrite.append(line);
        }
    }

    tmppacmanconf->write(toWrite);
    tmppacmanconf->flush();
    tmppacmanconf->reset();

    qDebug() << "BIZNEZ\n" << tmppacmanconf->readAll();

    QSettings settings(tmppacmanconf->fileName(), QSettings::IniFormat);
    QSettings writeSettings(tmpconf->fileName(), QSettings::IniFormat);

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
            QString server = d->retrieveServerFromPacmanConf(db);
            server.replace(db, "$repo");
            writeSettings.setValue("Server1", server);
            writeSettings.endGroup();
            coreset = true;
        } else if (db == "kdemod-core" || db == "kdemod-extragear" || db == "kdemod-unstable" ||
                   db == "kdemod-legacy" || db == "kdemod-testing" || db == "kdemod-playground") {
            if (kdemodset) {
                continue;
            }
            writeSettings.beginGroup("kdemod");
            QString server = d->retrieveServerFromPacmanConf(db);
            server.replace(db, "$repo");
            writeSettings.setValue("Server1", server);
            writeSettings.endGroup();
            kdemodset = true;
        } else {
            // Create a custom mirror section
            writeSettings.beginGroup(db);
            QString server = d->retrieveServerFromPacmanConf(db);
            server.replace(db, "$repo");
            writeSettings.setValue("Databases", QStringList() << db);
            writeSettings.setValue("Server1", server);
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

    QProcess proc;
    proc.start("arch");
    proc.waitForFinished(20000);

    QString arch = QString(proc.readAllStandardOutput()).remove('\n').remove(' ');

    writeSettings.setValue("Arch", arch);

    writeSettings.endGroup();
    writeSettings.sync();

    if (writeconf) {
        QFile::copy(tmpconf->fileName(), AQPM_CONFIGURATION_FILE);
        QProcess::execute(QString("chmod 755 ") + AQPM_CONFIGURATION_FILE);
    }

    tmpconf->open();
    QString result = tmpconf->readAll();
    qDebug() << result;
    tmpconf->close();
    tmpconf->deleteLater();
    tmppacmanconf->deleteLater();

    operationPerformed(true);
    qDebug() << result;
    return result;
}

void Configurator::operationPerformed(bool result)
{
    emit configuratorResult(result);
    startTemporizing();
}

}
