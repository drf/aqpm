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

#include <QEventLoop>
#include <libaqpmaur/AurBackend.h>

class PackageListConditionalEventLoop : public QEventLoop
{
    Q_OBJECT

    public:
        PackageListConditionalEventLoop(const QString &str, QObject *parent = 0) : QEventLoop(parent), m_cond(str) {}
        ~PackageListConditionalEventLoop() {};

    public Q_SLOTS:
        void requestQuit(const QString &str, const Aqpm::Aur::Package::List &p);

        inline Aqpm::Aur::Package::List packageList() const { return m_list; }

    private:
        QString m_cond;
        Aqpm::Aur::Package::List m_list;
};

class PackageConditionalEventLoop : public QEventLoop
{
    Q_OBJECT

    public:
        PackageConditionalEventLoop(int id, QObject *parent = 0) : QEventLoop(parent), m_id(id) {}
        ~PackageConditionalEventLoop();

    public Q_SLOTS:
        void requestQuit(int id, const Aqpm::Aur::Package &p) {
            if (m_id == id) {
                m_package = p;
                quit();
            }
        }

        inline Aqpm::Aur::Package package() const { return m_package; }

    private:
        int m_id;
        Aqpm::Aur::Package m_package;
};
