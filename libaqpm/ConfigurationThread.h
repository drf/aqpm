#ifndef CONFIGURATIONTHREAD_H
#define CONFIGURATIONTHREAD_H

#include <QObject>
#include <QVariantMap>

#include "Configuration.h"

namespace Aqpm {

class ConfigurationThread : public QObject
{
    Q_OBJECT

public:
    ConfigurationThread();
    virtual ~ConfigurationThread();

    bool saveConfiguration();
    void saveConfigurationAsync();

    void setValue(const QString &key, const QString &val);
    QVariant value(const QString &key);

    QStringList databases();
    QString getServerForDatabase(const QString &db);
    QStringList getServersForDatabase(const QString &db);

    QStringList getMirrorList(Configuration::MirrorType type);

    bool addMirrorToMirrorList(const QString &mirror, Configuration::MirrorType type);
    void addMirrorToMirrorListAsync(const QString &mirror, Configuration::MirrorType type);

    QStringList mirrors(bool thirdpartyonly = false);
    QStringList serversForMirror(const QString &mirror);
    QStringList databasesForMirror(const QString &mirror);

    void setDatabases(const QStringList &databases);
    void setDatabasesForMirror(const QStringList &database, const QString &mirror);
    void setServersForMirror(const QStringList &servers, const QString &mirror);

    void remove(const QString &key);
    bool exists(const QString &key, const QString &val = QString());

    void setOrUnset(bool set, const QString &key, const QString &val = QString());

public Q_SLOTS:
    void reload();

private Q_SLOTS:
    void configuratorResult(bool result);

protected:
    void customEvent(QEvent *event);

Q_SIGNALS:
    void configurationSaved(bool result);
    void actionPerformed(int type, QVariantMap result);

private:
    class Private;
    Private *d;
};

}

#endif // CONFIGURATIONTHREAD_H
