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

#include "ConfigurationParser.h"
#include "ConfigurationParser_p.h"

#include <iostream>
#include <string>
#include <alpm.h>
#include <alpm_list.h>
#include <stdlib.h> /* atoi */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h> /* time_t */

#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include "misc/Singleton.h"

namespace Aqpm
{

class ConfigurationParserHelper
{
public:
    ConfigurationParserHelper() : q(0) {}
    ~ConfigurationParserHelper() {
        delete q;
    }
    ConfigurationParser *q;
};

AQPM_GLOBAL_STATIC(ConfigurationParserHelper, s_globalParser)

ConfigurationParser *ConfigurationParser::instance()
{
    if (!s_globalParser->q) {
        new ConfigurationParser;
    }

    return s_globalParser->q;
}

QStringList ConfigurationParser::Private::setrepeatingoption(const QString &ptr)
{
    QStringList strlist;
    QStringList list;

    strlist = ptr.split(' ', QString::SkipEmptyParts);

    for (int i = 0; i < strlist.size(); ++i) {
        QString dest(strlist.at(i));

        list.append(dest);
    }

    return list;
}

void ConfigurationParser::Private::parsePacmanConfig(const QString &file, const QString &givensection,
        const QString &givendb)
{
    QFile fp(file);
    QString db(NULL), section(NULL);
    int linenum = 0, serverparsed = 0;

    if (!pacData.loaded) {
        pacData.syncdbs.clear();
        pacData.serverAssoc.clear();
    }

    pacData.loaded = true;

    if (!fp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        pacData.loaded = false;
        return;
    }

    QTextStream in(&fp);

    /* if we are passed a section, use it as our starting point */
    if (givensection != NULL)
        section.operator = (givensection);

    /* if we are passed a db, use it as our starting point */
    if (givendb != NULL)
        db.operator = (givendb);

    while (!in.atEnd()) {
        QString line = in.readLine();
        linenum++;
        //strtrim(line);
        while (line.contains('\n'))
            line.remove(line.indexOf('\n'), 1);

        /* ignore whole line and end of line comments */
        if (line.length() == 0 || line.startsWith('#'))
            continue;

        if (line.indexOf("#") != -1)
            line.truncate(line.indexOf("#"));

        if (line.contains('[') && line.contains(']')) {
            while (line.contains(' '))
                line.remove(' ');
        }


        if (line.startsWith('[') && line.endsWith(']')) {
            /* new config section, skip the '[' */
            line.remove(0, 1);
            line.remove(line.length() - 1, line.length());
            section.operator = (line);
            while (section.contains(' '))
                section.remove(section.indexOf(' '), 1);

            if (section.operator != ("options")) {
                QString toAdd(section);
                if (!pacData.syncdbs.contains(toAdd)) {
                    pacData.syncdbs.append(toAdd);
                    serverparsed = 0;
                }

                db.operator = (line);

                qDebug() << "Parsing " << toAdd;
            }
        } else {
            /* directive */
            QString key;
            QStringList templst;
            /* strsep modifies the 'line' string: 'key \0 ptr' */
            if (line.contains(QChar('='))) {
                templst = line.split(QChar('='));
                if (templst.size() != 2) {
                    pacData.loaded = false;
                    qDebug() << "= error";
                    return;
                }
                key.operator = (templst.at(0));
                line.operator = (templst.at(1));
            } else {
                key = line;
                line.clear();
            }

            if (key == NULL) {
                pacData.loaded = false;
                qDebug() << "Key not found error";
                return;
            }

            if (section == NULL) {
                pacData.loaded = false;
                qDebug() << "Section not found error";
                return;
            }

            while (key.contains(' '))
                key.remove(key.indexOf(' '), 1);

            if (line.isEmpty() && section.compare("options", Qt::CaseInsensitive) == 0) {
                /* directives without settings, all in [options] */
                if (key.compare("NoPassiveFTP", Qt::CaseInsensitive) == 0)
                    pacData.noPassiveFTP = 1;
                else if (key.compare("UseSyslog", Qt::CaseInsensitive) == 0)
                    pacData.useSysLog = 1;
                else if (key.compare("UseDelta", Qt::CaseInsensitive) == 0)
                    pacData.useDelta = 1;
                else if (key.compare("UseDelta", Qt::CaseInsensitive) == 0 ||
                         key.compare("ILoveCandy", Qt::CaseInsensitive) == 0)
                    continue;

            } else {
                /* directives with settings */
                if (key.compare("Include", Qt::CaseInsensitive) == 0) {
                    if (section == "options" || serverparsed != 1) {
                        while (line.contains(' '))
                            line.remove(line.indexOf(' '), 1);

                        if (QFile::exists(line)) {
                            parsePacmanConfig(line, section, db);
                            if (!pacData.loaded) {
                                pacData.loaded = false;
                                qDebug() << "Include error";
                                return;
                            }
                        } else if (serverparsed != 1)
                            pacData.syncdbs.removeLast();
                    }
                }
                /* Ignore include failures... assume non-critical */
                else if (section.compare("options", Qt::CaseInsensitive) == 0) {
                    if (key.compare("NoUpgrade", Qt::CaseInsensitive) == 0)
                        if (!pacData.NoUpgrade.isEmpty())
                            pacData.NoUpgrade = pacData.NoUpgrade + setrepeatingoption(line);
                        else
                            pacData.NoUpgrade = setrepeatingoption(line);

                    else if (key.compare("NoExtract", Qt::CaseInsensitive) == 0)
                        if (!pacData.NoExtract.isEmpty())
                            pacData.NoExtract = pacData.NoExtract + setrepeatingoption(line);
                        else
                            pacData.NoExtract = setrepeatingoption(line);

                    else if (key.compare("IgnorePkg", Qt::CaseInsensitive) == 0)
                        if (!pacData.IgnorePkg.isEmpty())
                            pacData.IgnorePkg = pacData.IgnorePkg + setrepeatingoption(line);
                        else
                            pacData.IgnorePkg = setrepeatingoption(line);

                    else if (key.compare("IgnoreGroup", Qt::CaseInsensitive) == 0)
                        if (!pacData.IgnoreGrp.isEmpty())
                            pacData.IgnoreGrp = pacData.IgnoreGrp + setrepeatingoption(line);
                        else
                            pacData.IgnoreGrp = setrepeatingoption(line);

                    else if (key.compare("HoldPkg", Qt::CaseInsensitive) == 0)
                        if (!pacData.HoldPkg.isEmpty())
                            pacData.HoldPkg = pacData.HoldPkg + setrepeatingoption(line);
                        else
                            pacData.HoldPkg = setrepeatingoption(line);

                    else if (key.compare("LogFile", Qt::CaseInsensitive) == 0) {
                        while (line.startsWith(QChar(' ')))
                            line.remove(0, 1);

                        pacData.logFile = line;

                        qDebug() << "Log File will be:" << line;
                    }

                    else if (key.compare("XferCommand", Qt::CaseInsensitive) == 0) {
                        while (line.startsWith(QChar(' ')))
                            line.remove(0, 1);

                        pacData.xferCommand = line;
                    }

                } else if (key.compare("Server", Qt::CaseInsensitive) == 0) {
                    if (serverparsed == 1)
                        continue;

                    serverparsed = 1;
                    /* let's attempt a replacement for the current repo */
                    while (line.contains(' '))
                        line.remove(line.indexOf(' '), 1);


                    if (line.contains('$')) {
                        QStringList tmplst = line.split(QString("$repo"),
                                                        QString::SkipEmptyParts, Qt::CaseInsensitive);
                        QString toAdd(tmplst.at(0));

                        toAdd.append(section);
                        toAdd.append(tmplst.at(1));

                        pacData.serverAssoc.append(toAdd);
                    } else {
                        pacData.serverAssoc.append(line);
                    }
                } else {
                    qDebug() << "Unknown error:" << key << line;
                    pacData.loaded = false;
                    return;
                }
            }
        }
    }
    fp.close();

    qDebug() << "Parser exited";
}

void ConfigurationParser::Private::parseABSConfig()
{
    QFile fp("/etc/abs.conf");

    absData.loaded = true;

    if (!fp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        absData.loaded = false;
        return;
    }

    QTextStream in(&fp);

    while (!in.atEnd()) {
        QString line = in.readLine();

        if (line.startsWith("REPOS")) {

            line.truncate(line.indexOf(QChar(')')));
            line = line.remove(0, line.indexOf(QChar('(')) + 1);

            absData.supfiles = line;
        } else if (line.startsWith("SYNCSERVER")) {
            line = line.remove(0, line.indexOf(QChar('"')) + 1);
            line.truncate(line.indexOf(QChar('"')));

            absData.rsyncsrv = line;
        }
    }

    fp.close();
}

void ConfigurationParser::Private::parseMakepkgConfig()
{
    QFile fp("/etc/makepkg.conf");

    makepkgData.loaded = true;

    if (!fp.open(QIODevice::ReadOnly | QIODevice::Text)) {
        makepkgData.loaded = false;
        return;
    }

    QTextStream in(&fp);

    while (!in.atEnd()) {
        QString line = in.readLine();

        if (line.startsWith("CFLAGS")) {
            line = line.remove(0, line.indexOf(QChar('"')) + 1);
            line.truncate(line.indexOf(QChar('"')));

            makepkgData.cflags = line;
        } else if (line.startsWith("CXXFLAGS")) {
            line = line.remove(0, line.indexOf(QChar('"')) + 1);
            line.truncate(line.indexOf(QChar('"')));

            makepkgData.cxxflags = line;
        } else if (line.startsWith("BUILDENV")) {
            line.truncate(line.indexOf(QChar(')')));
            line = line.remove(0, line.indexOf(QChar('(')) + 1);

            makepkgData.buildenv = line;
        } else if (line.startsWith("OPTIONS")) {
            line.truncate(line.indexOf(QChar(')')));
            line = line.remove(0, line.indexOf(QChar('(')) + 1);

            makepkgData.options = line;
        } else if (line.startsWith("DOC_DIRS")) {
            line.truncate(line.indexOf(QChar(')')));
            line = line.remove(0, line.indexOf(QChar('(')) + 1);

            makepkgData.docdirs = line;
        } else
            continue;
    }

    fp.close();
}

bool ConfigurationParser::editPacmanKey(const QString &key, const QString &value, int action)
{


    QFile fp("/etc/pacman.conf");
    QStringList list(key.split('/')), fileContent;
    QString key1(list.at(0)), key2(list.at(1)), realVal;

    if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

        return false;
    }

    QTextStream in(&fp);

    if (value != NULL) {
        if (value.contains('$')) {
            QStringList tmplst = value.split(QString("$repo"),
                                             QString::SkipEmptyParts, Qt::CaseInsensitive);
            QStringList tmp2lst = key.split(QString("/"),
                                            QString::SkipEmptyParts, Qt::CaseInsensitive);

            QString dserv(tmplst.at(0));

            QString repo(tmp2lst.at(0));

            if (repo == "kdemod-core")
                repo = "core";
            if (repo == "kdemod-extragear")
                repo = "extragear";
            if (repo == "kdemod-playground")
                repo = "playground";
            if (repo == "kdemod-testing")
                repo = "testing";
            if (repo == "kdemod-unstable")
                repo = "unstable";
            if (repo == "kdemod-legacy")
                repo = "legacy";

            dserv.append(repo);

            dserv.append(tmplst.at(1));
            realVal = dserv;
        } else
            realVal = value;
    }

    while (!in.atEnd()) {
        QString line(in.readLine());
        fileContent.append(line);
    }

    fp.close();

    if (action == 0) {
        // Add
        if (key1.compare("options")) {
            QString toFind("[");
            toFind.append(key1);
            toFind.append("]");
            if (!fileContent.filter(toFind).isEmpty()) {
                for (int i = 0; i < fileContent.size(); ++i) {
                    if (!fileContent.at(i).startsWith(toFind))
                        continue;

                    for (int j = i + 1; j < fileContent.size(); ++j) {
                        if (fileContent.at(j).startsWith(QChar('[')))
                            break;

                        if (fileContent.at(j).startsWith(key2)) {

                            return false;
                        }
                    }
                    QString toAdd2(key2);
                    toAdd2.append(QChar('='));
                    toAdd2.append(realVal);
                    fileContent.insert(i + 1, toAdd2);

                    QFile::remove("/etc/pacman.conf");
                    if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                        return false;
                    }

                    QTextStream str(&fp);

                    for (int i = 0; i < fileContent.size(); ++i)
                        str << fileContent.at(i) << endl;

                    fp.close();

                    return true;

                }
            }

            QString toAdd("[");
            toAdd.append(key1);
            toAdd.append(QChar(']'));
            fileContent.append(toAdd);
            QString toAdd2(key2);
            toAdd2.append(QChar('='));
            toAdd2.append(realVal);
            fileContent.append(toAdd2);

            QFile::remove("/etc/pacman.conf");
            if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                return false;
            }

            QTextStream str(&fp);

            for (int i = 0; i < fileContent.size(); ++i)
                str << fileContent.at(i) << endl;

            fp.close();



            return true;
        } else {
            for (int i = 0; i < fileContent.size(); ++i)
                if (fileContent.at(i).startsWith(key2)) {

                    return false;
                }

            for (int i = 0; i < fileContent.size(); ++i) {
                if (!fileContent.at(i).startsWith("[options]"))
                    continue;
                QString toAdd(key2);

                if (!realVal.isEmpty() || (!key2.compare("UseSyslog", Qt::CaseInsensitive) &&
                                           !key2.compare("NoPassiveFTP", Qt::CaseInsensitive)))
                    toAdd.append("=");

                toAdd.append(realVal);
                fileContent.insert(i + 1, toAdd);

                QFile::remove("/etc/pacman.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                    return false;
                }

                QTextStream str(&fp);

                for (int i = 0; i < fileContent.size(); ++i)
                    str << fileContent.at(i) << endl;

                fp.close();

                return true;

            }

            return false;
        }


        return false;

    } else if (action == 1) {
        //Edit
        QString toFindE("[");
        toFindE.append(key1);
        toFindE.append(QChar(']'));
        if (fileContent.filter(toFindE).isEmpty()) {

            return false;
        }

        int found = 0;

        for (int i = 0; i < fileContent.size(); ++i) {
            if (!fileContent.at(i).startsWith(toFindE) && found == 0)
                continue;

            found = 1;

            if (fileContent.at(i).startsWith(toFindE))
                continue;

            if (fileContent.at(i).startsWith(QChar('['))) {

                return false;
            }

            if (fileContent.at(i).startsWith(key2)) {
                QString toAdd(key2);
                toAdd.append(QChar('='));
                toAdd.append(realVal);
                QString check1(toAdd);
                QString check2(fileContent.at(i));
                check1.remove(' ');
                check2.remove(' ');
                if (!check1.compare(check2)) {

                    return false;
                }

                fileContent.replace(i, toAdd);

                QFile::remove("/etc/pacman.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                    return false;
                }

                QTextStream str(&fp);

                for (int i = 0; i < fileContent.size(); ++i)
                    str << fileContent.at(i) << endl;

                fp.close();



                return true;
            }
        }


        return false;
    } else if (action == 2) {
        //Remove
        QString toFindE("[");
        toFindE.append(key1);
        toFindE.append(QChar(']'));
        if (fileContent.filter(toFindE).isEmpty()) {

            return false;
        }

        int found = 0, i = 0;

        for (i = 0; i < fileContent.size(); ++i) {
            if (!fileContent.at(i).startsWith(toFindE) && found == 0)
                continue;

            found = 1;

            int removeroot = 1;
            bool changed = false;
            for (int j = i + 1; j < fileContent.size(); ++j) {
                if (fileContent.at(j).startsWith(QChar('[')))
                    break;

                if (!fileContent.at(j).startsWith(key2) && !fileContent.at(j).startsWith(QChar('#'))
                        && !fileContent.at(j).isEmpty())
                    removeroot = 0;

                if (fileContent.at(j).startsWith(key2)) {
                    fileContent.removeAt(j);

                    QFile::remove("/etc/pacman.conf");
                    if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                        return false;
                    }

                    QTextStream str(&fp);

                    for (int k = 0; k < fileContent.size(); ++k)
                        str << fileContent.at(k) << endl;

                    fp.close();
                    changed = true;
                }
            }

            if (removeroot == 1) {
                fileContent.removeAt(i);

                QFile::remove("/etc/pacman.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                    return false;
                }

                QTextStream str(&fp);

                for (int k = 0; k < fileContent.size(); ++k)
                    str << fileContent.at(k) << endl;

                fp.close();

                return true;
            }

            if (changed == false)
                return false;

            return true;
        }
        return false;
    } else
        return false;

    return false;
}

bool ConfigurationParser::editABSSection(const QString &section, const QString &value)
{

    QFile fp("/etc/abs.conf");
    QStringList fileContent;

    if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

        return false;
    }

    QTextStream in(&fp);

    /* Get the file contents */

    while (!in.atEnd()) {
        QString line(in.readLine());
        fileContent.append(line);
    }

    fp.close();

    if (section == "repos") {
        QString val("REPOS=(");
        val.append(value);
        val.append(')');

        if (!fileContent.filter("REPOS").isEmpty()) {
            for (int i = 0; i < fileContent.size(); ++i) {
                if (!fileContent.at(i).startsWith("REPOS"))
                    continue;

                if (fileContent.at(i) == val)
                    // No need to edit
                {

                    return true;
                }

                fileContent.replace(i, val);

                QFile::remove("/etc/abs.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                    return false;
                }

                QTextStream str(&fp);

                for (int k = 0; k < fileContent.size(); ++k)
                    str << fileContent.at(k) << endl;

                fp.close();

                return true;
            }


            return false;
        } else {
            fileContent.append(val);

            QFile::remove("/etc/abs.conf");
            if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                return false;
            }

            QTextStream str(&fp);

            for (int k = 0; k < fileContent.size(); ++k)
                str << fileContent.at(k) << endl;

            fp.close();

            return true;
        }
    }
    if (section == "rsync") {
        QString val("SYNCSERVER=\"");
        val.append(value);
        val.append('"');

        if (!fileContent.filter("SYNCSERVER").isEmpty()) {
            for (int i = 0; i < fileContent.size(); ++i) {
                if (!fileContent.at(i).startsWith("SYNCSERVER"))
                    continue;

                if (fileContent.at(i) == val)
                    // No need to edit
                {

                    return true;
                }

                fileContent.replace(i, val);

                QFile::remove("/etc/abs.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                    return false;
                }

                QTextStream str(&fp);

                for (int k = 0; k < fileContent.size(); ++k)
                    str << fileContent.at(k) << endl;

                fp.close();

                return true;
            }

            return false;
        } else {
            fileContent.append(val);

            QFile::remove("/etc/abs.conf");
            if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {

                return false;
            }

            QTextStream str(&fp);

            for (int k = 0; k < fileContent.size(); ++k)
                str << fileContent.at(k) << endl;

            fp.close();

            return true;
        }
    }

    return false;
}

bool ConfigurationParser::editMakepkgSection(const QString &section, const QString &value)
{
    QFile fp("/etc/makepkg.conf");
    QStringList fileContent;

    if (!fp.open(QIODevice::ReadOnly | QIODevice::Text)) {

        return false;
    }

    qDebug() << section << value;

    QTextStream in(&fp);

    /* Get the file contents */

    while (!in.atEnd()) {
        QString line(in.readLine());
        fileContent.append(line);
    }

    fp.close();

    if (section == "cflags") {
        QString val("CFLAGS=\"");
        val.append(value);
        val.append('"');

        qDebug() << "In CFLAGS";

        if (!fileContent.filter("CFLAGS").isEmpty()) {
            for (int i = 0; i < fileContent.size(); ++i) {
                if (!fileContent.at(i).startsWith("CFLAGS"))
                    continue;

                if (fileContent.at(i) == val)
                    // No need to edit
                {
                    return true;
                }

                fileContent.replace(i, val);

                QFile::remove("/etc/makepkg.conf");

                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text)) {
                    return false;
                }

                QTextStream str(&fp);

                for (int k = 0; k < fileContent.size(); ++k)
                    str << fileContent.at(k) << endl;

                fp.close();

                return true;
            }

            return false;
        } else {

            return false;
        }
    } else if (section == "cxxflags") {
        QString val("CXXFLAGS=\"");
        val.append(value);
        val.append('"');

        if (!fileContent.filter("CXXFLAGS").isEmpty()) {
            for (int i = 0; i < fileContent.size(); ++i) {
                if (!fileContent.at(i).startsWith("CXXFLAGS"))
                    continue;

                if (fileContent.at(i) == val)
                    // No need to edit
                    return true;

                fileContent.replace(i, val);

                QFile::remove("/etc/makepkg.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text))
                    return false;

                QTextStream str(&fp);

                for (int k = 0; k < fileContent.size(); ++k)
                    str << fileContent.at(k) << endl;

                fp.close();
                return true;
            }
            return false;
        } else
            return false;
    } else if (section == "buildenv") {
        QString val("BUILDENV=(");
        val.append(value);
        val.append(")");

        if (!fileContent.filter("BUILDENV").isEmpty()) {
            for (int i = 0; i < fileContent.size(); ++i) {
                if (!fileContent.at(i).startsWith("BUILDENV"))
                    continue;

                if (fileContent.at(i) == val)
                    // No need to edit
                    return true;

                fileContent.replace(i, val);

                QFile::remove("/etc/makepkg.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text))
                    return false;

                QTextStream str(&fp);

                for (int k = 0; k < fileContent.size(); ++k)
                    str << fileContent.at(k) << endl;

                fp.close();
                return true;
            }
            return false;
        } else
            return false;
    } else if (section == "options") {
        QString val("OPTIONS=(");
        val.append(value);
        val.append(")");

        if (!fileContent.filter("OPTIONS").isEmpty()) {
            for (int i = 0; i < fileContent.size(); ++i) {
                if (!fileContent.at(i).startsWith("OPTIONS"))
                    continue;

                if (fileContent.at(i) == val)
                    // No need to edit
                    return true;

                fileContent.replace(i, val);

                QFile::remove("/etc/makepkg.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text))
                    return false;

                QTextStream str(&fp);

                for (int k = 0; k < fileContent.size(); ++k)
                    str << fileContent.at(k) << endl;

                fp.close();
                return true;
            }
            return false;
        } else
            return false;
    } else if (section == "docdirs") {
        QString val("DOC_DIRS=(");
        val.append(value);
        val.append(")");

        if (!fileContent.filter("DOC_DIRS").isEmpty()) {
            for (int i = 0; i < fileContent.size(); ++i) {
                if (!fileContent.at(i).startsWith("DOC_DIRS"))
                    continue;

                if (fileContent.at(i) == val)
                    // No need to edit
                    return true;

                fileContent.replace(i, val);

                QFile::remove("/etc/makepkg.conf");
                if (!fp.open(QIODevice::ReadWrite | QIODevice::Text))
                    return false;

                QTextStream str(&fp);

                for (int k = 0; k < fileContent.size(); ++k)
                    str << fileContent.at(k) << endl;

                fp.close();
                return true;
            }
            return false;
        } else
            return false;
    } else
        return false;
}

ConfigurationParser::ConfigurationParser()
        : d(new Private())
{
    Q_ASSERT(!s_globalParser->q);
    s_globalParser->q = this;

    d->initABSData();
    d->initMakepkgData();
    d->initPacData();
}

ConfigurationParser::~ConfigurationParser()
{
    delete d;
}

PacmanConf ConfigurationParser::getPacmanConf(bool forcereload)
{
    if (d->pacData.loaded && !forcereload) {
        return d->pacData;
    }

    d->initPacData();

    d->parsePacmanConfig("/etc/pacman.conf", NULL, NULL);

    return d->pacData;
}

ABSConf ConfigurationParser::getABSConf(bool forcereload)
{
    if (d->absData.loaded && !forcereload) {
        return d->absData;
    }

    d->initABSData();

    d->parseABSConfig();

    return d->absData;
}

MakePkgConf ConfigurationParser::getMakepkgConf(bool forcereload)
{
    if (d->makepkgData.loaded && !forcereload)
        return d->makepkgData;

    d->initMakepkgData();

    d->parseMakepkgConfig();

    return d->makepkgData;
}

}
