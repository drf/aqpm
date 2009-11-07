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

#ifndef BUILDERBACKEND_H
#define BUILDERBACKEND_H

#include "Visibility.h"

#include <QtCore/QObject>
#include <QtCore/QStringList>

namespace Aqpm
{

namespace Builder
{

class AQPM_EXPORT QueueItem
{
public:
    enum Option {
        None = 0,
        IgnoreArch = 1,
        Clean = 2,
        CleanCache = 4,
        NoDeps = 8,
        SkipIntegrityChecks = 16,
        HoldVersion = 32
    };
    Q_DECLARE_FLAGS(Options, Option)

    QueueItem();
    QueueItem(const QString &path, Options options);

    typedef QList<QueueItem> List;

    QString environmentPath;
    Options options;
};

class AQPM_EXPORT Backend : public QObject
{
    Q_OBJECT

public:
    enum Status {
        Queued = 0,
        Processing = 1,
        Succeeded = 2,
        Failed = 3
    };

    static Backend *instance();

    virtual ~Backend();

    bool addBuildEnvironmentToQueue(const QString &path, QueueItem::Options options = QueueItem::None) const;
    QueueItem::List queue() const;

    QStringList makeDependsForQueue() const;
    QStringList dependsForQueue() const;
    QStringList allDependsForQueue() const;

    void processQueue();
    void clearQueue();

Q_SIGNALS:
    void operationFinished(bool result);
    void operationProgress(const QString &progress);
    void buildEnvironmentStatusChanged(const QString &environment, Aqpm::Builder::Backend::Status status);

private:
    Backend(QObject* parent = 0);

    class Private;
    Private * const d;

    Q_PRIVATE_SLOT(d, void __k__itemFinished(int, QProcess::ExitStatus))
    Q_PRIVATE_SLOT(d, void __k__processOutput())
    Q_PRIVATE_SLOT(d, void __k__processNextItem())
};

}

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Aqpm::Builder::QueueItem::Options)

#endif // BUILDERBACKEND_H
