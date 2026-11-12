#include "LanSession.hpp"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMessageAuthenticationCode>
#include <QPointer>
#include <QQueue>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTimer>
#include <QUuid>
#include <algorithm>
#include <cmath>
#include <limits>
namespace VoidframeLan {
namespace {
constexpr int MaxFrame=256*1024,MaxPayload=128*1024,BlockSize=64*1024;
constexpr qint64 MaxQueued=24*1024*1024,MaxCache=1024LL*1024*1024;
QByteArray randomBytes(int n) {
    QByteArray b(n,0);
    for(int i=0;i<n;i++) b[i]=char(QRandomGenerator::system()->generate()&255);
    return b;
}
QByteArray mac(const QByteArray& key,const QByteArray& data) { return QMessageAuthenticationCode::hash(data,key,QCryptographicHash::Sha256); }
bool equal(const QByteArray& a,const QByteArray& b) {
    if(a.size()!=b.size()) return false;
    unsigned diff=0;for(int i=0;i<a.size();i++) diff|=static_cast<unsigned char>(a[i])^static_cast<unsigned char>(b[i]);return diff==0;
}
QByteArray decode(const QString& text,int max) {
    if(text.size()>max*4/3+8) return {};
    auto v=QByteArray::fromBase64Encoding(text.toLatin1(),QByteArray::AbortOnBase64DecodingErrors);
    return v&&v.decoded.size()<=max?v.decoded:QByteArray();
}
bool validHash(const QString& h) {
    static const QRegularExpression r("^[0-9a-f]{64}$");return r.match(h).hasMatch();
}
QString uid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
QJsonArray strings(const QStringList& s) { QJsonArray a;for(const QString& x:s)a.append(x);return a; }
QJsonObject grantJson(const Grant& g) {
    QStringList sorted=g.global.values();sorted.sort();QJsonObject objects;
    for(auto it=g.objects.begin();it!=g.objects.end();++it) {QStringList list=it.value().values();list.sort();objects[it.key()]=strings(list);}
    return {{"global",strings(sorted)},{"objects",objects}};
}
bool poseValid(const QJsonObject& p) {
    if(!State::vector3(p.value("position")))return false;
    const auto q=p.value("rotation").toArray();if(q.size()!=4)return false;
    double length=0;for(const auto& c:q) {if(!State::number(c))return false;length+=c.toDouble()*c.toDouble();}
    return length>.999&&length<1.001&&State::number(p.value("fov"))&&p.value("fov").toDouble()>=5&&p.value("fov").toDouble()<=170;
}
}
struct Session::Impl {
    struct Peer {
        QPointer<QTcpSocket> socket;
        QString id,name;
        QByteArray input,key,nonce;
        QQueue<QByteArray> output;
        qint64 queued=0,lastRead=0,born=0,rateAt=0,bytesAt=0,lastPose=-1000;
        quint64 sent=0,received=0;
        int messages=0;
        bool ready=false;
        QByteArray snapshot;
        QString snapshotHash;
        int snapshotSize=0;
    };
    Session* owner;
    QString build,id,name;
    QByteArray secret;
    QTcpServer server;
    QTimer timer;
    QElapsedTimer clock;
    QTemporaryDir cache;
    qint64 cacheBytes=0;
    int cacheBlocks=0;
    State state;
    QHash<QTcpSocket*,std::shared_ptr<Peer>> peers;
    QJsonObject memberList;
    bool isHost=false,joining=false;
    qint64 lastPresence=-1000;
    Impl(Session* o,const QString& compatibility):owner(o),build(compatibility),server(o),timer(o) {
        clock.start();
        QObject::connect(&server,&QTcpServer::newConnection,o,[this]{accept();});
        QObject::connect(&timer,&QTimer::timeout,o,[this]{tick();});
        timer.start(1000);
    }
    void status(const QString& text) {if(owner->onStatus)owner->onStatus(text);}
    void event(const QJsonObject& e) {if(owner->onEvent)owner->onEvent(e);}
    QByteArray proof(const Peer& p,const QByteArray& label) const {
        return mac(secret,label+'\0'+p.nonce+'\0'+p.id.toUtf8()+'\0'+build.toUtf8());
    }
    void cleanup(QTcpSocket* socket) {
        auto p=peers.take(socket);if(!p)return;
        state.disconnected(p->id);lastSnapshotRequest.remove(p->id);socket->deleteLater();
        if(isHost)sendMembers();else {joining=false;id.clear();memberList=QJsonObject();status("Disconnected; local uncommitted edits must not be treated as host state");}
        event({{"type","left"},{"peer",p->id}});
    }
    void drop(Peer& p,const QString& reason) {
        status(reason);auto* socket=p.socket.data();
        if(socket){socket->abort();cleanup(socket);}
    }
    void flush(Peer& p) {
        if(!p.socket)return;
        while(!p.output.isEmpty()&&p.socket->bytesToWrite()<256*1024) {
            const QByteArray frame=p.output.dequeue();p.queued-=frame.size();
            if(p.socket->write(frame)!=frame.size()) {drop(p,"Socket write failed");return;}
        }
    }
    bool frame(Peer& p,const QJsonObject& message,bool signedFrame) {
        QByteArray payload=QJsonDocument(message).toJson(QJsonDocument::Compact);
        if(payload.size()>MaxPayload)return false;
        QJsonObject wrapped=message;
        if(signedFrame) {
            if(!p.ready||p.sent==std::numeric_limits<quint64>::max())return false;
            const QByteArray sequence=QByteArray::number(++p.sent);
            const QByteArray direction=isHost?"S":"C";
            wrapped={{"seq",QString::fromLatin1(sequence)},{"payload",QString::fromLatin1(payload.toBase64())},
                     {"mac",QString::fromLatin1(mac(p.key,direction+sequence+'\0'+payload).toHex())}};
        }
        const QByteArray bytes=QJsonDocument(wrapped).toJson(QJsonDocument::Compact);
        if(bytes.size()>MaxFrame||p.queued+(p.socket?p.socket->bytesToWrite():0)+bytes.size()>MaxQueued) {drop(p,"Slow peer exceeded output budget");return false;}
        const quint32 n=quint32(bytes.size());QByteArray header(4,0);
        header[0]=char(n>>24);header[1]=char(n>>16);header[2]=char(n>>8);header[3]=char(n);
        p.output.enqueue(header+bytes);p.queued+=bytes.size()+4;flush(p);return true;
    }
    bool sendClient(const QJsonObject& o) {
        if(isHost||peers.isEmpty())return false;
        const auto values=peers.values();for(const auto& p:values)if(p->ready)return frame(*p,o,true);
        return false;
    }
    void broadcast(const QJsonObject& o) {
        const auto values=peers.values();for(const auto& p:values)if(p->ready)frame(*p,o,true);
    }
    void sendMembers() {
        QJsonObject m;
        m["host"]=QJsonObject{{"name","Host"},{"color",0}};
        for(const auto& p:peers)if(p->ready)m[p->id]=QJsonObject{{"name",p->name},{"color",int(qHash(p->id)%8)}};
        memberList=m;broadcast({{"type","members"},{"members",m}});event({{"type","members"},{"members",m}});
    }
    void wire(QTcpSocket* socket,const std::shared_ptr<Peer>& p) {
        p->socket=socket;p->born=p->lastRead=clock.elapsed();peers.insert(socket,p);
        socket->setReadBufferSize(MaxFrame+4);
        QObject::connect(socket,&QTcpSocket::readyRead,owner,[this,socket]{auto p=peers.value(socket);if(p)read(*p);});
        QObject::connect(socket,&QTcpSocket::bytesWritten,owner,[this,socket](qint64){auto p=peers.value(socket);if(p)flush(*p);});
        QObject::connect(socket,&QTcpSocket::disconnected,owner,[this,socket]{cleanup(socket);});
        QObject::connect(socket,&QAbstractSocket::errorOccurred,owner,[this,socket](QAbstractSocket::SocketError){
            auto p=peers.value(socket);if(p)drop(*p,socket->errorString());
        });
    }
    void accept() {
        while(server.hasPendingConnections()) {
            auto* s=server.nextPendingConnection();
            if(peers.size()>=8) {s->abort();s->deleteLater();continue;}
            auto p=std::make_shared<Peer>();p->id=uid();p->nonce=randomBytes(32);wire(s,p);
            frame(*p,{{"type","challenge"},{"protocol",ProtocolVersion},{"build",build},{"id",p->id},{"nonce",QString::fromLatin1(p->nonce.toBase64())}},false);
        }
    }
    void read(Peer& p) {
        while(p.socket&&p.socket->bytesAvailable()>0) {
            const int remaining=MaxFrame+4-p.input.size();if(remaining<=0){drop(p,"Input buffer overflow");return;}
            const QByteArray b=p.socket->read(std::min(remaining,64*1024));if(b.isEmpty())break;
            p.input+=b;
            while(p.input.size()>=4) {
                const auto* x=reinterpret_cast<const unsigned char*>(p.input.constData());
                const quint32 n=(quint32(x[0])<<24)|(quint32(x[1])<<16)|(quint32(x[2])<<8)|x[3];
                if(!n||n>quint32(MaxFrame)){drop(p,"Invalid frame size");return;}
                if(p.input.size()<int(n)+4)break;
                const QByteArray packet=p.input.mid(4,int(n));p.input.remove(0,int(n)+4);
                const qint64 now=clock.elapsed();
                if(now-p.rateAt>=1000){p.rateAt=now;p.messages=0;p.bytesAt=0;}
                p.messages++;p.bytesAt+=packet.size();
                if(p.messages>512||p.bytesAt>32*1024*1024){drop(p,"Peer rate limit exceeded");return;}
                QJsonParseError error;const auto parsed=QJsonDocument::fromJson(packet,&error);
                if(error.error!=QJsonParseError::NoError||!parsed.isObject()){drop(p,"Malformed JSON frame");return;}
                QJsonObject message=parsed.object();
                if(p.ready) {
                    quint64 seq=0;const QByteArray body=decode(message.value("payload").toString(),MaxPayload);
                    const QString tagText=message.value("mac").toString();
                    if(!State::parseRevision(message.value("seq"),seq)||p.received==std::numeric_limits<quint64>::max()||seq!=p.received+1||body.isEmpty()||!validHash(tagText)) {drop(p,"Bad message sequence/envelope");return;}
                    const QByteArray direction=isHost?"C":"S";
                    if(!equal(QByteArray::fromHex(tagText.toLatin1()),mac(p.key,direction+QByteArray::number(seq)+'\0'+body))) {drop(p,"Message authentication failed");return;}
                    const auto doc=QJsonDocument::fromJson(body,&error);
                    if(error.error!=QJsonParseError::NoError||!doc.isObject()){drop(p,"Invalid signed payload");return;}
                    message=doc.object();p.received=seq;
                }
                p.lastRead=now;
                if(!p.ready)handshake(p,message);else if(isHost)hostMessage(p,message);else clientMessage(p,message);
                if(!p.socket||p.socket->state()==QAbstractSocket::UnconnectedState)return;
            }
        }
    }
    void handshake(Peer& p,const QJsonObject& m) {
        const QString type=m.value("type").toString();
        if(isHost) {
            const QString peerName=m.value("name").toString();
            if(type!="authenticate"||peerName.trimmed().isEmpty()||peerName.size()>48||m.value("build").toString()!=build||
               !equal(decode(m.value("proof").toString(),32),proof(p,"client"))) {drop(p,"Authentication/build check failed");return;}
            p.name=peerName.trimmed();p.key=proof(p,"session-key");
            frame(p,{{"type","authenticated"},{"proof",QString::fromLatin1(proof(p,"server").toBase64())}},false);
            p.ready=true;state.setGrant(p.id,Grant()); // read-only until HOST grants rights
            frame(p,{{"type","permissions"},{"permissions",grantJson(state.grant(p.id))}},true);
            sendMembers();sendSnapshot(p);event({{"type","joined"},{"peer",p.id},{"name",p.name}});
        } else if(type=="challenge"&&p.nonce.isEmpty()) {
            const auto nonce=decode(m.value("nonce").toString(),32);const QString peerId=m.value("id").toString();
            if(m.value("protocol").toInt(-1)!=ProtocolVersion||m.value("build").toString()!=build||nonce.size()!=32||!State::validId(peerId)||peerId=="host") {drop(p,"Incompatible server");return;}
            p.nonce=nonce;p.id=peerId;
            frame(p,{{"type","authenticate"},{"name",name},{"build",build},{"proof",QString::fromLatin1(proof(p,"client").toBase64())}},false);
        } else if(type=="authenticated"&&!p.nonce.isEmpty()) {
            if(!equal(decode(m.value("proof").toString(),32),proof(p,"server"))){drop(p,"Server authentication failed");return;}
            p.key=proof(p,"session-key");p.ready=true;id=p.id;joining=false;status("Connected; waiting for authoritative snapshot");
        } else drop(p,"Unexpected handshake message");
    }
    void sendSnapshot(Peer& p) {
        const QByteArray bytes=QJsonDocument(state.snapshot()).toJson(QJsonDocument::Compact);
        if(bytes.size()>MaxDocumentBytes+1024*1024){drop(p,"Snapshot exceeds transport budget");return;}
        const QString hash=QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
        if(!frame(p,{{"type","snapshot.begin"},{"size",bytes.size()},{"hash",hash}},true))return;
        for(int offset=0;offset<bytes.size();offset+=BlockSize)
            if(!frame(p,{{"type","snapshot.chunk"},{"offset",offset},{"data",QString::fromLatin1(bytes.mid(offset,BlockSize).toBase64())}},true))return;
        frame(p,{{"type","snapshot.end"}},true);
    }
    QString blockPath(const QString& hash) const {return cache.path()+"/"+hash;}
    bool saveBlock(const QString& hash,const QByteArray& b) {
        if(!cache.isValid()||!validHash(hash)||b.isEmpty()||b.size()>BlockSize||QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex())!=hash)return false;
        if(QFile::exists(blockPath(hash)))return true;
        if(cacheBytes+b.size()>MaxCache||cacheBlocks>=16384)return false;
        QSaveFile file(blockPath(hash));
        if(!file.open(QIODevice::WriteOnly)||file.write(b)!=b.size()||!file.commit())return false;
        cacheBytes+=b.size();cacheBlocks++;return true;
    }
    QByteArray block(const QString& hash) const {
        if(!validHash(hash))return {};
        QFile f(blockPath(hash));if(!f.open(QIODevice::ReadOnly)||f.size()>BlockSize)return {};
        const QByteArray b=f.readAll();
        return QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex())==hash?b:QByteArray();
    }
    void commitEvent(const QString& actor,const Result& r) {
        if(!r.accepted)return;
        QJsonObject e{{"type","commit"},{"actor",actor},{"revision",QString::number(r.revision)},{"changes",r.changes}};
        // A large delete can affect many fields; chunked snapshot is the fallback.
        if(QJsonDocument(e).toJson(QJsonDocument::Compact).size()>MaxPayload) {
            const auto values=peers.values();for(const auto& p:values)if(p->ready)sendSnapshot(*p);
            event({{"type","snapshot"},{"state",state.snapshot()}});
        } else {broadcast(e);event(e);}
    }
    void hostMessage(Peer& p,const QJsonObject& m) {
        const QString type=m.value("type").toString();
        if(type=="ping"){frame(p,{{"type","pong"}},true);return;}
        if(type=="pong")return;
        if(type=="transaction") {
            if(!m.value("transaction").isObject()){drop(p,"Invalid transaction packet");return;}
            const auto t=m.value("transaction").toObject();const auto r=state.transact(p.id,t,clock.elapsed());
            frame(p,{{"type","result"},{"request",t.value("request")},{"accepted",r.accepted},{"error",r.error},{"revision",QString::number(r.revision)}},true);
            // Idempotent retries return the cached result but must not emit the
            // already-applied delta again (clients would see a revision gap).
            if(r.accepted&&r.revision==state.revision()&&r.revision>lastBroadcastRevision) {
                lastBroadcastRevision=r.revision;commitEvent(p.id,r);
            }
        } else if(type=="lease.acquire"||type=="lease.release") {
            const auto a=m.value("entities").toArray();if(a.isEmpty()||a.size()>MaxOperations){drop(p,"Invalid lease list");return;}
            QStringList ids;for(const auto& v:a){if(!v.isString()||!State::validId(v.toString())){drop(p,"Invalid lease ID");return;}ids.append(v.toString());}
            QString error;bool ok=true;
            if(type=="lease.acquire")ok=state.acquire(p.id,ids,clock.elapsed(),m.value("duration").toInt(),error);
            else state.release(p.id,ids);
            frame(p,{{"type","lease.result"},{"entities",a},{"accepted",ok},{"error",error},{"duration",m.value("duration")}},true);
        } else if(type=="snapshot.get") {
            const qint64 now=clock.elapsed();
            if(now-lastSnapshotRequest.value(p.id,-10000)<2000){frame(p,{{"type","error"},{"error","Snapshot requests are rate limited"}},true);return;}
            lastSnapshotRequest[p.id]=now;sendSnapshot(p);
        } else if(type=="presence") {
            const auto pose=m.value("pose").toObject();const qint64 now=clock.elapsed();
            if(!poseValid(pose)){drop(p,"Invalid camera pose");return;}
            if(now-p.lastPose<50)return;p.lastPose=now;
            QJsonObject e{{"type","presence"},{"peer",p.id},{"pose",pose}};broadcast(e);event(e);
        } else if(type=="blob.put") {
            if(!state.allowed(p.id,"resources","resources.publish")){frame(p,{{"type","error"},{"error","Resource upload permission denied"}},true);return;}
            const QString hash=m.value("hash").toString();const QByteArray b=decode(m.value("data").toString(),BlockSize);
            const bool ok=saveBlock(hash,b);frame(p,{{"type","blob.result"},{"hash",hash},{"accepted",ok}},true);
        } else if(type=="blob.get") {
            const QString hash=m.value("hash").toString();const QByteArray b=block(hash);
            frame(p,{{"type","blob"},{"hash",hash},{"data",QString::fromLatin1(b.toBase64())}},true);
        } else {drop(p,"Unknown/client-forbidden operation");}
    }
    quint64 lastBroadcastRevision=0;
    QHash<QString,qint64> lastSnapshotRequest;
    void clientMessage(Peer& p,const QJsonObject& m) {
        const QString type=m.value("type").toString();
        if(type=="ping"){frame(p,{{"type","pong"}},true);return;}
        if(type=="pong")return;
        if(type=="snapshot.begin") {
            const int n=m.value("size").toInt(-1);const QString hash=m.value("hash").toString();
            if(n<=0||n>MaxDocumentBytes+1024*1024||!validHash(hash)){drop(p,"Invalid snapshot header");return;}
            p.snapshot.clear();p.snapshotSize=n;p.snapshotHash=hash;return;
        }
        if(type=="snapshot.chunk") {
            const int offset=m.value("offset").toInt(-1);const QByteArray bytes=decode(m.value("data").toString(),BlockSize);
            if(p.snapshotSize<=0||offset!=p.snapshot.size()||bytes.isEmpty()||bytes.size()>p.snapshotSize-offset){drop(p,"Invalid snapshot chunk");return;}
            p.snapshot+=bytes;return;
        }
        if(type=="snapshot.end") {
            if(p.snapshotSize<=0||p.snapshot.size()!=p.snapshotSize||QString::fromLatin1(QCryptographicHash::hash(p.snapshot,QCryptographicHash::Sha256).toHex())!=p.snapshotHash){drop(p,"Incomplete/corrupt snapshot");return;}
            QJsonParseError e;const auto doc=QJsonDocument::fromJson(p.snapshot,&e);
            if(e.error!=QJsonParseError::NoError||!doc.isObject()||!doc.object().value("entities").isObject()){drop(p,"Invalid snapshot document");return;}
            p.snapshot.clear();p.snapshotSize=0;p.snapshotHash.clear();event({{"type","snapshot"},{"state",doc.object()}});return;
        }
        if(type=="members") {if(!m.value("members").isObject()){drop(p,"Invalid member list");return;}memberList=m.value("members").toObject();}
        else if(type=="blob") {
            const QString hash=m.value("hash").toString();const QByteArray b=decode(m.value("data").toString(),BlockSize);
            if(b.isEmpty()){event({{"type","blob.missing"},{"hash",hash}});return;}
            if(!saveBlock(hash,b)){drop(p,"Invalid resource block or cache full");return;}
        } else if(type!="commit"&&type!="permissions"&&type!="result"&&type!="lease.result"&&type!="presence"&&type!="blob.result"&&type!="error") {
            drop(p,"Unknown server operation");return;
        }
        event(m);
    }
    void tick() {
        const qint64 now=clock.elapsed();const auto values=peers.values();
        for(const auto& p:values) {
            if((!p->ready&&now-p->born>10000)||(p->ready&&now-p->lastRead>30000)){drop(*p,"Peer timed out");continue;}
            if(p->ready)frame(*p,{{"type","ping"}},true);
        }
    }
};
Session::Session(const QString& build,QObject* parent):QObject(parent),d(new Impl(this,build)){}
Session::~Session(){close();}
State& Session::authority(){return d->state;}
bool Session::host(const QHostAddress& address,quint16 port,QString& error){
    if(d->isHost||d->joining||!d->peers.isEmpty()){error="Close the current session first";return false;}
    if(d->build.isEmpty()||d->build.size()>128||!d->cache.isValid()){error="Invalid compatibility ID/cache";return false;}
    d->state.reset();d->secret=randomBytes(32);d->isHost=true;d->id="host";d->lastBroadcastRevision=0;
    if(!d->server.listen(address,port)){error=d->server.errorString();d->isHost=false;d->id.clear();d->secret.clear();return false;}
    d->sendMembers();d->status("LAN host listening on port "+QString::number(d->server.serverPort()));return true;
}
QString Session::invitationCode() const {return d->isHost?QString::fromLatin1(d->secret.toBase64()):QString();}
bool Session::join(const QString& address,quint16 port,const QString& name,const QString& code,QString& error){
    const QByteArray key=decode(code,32);
    if(d->isHost||d->joining||!d->peers.isEmpty()||!port||address.trimmed().isEmpty()||address.size()>253||name.trimmed().isEmpty()||name.size()>48||key.size()!=32||d->build.isEmpty()||d->build.size()>128){error="Invalid join request or session already active";return false;}
    d->secret=key;d->name=name.trimmed();d->joining=true;
    auto* socket=new QTcpSocket(this);auto p=std::make_shared<Impl::Peer>();d->wire(socket,p);socket->connectToHost(address,port);return true;
}
void Session::close(){
    d->isHost=false;d->joining=false;d->server.close();
    const auto values=d->peers.values();for(const auto& p:values)if(p->socket){QObject::disconnect(p->socket.data(),nullptr,this,nullptr);p->socket->abort();p->socket->deleteLater();}
    d->peers.clear();d->state.reset();
    // Never make cached assets from an earlier project available to a later session.
    if(d->cache.isValid()) for(const QString& hash:QDir(d->cache.path()).entryList(QDir::Files))
        if(validHash(hash))QFile::remove(d->blockPath(hash));
    d->cacheBytes=0;d->cacheBlocks=0;d->lastPresence=-1000;
    d->secret.fill(0);d->secret.clear();d->id.clear();d->memberList=QJsonObject();d->lastSnapshotRequest.clear();d->lastBroadcastRevision=0;
}
bool Session::hosting()const{return d->isHost;}
bool Session::connected()const{if(d->isHost)return true;for(const auto& p:d->peers)if(p->ready)return true;return false;}
quint16 Session::port()const{return d->server.serverPort();}
QString Session::localId()const{return d->id;}
QJsonObject Session::members()const{return d->memberList;}
void Session::setGrant(const QString& peer,const Grant& g){
    if(!d->isHost)return;
    const auto values=d->peers.values();
    for(const auto& p:values)if(p->ready&&p->id==peer){d->state.setGrant(peer,g);d->frame(*p,{{"type","permissions"},{"permissions",grantJson(g)}},true);}
}
Result Session::hostTransaction(const QJsonObject& t){
    if(!d->isHost){Result r;r.error="Not the host";return r;}
    const Result r=d->state.transact("host",t,d->clock.elapsed());
    if(r.accepted&&r.revision>d->lastBroadcastRevision){d->lastBroadcastRevision=r.revision;d->commitEvent("host",r);}return r;
}
bool Session::submit(const QJsonObject& t){return d->sendClient({{"type","transaction"},{"transaction",t}});}
bool Session::acquire(const QStringList& e,int ms){return d->sendClient({{"type","lease.acquire"},{"entities",strings(e)},{"duration",ms}});}
bool Session::release(const QStringList& e){return d->sendClient({{"type","lease.release"},{"entities",strings(e)}});}
bool Session::requestSnapshot(){return d->sendClient({{"type","snapshot.get"}});}
bool Session::publishPresence(const QJsonObject& p){
    const qint64 now=d->clock.elapsed();if(!poseValid(p)||now-d->lastPresence<50||!connected())return false;d->lastPresence=now;
    if(d->isHost){d->broadcast({{"type","presence"},{"peer","host"},{"pose",p}});return true;}
    return d->sendClient({{"type","presence"},{"pose",p}});
}
bool Session::uploadBlock(const QByteArray& bytes){
    if(bytes.isEmpty()||bytes.size()>BlockSize)return false;
    const QString hash=QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
    if(d->isHost)return d->saveBlock(hash,bytes);
    return d->sendClient({{"type","blob.put"},{"hash",hash},{"data",QString::fromLatin1(bytes.toBase64())}});
}
bool Session::requestBlock(const QString& hash){return validHash(hash)&&d->sendClient({{"type","blob.get"},{"hash",hash}});}
QByteArray Session::cachedBlock(const QString& hash)const{return d->block(hash);}
}
