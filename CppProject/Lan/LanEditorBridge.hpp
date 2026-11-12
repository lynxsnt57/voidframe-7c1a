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
// Install the codec before hosting/joining. Never restart the transport from
// a codec callback; stop() is allowed, and cancels the in-flight application.
class EditorBridge {
public:
    using Apply=std::function<bool(const QJsonObject&amp;,QString&amp;)>;
    Apply applyProject;
    std::function<void(const QJsonObject&amp;)> onNotice;
    explicit EditorBridge(const QString&amp; build):session\_(build) {
        clock\_.start();
        session\_.onEvent=\[this\](const QJsonObject&amp; event){
            const int bytes=QJsonDocument(event).toJson(QJsonDocument::Compact).size();
            if(overflow\_||pending\_.size()&gt;=4096||queuedBytes\_+bytes&gt;20971520){overflow\_=true;return;}
            pending\_.enqueue(Item{event,bytes,clock\_.elapsed()});queuedBytes\_+=bytes;
        };
    }
    Session&amp; transport(){return session\_;}
    const Replica&amp; canonical()const{return replica\_;}
    const Presence&amp; cameras()const{return presence\_;}
    int pendingEvents()const{return pending\_.size();}
    bool editable()const{return bool(applyProject)&amp;&amp;session\_.connected()&amp;&amp;replica\_.ready()&amp;&amp;!frozen\_&amp;&amp;!overflow\_;}
    // Call after seeding host authority from the already-open local project,
    // BEFORE processing Qt events / accepting edits from participants.
    bool adoptHostState(QString&amp; error){
        if(draining\_){error="Cannot adopt host state inside an editor callback";return false;}
        if(!session\_.hosting()){error="Not hosting";return false;}
        epoch\_++;replica\_.reset();pending\_.clear();queuedBytes\_=0;overflow\_=false;
        frozen\_=!replica\_.consume({{"type","snapshot"},{"state",session\_.authority().snapshot()}},error);
        return !frozen\_;
    }
    void stop(){
        epoch\_++;session\_.close();pending\_.clear();queuedBytes\_=0;overflow\_=false;
        frozen\_=true;replica\_.reset();presence\_.clear();lastResync\_=-10000;
    }
    // busy=true during load/export/history replay or another editor transaction.
    // No project callback, presence update or canonical mutation occurs then.
    int drain(bool busy,QString&amp; error,int budget=128){
        if(busy||draining\_||budget&lt;1)return 0;
        QScopedValueRollback<bool> guard(draining\_,true);
        if(overflow\_){error="Network event queue overflow; session closed safely";stop();return 0;}
        int count=0;
        while(!pending\_.isEmpty()&amp;&amp;count&lt;budget){
            const Item item=pending\_.dequeue();queuedBytes\_-=item.bytes;count++;
            const auto event=item.event;const QString type=event.value("type").toString();
            if(type=="snapshot"||type=="commit"){
                if(frozen\_&amp;&amp;type=="commit")continue;
                Replica next=replica\_;
                if(!next.consume(event,error)){
                    frozen\_=true;
                    if(session\_.hosting()){stop();return count;}
                    continue;
                }
                if(!applyProject){error="Project adapter is not installed";stop();return count;}
                const auto apply=applyProject;
                const quint64 epoch=epoch\_;
                frozen\_=true;
                try {
                    if(!apply(event,error)){
                        if(error.isEmpty())error="Project adapter rejected the batch";
                        stop();return count;
                    }
                }catch(...){error="Project adapter threw an exception; session closed safely";stop();return count;}
                if(epoch!=epoch\_||!session\_.connected()){
                    error="Session changed during project application";stop();return count;
                }
                replica\_=next;frozen\_=false;
            }else if(type=="presence"){
                const QString peer=event.value("peer").toString();
                if(peer!=session\_.localId())presence\_.update(peer,event.value("pose").toObject(),item.received);
            }else if(type=="left"){
                presence\_.remove(event.value("peer").toString());
                if(!session\_.hosting()){frozen\_=true;replica\_.reset();presence\_.clear();}
            }
            const auto notice=onNotice;
            if(notice)notice(event);
        }
        if(frozen\_&amp;&amp;session\_.connected()&amp;&amp;!session\_.hosting()&amp;&amp;clock\_.elapsed()-lastResync\_&gt;=2000){
            lastResync\_=clock\_.elapsed();session\_.requestSnapshot();
        }
        return count;
    }
private:
    struct Item {QJsonObject event;int bytes;qint64 received;};
    QQueue<Item> pending\_;
    QElapsedTimer clock\_;
    Replica replica\_;
    Presence presence\_;
    qint64 queuedBytes\_=0,lastResync\_=-10000;
    quint64 epoch\_=0;
    bool frozen\_=true,overflow\_=false,draining\_=false;
    // Destroy transport before the callback targets above.
    Session session\_;
};
}