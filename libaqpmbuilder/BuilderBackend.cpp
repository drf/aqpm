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

#include "BuilderBackend.h"

#include <QFile>
#include <qtextstream.h>
#include <QPointer>
#include <QProcess>
#include <QTimer>
#include <QDebug>

namespace Aqpm
{

namespace Builder
{

class BuilderBackendHelper
{
public:
    BuilderBackendHelper() : q(0) {}
    ~BuilderBackendHelper() {
        delete q;
    }
    Backend *q;
};

Q_GLOBAL_STATIC(BuilderBackendHelper, s_globalBuilderBackend)

Backend *Backend::instance()
{
    if (!s_globalBuilderBackend()->q) {
        new Backend;
    }

    return s_globalBuilderBackend()->q;
}

class Backend::Private
{
public:
    Private() {}

    QStringList fieldToStringList(const QString &pkgbuild, const QString &field) const;
    QStringList optionsToStringList(QueueItem::Options options) const;

    // Q_PRIVATE_SLOTS
    void __k__itemFinished(int code, QProcess::ExitStatus eS);
    void __k__processOutput();
    void __k__processNextItem();

    Backend *q;
    QueueItem::List queue;
    QueueItem::List::const_iterator queueIterator;
    QPointer<QProcess> process;
};

QStringList Backend::Private::fieldToStringList(const QString& pkgbuild, const QString &field) const
{
    QFile fp(pkgbuild);
    if (!fp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringList();
    }

    QTextStream in(&fp);
    QStringList retlist;

    while (!in.atEnd()) {
        QString line = in.readLine();

        if (!line.startsWith(field)) {
            continue;
        }

        QString testline(line);
        testline.remove(' ');

        while (true) {
            if (line.contains('(')) {
                line = line.split('(').at(1);
            }

            foreach(const QString &rdep, line.split(QChar('\''), QString::SkipEmptyParts)) {
                QString dep(rdep);

                if (!dep.contains(')') && !dep.contains(' ')) {
                    if (dep.contains('>')) {
                        dep.truncate(dep.indexOf('>'));
                    }

                    if (dep.contains('<')) {
                        dep.truncate(dep.indexOf('<'));
                    }

                    if (dep.contains('=')) {
                        dep.truncate(dep.indexOf('='));
                    }

                    qDebug() << "Adding" << dep;

                    retlist.append(dep);
                }
            }

            if (testline.endsWith(')')) {
                break;
            }

            line = in.readLine();

            testline = line;
            testline.remove(' ');
        }

        break;
    }

    fp.close();
    return retlist;
}

QStringList Backend::Private::optionsToStringList(QueueItem::Options options) const
{
    QStringList retlist;

    if (options & QueueItem::Clean) {
        retlist.append("--clean");
    }
    if (options & QueueItem::CleanCache) {
        retlist.append("--cleancache");
    }
    if (options & QueueItem::HoldVersion) {
        retlist.append("--holdver");
    }
    if (options & QueueItem::IgnoreArch) {
        retlist.append("--ignorearch");
    }
    if (options & QueueItem::NoDeps) {
        retlist.append("--nodeps");
    }
    if (options & QueueItem::SkipIntegrityChecks) {
        retlist.append("--skipinteg");
    }

    return retlist;
}

void Backend::Private::__k__processNextItem()
{
    if (queueIterator == queue.constEnd()) {
        // Queue finished! If we got here, it's a success
        emit q->operationFinished(true);

        // Cleanup
        if (process) {
            process->deleteLater();
        }
        queue.clear();

        return;
    }

    if (process) {
        process->deleteLater();
    }

    process = new QProcess();

    process->setWorkingDirectory((*queueIterator).environmentPath);

    connect(process, SIGNAL(readyReadStandardOutput()),
            q, SLOT(__k__processOutput()));
    connect(process, SIGNAL(readyReadStandardError()),
            q, SLOT(__k__processOutput()));
    connect(process, SIGNAL(finished(int, QProcess::ExitStatus)),
            q, SLOT(__k__itemFinished(int, QProcess::ExitStatus)));

    process->setReadChannelMode(QProcess::MergedChannels);

    if (optionsToStringList((*queueIterator).options).isEmpty()) {
        process->start("makepkg");
    } else {
        process->start("makepkg", optionsToStringList((*queueIterator).options));
    }

    emit q->buildEnvironmentStatusChanged((*queueIterator).environmentPath, Backend::Processing);
}

void Backend::Private::__k__processOutput()
{
    emit q->operationProgress(QString::fromLocal8Bit(process->readLine(1024)));
}

void Backend::Private::__k__itemFinished(int code, QProcess::ExitStatus /*eS*/)
{
    if (code != 0) {
        // Item has failed. Clean it up
        emit q->buildEnvironmentStatusChanged((*queueIterator).environmentPath, Backend::Failed);
        emit q->operationFinished(false);

        if (process) {
            process->deleteLater();
        }
        queue.clear();

        return;
    } else {
        // Success! Notify and move on
        emit q->buildEnvironmentStatusChanged((*queueIterator).environmentPath, Backend::Succeeded);
        ++queueIterator;
        // Don't clutter the call stack, stack it into the event loop instead
        QTimer::singleShot(0, q, SLOT(__k__processNextItem()));
    }
}

Backend::Backend(QObject* parent)
        : QObject(parent)
        , d(new Private)
{
    Q_ASSERT(!s_globalBuilderBackend()->q);
    s_globalBuilderBackend()->q = this;

    d->q = this;
}

Backend::~Backend()
{
}

bool Backend::addBuildEnvironmentToQueue(const QString& path, QueueItem::Options options) const
{
    if (!QFile::exists(path + "/PKGBUILD")) {
        return false;
    }

    d->queue.append(QueueItem(path, options));
    return true;
}

void Backend::clearQueue()
{
    d->queue.clear();
}

QueueItem::List Backend::queue() const
{
    return d->queue;
}

void Backend::processQueue()
{
    d->queueIterator = d->queue.constBegin();
    QTimer::singleShot(0, this, SLOT(__k__processNextItem()));
}

QStringList Backend::dependsForQueue() const
{
    QStringList retlist;
    foreach(const QueueItem &item, d->queue) {
        retlist.append(d->fieldToStringList(item.environmentPath + "/PKGBUILD", "depends"));
    }
    retlist.removeDuplicates();
    return retlist;
}

QStringList Backend::makeDependsForQueue() const
{
    QStringList retlist;
    foreach(const QueueItem &item, d->queue) {
        retlist.append(d->fieldToStringList(item.environmentPath + "/PKGBUILD", "makedepends"));
    }
    retlist.removeDuplicates();
    return retlist;
}

QStringList Backend::allDependsForQueue() const
{
    QStringList retlist;
    foreach(const QueueItem &item, d->queue) {
        retlist.append(d->fieldToStringList(item.environmentPath + "/PKGBUILD", "depends"));
        retlist.append(d->fieldToStringList(item.environmentPath + "/PKGBUILD", "makedepends"));
    }
    retlist.removeDuplicates();
    return retlist;
}

//

QueueItem::QueueItem()
{

}

QueueItem::QueueItem(const QString& path, QueueItem::Options opt)
        : environmentPath(path)
        , options(opt)
{

}

}

}

#include "BuilderBackend.moc"
