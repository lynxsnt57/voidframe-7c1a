#pragma once
#include <QColor>
#include <QHash>
#include <QJsonObject>
#include <QQuaternion>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QVector3D>
namespace VoidframeLan {
struct CameraPose { QVector3D position;QQuaternion rotation;float fov=60; };
struct CameraLine { QVector3D from,to;QColor colour; };
class Presence {
public:
    // nowMs must be local monotonic time, NOT a sender-supplied timestamp.
    bool update(const QString& peer,const QJsonObject& pose,qint64 nowMs);
    void remove(const QString& peer){tracks_.remove(peer);}
    void clear(){tracks_.clear();}
    bool sample(const QString& peer,qint64 nowMs,CameraPose& pose)const;
    QStringList active(qint64 nowMs)const;
    // Overlay-only drone: diamond shell, four outriggers, circular lens and
    // view frustum. The bridge must EXCLUDE these lines from export/shadow/pick.
    static QVector<CameraLine> drone(const CameraPose& pose,int colourIndex,float size=1.f);
private:
    struct Track { CameraPose previous,target;qint64 received=0; };
    QHash<QString,Track> tracks_;
};
}
