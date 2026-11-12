#pragma once
#include "LanState.hpp"
#include <QByteArray>
#include <QHostAddress>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>
namespace VoidframeLan {
// Authenticated LAN transport. NOT encrypted: trusted local networks only.
// Register schemas before hosting. Queue callbacks before touching editor state.
class Session : public QObject {
public:
    explicit Session(const QString& compatibilityId,QObject* parent=nullptr);
    ~Session() override;
    Session(const Session&)=delete;
    Session& operator=(const Session&)=delete;
    State& authority();
    bool host(const QHostAddress& address,quint16 port,QString& error);
    QString invitationCode() const;
    bool join(const QString& address,quint16 port,const QString& name,const QString& code,QString& error);
    void close();
    bool hosting() const;
    bool connected() const;
    quint16 port() const;
    QString localId() const;
    QJsonObject members() const;
    void setGrant(const QString& peerId,const Grant& grant);
    Result hostTransaction(const QJsonObject& transaction);
    bool submit(const QJsonObject& transaction);
    bool acquire(const QStringList& entities,int durationMs=10000);
    bool release(const QStringList& entities);
    bool requestSnapshot();
    bool publishPresence(const QJsonObject& pose);
    bool uploadBlock(const QByteArray& data);
    bool requestBlock(const QString& hash);
    QByteArray cachedBlock(const QString& hash) const;
    std::function<void(const QJsonObject&)> onEvent;
    std::function<void(const QString&)> onStatus;
private:
    struct Impl;
    std::unique_ptr<Impl> d;
};
}
