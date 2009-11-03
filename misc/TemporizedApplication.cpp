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

#include "TemporizedApplication_p.h"

#include <QCoreApplication>

TemporizedApplication::TemporizedApplication()
        : m_timer(new QTimer(0))
        , m_temporized(false)
{
    m_timer->connect(m_timer, SIGNAL(timeout()), QCoreApplication::instance(), SLOT(quit()));
}

TemporizedApplication::~TemporizedApplication()
{
    m_timer->deleteLater();
}

void TemporizedApplication::setIsTemporized(bool is)
{
    m_temporized = is;
}

bool TemporizedApplication::isTemporized() const
{
    return m_temporized;
}

void TemporizedApplication::setTimeout(int timeout)
{
    m_timer->setInterval(timeout);
}

int TemporizedApplication::timeout() const
{
    return m_timer->interval();
}

void TemporizedApplication::startTemporizing() const
{
    if (isTemporized()) {
        m_timer->start();
    }
}

void TemporizedApplication::stopTemporizing() const
{
    if (isTemporized()) {
        m_timer->stop();
    }
}
