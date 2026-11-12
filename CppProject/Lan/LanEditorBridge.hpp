#pragma once
#include "LanSession.hpp"
#include "LanReplica.hpp"
#include "LanPresence.hpp"
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QQueue>
#include <QScopedValueRollback>
namespace VoidframeLan {
// Own on the editor thread. Socket callbacks enqueue only. Call drain() at a
// safe main-loop boundary. The project codec must apply each event atomically.
class EditorBridge {
public:
    using Apply=std::function<bool(const QJsonObject&,QString&)>;
    Apply applyProject;
    std::function<void(const QJsonObject&)> onNotice;
    explicit EditorBridge(const QString& build):session_(build) {
        clock_.start();
        session_.onEvent=[this](const QJsonObject& event){
            const int bytes=QJsonDocument(event).toJson(QJsonDocument::Compact).size();
            if(overflow_||pending_.size()>=4096||queuedBytes_+bytes>20971520){overflow_=true;return;}
            pending_.enqueue(Item{event,bytes,clock_.elapsed()});queuedBytes_+=bytes;
        };
    }
    Session& transport(){return session_;}
    const Replica& canonical()const{return replica_;}
    const Presence& cameras()const{return presence_;}
    int pendingEvents()const{return pending_.size();}
    bool editable()const{return session_.connected()&&replica_.ready()&&!frozen_&&!overflow_;}
    // Call after seeding host authority from the already-open local project,
    // BEFORE processing Qt events / accepting edits from participants.
    bool adoptHostState(QString& error){
        if(!session_.hosting()){error="Not hosting";return false;}
        replica_.reset();pending_.clear();queuedBytes_=0;overflow_=false;
        frozen_=!replica_.consume({{"type","snapshot"},{"state",session_.authority().snapshot()}},error);
        return !frozen_;
    }
    void stop(){
        session_.close();pending_.clear();queuedBytes_=0;overflow_=false;
        frozen_=true;replica_.reset();presence_.clear();lastResync_=-10000;
    }
    // busy=true during load/export/history replay or another editor transaction.
    // No project callback, presence update or canonical mutation occurs then.
    int drain(bool busy,QString& error,int budget=128){
        if(busy||draining_||budget<1)return 0;
        QScopedValueRollback<bool> guard(draining_,true);
        if(overflow_){error="Network event queue overflow; session closed safely";stop();return 0;}
        int count=0;
        while(!pending_.isEmpty()&&count<budget){
            const Item item=pending_.dequeue();queuedBytes_-=item.bytes;count++;
            const auto event=item.event;const QString type=event.value("type").toString();
            if(type=="snapshot"||type=="commit"){
                if(frozen_&&type=="commit")continue;
                Replica next=replica_;
                if(!next.consume(event,error)){
                    frozen_=true;
                    if(session_.hosting()){stop();return count;}
                    continue;
                }
                if(!applyProject){error="Project adapter is not installed";frozen_=true;continue;}
                if(!applyProject(event,error)){stop();return count;}
                replica_=next;frozen_=false;
            }else if(type=="presence"){
                const QString peer=event.value("peer").toString();
                if(peer!=session_.localId())presence_.update(peer,event.value("pose").toObject(),item.received);
            }else if(type=="left"){
                presence_.remove(event.value("peer").toString());
                if(!session_.hosting()){frozen_=true;replica_.reset();presence_.clear();}
            }
            if(onNotice)onNotice(event);
        }
        if(frozen_&&session_.connected()&&!session_.hosting()&&clock_.elapsed()-lastResync_>=2000){
            lastResync_=clock_.elapsed();session_.requestSnapshot();
        }
        return count;
    }
private:
    struct Item {QJsonObject event;int bytes;qint64 received;};
    QQueue<Item> pending_;
    QElapsedTimer clock_;
    Replica replica_;
    Presence presence_;
    qint64 queuedBytes_=0,lastResync_=-10000;
    bool frozen_=true,overflow_=false,draining_=false;
    // Destroy transport before the callback targets above.
    Session session_;
};
}
