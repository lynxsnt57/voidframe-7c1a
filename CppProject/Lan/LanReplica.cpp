#include "LanReplica.hpp"
#include <QJsonDocument>
#include <limits>
namespace VoidframeLan {
namespace {
bool validEntity(const QJsonValue& raw,quint64 latest) {
    if(!raw.isObject())return false;
    auto obj=raw.toObject();quint64 revision=0;
    return State::validId(obj.value("kind").toString())&&State::parseRevision(obj.value("revision"),revision)&&revision>0&&revision<=latest&&
        obj.value("fields").isObject()&&State::finiteValue(obj.value("fields"));
}
}
bool Replica::consume(const QJsonObject& e,QString& error) {
    const QString type=e.value("type").toString();
    if(type=="snapshot") {
        const QJsonObject state=e.value("state").toObject();quint64 revision=0;
        if(!State::parseRevision(state.value("revision"),revision)||!state.value("entities").isObject()){error="Invalid snapshot";return false;}
        if(ready_&&revision<revision_){error="Snapshot older than canonical state";return false;}
        const auto objects=state.value("entities").toObject();
        if(objects.size()>MaxEntities||QJsonDocument(objects).toJson(QJsonDocument::Compact).size()>MaxDocumentBytes){error="Snapshot budget exceeded";return false;}
        for(auto i=objects.begin();i!=objects.end();++i)if(!State::validId(i.key())||!validEntity(i.value(),revision)){error="Invalid snapshot entity";return false;}
        entities_=objects;revision_=revision;ready_=true;return true;
    }
    if(type!="commit")return true;
    quint64 revision=0;
    if(!ready_||!State::parseRevision(e.value("revision"),revision)||revision_==std::numeric_limits<quint64>::max()||revision!=revision_+1||!e.value("changes").isArray()){
        error="Revision gap; suspend project edits and request a fresh snapshot";return false;
    }
    auto staged=entities_;QSet<QString> seen;const auto changes=e.value("changes").toArray();
    if(changes.isEmpty()||changes.size()>MaxOperations){error="Invalid commit size";return false;}
    for(const auto& value:changes) {
        if(!value.isObject()){error="Invalid commit entry";return false;}
        const auto change=value.toObject();const QString id=change.value("entity").toString();
        if(!State::validId(id)||seen.contains(id)||!change.value("deleted").isBool()){error="Invalid/duplicate commit ID";return false;}
        seen.insert(id);
        if(change.value("deleted").toBool()) {
            if(!staged.contains(id)){error="Deleting unknown entity";return false;}staged.remove(id);
        } else {
            const auto raw=change.value("state");quint64 objectRevision=0;
            if(!validEntity(raw,revision)||!State::parseRevision(raw.toObject().value("revision"),objectRevision)||objectRevision!=revision){error="Invalid committed state";return false;}
            staged[id]=raw;
        }
    }
    if(staged.size()>MaxEntities||QJsonDocument(staged).toJson(QJsonDocument::Compact).size()>MaxDocumentBytes){error="Commit budget exceeded";return false;}
    entities_=staged;revision_=revision;return true;
}
}
