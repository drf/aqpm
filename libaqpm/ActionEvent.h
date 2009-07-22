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

#ifndef ACTIONEVENT_H
#define ACTIONEVENT_H

#include <QEvent>

#include "Backend.h"

namespace Aqpm {

class ActionEvent : public QEvent
{
public:
    ActionEvent(Type type, Backend::ActionType actiontype, const QVariantMap &args);

    Backend::ActionType actionType() const;
    QVariantMap args() const;

private:
    Backend::ActionType m_actionType;
    QVariantMap m_args;
};

}

#endif // ACTIONEVENT_H
