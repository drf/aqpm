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

#ifndef CONFIGURATOR_H
#define CONFIGURATOR_H

#include <QtCore/QObject>
#include <QtDBus/QDBusContext>

#include "TemporizedApplication.h"

namespace AqpmConfigurator
{

class Configurator : public QObject, protected QDBusContext, private TemporizedApplication
{
    Q_OBJECT

public:
    Configurator(bool temporize, QObject *parent = 0);
    virtual ~Configurator();

public Q_SLOTS:
    void saveConfiguration(const QString &conf, const QString &filename);
    void addMirror(const QString &mirror, int type);
    QString pacmanConfToAqpmConf(bool writeconf, const QString &filename);

Q_SIGNALS:
    void errorOccurred(int code, const QVariantMap &details);
    void configuratorResult(bool);

private Q_SLOTS:
    void operationPerformed(bool result);

private:
    class Private;
    Private *d;
};

}

#endif /* CONFIGURATOR_H */
