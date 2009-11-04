// This is an example not a library
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef BUILDER_H
#define BUILDER_H

#include <QCoreApplication>
#include <QEventLoop>
#include <QDebug>

#include "../BuilderBackend.h"
#include <QDir>

class BuilderTest : public QObject
{
    Q_OBJECT

    public:
        BuilderTest() {}

    public Q_SLOTS:
        void buildFinished(bool result);
        void newOutput(const QString &output);
        void buildEnvironmentStatusChanged(const QString &package, Aqpm::Builder::Backend::Status status);
};

#endif /*ABSTEST_H*/
