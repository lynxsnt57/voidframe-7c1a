#include "LanState.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QRegularExpression>
#include <cmath>
#include <limits>
#include <utility>
namespace VoidframeLan {
bool Grant::allows(const QString&amp; object,const QString&amp; p) const {
    auto it=objects.constFind(object);
    return it==objects.cend()?global.contains(p):it.value().contains(p);
}
bool State::validId(const QString&amp; id) {
    static const QRegularExpression r("^\[a-zA-Z0-9\_-\]{1,96}$");
    return r.match(id).hasMatch();
}
bool State::parseRevision(const QJsonValue&amp; v,quint64&amp; n) {
    // Decimal strings avoid loss beyond JavaScript/JSON's exact-integer range.
    if(!v.isString()) return false;
    const QString s=v.toString();
    if(s.isEmpty() || s.size()&gt;20 || (s.size()&gt;1 &amp;&amp; s\[0\]=='0')) return false;
    for(QChar c:s) if(c&lt;'0'||c&gt;'9') return false;
    bool ok=false; n=s.toULongLong(&amp;ok); return ok;
}
bool State::number(const QJsonValue&amp; v) { return v.isDouble()&amp;&amp;std::isfinite(v.toDouble()); }
bool State::boolean(const QJsonValue&amp; v) { return v.isBool(); }
bool State::shortString(const QJsonValue&amp; v) { return v.isString()&amp;&amp;v.toString().size()&lt;=4096; }
bool State::vector3(const QJsonValue&amp; v) {
    if(!v.isArray()||v.toArray().size()!=3) return false;
    for(const auto&amp; x:v.toArray()) if(!number(x)||std::abs(x.toDouble())&gt;1e7) return false;
    return true;
}
bool State::finiteValue(const QJsonValue&amp; v,int depth) {
    if(depth&gt;32||v.isUndefined()) return false;
    if(v.isDouble()) return number(v);
    if(v.isString()) return v.toString().size()&lt;=1024*1024;
    if(v.isArray()) {
        const auto a=v.toArray(); if(a.size()&gt;100000) return false;
        for(const auto&amp; x:a) if(!finiteValue(x,depth+1)) return false;
    }
    if(v.isObject()) {
        const auto o=v.toObject(); if(o.size()&gt;100000) return false;
        for(auto i=o.begin();i!=o.end();++i) if(i.key().size()&gt;128||!finiteValue(i.value(),depth+1)) return false;
    }
    return true;
}
void State::registerKind(const QString&amp; k,const KindRule&amp; r) { schema\_\[k\]=r; }
void State::setGraphValidator(GraphValidator v) { graph\_=std::move(v); }
void State::setGrant(const QString&amp; actor,const Grant&amp; g) {
    grants\_\[actor\]=g;
    // Revocation immediately invalidates leases; no rights are cached in a lock.
    for(auto it=leases\_.begin();it!=leases\_.end();) {
        if(it-&gt;actor==actor) it=leases\_.erase(it); else++it;
    }
}
Grant State::grant(const QString&amp; a) const { return grants\_.value(a); }
bool State::allowed(const QString&amp; a,const QString&amp; id,const QString&amp; p) const {
    return a=="host" || (!p.isEmpty()&amp;&amp;grants\_.value(a).allows(id,p));
}
void State::reset() { entities\_.clear();allocatedIds\_.clear();grants\_.clear();leases\_.clear();results\_.clear();order\_.clear();revision\_=0; }
bool State::acquire(const QString&amp; actor,const QStringList&amp; ids,qint64 now,int duration,QString&amp; error) {
    if(ids.isEmpty()||ids.size()&gt;MaxOperations||duration&lt;1000||duration&gt;15000) { error="Invalid lease request";return false; }
    QSet<QString> unique;
    for(const QString&amp; id:ids) {
        if(unique.contains(id)||!entities\_.contains(id)) { error="Unknown/duplicate lease object";return false; }
        unique.insert(id);
        const auto&amp; entity=entities\_\[id\]; const auto rule=schema\_.value(entity.kind);
        bool canEdit=allowed(actor,id,rule.deletePermission);
        for(auto f=rule.fields.begin();f!=rule.fields.end();++f) canEdit|=allowed(actor,id,f-&gt;permission);
        if(!canEdit) { error="Permission denied";return false; }
        const auto lease=leases\_.value(id);
        if(lease.expires&gt;now&amp;&amp;lease.actor!=actor) { error="Object is being edited by another participant";return false; }
    }
    for(const QString&amp; id:ids) leases\_\[id\]=Lease{actor,now+duration};
    return true;
}
void State::release(const QString&amp; a,const QStringList&amp; ids) {
    for(const QString&amp; id:ids) if(leases\_.value(id).actor==a) leases\_.remove(id);
}
void State::disconnected(const QString&amp; a) {
    for(auto it=leases\_.begin();it!=leases\_.end();) { if(it-&gt;actor==a) it=leases\_.erase(it);else++it; }
    grants\_.remove(a);results\_.remove(a);order\_.remove(a);
}
QJsonObject State::snapshot() const {
    QJsonObject entities;
    for(auto i=entities\_.begin();i!=entities\_.end();++i) entities\[i.key()\]=QJsonObject{
        {"kind",i-&gt;kind},{"revision",QString::number(i-&gt;revision)},{"fields",i-&gt;fields}};
    return {{"revision",QString::number(revision\_)},{"entities",entities}};
}
Result State::transact(const QString&amp; actor,const QJsonObject&amp; t,qint64 now) {
    Result out;out.revision=revision\_;
    const QString request=t.value("request").toString();
    const QByteArray encoded=QJsonDocument(t).toJson(QJsonDocument::Compact);
    if(!validId(request)||encoded.size()&gt;128*1024||!t.value("ops").isArray()) { out.error="Malformed transaction";return out; }
    const QByteArray digest=QCryptographicHash::hash(encoded,QCryptographicHash::Sha256);
    auto actorCache=results\_.constFind(actor);
    if(actorCache!=results\_.cend()) {
        auto cached=actorCache-&gt;constFind(request);
        if(cached!=actorCache-&gt;cend()) {
            if(cached-&gt;digest!=digest) { out.error="Request ID reused with different content";return out; }
            return cached-&gt;result;
        }
    }
    const QJsonArray ops=t.value("ops").toArray();
    if(ops.isEmpty()||ops.size()&gt;MaxOperations) { out.error="Invalid operation count";return out; }
    // Work on a copy. ANY failed operation rolls back the ENTIRE transaction.
    auto staged=entities\_;QSet<QString> touched,deleted,created;
    auto fail=\[&amp;\](const QString&amp; s) {out.error=s;out.changes=QJsonArray();return out;};
    for(const auto&amp; raw:ops) {
        if(!raw.isObject()) return fail("Invalid operation");
        const auto op=raw.toObject();const QString action=op.value("op").toString(), id=op.value("entity").toString();
        if(!validId(id)) return fail("Invalid entity ID");
        if(deleted.contains(id)) return fail("Cannot edit a deleted object in the same transaction");
        const bool exists=entities\_.contains(id);
        quint64 expected=0;
        if(!parseRevision(op.value("expected"),expected)) return fail("Expected revision must be a decimal string");
        if(action=="create") {
            if(exists||staged.contains(id)||allocatedIds\_.contains(id)||expected!=0||allocatedIds\_.size()+created.size()&gt;=MaxEntities) return fail("Object already exists or object budget exceeded");
            const QString kind=op.value("kind").toString();
            if(!schema\_.contains(kind)) return fail("Unregistered object kind");
            const auto rule=schema\_.value(kind);
            if(!allowed(actor,id,rule.createPermission)) return fail("Create permission denied");
            if(!op.value("fields").isObject()) return fail("Missing create fields");
            const auto fields=op.value("fields").toObject();
            for(auto f=fields.begin();f!=fields.end();++f) {
                if(!rule.fields.contains(f.key())) return fail("Unregistered field: "+f.key());
                const auto spec=rule.fields.value(f.key());
                if(!allowed(actor,id,spec.permission)||!finiteValue(f.value())||!spec.validate||!spec.validate(f.value())) return fail("Create field validation/permission failed: "+f.key());
            }
            staged.insert(id,Entity{kind,0,fields});created.insert(id);touched.insert(id);continue;
        }
        // New objects cannot be altered again until acknowledged by the host.
        if(!exists||expected!=entities\_.value(id).revision) return fail("Stale or missing object; request a fresh snapshot");
        const Lease lease=leases\_.value(id);
        // Host observes other users' locks too, rather than silently stealing them.
        if(lease.expires&gt;now&amp;&amp;lease.actor!=actor) return fail("Object locked by another participant");
        if(actor!="host"&amp;&amp;(lease.actor!=actor||lease.expires&lt;=now)) return fail("A valid edit lease is required");
        const auto kind=entities\_.value(id).kind;
        if(!schema\_.contains(kind)) return fail("Unregistered object kind");
        const auto rule=schema\_.value(kind);
        if(action=="delete") {
            if(!allowed(actor,id,rule.deletePermission)) return fail("Delete permission denied");
            staged.remove(id);deleted.insert(id);touched.insert(id);
        } else if(action=="set") {
            const QString field=op.value("field").toString();
            if(!rule.fields.contains(field)) return fail("Unregistered field: "+field);
            const auto spec=rule.fields.value(field);const auto value=op.value("value");
            if(!allowed(actor,id,spec.permission)) return fail("Field permission denied: "+field);
            if(!spec.validate||!finiteValue(value)||!spec.validate(value)) return fail("Invalid field value: "+field);
            staged\[id\].fields\[field\]=value;touched.insert(id);
        } else return fail("Unknown operation");
    }
    // Reference checks happen after ALL edits, allowing atomic reparent/delete.
    for(auto e=staged.begin();e!=staged.end();++e) {
        const auto rule=schema\_.value(e-&gt;kind);
        for(auto f=rule.fields.begin();f!=rule.fields.end();++f) if(f-&gt;reference&amp;&amp;e-&gt;fields.contains(f.key())) {
            const auto value=e-&gt;fields.value(f.key());
            if(!value.isNull()&amp;&amp;(!value.isString()||!staged.contains(value.toString()))) return fail("Dangling object reference");
        }
    }
    QString graphError;
    if(graph\_&amp;&amp;!graph\_(staged,graphError)) return fail("Invalid project graph: "+graphError);
    if(revision\_==std::numeric\_limits<quint64>::max()) return fail("Revision range exhausted; restart session");
    const quint64 next=revision\_+1;
    QStringList sorted=touched.values();sorted.sort();
    for(const QString&amp; id:sorted) {
        if(staged.contains(id)) staged\[id\].revision=next;
        out.changes.append(QJsonObject{{"entity",id},{"deleted",!staged.contains(id)},
            {"state",staged.contains(id)?QJsonValue(QJsonObject{{"kind",staged\[id\].kind},{"revision",QString::number(next)},{"fields",staged\[id\].fields}}):QJsonValue(QJsonValue::Null)}});
    }
    QJsonObject document;
    for(auto e=staged.begin();e!=staged.end();++e) document\[e.key()\]=QJsonObject{{"kind",e-&gt;kind},{"revision",QString::number(e-&gt;revision)},{"fields",e-&gt;fields}};
    if(QJsonDocument(document).toJson(QJsonDocument::Compact).size()&gt;MaxDocumentBytes) return fail("Session state exceeds 8 MiB");
    entities\_=std::move(staged);allocatedIds\_.unite(created);revision\_=next;
    for(const QString&amp; id:deleted) leases\_.remove(id);
    out.accepted=true;out.revision=next;
    results\_\[actor\].insert(request,Cached{digest,out});order\_\[actor\].append(request);
    while(order\_\[actor\].size()&gt;128) results\_\[actor\].remove(order\_\[actor\].takeFirst());
    return out;
}
}