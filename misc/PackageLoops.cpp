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

#include "PackageLoops_p.h"

void PackageListConditionalEventLoop::requestQuit(const QString &str, const Aqpm::Aur::Package::List &p)
{
    if (m_cond == str) {
        m_list = p;
        quit();
    }
}

IntConditionalEventLoop::IntConditionalEventLoop(int id, QObject* parent)
: QEventLoop(parent)
, m_id(id)
{

}

IntConditionalEventLoop::~IntConditionalEventLoop()
{

}

void IntConditionalEventLoop::requestQuit(int id)
{
    if (m_id == id) {
        quit();
    }
}
