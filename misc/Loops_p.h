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

#ifndef LOOPS_P_H
#define LOOPS_P_H

#include <QtCore/QEventLoop>

#include "libaqpm/Backend.h"
#include "libaqpm/Configuration.h"

namespace Aqpm
{

class SynchronousLoop : public QEventLoop
{
    Q_OBJECT

public:
    SynchronousLoop(Backend::ActionType type, const QVariantMap &args);
    SynchronousLoop(Configuration::ActionType type, const QVariantMap &args);

    QVariantMap result() const;

public Q_SLOTS:
    void actionPerformed(int type, const QVariantMap &result);

private:
    QVariantMap m_result;
    int m_type;
};

class StringConditionalEventLoop : public QEventLoop
{
    Q_OBJECT

public:
    explicit StringConditionalEventLoop(const QString &str, QObject *parent = 0);
    virtual ~StringConditionalEventLoop();

public Q_SLOTS:
    void requestQuit(const QString &str);

private:
    QString m_cond;
};

}

#endif // LOOPS_P_H
