#include "LanSession.hpp"
#include <QCryptographicHash>
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
#include <limits>
namespace VoidframeLan {
namespace {
constexpr int MaxFrame=256*1024,MaxPayload=128*1024,BlockSize=64*1024;
constexpr qint64 MaxQueued=24*1024*1024,MaxCache=1024LL*1024*1024;
QByteArray randomBytes(int n) {
    QByteArray b(n,0);
    for(int i=0;i<n;i++) b\[i\]=char(QRandomGenerator::system()->generate()&amp;255);
    return b;
}
QByteArray mac(const QByteArray&amp; key,const QByteArray&amp; data) { return QMessageAuthenticationCode::hash(data,key,QCryptographicHash::Sha256); }
bool equal(const QByteArray&amp; a,const QByteArray&amp; b) {
    if(a.size()!=b.size()) return false;
    unsigned diff=0;for(int i=0;i<a.size();i++) diff|=static\_cast&lt;unsigned char>(a\[i\])^static\_cast<unsigned char>(b\[i\]);return diff==0;
}
QByteArray decode(const QString&amp; text,int max) {
    if(text.size()&gt;max*4/3+8) return {};
    auto v=QByteArray::fromBase64Encoding(text.toLatin1(),QByteArray::AbortOnBase64DecodingErrors);
    return v&amp;&amp;v.decoded.size()&lt;=max?v.decoded:QByteArray();
}
bool validHash(const QString&amp; h) {
    static const QRegularExpression r("^\[0-9a-f\]{64}$");return r.match(h).hasMatch();
}
QString uid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
QJsonArray strings(const QStringList&amp; s) { QJsonArray a;for(const QString&amp; x:s)a.append(x);return a; }
QJsonObject grantJson(const Grant&amp; g) {
    QStringList sorted=g.global.values();sorted.sort();QJsonObject objects;
    for(auto it=g.objects.begin();it!=g.objects.end();++it) {QStringList list=it.value().values();list.sort();objects\[it.key()\]=strings(list);}
    return {{"global",strings(sorted)},{"objects",objects}};
}
bool poseValid(const QJsonObject&amp; p) {
    if(!State::vector3(p.value("position")))return false;
    const auto q=p.value("rotation").toArray();if(q.size()!=4)return false;
    double length=0;for(const auto&amp; c:q) {if(!State::number(c))return false;length+=c.toDouble()*c.toDouble();}
    return length&gt;.999&amp;&amp;length&lt;1.001&amp;&amp;State::number(p.value("fov"))&amp;&amp;p.value("fov").toDouble()&gt;=5&amp;&amp;p.value("fov").toDouble()&lt;=170;
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
    QHash<QTcpSocket\*,std::shared\_ptr&lt;Peer>&gt; peers;
    QJsonObject memberList;
    bool isHost=false,joining=false;
    qint64 lastPresence=-1000;
    quint64 lastBroadcastRevision=0;
    QHash<QString,qint64> lastSnapshotRequest;
    Impl(Session\* o,const QString&amp; compatibility):owner(o),build(compatibility),server(o),timer(o) {
        clock.start();
        QObject::connect(&amp;server,&amp;QTcpServer::newConnection,o,\[this\]{accept();});
        QObject::connect(&amp;timer,&amp;QTimer::timeout,o,\[this\]{tick();});
        timer.start(1000);
    }
    void status(const QString&amp; text) {if(owner-&gt;onStatus)owner-&gt;onStatus(text);}
    void event(const QJsonObject&amp; e) {if(owner-&gt;onEvent)owner-&gt;onEvent(e);}
    QByteArray proof(const Peer&amp; p,const QByteArray&amp; label) const {
        return mac(secret,label+'\\0'+p.nonce+'\\0'+p.id.toUtf8()+'\\0'+build.toUtf8());
    }
    void cleanup(QTcpSocket\* socket) {
        auto p=peers.take(socket);if(!p)return;
        state.disconnected(p-&gt;id);lastSnapshotRequest.remove(p-&gt;id);socket-&gt;deleteLater();
        if(isHost)sendMembers();else {joining=false;id.clear();memberList=QJsonObject();status("Disconnected; discard uncommitted speculative edits");}
        event({{"type","left"},{"peer",p-&gt;id}});
    }
    void drop(Peer&amp; p,const QString&amp; reason) {
        status(reason);auto\* socket=p.socket.data();
        if(socket){socket-&gt;abort();cleanup(socket);}
    }
    void flush(Peer&amp; p) {
        if(!p.socket)return;
        while(!p.output.isEmpty()&amp;&amp;p.socket-&gt;bytesToWrite()&lt;256\*1024) {
            const QByteArray f=p.output.dequeue();p.queued-=f.size();
            if(p.socket-&gt;write(f)!=f.size()) {drop(p,"Socket write failed");return;}
        }
    }
    bool frame(Peer&amp; p,const QJsonObject&amp; message,bool signedFrame) {
        QByteArray payload=QJsonDocument(message).toJson(QJsonDocument::Compact);
        if(payload.size()&gt;MaxPayload)return false;
        QJsonObject wrapped=message;
        if(signedFrame) {
            if(!p.ready||p.sent==std::numeric\_limits<quint64>::max())return false;
            const QByteArray sequence=QByteArray::number(++p.sent);
            const QByteArray direction=isHost?"S":"C";
            wrapped={{"seq",QString::fromLatin1(sequence)},{"payload",QString::fromLatin1(payload.toBase64())},
                     {"mac",QString::fromLatin1(mac(p.key,direction+sequence+'\\0'+payload).toHex())}};
        }
        const QByteArray bytes=QJsonDocument(wrapped).toJson(QJsonDocument::Compact);
        if(bytes.size()&gt;MaxFrame||p.queued+(p.socket?p.socket-&gt;bytesToWrite():0)+bytes.size()&gt;MaxQueued) {drop(p,"Slow peer exceeded output budget");return false;}
        const quint32 n=quint32(bytes.size());QByteArray header(4,0);
        header\[0\]=char(n&gt;&gt;24);header\[1\]=char(n&gt;&gt;16);header\[2\]=char(n&gt;&gt;8);header\[3\]=char(n);
        p.output.enqueue(header+bytes);p.queued+=bytes.size()+4;flush(p);return true;
    }
    bool sendClient(const QJsonObject&amp; o) {
        if(isHost||peers.isEmpty())return false;
        const auto values=peers.values();for(const auto&amp; p:values)if(p-&gt;ready)return frame(*p,o,true);
        return false;
    }
    void broadcast(const QJsonObject&amp; o) {
        const auto values=peers.values();for(const auto&amp; p:values)if(p-&gt;ready)frame(p,o,true);
    }
    void sendMembers() {
        QJsonObject m;
        m\["host"\]=QJsonObject{{"name","Host"},{"color",0}};
        for(const auto&amp; p:peers)if(p-&gt;ready)m\[p-&gt;id\]=QJsonObject{{"name",p-&gt;name},{"color",int(qHash(p-&gt;id)%8)}};
        memberList=m;broadcast({{"type","members"},{"members",m}});event({{"type","members"},{"members",m}});
    }
    void wire(QTcpSocket socket,const std::shared\_ptr<Peer>&amp; p) {
        p-&gt;socket=socket;p-&gt;born=p-&gt;lastRead=clock.elapsed();peers.insert(socket,p);
        socket-&gt;setReadBufferSize(MaxFrame+4);
        QObject::connect(socket,&amp;QTcpSocket::readyRead,owner,\[this,socket\]{auto p=peers.value(socket);if(p)read(p);});
        QObject::connect(socket,&amp;QTcpSocket::bytesWritten,owner,[this,socket](qint64){auto p=peers.value(socket);if(p)flush(p);});
        QObject::connect(socket,&amp;QTcpSocket::disconnected,owner,\[this,socket\]{cleanup(socket);});
        QObject::connect(socket,&amp;QAbstractSocket::errorOccurred,owner,[this,socket](QAbstractSocket::SocketError){
            auto p=peers.value(socket);if(p)drop(p,socket-&gt;errorString());
        });
    }
    void accept() {
        while(server.hasPendingConnections()) {
            auto s=server.nextPendingConnection();
            if(peers.size()&gt;=8) {s-&gt;abort();s-&gt;deleteLater();continue;}
            auto p=std::make\_shared<Peer>();p-&gt;id=uid();p-&gt;nonce=randomBytes(32);wire(s,p);
            frame(p,{{"type","challenge"},{"protocol",ProtocolVersion},{"build",build},{"id",p-&gt;id},{"nonce",QString::fromLatin1(p-&gt;nonce.toBase64())}},false);
        }
    }
    void read(Peer&amp; p) {
        while(p.socket&amp;&amp;p.socket-&gt;bytesAvailable()&gt;0) {
            const int remaining=MaxFrame+4-p.input.size();if(remaining&lt;=0){drop(p,"Input buffer overflow");return;}
            const QByteArray b=p.socket-&gt;read(std::min(remaining,641024));if(b.isEmpty())break;
            p.input+=b;
            while(p.input.size()&gt;=4) {
                const auto x=reinterpret\_cast<const unsigned char>(p.input.constData());
                const quint32 n=(quint32(x\[0\])&lt;&lt;24)|(quint32(x\[1\])&lt;&lt;16)|(quint32(x\[2\])&lt;&lt;8)|x\[3\];
                if(!n||n&gt;quint32(MaxFrame)){drop(p,"Invalid frame size");return;}
                if(p.input.size()&lt;int(n)+4)break;
                const QByteArray packet=p.input.mid(4,int(n));p.input.remove(0,int(n)+4);
                const qint64 now=clock.elapsed();
                if(now-p.rateAt&gt;=1000){p.rateAt=now;p.messages=0;p.bytesAt=0;}
                p.messages++;p.bytesAt+=packet.size();
                if(p.messages&gt;512||p.bytesAt&gt;32*1024*1024){drop(p,"Peer rate limit exceeded");return;}
                QJsonParseError error;const auto parsed=QJsonDocument::fromJson(packet,&amp;error);
                if(error.error!=QJsonParseError::NoError||!parsed.isObject()){drop(p,"Malformed JSON frame");return;}
                QJsonObject message=parsed.object();
                if(p.ready) {
                    quint64 seq=0;const QByteArray body=decode(message.value("payload").toString(),MaxPayload);
                    const QString tagText=message.value("mac").toString();
                    if(!State::parseRevision(message.value("seq"),seq)||p.received==std::numeric\_limits<quint64>::max()||seq!=p.received+1||body.isEmpty()||!validHash(tagText)) {drop(p,"Bad message sequence/envelope");return;}
                    const QByteArray direction=isHost?"C":"S";
                    if(!equal(QByteArray::fromHex(tagText.toLatin1()),mac(p.key,direction+QByteArray::number(seq)+'\\0'+body))) {drop(p,"Message authentication failed");return;}
                    const auto doc=QJsonDocument::fromJson(body,&amp;error);
                    if(error.error!=QJsonParseError::NoError||!doc.isObject()){drop(p,"Invalid signed payload");return;}
                    message=doc.object();p.received=seq;
                }
                p.lastRead=now;
                if(!p.ready)handshake(p,message);else if(isHost)hostMessage(p,message);else clientMessage(p,message);
                if(!p.socket||p.socket-&gt;state()==QAbstractSocket::UnconnectedState)return;
            }
        }
    }
    void handshake(Peer&amp; p,const QJsonObject&amp; m) {
        const QString type=m.value("type").toString();
        if(isHost) {
            const QString peerName=m.value("name").toString();
            if(type!="authenticate"||peerName.trimmed().isEmpty()||peerName.size()&gt;48||m.value("build").toString()!=build||
               !equal(decode(m.value("proof").toString(),32),proof(p,"client"))) {drop(p,"Authentication/build check failed");return;}
            p.name=peerName.trimmed();p.key=proof(p,"session-key");
            frame(p,{{"type","authenticated"},{"proof",QString::fromLatin1(proof(p,"server").toBase64())}},false);
            p.ready=true;state.setGrant(p.id,Grant());
            frame(p,{{"type","permissions"},{"permissions",grantJson(state.grant(p.id))}},true);
            sendMembers();sendSnapshot(p);event({{"type","joined"},{"peer",p.id},{"name",p.name}});
        } else if(type=="challenge"&amp;&amp;p.nonce.isEmpty()) {
            const auto nonce=decode(m.value("nonce").toString(),32);const QString peerId=m.value("id").toString();
            if(m.value("protocol").toInt(-1)!=ProtocolVersion||m.value("build").toString()!=build||nonce.size()!=32||!State::validId(peerId)||peerId=="host") {drop(p,"Incompatible server");return;}
            p.nonce=nonce;p.id=peerId;
            frame(p,{{"type","authenticate"},{"name",name},{"build",build},{"proof",QString::fromLatin1(proof(p,"client").toBase64())}},false);
        } else if(type=="authenticated"&amp;&amp;!p.nonce.isEmpty()) {
            if(!equal(decode(m.value("proof").toString(),32),proof(p,"server"))){drop(p,"Server authentication failed");return;}
            p.key=proof(p,"session-key");p.ready=true;id=p.id;joining=false;status("Connected; waiting for authoritative snapshot");
        } else drop(p,"Unexpected handshake message");
    }
    void sendSnapshot(Peer&amp; p) {
        const QByteArray bytes=QJsonDocument(state.snapshot()).toJson(QJsonDocument::Compact);
        if(bytes.size()&gt;MaxDocumentBytes+1024*1024){drop(p,"Snapshot exceeds transport budget");return;}
        const QString hash=QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
        if(!frame(p,{{"type","snapshot.begin"},{"size",bytes.size()},{"hash",hash}},true))return;
        for(int offset=0;offset&lt;bytes.size();offset+=BlockSize)
            if(!frame(p,{{"type","snapshot.chunk"},{"offset",offset},{"data",QString::fromLatin1(bytes.mid(offset,BlockSize).toBase64())}},true))return;
        frame(p,{{"type","snapshot.end"}},true);
    }
    QString blockPath(const QString&amp; hash) const {return cache.path()+"/"+hash;}
    bool saveBlock(const QString&amp; hash,const QByteArray&amp; b) {
        if(!cache.isValid()||!validHash(hash)||b.isEmpty()||b.size()&gt;BlockSize||QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex())!=hash)return false;
        if(QFile::exists(blockPath(hash)))return true;
        if(cacheBytes+b.size()&gt;MaxCache||cacheBlocks&gt;=16384)return false;
        QSaveFile file(blockPath(hash));
        if(!file.open(QIODevice::WriteOnly)||file.write(b)!=b.size()||!file.commit())return false;
        cacheBytes+=b.size();cacheBlocks++;return true;
    }
    QByteArray block(const QString&amp; hash) const {
        if(!validHash(hash))return {};
        QFile f(blockPath(hash));if(!f.open(QIODevice::ReadOnly)||f.size()&gt;BlockSize)return {};
        const QByteArray b=f.readAll();
        return QString::fromLatin1(QCryptographicHash::hash(b,QCryptographicHash::Sha256).toHex())==hash?b:QByteArray();
    }
    void commitEvent(const QString&amp; actor,const Result&amp; r) {
        if(!r.accepted)return;
        QJsonObject e{{"type","commit"},{"actor",actor},{"revision",QString::number(r.revision)},{"changes",r.changes}};
        if(QJsonDocument(e).toJson(QJsonDocument::Compact).size()&gt;MaxPayload) {
            const auto values=peers.values();for(const auto&amp; p:values)if(p-&gt;ready)sendSnapshot(*p);
            event({{"type","snapshot"},{"state",state.snapshot()}});
        } else {broadcast(e);event(e);}
    }
    void hostMessage(Peer&amp; p,const QJsonObject&amp; m) {
        const QString type=m.value("type").toString();
        if(type=="ping"){frame(p,{{"type","pong"}},true);return;}
        if(type=="pong")return;
        if(type=="transaction") {
            if(!m.value("transaction").isObject()){drop(p,"Invalid transaction packet");return;}
            const auto t=m.value("transaction").toObject();const auto r=state.transact(p.id,t,clock.elapsed());
            frame(p,{{"type","result"},{"request",t.value("request")},{"accepted",r.accepted},{"error",r.error},{"revision",QString::number(r.revision)}},true);
            if(r.accepted&amp;&amp;r.revision==state.revision()&amp;&amp;r.revision&gt;lastBroadcastRevision) {
                lastBroadcastRevision=r.revision;commitEvent(p.id,r);
            }
        } else if(type=="lease.acquire"||type=="lease.release") {
            const auto a=m.value("entities").toArray();if(a.isEmpty()||a.size()&gt;MaxOperations){drop(p,"Invalid lease list");return;}
            QStringList ids;for(const auto&amp; v:a){if(!v.isString()||!State::validId(v.toString())){drop(p,"Invalid lease ID");return;}ids.append(v.toString());}
            QString error;bool ok=true;
            if(type=="lease.acquire")ok=state.acquire(p.id,ids,clock.elapsed(),m.value("duration").toInt(),error);
            else state.release(p.id,ids);
            frame(p,{{"type","lease.result"},{"entities",a},{"accepted",ok},{"error",error},{"duration",m.value("duration")}},true);
        } else if(type=="snapshot.get") {
            const qint64 now=clock.elapsed();
            if(now-lastSnapshotRequest.value(p.id,-10000)&lt;2000){frame(p,{{"type","error"},{"error","Snapshot requests are rate limited"}},true);return;}
            lastSnapshotRequest\[p.id\]=now;sendSnapshot(p);
        } else if(type=="presence") {
            const auto pose=m.value("pose").toObject();const qint64 now=clock.elapsed();
            if(!poseValid(pose)){drop(p,"Invalid camera pose");return;}
            if(now-p.lastPose&lt;50)return;
            p.lastPose=now;
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
    void clientMessage(Peer&amp; p,const QJsonObject&amp; m) {
        const QString type=m.value("type").toString();
        if(type=="ping"){frame(p,{{"type","pong"}},true);return;}
        if(type=="pong")return;
        if(type=="snapshot.begin") {
            const int n=m.value("size").toInt(-1);const QString hash=m.value("hash").toString();
            if(n&lt;=0||n&gt;MaxDocumentBytes+1024*1024||!validHash(hash)){drop(p,"Invalid snapshot header");return;}
            p.snapshot.clear();p.snapshotSize=n;p.snapshotHash=hash;return;
        }
        if(type=="snapshot.chunk") {
            const int offset=m.value("offset").toInt(-1);const QByteArray bytes=decode(m.value("data").toString(),BlockSize);
            if(p.snapshotSize&lt;=0||offset!=p.snapshot.size()||bytes.isEmpty()||bytes.size()&gt;p.snapshotSize-offset){drop(p,"Invalid snapshot chunk");return;}
            p.snapshot+=bytes;return;
        }
        if(type=="snapshot.end") {
            if(p.snapshotSize&lt;=0||p.snapshot.size()!=p.snapshotSize||QString::fromLatin1(QCryptographicHash::hash(p.snapshot,QCryptographicHash::Sha256).toHex())!=p.snapshotHash){drop(p,"Incomplete/corrupt snapshot");return;}
            QJsonParseError e;const auto doc=QJsonDocument::fromJson(p.snapshot,&amp;e);
            if(e.error!=QJsonParseError::NoError||!doc.isObject()||!doc.object().value("entities").isObject()){drop(p,"Invalid snapshot document");return;}
            p.snapshot.clear();p.snapshotSize=0;p.snapshotHash.clear();event({{"type","snapshot"},{"state",doc.object()}});return;
        }
        if(type=="members") {if(!m.value("members").isObject()){drop(p,"Invalid member list");return;}memberList=m.value("members").toObject();}
        else if(type=="blob") {
            const QString hash=m.value("hash").toString();const QByteArray b=decode(m.value("data").toString(),BlockSize);
            if(b.isEmpty()){event({{"type","blob.missing"},{"hash",hash}});return;}
            if(!saveBlock(hash,b)){drop(p,"Invalid resource block or cache full");return;}
        } else if(type!="commit"&amp;&amp;type!="permissions"&amp;&amp;type!="result"&amp;&amp;type!="lease.result"&amp;&amp;type!="presence"&amp;&amp;type!="blob.result"&amp;&amp;type!="error") {
            drop(p,"Unknown server operation");return;
        }
        event(m);
    }
    void tick() {
        const qint64 now=clock.elapsed();const auto values=peers.values();
        for(const auto&amp; p:values) {
            if((!p-&gt;ready&amp;&amp;now-p-&gt;born&gt;10000)||(p-&gt;ready&amp;&amp;now-p-&gt;lastRead&gt;30000)){drop(*p,"Peer timed out");continue;}
            if(p-&gt;ready)frame(p,{{"type","ping"}},true);
        }
    }
};
Session::Session(const QString&amp; build,QObject parent):QObject(parent),d(new Impl(this,build)){}
Session::\~Session(){close();}
State&amp; Session::authority(){return d-&gt;state;}
bool Session::host(const QHostAddress&amp; address,quint16 port,QString&amp; error){
    if(d-&gt;isHost||d-&gt;joining||!d-&gt;peers.isEmpty()){error="Close the current session first";return false;}
    if(d-&gt;build.isEmpty()||d-&gt;build.size()&gt;128||!d-&gt;cache.isValid()){error="Invalid compatibility ID/cache";return false;}
    d-&gt;state.reset();d-&gt;secret=randomBytes(32);d-&gt;isHost=true;d-&gt;id="host";d-&gt;lastBroadcastRevision=0;
    if(!d-&gt;server.listen(address,port)){error=d-&gt;server.errorString();d-&gt;isHost=false;d-&gt;id.clear();d-&gt;secret.clear();return false;}
    d-&gt;sendMembers();d-&gt;status("LAN host listening on port "+QString::number(d-&gt;server.serverPort()));return true;
}
QString Session::invitationCode() const {return d-&gt;isHost?QString::fromLatin1(d-&gt;secret.toBase64()):QString();}
bool Session::join(const QString&amp; address,quint16 port,const QString&amp; name,const QString&amp; code,QString&amp; error){
    const QByteArray key=decode(code,32);
    if(d-&gt;isHost||d-&gt;joining||!d-&gt;peers.isEmpty()||!port||address.trimmed().isEmpty()||address.size()&gt;253||name.trimmed().isEmpty()||name.size()&gt;48||key.size()!=32||d-&gt;build.isEmpty()||d-&gt;build.size()&gt;128){error="Invalid join request or session already active";return false;}
    d-&gt;secret=key;d-&gt;name=name.trimmed();d-&gt;joining=true;
    auto* socket=new QTcpSocket(this);auto p=std::make\_shared<Impl::Peer>();d-&gt;wire(socket,p);socket-&gt;connectToHost(address,port);return true;
}
void Session::close(){
    d-&gt;isHost=false;d-&gt;joining=false;d-&gt;server.close();
    const auto values=d-&gt;peers.values();for(const auto&amp; p:values)if(p-&gt;socket){QObject::disconnect(p-&gt;socket.data(),nullptr,this,nullptr);p-&gt;socket-&gt;abort();p-&gt;socket-&gt;deleteLater();}
    d-&gt;peers.clear();d-&gt;state.reset();
    if(d-&gt;cache.isValid()) for(const QString&amp; hash:QDir(d-&gt;cache.path()).entryList(QDir::Files))
        if(validHash(hash))QFile::remove(d-&gt;blockPath(hash));
    d-&gt;cacheBytes=0;d-&gt;cacheBlocks=0;d-&gt;lastPresence=-1000;
    d-&gt;secret.fill(0);d-&gt;secret.clear();d-&gt;id.clear();d-&gt;memberList=QJsonObject();d-&gt;lastSnapshotRequest.clear();d-&gt;lastBroadcastRevision=0;
}
bool Session::hosting()const{return d-&gt;isHost;}
bool Session::connected()const{if(d-&gt;isHost)return true;for(const auto&amp; p:d-&gt;peers)if(p-&gt;ready)return true;return false;}
quint16 Session::port()const{return d-&gt;server.serverPort();}
QString Session::localId()const{return d-&gt;id;}
QJsonObject Session::members()const{return d-&gt;memberList;}
void Session::setGrant(const QString&amp; peer,const Grant&amp; g){
    if(!d-&gt;isHost)return;
    const auto values=d-&gt;peers.values();
    for(const auto&amp; p:values)if(p-&gt;ready&amp;&amp;p-&gt;id==peer){d-&gt;state.setGrant(peer,g);d-&gt;frame(\*p,{{"type","permissions"},{"permissions",grantJson(g)}},true);}
}
Result Session::hostTransaction(const QJsonObject&amp; t){
    if(!d-&gt;isHost){Result r;r.error="Not the host";return r;}
    const Result r=d-&gt;state.transact("host",t,d-&gt;clock.elapsed());
    if(r.accepted&amp;&amp;r.revision&gt;d-&gt;lastBroadcastRevision){d-&gt;lastBroadcastRevision=r.revision;d-&gt;commitEvent("host",r);}return r;
}
bool Session::submit(const QJsonObject&amp; t){return d-&gt;sendClient({{"type","transaction"},{"transaction",t}});}
bool Session::acquire(const QStringList&amp; e,int ms){return d-&gt;sendClient({{"type","lease.acquire"},{"entities",strings(e)},{"duration",ms}});}
bool Session::release(const QStringList&amp; e){return d-&gt;sendClient({{"type","lease.release"},{"entities",strings(e)}});}
bool Session::requestSnapshot(){return d-&gt;sendClient({{"type","snapshot.get"}});}
bool Session::publishPresence(const QJsonObject&amp; p){
    const qint64 now=d-&gt;clock.elapsed();if(!poseValid(p)||now-d-&gt;lastPresence&lt;50||!connected())return false;d-&gt;lastPresence=now;
    if(d-&gt;isHost){d-&gt;broadcast({{"type","presence"},{"peer","host"},{"pose",p}});return true;}
    return d-&gt;sendClient({{"type","presence"},{"pose",p}});
}
bool Session::uploadBlock(const QByteArray&amp; bytes){
    if(bytes.isEmpty()||bytes.size()&gt;BlockSize)return false;
    const QString hash=QString::fromLatin1(QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex());
    if(d-&gt;isHost)return d-&gt;saveBlock(hash,bytes);
    return d-&gt;sendClient({{"type","blob.put"},{"hash",hash},{"data",QString::fromLatin1(bytes.toBase64())}});
}
bool Session::requestBlock(const QString&amp; hash){return validHash(hash)&amp;&amp;d-&gt;sendClient({{"type","blob.get"},{"hash",hash}});}
QByteArray Session::cachedBlock(const QString&amp; hash)const{return d-&gt;block(hash);}
}