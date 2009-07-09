/***************************************************************************
 *   Copyright (C) 2008 by Dario Freddi                                    *
 *   drf54321@yahoo.it                                                     *
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

#include "SynchronousLoop.h"

#include "BackendThread.h"

namespace Aqpm {

SynchronousLoop::SynchronousLoop(Backend::ActionType type)
        : m_result(QVariantMap())
        , m_type(type)
{
    connect(Backend::instance()->getInnerThread(), SIGNAL(actionPerformed(Backend::ActionType,QVariantMap)),
            this, SLOT(actionPerformed(Backend::ActionType,QVariantMap)));

    exec();
}

void SynchronousLoop::actionPerformed(Backend::ActionType type, const QVariantMap &result)
{
    if (type == m_type) {
        m_result = result;
        exit();
    }
}

QVariantMap SynchronousLoop::result() const
{
    return m_result;
}

}
