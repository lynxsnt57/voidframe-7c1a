#include "LanState.hpp"
#include <QRegularExpression>
#include <cmath>
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
    if(v.isString()) return v.toString().size()&lt;=1024\*1024;
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
}