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

#ifndef BUILDINGHANDLER_H
#define BUILDINGHANDLER_H

#include <QProcess>

namespace Aqpm
{

class BuildItem
{
public:
    BuildItem() {};
    explicit BuildItem(const QString &n, const QString &p = QString())
            : name(n),
            env_path(p) {};

    typedef QList<BuildItem*> List;

    QString name;
    QString env_path;
};

class BuildingHandler : public QObject
{
    Q_OBJECT

public:

    BuildingHandler();
    virtual ~BuildingHandler();

    void addItemToQueue(BuildItem *itm);

    void clearQueue();
    void processQueue();

    QStringList getBuiltPackages();
    void clearBuiltPackages();

Q_SIGNALS:
    void buildingStarted();
    void buildingFinished(bool success);

    void newOutput(const QString &output, QProcess::ProcessChannel channel);

    void packageBuilt(const QString &path);

private Q_SLOTS:
    void processOutput();

private:
    void processNextItem();

private:
    class Private;
    Private *d;
};

}

#endif /* BUILDINGHANDLER_H */
