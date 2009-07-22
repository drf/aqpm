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

#ifndef CONFIGURATIONPARSER_H
#define CONFIGURATIONPARSER_H

#include "Visibility.h"

#include <iostream>
#include <string>
#include <alpm.h>
#include <alpm_list.h>
#include <string.h>

#include <QString>
#include <QStringList>
#include <QPointer>

using namespace std;

namespace Aqpm
{

class PacmanConf
{
public:
    QStringList syncdbs;
    QStringList NoUpgrade;
    QStringList NoExtract;
    QStringList IgnorePkg;
    QStringList IgnoreGrp;
    QStringList HoldPkg;
    int noPassiveFTP;
    int useDelta;
    int useSysLog;
    QString xferCommand;
    QString logFile;
    QStringList serverAssoc;
    bool loaded;
};

class ABSConf
{
public:
    QString supfiles;
    QString rsyncsrv;
    bool loaded;
};

class MakePkgConf
{
public:
    QString cflags;
    QString cxxflags;
    QString buildenv;
    QString options;
    QString docdirs;
    bool loaded;
};

class AQPM_EXPORT ConfigurationParser
{
public:

    static ConfigurationParser *instance();

    ConfigurationParser();
    virtual ~ConfigurationParser();

    PacmanConf getPacmanConf(bool forcereload = false);
    ABSConf getABSConf(bool forcereload = false);
    MakePkgConf getMakepkgConf(bool forcereload = false);

    bool editPacmanKey(const QString &key, const QString &value, int action);
    bool editABSSection(const QString &section, const QString &value);
    bool editMakepkgSection(const QString &section, const QString &value);

private:
    class Private;
    Private *d;
};

}

#endif /*CONFIGURATIONPARSER_H*/
