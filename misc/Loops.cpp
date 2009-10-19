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

#include "Loops.h"

#include <QCoreApplication>
#include <QDebug>

#include "libaqpm/BackendThread.h"
#include "libaqpm/ConfigurationThread.h"
#include "libaqpm/ActionEvent.h"

namespace Aqpm {

SynchronousLoop::SynchronousLoop(Backend::ActionType type, const QVariantMap &args)
        : QEventLoop(0)
        , m_result(QVariantMap())
        , m_type((int)type)
{
    connect(Backend::instance()->getInnerThread(), SIGNAL(actionPerformed(int,QVariantMap)),
            this, SLOT(actionPerformed(int,QVariantMap)));

    QCoreApplication::postEvent(Backend::instance()->getInnerThread(),
                                new ActionEvent(Backend::instance()->getEventTypeFor(Backend::PerformAction), type, args));

    exec();
}

SynchronousLoop::SynchronousLoop(Configuration::ActionType type, const QVariantMap &args)
        : QEventLoop(0)
        , m_result(QVariantMap())
        , m_type((int)type)
{
    connect(Configuration::instance()->getInnerThread(), SIGNAL(actionPerformed(int,QVariantMap)),
            this, SLOT(actionPerformed(int,QVariantMap)));

    QCoreApplication::postEvent(Configuration::instance()->getInnerThread(),
                                new ActionEvent(Configuration::instance()->eventType(), type, args));

    exec();
}

void SynchronousLoop::actionPerformed(int type, const QVariantMap &result)
{
    if ((Backend::ActionType)type == m_type) {
        m_result = result;
        exit();
    }
}

QVariantMap SynchronousLoop::result() const
{
    return m_result;
}

StringConditionalEventLoop::StringConditionalEventLoop(const QString &str, QObject *parent)
        : QEventLoop(parent)
        , m_cond(str)
{
}

StringConditionalEventLoop::~StringConditionalEventLoop()
{
}

void StringConditionalEventLoop::requestQuit(const QString &str)
{
    if (str == m_cond) {
        quit();
    }
}

}
