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

#include "Worker.h"

#include <QCoreApplication>
#include <QStringList>
#include <QDebug>

#include <alpm.h>
#include <signal.h>
#include <stdio.h>

static void cleanup( int signum )
{
    if ( signum == SIGSEGV ) {
        // Try at least to recover
        alpm_trans_interrupt();
        alpm_trans_release();
        exit(signum);
    } else if ((signum == SIGINT)) {
        if (alpm_trans_interrupt() == 0) {
            /* a transaction is being interrupted, don't exit the worker yet. */
            return;
        }
        /* no committing transaction, we can release it now and then exit pacman */
        alpm_trans_release();
        /* output a newline to be sure we clear any line we may be on */
        printf("\n");
    }

    /* free alpm library resources */
    if (alpm_release() == -1) {
        qCritical() << QString::fromLocal8Bit(alpm_strerrorlast());
    }

    exit(signum);
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setOrganizationName("chakra");
    QCoreApplication::setOrganizationDomain("chakra-project.org");
    QCoreApplication::setApplicationName("aqpmworker");
    QCoreApplication::setApplicationVersion("0.2");

    QStringList arguments = app.arguments();

    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    signal(SIGSEGV, cleanup);

    AqpmWorker::Worker w;

    app.exec();
}
