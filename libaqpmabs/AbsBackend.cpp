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
    Private() {}

    QString absPath(const QString &package) const;
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

Backend::Backend(QObject* parent)
        : QObject(parent)
        , d(new Private)
{

}

bool Backend::prepareBuildEnvironment(const QString& package, const QString& p, bool privileged) const
{
    QString path = path;

    if (!path.endsWith(QChar('/'))) {
        path.append(QChar('/'));
    }

    path.append(package);

    QDir pathDir(path);

    if (!pathDir.mkpath(path)) {
        return false;
    }

    QString abspath(d->absPath(package));

    if (abspath.isEmpty()) {
        qDebug() << "Couldn't find a matching ABS Dir!!";
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

}

void Backend::updateAll(bool tarball)
{

}

}

}

#include "Backend.moc"
