#include "LanState.hpp"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <limits>
#include <utility>
namespace VoidframeLan {
Result State::transact(const QString& actor,const QJsonObject& t,qint64 now) {
    Result out;out.revision=revision_;
    const QString request=t.value("request").toString();
    const QByteArray encoded=QJsonDocument(t).toJson(QJsonDocument::Compact);
    if(!validId(request)||encoded.size()>128*1024||!t.value("ops").isArray()) { out.error="Malformed transaction";return out; }
    const QByteArray digest=QCryptographicHash::hash(encoded,QCryptographicHash::Sha256);
    auto actorCache=results_.constFind(actor);
    if(actorCache!=results_.cend()) {
        auto cached=actorCache->constFind(request);
        if(cached!=actorCache->cend()) {
            if(cached->digest!=digest) { out.error="Request ID reused with different content";return out; }
            return cached->result;
        }
    }
    const QJsonArray ops=t.value("ops").toArray();
    if(ops.isEmpty()||ops.size()>MaxOperations) { out.error="Invalid operation count";return out; }
    // Work on a copy: failure rolls back the ENTIRE transaction.
    auto staged=entities_;QSet<QString> touched,deleted,created;
    auto fail=[&](const QString& s) {out.error=s;out.changes=QJsonArray();return out;};
    for(const auto& raw:ops) {
        if(!raw.isObject()) return fail("Invalid operation");
        const auto op=raw.toObject();const QString action=op.value("op").toString(), id=op.value("entity").toString();
        if(!validId(id)) return fail("Invalid entity ID");
        if(deleted.contains(id)) return fail("Cannot edit a deleted object in the same transaction");
        const bool exists=entities_.contains(id);
        quint64 expected=0;
        if(!parseRevision(op.value("expected"),expected)) return fail("Expected revision must be a decimal string");
        if(action=="create") {
            if(exists||staged.contains(id)||allocatedIds_.contains(id)||expected!=0||allocatedIds_.size()+created.size()>=MaxEntities) return fail("Object already exists or object budget exceeded");
            const QString kind=op.value("kind").toString();
            if(!schema_.contains(kind)) return fail("Unregistered object kind");
            const auto rule=schema_.value(kind);
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
        if(!exists||expected!=entities_.value(id).revision) return fail("Stale or missing object; request a fresh snapshot");
        const Lease lease=leases_.value(id);
        if(lease.expires>now&&lease.actor!=actor) return fail("Object locked by another participant");
        if(actor!="host"&&(lease.actor!=actor||lease.expires<=now)) return fail("A valid edit lease is required");
        const auto kind=entities_.value(id).kind;
        if(!schema_.contains(kind)) return fail("Unregistered object kind");
        const auto rule=schema_.value(kind);
        if(action=="delete") {
            if(!allowed(actor,id,rule.deletePermission)) return fail("Delete permission denied");
            staged.remove(id);deleted.insert(id);touched.insert(id);
        } else if(action=="set") {
            const QString field=op.value("field").toString();
            if(!rule.fields.contains(field)) return fail("Unregistered field: "+field);
            const auto spec=rule.fields.value(field);const auto value=op.value("value");
            if(!allowed(actor,id,spec.permission)) return fail("Field permission denied: "+field);
            if(!spec.validate||!finiteValue(value)||!spec.validate(value)) return fail("Invalid field value: "+field);
            staged[id].fields[field]=value;touched.insert(id);
        } else return fail("Unknown operation");
    }
    // Check references after ALL edits, allowing atomic reparent/delete.
    for(auto e=staged.begin();e!=staged.end();++e) {
        const auto rule=schema_.value(e->kind);
        for(auto f=rule.fields.begin();f!=rule.fields.end();++f) if(f->reference&&e->fields.contains(f.key())) {
            const auto value=e->fields.value(f.key());
            if(!value.isNull()&&(!value.isString()||!staged.contains(value.toString()))) return fail("Dangling object reference");
        }
    }
    QString graphError;
    if(graph_&&!graph_(staged,graphError)) return fail("Invalid project graph: "+graphError);
    if(revision_==std::numeric_limits<quint64>::max()) return fail("Revision range exhausted; restart session");
    const quint64 next=revision_+1;
    QStringList sorted=touched.values();sorted.sort();
    for(const QString& id:sorted) {
        if(staged.contains(id)) staged[id].revision=next;
        out.changes.append(QJsonObject{{"entity",id},{"deleted",!staged.contains(id)},
            {"state",staged.contains(id)?QJsonValue(QJsonObject{{"kind",staged[id].kind},{"revision",QString::number(next)},{"fields",staged[id].fields}}):QJsonValue(QJsonValue::Null)}});
    }
    QJsonObject document;
    for(auto e=staged.begin();e!=staged.end();++e) document[e.key()]=QJsonObject{{"kind",e->kind},{"revision",QString::number(e->revision)},{"fields",e->fields}};
    if(QJsonDocument(document).toJson(QJsonDocument::Compact).size()>MaxDocumentBytes) return fail("Session state exceeds 8 MiB");
    entities_=std::move(staged);allocatedIds_.unite(created);revision_=next;
    for(const QString& id:deleted) leases_.remove(id);
    out.accepted=true;out.revision=next;
    results_[actor].insert(request,Cached{digest,out});order_[actor].append(request);
    while(order_[actor].size()>128) results_[actor].remove(order_[actor].takeFirst());
    return out;
}
}
