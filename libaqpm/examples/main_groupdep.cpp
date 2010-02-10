
#include "../Backend.h"
#include <QCoreApplication>
#include <QSet>
#include <QDebug>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);

    Aqpm::Backend::instance()->safeInitialization();

    Aqpm::Group *g = Aqpm::Backend::instance()->group("kdemod");
    Aqpm::Package::List deps;

    foreach (Aqpm::Package *p, g->packages()) {
        deps.append(p->dependsOn());
    }

    foreach (Aqpm::Package *p, deps.toSet()) {
        if (!g->packages().contains(p)) {
            qDebug() << p->name();
        }
    }
}
