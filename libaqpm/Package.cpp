/***************************************************************************
 *   Copyright (C) 2009 by Dario Freddi                                    *
 *   drf54321@gmail.com                                                    *
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

#include "Package.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>

#include <alpm.h>

#define CLBUF_SIZE 4096
#include <QVariant>
#include <Loops_p.h>

using namespace Aqpm;

class Package::Private
{
public:

    Private(pmpkg_t *pkg);

    pmpkg_t *underlying;
};

Package::Private::Private(pmpkg_t *pkg)
{
    underlying = pkg;
}

Package::Package(pmpkg_t *pkg)
        : d(new Private(pkg))
{
}

Package::Package()
        : d(new Private(NULL))
{
}

Package::~Package()
{
    delete d->underlying;
    delete d;
}

QString Package::arch() const
{
    return alpm_pkg_get_arch(d->underlying);
}

QDateTime Package::builddate() const
{
    return QDateTime::fromTime_t(alpm_pkg_get_builddate(d->underlying));
}

QDateTime Package::installdate() const
{
    return QDateTime::fromTime_t(alpm_pkg_get_installdate(d->underlying));
}

QString Package::desc() const
{
    return alpm_pkg_get_desc(d->underlying);
}

QString Package::filename() const
{
    return alpm_pkg_get_filename(d->underlying);
}

qint32 Package::isize() const
{
    return alpm_pkg_get_isize(d->underlying);
}

QString Package::md5sum() const
{
    return alpm_pkg_get_md5sum(d->underlying);
}

QString Package::name() const
{
    return alpm_pkg_get_name(d->underlying);
}

QString Package::packager() const
{
    return alpm_pkg_get_packager(d->underlying);
}

qint32 Package::size() const
{
    return alpm_pkg_get_size(d->underlying);
}

QString Package::url() const
{
    return alpm_pkg_get_url(d->underlying);
}

QString Package::version() const
{
    return alpm_pkg_get_version(d->underlying);
}

pmpkg_t *Package::alpmPackage() const
{
    return d->underlying;
}

QString Package::retrieveChangelog() const
{
    void *fp = NULL;

    if ((fp = alpm_pkg_changelog_open(d->underlying)) == NULL) {
        return QString();
    } else {
        QString text;
        /* allocate a buffer to get the changelog back in chunks */
        char buf[CLBUF_SIZE];
        int ret = 0;
        while ((ret = alpm_pkg_changelog_read(buf, CLBUF_SIZE, d->underlying, fp))) {
            if (ret < CLBUF_SIZE) {
                /* if we hit the end of the file, we need to add a null terminator */
                *(buf + ret) = '\0';
            }
            text.append(buf);
        }
        alpm_pkg_changelog_close(d->underlying, fp);
        return text;
    }

    return QString();
}

QString Package::retrieveLoggedActions() const
{
    QFile fp(alpm_option_get_logfile());

    QStringList contents;

    if (!fp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    QTextStream in(&fp);

    while (!in.atEnd()) {
        QString line = in.readLine();
        contents.append(line);
    }

    fp.close();

    QString toShow;
    QString pkgName(name());

    foreach(const QString &ent, contents) {
        if (!ent.contains(pkgName, Qt::CaseInsensitive)) {
            continue;
        }

        toShow.append(ent + QChar('\n'));
    }

    return toShow;
}

bool Package::hasScriptlet() const
{
    return alpm_pkg_has_scriptlet(d->underlying);
}

QDateTime Package::buildDate() const
{
    return QDateTime::fromTime_t(alpm_pkg_get_builddate(d->underlying));
}

QDateTime Package::installDate() const
{
    return QDateTime::fromTime_t(alpm_pkg_get_installdate(d->underlying));
}

QStringList Package::files() const
{
    alpm_list_t *files;
    QStringList retlist;

    files = alpm_pkg_get_files(d->underlying);

    while (files != NULL) {
        retlist.append(QString((char*)alpm_list_getdata(files)).prepend(alpm_option_get_root()));
        files = alpm_list_next(files);
    }

    return retlist;
}

bool Package::isValid() const
{
    return d->underlying != NULL;
}

Database* Package::database(bool checkver)
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(this);
    args["checkver"] = QVariant::fromValue(checkver);
    SynchronousLoop s(Backend::GetPackageDatabase, args);
    return s.result()["retvalue"].value<Database*>();
}

Package::List Package::dependsOn() const
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(this);
    SynchronousLoop s(Backend::GetPackageDependencies, args);
    return s.result()["retvalue"].value<Package::List>();
}

Group::List Package::groups() const
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(this);
    SynchronousLoop s(Backend::GetPackageGroups, args);
    return s.result()["retvalue"].value<Group::List>();
}

bool Package::isInstalled()
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(this);
    SynchronousLoop s(Backend::IsInstalled, args);
    return s.result()["retvalue"].toBool();
}

QStringList Package::providers() const
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(this);
    SynchronousLoop s(Backend::GetProviders, args);
    return s.result()["retvalue"].toStringList();
}

Package::List Package::requiredBy() const
{
    QVariantMap args;
    args["package"] = QVariant::fromValue(this);
    SynchronousLoop s(Backend::GetDependenciesOnPackage, args);
    return s.result()["retvalue"].value<Package::List>();
}
