#include "LanPresence.hpp"
#include "LanState.hpp"
#include <QJsonArray>
#include <QMatrix4x4>
#include <algorithm>
#include <cmath>
namespace VoidframeLan {
bool Presence::update(const QString& peer,const QJsonObject& raw,qint64 now){
    if(!State::validId(peer)||!State::vector3(raw.value("position"))||!State::number(raw.value("fov")))return false;
    const auto p=raw.value("position").toArray(),q=raw.value("rotation").toArray();
    if(q.size()!=4||raw.value("fov").toDouble()<5||raw.value("fov").toDouble()>170)return false;
    double n=0;for(const auto& x:q){if(!State::number(x))return false;n+=x.toDouble()*x.toDouble();}if(n<.999||n>1.001)return false;
    CameraPose target{QVector3D(float(p[0].toDouble()),float(p[1].toDouble()),float(p[2].toDouble())),
        QQuaternion(float(q[3].toDouble()),float(q[0].toDouble()),float(q[1].toDouble()),float(q[2].toDouble())).normalized(),float(raw.value("fov").toDouble())};
    CameraPose previous=target;sample(peer,now,previous);
    if((previous.position-target.position).lengthSquared()>1000000)previous=target;
    tracks_[peer]=Track{previous,target,now};return true;
}
bool Presence::sample(const QString& peer,qint64 now,CameraPose& pose)const{
    auto it=tracks_.constFind(peer);if(it==tracks_.cend()||now-it->received>5000||now<it->received)return false;
    const float t=std::max(0.f,std::min(1.f,float(now-it->received)/100.f));
    pose.position=it->previous.position*(1-t)+it->target.position*t;
    pose.rotation=QQuaternion::slerp(it->previous.rotation,it->target.rotation,t);
    pose.fov=it->previous.fov*(1-t)+it->target.fov*t;return true;
}
QStringList Presence::active(qint64 now)const{
    QStringList ids;for(auto it=tracks_.begin();it!=tracks_.end();++it)if(now>=it->received&&now-it->received<=5000)ids.append(it.key());ids.sort();return ids;
}
QVector<CameraLine> Presence::drone(const CameraPose& pose,int index,float size){
    QVector<CameraLine> out;
    if(!std::isfinite(size)||size<=0||size>1000)return out;
    const QColor palette[]={QColor("#49c5ff"),QColor("#ffbb45"),QColor("#a98aff"),QColor("#ff729a"),QColor("#4ee0ac"),QColor("#ed925b"),QColor("#d4e05a"),QColor("#83a8ff")};
    const QColor colour=palette[((index%8)+8)%8];QMatrix4x4 matrix;matrix.translate(pose.position);matrix.rotate(pose.rotation);matrix.scale(size);
    auto line=[&](const QVector3D& a,const QVector3D& b,const QColor& c){out.append(CameraLine{matrix.map(a),matrix.map(b),c});};
    const QVector3D ring[]={QVector3D(-.35f,0,0),QVector3D(0,.2f,0),QVector3D(.35f,0,0),QVector3D(0,-.2f,0)};
    for(int i=0;i<4;i++){
        line(ring[i],ring[(i+1)%4],colour);line(ring[i],QVector3D(0,0,.25f),colour);line(ring[i],QVector3D(0,0,-.25f),colour);
        const float x=(i&1)?.65f:-.65f,y=(i&2)?.4f:-.4f;
        line(QVector3D(x*.4f,y*.4f,0),QVector3D(x,y,0),colour);
        for(int k=0;k<12;k++){
            const float a=float(k)*6.28318530718f/12,b=float(k+1)*6.28318530718f/12;
            line(QVector3D(x+std::cos(a)*.13f,y+std::sin(a)*.13f,0),QVector3D(x+std::cos(b)*.13f,y+std::sin(b)*.13f,0),colour);
        }
    }
    for(int k=0;k<16;k++){
        const float a=float(k)*6.28318530718f/16,b=float(k+1)*6.28318530718f/16;
        line(QVector3D(std::cos(a)*.14f,std::sin(a)*.14f,-.35f),QVector3D(std::cos(b)*.14f,std::sin(b)*.14f,-.35f),QColor("#f4fbff"));
    }
    const float h=std::tan(std::max(5.f,std::min(170.f,pose.fov))*3.14159265359f/360.f)*1.3f,w=h*16.f/9.f;
    const QVector3D frame[]={QVector3D(-w,-h,-1.65f),QVector3D(w,-h,-1.65f),QVector3D(w,h,-1.65f),QVector3D(-w,h,-1.65f)};
    QColor faint=colour;faint.setAlpha(100);
    for(int i=0;i<4;i++){line(frame[i],frame[(i+1)%4],faint);line(QVector3D(0,0,-.35f),frame[i],faint);}
    return out;
}
}
