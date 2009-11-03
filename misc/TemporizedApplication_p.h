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

#ifndef TEMPORIZEDAPPLICATION_P_H
#define TEMPORIZEDAPPLICATION_P_H

#include <QtCore/QTimer>

class TemporizedApplication
{
public:
    TemporizedApplication();
    virtual ~TemporizedApplication();

    void setIsTemporized(bool is);
    bool isTemporized() const;

    void setTimeout(int timeout);
    int timeout() const;

    void startTemporizing() const;
    void stopTemporizing() const;

private:
    QTimer *m_timer;
    bool m_temporized;
};

#endif // TEMPORIZEDAPPLICATION_P_H
