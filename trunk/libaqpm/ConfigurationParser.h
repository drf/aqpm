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

#ifndef CONFIGURATIONPARSER_H
#define CONFIGURATIONPARSER_H

#include <iostream>
#include <string>
#include <alpm.h>
#include <alpm_list.h>
#include <string.h>

#include <QString>
#include <QStringList>
#include <QPointer>

using namespace std;

struct PcCnf {
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

struct absCnf {
    QString supfiles;
    QString rsyncsrv;
    bool loaded;
};

struct mkpkCnf {
    QString cflags;
    QString cxxflags;
    QString buildenv;
    QString options;
    QString docdirs;
    bool loaded;
};

typedef struct PcCnf PacmanConf;
typedef struct absCnf ABSConf;
typedef struct mkpkCnf MakePkgConf;

class ConfigurationParser
{
    /* This parser will play bad tricks with your guts. It can parse
     * and write on pacman.conf, abs.conf and makepkg.conf. It is tha
     * tool to edit alpm configuration in any way you like. Can be used
     * with inheritance (best, since it tries to avoid parsing the same
     * file multiple times), or just by using it as an object.
     */

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

protected:
    QStringList setrepeatingoption(const QString &ptr);

private:
    void parsePacmanConfig(const QString &file, const QString &givensection,
                           const QString &givendb);
    void parseABSConfig();
    void parseMakepkgConfig();

private:
    PacmanConf pacData;
    ABSConf absData;
    MakePkgConf makepkgData;
};

#endif /*CONFIGURATIONPARSER_H*/
