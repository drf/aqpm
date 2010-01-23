// This is an example not a library
/***************************************************************************
*   Copyright (C) 2009 by Dario Freddi                                    *
*   drf@kde.org                                                           *
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
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
***************************************************************************/

#include <QApplication>
#include <QDebug>

#include "../AurBackend.h"
#include <QStringList>
#include <QDir>
#include "main_aursearcher.h"
#include <QTimer>
#include "config-aqpm.h"
#ifdef KDE4_INTEGRATION
#include <KCmdLineArgs>
#include <KApplication>
#include <KAboutData>
#endif

int main(int argc, char *argv[])
{
#ifndef KDE4_INTEGRATION
    QCoreApplication app(argc, argv);

    AurSearcher searcher;
    searcher.arguments = QCoreApplication::arguments();
#else
KAboutData aboutData("shaman", 0, ki18n("Shaman"), "1.90.0",
                         ki18n("A Modular Package Manager for KDE"),
                         KAboutData::License_GPL,
                         ki18n("(C) 2008 - 2009, Dario Freddi - Lukas Appelhans"));
    aboutData.addAuthor(ki18n("Dario Freddi"), ki18n("Maintainer"), "drf@kde.org", "http://drfav.wordpress.com");
    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineOptions options;
    options.add("+command", ki18n("A short binary option"));
    options.add("+id", ki18n("A short binary option"));
    KCmdLineArgs::addCmdLineOptions(options);

    KApplication app;

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    AurSearcher searcher;
    searcher.arguments.append("bla");
    searcher.arguments.append(args->arg(0));
    searcher.arguments.append(args->arg(1));
#endif

    QTimer::singleShot(100, &searcher, SLOT(process()));

    return app.exec();
}

AurSearcher::AurSearcher(QObject* parent)
    : QObject(parent)
{

}

AurSearcher::~AurSearcher()
{

}

void AurSearcher::process()
{
    if (arguments.count() != 3) {
        printf("Usage: aursearcher (search|info|download) <package>\n<package> is a string for search, "
               "and a number (the package ID) for the other commands\n");
        QCoreApplication::instance()->quit();
    }

    if (arguments.at(1) == "search") {
        Aqpm::Aur::Package::List retlist = Aqpm::Aur::Backend::instance()->searchSync(arguments.at(2));

        foreach(Aqpm::Aur::Package *result, retlist) {
            printf("%s - %s (ID: %i)\n", result->name.toAscii().data(), result->version.toAscii().data(), result->id);
        }
    } else if (arguments.at(1) == "info") {
        Aqpm::Aur::Package *package = Aqpm::Aur::Backend::instance()->infoSync(QCoreApplication::arguments().at(2).toInt());

        printf("Name: %s\nVersion: %s\nDescription: %s\nUrl: %s\nVotes: %i\n%s\n", package->name.toAscii().data(),
               package->version.toAscii().data(), package->description.toAscii().data(), package->url.toAscii().data(),
               package->votes, package->outOfDate ? "The package is out of date" : "");
    } else if (arguments.at(1) == "download") {
        Aqpm::Aur::Backend::instance()->prepareBuildEnvironmentSync(arguments.at(2).toInt(), QDir::currentPath());
    } else {
        printf("Usage: aursearcher (search|info|download) <package>\n<package> is a string for search, "
               "and a number (the package ID) for the other commands\n");
    }

    QCoreApplication::instance()->quit();
}
