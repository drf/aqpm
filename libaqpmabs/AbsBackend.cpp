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

#include "AbsBackend.h"

#include <QDir>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <polkit-qt/auth.h>

namespace Aqpm {

namespace Abs {

class AbsBackendHelper
{
public:
    AbsBackendHelper() : q(0) {}
    ~AbsBackendHelper() {
        delete q;
    }
    Backend *q;
};

Q_GLOBAL_STATIC(AbsBackendHelper, s_globalAbsBackend)

Backend *Backend::instance()
{
    if (!s_globalAbsBackend()->q) {
        new Backend;
    }

    return s_globalAbsBackend()->q;
}

class Backend::Private
{
public:
    Private() : handleAuth(false) {}

    QString absPath(const QString &package) const;
    bool initWorker(const QString &polkitAction);

    // Q_PRIVATE_SLOTS
    void __k__operationFinished(bool result);
    void __k__newOutput(const QString &output);

    bool handleAuth;
    Backend *q;
};

QString Backend::Private::absPath(const QString& package) const
{
    QDir absDir("/var/abs");
    absDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    int found = 0;
    QString absSource;

    QFileInfoList list = absDir.entryInfoList();

    for (int i = 0; i < list.size(); ++i) {
        QDir subAbsDir(list.at(i).absoluteFilePath());
        subAbsDir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        QFileInfoList subList = subAbsDir.entryInfoList();

        for (int j = 0; j < subList.size(); ++j) {
            if (!subList.at(j).baseName().compare(package)) {
                found = 1;
                absSource = subList.at(j).absoluteFilePath();
                break;
            }
        }

        if (found == 1) {
            break;
        }
    }

    if (!found) {
        return QString();
    }

    qDebug() << "ABS Dir is " << absSource;
}

bool Backend::Private::initWorker(const QString &polkitAction)
{
    if (handleAuth) {
        if (!PolkitQt::Auth::computeAndObtainAuth(polkitAction)) {
            qDebug() << "User unauthorized";
            return false;
        }
    }

    if (!QDBusConnection::systemBus().interface()->isServiceRegistered("org.chakraproject.aqpmabsworker")) {
        qDebug() << "Requesting service start";
        QDBusConnection::systemBus().interface()->startService("org.chakraproject.aqpmabsworker");
    }

    QDBusConnection::systemBus().connect("org.chakraproject.aqpmabsworker", "/Worker", "org.chakraproject.aqpmabsworker",
                                         "absUpdated", q, SLOT(__k__operationFinished(bool)));
    QDBusConnection::systemBus().connect("org.chakraproject.aqpmabsworker", "/Worker", "org.chakraproject.aqpmabsworker",
                                         "newOutput", q, SLOT(__k__newOutput(QString)));
    connect(QDBusConnection::systemBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            q, SLOT(serviceOwnerChanged(QString,QString,QString)));

    return true;
}

void Backend::Private::__k__newOutput(const QString& output)
{
    emit q->operationProgress(output);
}

void Backend::Private::__k__operationFinished(bool result)
{
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmabsworker", "/Worker", "org.chakraproject.aqpmabsworker",
                                            "absUpdated", q, SLOT(__k__operationFinished(bool)));
    QDBusConnection::systemBus().disconnect("org.chakraproject.aqpmabsworker", "/Worker", "org.chakraproject.aqpmabsworker",
                                            "newOutput", q, SLOT(__k__newOutput(QString)));
    disconnect(QDBusConnection::systemBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
               q, SLOT(serviceOwnerChanged(QString,QString,QString)));

    emit q->operationFinished(result);
}

//

Backend::Backend(QObject* parent)
        : QObject(parent)
        , d(new Private)
{
    Q_ASSERT(!s_globalAbsBackend()->q);
    s_globalAbsBackend()->q = this;

    d->q = this;
}

bool Backend::prepareBuildEnvironment(const QString& package, const QString& path, bool privileged) const
{
    QString abspath(d->absPath(package));

    if (abspath.isEmpty()) {
        qDebug() << "Couldn't find a matching ABS Dir!!";
        return false;
    }

    if (privileged) {
        if (!d->initWorker("org.chakraproject.aqpm.preparebuildenvironment")) {
            return false;
        }

        QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmabsworker",
                "/Worker",
                "org.chakraproject.aqpmabsworker",
                QLatin1String("prepareBuildEnvironment"));
        message << abspath;
        message << path;
        QDBusMessage reply = QDBusConnection::systemBus().call(message, QDBus::BlockWithGui);

        if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().count() < 1) {
            return false;
        } else {
            return reply.arguments().first().toBool();
        }

        return false;
    }

    QDir pathDir(path);

    if (!pathDir.mkpath(path)) {
        return false;
    }

    QDir absPDir(abspath);
    absPDir.setFilter(QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    QFileInfoList Plist = absPDir.entryInfoList();

    for (int i = 0; i < Plist.size(); ++i) {
        QString dest(path);
        if (!dest.endsWith(QChar('/'))) {
            dest.append(QChar('/'));
        }
        dest.append(Plist.at(i).fileName());

        qDebug() << "Copying " << Plist.at(i).absoluteFilePath() << " to " << dest;

        if (!QFile::copy(Plist.at(i).absoluteFilePath(), dest)) {
            return false;
        }
    }

    return true;
}

void Backend::update(const QStringList& targets, bool tarball)
{
    if (!d->initWorker("org.chakraproject.aqpm.updateabs")) {
        emit operationFinished(false);
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmabsworker",
              "/Worker",
              "org.chakraproject.aqpmabsworker",
              QLatin1String("update"));
    message << targets;
    message << tarball;
    QDBusConnection::systemBus().asyncCall(message);
}

void Backend::updateAll(bool tarball)
{
    if (!d->initWorker("org.chakraproject.aqpm.updateabs")) {
        emit operationFinished(false);
        return;
    }

    QDBusMessage message = QDBusMessage::createMethodCall("org.chakraproject.aqpmabsworker",
              "/Worker",
              "org.chakraproject.aqpmabsworker",
              QLatin1String("updateAll"));
    message << tarball;
    QDBusConnection::systemBus().asyncCall(message);
}

void Backend::setShouldHandleAuthorization(bool should)
{
    d->handleAuth = should;
}

bool Backend::shouldHandleAuthorization() const
{
    return d->handleAuth;
}

}

}

#include "AbsBackend.moc"
