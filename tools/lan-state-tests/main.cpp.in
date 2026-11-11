#include "LanState.hpp"
#include <QCoreApplication>
#include <QTextStream>
#include <cstdio>
#include <stdexcept>
using namespace VoidframeLan;
namespace {
void require(bool ok,const char* text) { if(!ok) throw std::runtime_error(text); }
KindRule rule() {
    KindRule r;r.createPermission="objects.create";r.deletePermission="objects.delete";
    r.fields["position"]={"objects.transform",State::vector3,false};
    r.fields["name"]={"objects.properties",State::shortString,false};
    r.fields["parent"]={"objects.hierarchy",[](const QJsonValue& v){return v.isNull()||(v.isString()&&State::validId(v.toString()));},true};
    return r;
}
QJsonObject create(const QString& request,const QString& id) {
    QJsonObject fields{{"position",QJsonArray{0,0,0}},{"name","Test"}};
    QJsonObject op{{"op","create"},{"entity",id},{"kind","timeline"},{"expected","0"},{"fields",fields}};
    return {{"request",request},{"ops",QJsonArray{op}}};
}
QJsonObject set(const QString& request,const QString& id,const QString& expected,const QString& field,const QJsonValue& value) {
    QJsonObject op{{"op","set"},{"entity",id},{"expected",expected},{"field",field},{"value",value}};
    return {{"request",request},{"ops",QJsonArray{op}}};
}
void tests() {
    State s;s.registerKind("timeline",rule());
    auto initial=s.transact("host",create("c1","body"),0);
    require(initial.accepted&&s.revision()==1,"host create");
    require(!s.transact("a",set("r1","body","1","position",QJsonArray{1,2,3}),0).accepted,"read-only denied");
    Grant g;g.global={"objects.create","objects.delete","objects.transform","objects.properties","objects.hierarchy"};
    s.setGrant("a",g);QString error;
    require(s.acquire("a",{"body"},0,10000,error),"lease granted");
    auto t=set("move","body","1","position",QJsonArray{1,2,3});
    auto moved=s.transact("a",t,10);
    require(moved.accepted&&moved.revision==2,"allowed transform");
    require(s.transact("a",t,20).accepted&&s.revision()==2,"idempotent transaction");
    auto reused=t;reused["ops"]=set("unused","body","2","position",QJsonArray{9,9,9})["ops"];
    require(!s.transact("a",reused,20).accepted,"request ID collision denied");
    require(!s.transact("a",set("old","body","1","position",QJsonArray{2,3,4}),20).accepted,"stale change denied");
    auto atomic=set("atomic","body","2","position",QJsonArray{8,8,8});auto ops=atomic["ops"].toArray();
    ops.append(QJsonObject{{"op","set"},{"entity","body"},{"expected","2"},{"field","not_registered"},{"value",true}});atomic["ops"]=ops;
    require(!s.transact("a",atomic,30).accepted&&s.revision()==2,"transaction rolled back");
    require(s.entities().value("body").fields.value("position").toArray()[0].toInt()==1,"rollback preserves data");
    s.setGrant("b",g);
    require(!s.acquire("b",{"body"},100,10000,error),"conflicting lease denied");
    require(!s.transact("host",set("locked","body","2","name","Locked"),100).accepted,"host observes participant lock");
    Grant denied=g;denied.objects["body"]=QSet<QString>();s.setGrant("a",denied);
    require(!s.acquire("a",{"body"},100,10000,error),"object deny overrides global rights");
    require(s.acquire("b",{"body"},100,10000,error),"revocation releases lease");
    require(!s.transact("a",set("revoked","body","2","name","Denied"),101).accepted,"revocation enforced");
    s.disconnected("b");s.setGrant("a",g);
    require(s.acquire("a",{"body"},200,10000,error),"disconnect releases locks");
    require(!s.transact("a",set("expired","body","2","name","Expired"),10300).accepted,"expired lease denied");
    QJsonObject del{{"request","delete"},{"ops",QJsonArray{QJsonObject{{"op","delete"},{"entity","body"},{"expected","2"}}}}};
    require(s.transact("host",del,20000).accepted,"host delete");
    require(!s.transact("host",create("recreate","body"),20001).accepted,"retired ID cannot be reused");
    require(s.transact("host",create("child_create","child"),20002).accepted,"child create");
    require(!s.transact("host",set("dangling","child","4","parent","missing"),20003).accepted,"dangling reference denied");
    quint64 n=0;
    require(State::parseRevision("18446744073709551615",n),"64-bit revisions supported");
    require(!State::parseRevision("18446744073709551616",n)&&!State::parseRevision("01",n)&&!State::parseRevision(1,n),"malformed revisions denied");
    s.reset();
    require(s.revision()==0&&s.entities().isEmpty(),"session reset clears state");
    require(s.transact("host",create("fresh","body"),0).accepted,"reset retains schema and starts new ID namespace");
}
}
int main(int argc,char** argv) {
    QCoreApplication app(argc,argv);
    try { tests();QTextStream(stdout)<<"LAN authority tests passed\n";return 0; }
    catch(const std::exception& e) { QTextStream(stderr)<<"FAIL: "<<e.what()<<'\n';return 1; }
}
