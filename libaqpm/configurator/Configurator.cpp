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

    QFile::remove("/etc/pacman.conf");
    QFile file("/etc/pacman.conf");

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

void Configurator::operationPerformed(bool result)
{
    emit configuratorResult(result);
    startTemporizing();
}

}
