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

#ifndef BACKEND_H
#define BACKEND_H

#include <alpm.h>

#include "misc/Visibility.h"

#include <QThread>
#include <QPointer>
#include <QTextStream>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QStringList>

class TrCommitThread : public QThread
{
    Q_OBJECT

public:
    TrCommitThread(pmtranstype_t type, const QStringList &packages, QObject *parent = 0);
    void run();

    bool isError() {
        return m_error;
    }

signals:
    void error(const QString &errorString);

private:
    bool m_error;
    pmtranstype_t m_type;
    QStringList m_packages;
};

class UpDbThread : public QThread
{
    Q_OBJECT

public:
    explicit UpDbThread(pmdb_t *db, QObject *parent = 0);
    void run();

    bool isError() {
        return m_error;
    }

signals:
    void error(const QString &errorString);

private:
    pmdb_t *m_db;
    bool m_error;
};

class AQPM_EXPORT Backend : public QObject
{
    Q_OBJECT

public:

    enum InstallMode {
        KeepPackages = 1,
        InstallBase = 2,
        InstallComplete = 4,
        NewPackages = 8
    };

    static Backend *instance();

    Backend();
    virtual ~Backend();

    QMutex *backendMutex();
    QWaitCondition *backendWCond();

    void initAlpm();

    void updateKdemodDatabase();
    QStringList getUpgradeablePackages();

    void removeAllKdemodPackages();
    void installKdemod(InstallMode mode, const QStringList &packages = QStringList());

    QStringList getInstalledKdemodPackages();
    QStringList getAvailableKdemodPackages();

Q_SIGNALS:
    void dbUpdated();

    void streamTransDlProg(char *c, int bytedone, int bytetotal, int speed,
                           int listdone, int listtotal, int speedtotal);

    void streamTransProgress(pmtransprog_t event, char *pkgname, int percent,
                             int howmany, int remain);

    void streamTransEvent(pmtransevt_t event, void *data1, void *data2);

    void errorOccurred(const QString &errorString);

    void operationSuccessful();
    void operationFailed();

private Q_SLOTS:
    void computeTransactionResult();

private:
    class Private;
    Private *d;
};

#endif /* BACKEND_H */
