#pragma once
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QSet>
#include <QStringList>
#include <functional>

namespace VoidframeLan {
constexpr int ProtocolVersion = 1;
constexpr int MaxEntities = 50000;
constexpr int MaxOperations = 256;
constexpr int MaxDocumentBytes = 8*1024*1024;
struct FieldRule {
    QString permission;
    std::function<bool(const QJsonValue&)> validate;
    bool reference=false;
};
struct KindRule {
    QString createPermission, deletePermission;
    QMap<QString,FieldRule> fields;
};
struct Entity {
    QString kind;
    quint64 revision=0;
    QJsonObject fields;
};
struct Grant {
    QSet<QString> global;
    // An object override REPLACES global rights for that object. Empty = deny.
    QHash<QString,QSet<QString>> objects;
    bool allows(const QString& object,const QString& permission) const;
};
struct Lease { QString actor; qint64 expires=0; };
struct Result {
    bool accepted=false;
    QString error;
    quint64 revision=0;
    QJsonArray changes;
};
// This class has no socket or editor dependencies. All methods run on ONE thread.
// Network actor identity must come from the authenticated connection, never JSON.
class State {
public:
    using GraphValidator=std::function<bool(const QMap<QString,Entity>&,QString&)>;
    void registerKind(const QString& kind,const KindRule& rule);
    void setGraphValidator(GraphValidator validator);
    void setGrant(const QString& actor,const Grant& grant);
    Grant grant(const QString& actor) const;
    bool allowed(const QString& actor,const QString& id,const QString& permission) const;
    bool acquire(const QString& actor,const QStringList& ids,qint64 now,int durationMs,QString& error);
    void release(const QString& actor,const QStringList& ids);
    void disconnected(const QString& actor);
    void reset(); // Retains schema, clears session state/permissions/locks/idempotency.
    Result transact(const QString& actor,const QJsonObject& transaction,qint64 now);
    QJsonObject snapshot() const;
    quint64 revision() const { return revision_; }
    QMap<QString,Entity> entities() const { return entities_; }
    static bool validId(const QString& id);
    static bool parseRevision(const QJsonValue& value,quint64& revision);
    static bool finiteValue(const QJsonValue& value,int depth=0);
    static bool vector3(const QJsonValue& value);
    static bool number(const QJsonValue& value);
    static bool boolean(const QJsonValue& value);
    static bool shortString(const QJsonValue& value);
private:
    QMap<QString,KindRule> schema_;
    QMap<QString,Entity> entities_;
    QHash<QString,Grant> grants_;
    QHash<QString,Lease> leases_;
    QSet<QString> allocatedIds_; // IDs are never reused within a session.
    struct Cached { QByteArray digest; Result result; };
    QHash<QString,QHash<QString,Cached>> results_;
    QHash<QString,QStringList> order_;
    GraphValidator graph_;
    quint64 revision_=0;
};
}
