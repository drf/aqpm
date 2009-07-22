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

#ifndef CONFIGURATIONPARSER_P_H
#define CONFIGURATIONPARSER_P_H

#include "ConfigurationParser.h"

namespace Aqpm
{

class ConfigurationParser::Private
{
public:
    Private() {};

    void initPacData();
    void initMakepkgData();
    void initABSData();

    QStringList setrepeatingoption(const QString &ptr);

    void parsePacmanConfig(const QString &file, const QString &givensection,
                           const QString &givendb);
    void parseABSConfig();
    void parseMakepkgConfig();

    PacmanConf pacData;
    ABSConf absData;
    MakePkgConf makepkgData;
};

void ConfigurationParser::Private::initPacData()
{
    pacData.syncdbs.clear();
    pacData.serverAssoc.clear();

    pacData.useDelta = 0;
    pacData.useSysLog = 0;
    pacData.noPassiveFTP = 0;
    pacData.xferCommand.clear();
    pacData.logFile.clear();
    pacData.IgnoreGrp.clear();
    pacData.IgnorePkg.clear();
    pacData.NoExtract.clear();
    pacData.NoUpgrade.clear();
    pacData.HoldPkg.clear();
    pacData.loaded = false;
}

void ConfigurationParser::Private::initMakepkgData()
{
    makepkgData.buildenv.clear();
    makepkgData.cflags.clear();
    makepkgData.cxxflags.clear();
    makepkgData.docdirs.clear();
    makepkgData.options.clear();
    makepkgData.loaded = false;
}

void ConfigurationParser::Private::initABSData()
{
    absData.rsyncsrv.clear();
    absData.supfiles.clear();
    absData.loaded = false;
}

}

#endif /*CONFIGURATIONPARSER_P_H*/
