#pragma once
#include "LanState.hpp"
namespace VoidframeLan {
// A client-side canonical mirror. An editor may preview a drag separately, but
// must never overwrite this mirror with speculative local edits.
class Replica {
public:
    bool consume(const QJsonObject& event,QString& error);
    QJsonObject snapshot() const { return {{"revision",QString::number(revision_)},{"entities",entities_}}; }
    void reset(){ready_=false;revision_=0;entities_=QJsonObject();}
    bool ready()const{return ready_;}
    quint64 revision()const{return revision_;}
    QJsonObject entity(const QString& id)const{return entities_.value(id).toObject();}
private:
    bool ready_=false;
    quint64 revision_=0;
    QJsonObject entities_;
};
}
