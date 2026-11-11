#include "LanState.hpp"
#include <QRegularExpression>
#include <cmath>
#include <utility>
namespace VoidframeLan {
bool Grant::allows(const QString& object,const QString& p) const {
    auto it=objects.constFind(object);
    return it==objects.cend()?global.contains(p):it.value().contains(p);
}
bool State::validId(const QString& id) {
    static const QRegularExpression r("^[a-zA-Z0-9_-]{1,96}$");
    return r.match(id).hasMatch();
}
bool State::parseRevision(const QJsonValue& v,quint64& n) {
    if(!v.isString()) return false;
    const QString s=v.toString();
    if(s.isEmpty() || s.size()>20 || (s.size()>1 && s[0]=='0')) return false;
    for(QChar c:s) if(c<'0'||c>'9') return false;
    bool ok=false; n=s.toULongLong(&ok); return ok;
}
bool State::number(const QJsonValue& v) { return v.isDouble()&&std::isfinite(v.toDouble()); }
bool State::boolean(const QJsonValue& v) { return v.isBool(); }
bool State::shortString(const QJsonValue& v) { return v.isString()&&v.toString().size()<=4096; }
bool State::vector3(const QJsonValue& v) {
    if(!v.isArray()||v.toArray().size()!=3) return false;
    for(const auto& x:v.toArray()) if(!number(x)||std::abs(x.toDouble())>1e7) return false;
    return true;
}
bool State::finiteValue(const QJsonValue& v,int depth) {
    if(depth>32||v.isUndefined()) return false;
    if(v.isDouble()) return number(v);
    if(v.isString()) return v.toString().size()<=1024*1024;
    if(v.isArray()) {
        const auto a=v.toArray(); if(a.size()>100000) return false;
        for(const auto& x:a) if(!finiteValue(x,depth+1)) return false;
    }
    if(v.isObject()) {
        const auto o=v.toObject(); if(o.size()>100000) return false;
        for(auto i=o.begin();i!=o.end();++i) if(i.key().size()>128||!finiteValue(i.value(),depth+1)) return false;
    }
    return true;
}
void State::registerKind(const QString& k,const KindRule& r) { schema_[k]=r; }
void State::setGraphValidator(GraphValidator v) { graph_=std::move(v); }
void State::setGrant(const QString& actor,const Grant& g) {
    grants_[actor]=g;
    for(auto it=leases_.begin();it!=leases_.end();) {
        if(it->actor==actor) it=leases_.erase(it); else++it;
    }
}
Grant State::grant(const QString& a) const { return grants_.value(a); }
bool State::allowed(const QString& a,const QString& id,const QString& p) const {
    return a=="host" || (!p.isEmpty()&&grants_.value(a).allows(id,p));
}
void State::reset() { entities_.clear();allocatedIds_.clear();grants_.clear();leases_.clear();results_.clear();order_.clear();revision_=0; }
bool State::acquire(const QString& actor,const QStringList& ids,qint64 now,int duration,QString& error) {
    if(ids.isEmpty()||ids.size()>MaxOperations||duration<1000||duration>15000) { error="Invalid lease request";return false; }
    QSet<QString> unique;
    for(const QString& id:ids) {
        if(unique.contains(id)||!entities_.contains(id)) { error="Unknown/duplicate lease object";return false; }
        unique.insert(id);
        const auto& entity=entities_[id]; const auto rule=schema_.value(entity.kind);
        bool canEdit=allowed(actor,id,rule.deletePermission);
        for(auto f=rule.fields.begin();f!=rule.fields.end();++f) canEdit|=allowed(actor,id,f->permission);
        if(!canEdit) { error="Permission denied";return false; }
        const auto lease=leases_.value(id);
        if(lease.expires>now&&lease.actor!=actor) { error="Object is being edited by another participant";return false; }
    }
    for(const QString& id:ids) leases_[id]=Lease{actor,now+duration};
    return true;
}
void State::release(const QString& a,const QStringList& ids) {
    for(const QString& id:ids) if(leases_.value(id).actor==a) leases_.remove(id);
}
void State::disconnected(const QString& a) {
    for(auto it=leases_.begin();it!=leases_.end();) { if(it->actor==a) it=leases_.erase(it);else++it; }
    grants_.remove(a);results_.remove(a);order_.remove(a);
}
QJsonObject State::snapshot() const {
    QJsonObject entities;
    for(auto i=entities_.begin();i!=entities_.end();++i) entities[i.key()]=QJsonObject{
        {"kind",i->kind},{"revision",QString::number(i->revision)},{"fields",i->fields}};
    return {{"revision",QString::number(revision_)},{"entities",entities}};
}
}