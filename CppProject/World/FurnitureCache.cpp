#include "FurnitureCache.hpp"
#include "FurnitureData.hpp"
#include "FurnitureBake.hpp"
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <algorithm>

namespace CppProject {
namespace {
QStringList furnitureFiles(const QString& instance) {
 QStringList out;
 QDir saves(instance+"/saves");
 for(const QFileInfo& world:saves.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot,QDir::Name)) {
  QDir hashes(world.absoluteFilePath()+"/immersive_furniture/hash");
  for(const QFileInfo& file:hashes.entryInfoList(QStringList()<<"*.nbt",QDir::Files|QDir::Readable,QDir::Name))
   if(QRegularExpression("^[a-f0-9]{64}\\.nbt$").match(file.fileName()).hasMatch())out.append(file.absoluteFilePath());
 }
 return out;
}
bool writeJson(const QString& path,const QJsonObject& object) {
 QDir().mkpath(QFileInfo(path).absolutePath());QFile file(path);
 if (!file.open(QIODevice::WriteOnly|QIODevice::Truncate)) return false;
 QByteArray json = QJsonDocument(object).toJson(QJsonDocument::Compact);
 return file.write(json) == json.size();
}
QJsonArray point(const std::array<float,3>& p){return {p[0],p[1],p[2]};}
QString textureName(const std::string& s){
 QString t=QString::fromStdString(s).toLower();
 if (!t.contains(':')) t="minecraft:"+t;
 QStringList pieces=t.split(':');
 if (pieces.size()!=2 || !QRegularExpression("^[a-z0-9_.-]+$").match(pieces[0]).hasMatch() ||
     !QRegularExpression("^[a-z0-9_./-]+$").match(pieces[1]).hasMatch()) return {};
 for (const QString& part:pieces[1].split('/')) if(part.isEmpty() || part=="." || part=="..") return {};
 return pieces[0]=="minecraft" ? pieces[1] : t;
}
}
QByteArray FurnitureSignature(const QString& instance) {
 QByteArray sig;
 for(const QString& filename:furnitureFiles(instance)) {QFileInfo f(filename);sig+=filename.toUtf8()+"|"+QByteArray::number(f.size())+"|"+QByteArray::number(f.lastModified().toMSecsSinceEpoch())+"\n";}
 return sig;
}
void PrepareFurniture(const QString& instance,const QString& cache,QJsonArray& blocks,QSet<QString>& textures,QSet<QString>& sprites,QStringList& report) {
 QSet<QString> seen;
 for(const QString& path:furnitureFiles(instance)) {
  QString hash=QFileInfo(path).baseName();if(seen.contains(hash))continue;seen.insert(hash);
  QFile file(path);if(file.size()>16*1024*1024 || !file.open(QIODevice::ReadOnly)){report<<hash+": could not read";continue;}
  QByteArray bytes=file.readAll();ImmersiveFurniture::Model model;ImmersiveFurniture::BakedModel baked;std::string error;
  if(!ImmersiveFurniture::ReadGzip(bytes.constData(),bytes.size(),model,error)){report<<hash+": "+QString::fromStdString(error);continue;}
  // Scoped workaround: match this Lab Tube's Minecraft appearance without changing other furniture.
  if(hash=="d2e4b086ef1106c7c237df118a3707488329cff1dc3f39d320aafb893e7e33d4")
   ImmersiveFurniture::MakeLabTubeGlassOpaque(model);
  if(!ImmersiveFurniture::Bake(model,baked,error)){report<<hash+": "+QString::fromStdString(error);continue;}
  QString ns="mineimator_immersive", id=ns+":"+hash;
  QString base=cache+"/assets/"+ns;
  bool good=true;QStringList localTextures;
  for(int i=0;i<(int)baked.atlases.size();i++){
   const auto& src=baked.atlases[i];QImage image(128,128,QImage::Format_ARGB32);
   for(int y=0;y<128;y++)for(int x=0;x<128;x++)image.setPixel(x,y,ImmersiveFurniture::NativeAbgrToArgb(src.pixels[y*128+x]));
   QString name="block/"+hash+"_"+QString::number(i), filename=base+"/textures/"+name+".png";
   QDir().mkpath(QFileInfo(filename).absolutePath());if(!image.save(filename,"PNG")){good=false;break;}localTextures<<ns+":"+name;
  }
  if(!good){report<<hash+": cannot save atlas";continue;}
  QSet<QString> localSprites;
  for(int state=0;state<2 && good;state++) {
   QJsonArray elements;QJsonObject textureMap;int n=0;
   for(const auto& e:baked.states[state]) {
    QJsonObject item,faceMap;item["from"]=point(e.source.from);item["to"]=point(e.source.to);
    if(e.source.rotation!=0){QJsonArray origin;for(int j=0;j<3;j++)origin.append((e.source.from[j]+e.source.to[j])/2);QJsonObject rot;rot["origin"]=origin;rot["axis"]=QString(1, QChar(e.source.axis));rot["angle"]=e.source.rotation;item["rotation"]=rot;}
    if(e.source.emission>0)item["light_emission"]=e.source.emission;
    for(const auto& f:e.faces){QString key="t"+QString::number(f.rect.atlas);textureMap[key]=localTextures.value(f.rect.atlas);QJsonObject face;face["texture"]="#"+key;face["mineimator_render_type"]=QString::fromStdString(e.source.transparency);face["uv"]=QJsonArray{f.rect.x/8.0,f.rect.y/8.0,(f.rect.x+f.rect.width)/8.0,(f.rect.y+f.rect.height)/8.0};faceMap[QString::fromStdString(f.direction)]=face;}
    if(e.source.kind==ImmersiveFurniture::Element::Kind::Sprite && !e.source.sprite.item && !e.source.sprite.tiled && e.source.sprite.size>0){
     QString tex=textureName(e.source.sprite.texture);QString key="s"+QString::number(n);
     if(tex.isEmpty()) { n++; continue; }
     textureMap[key]=tex;localSprites.insert(tex);
     float w=(e.source.to[0]-e.source.from[0])/e.source.sprite.size,h=(e.source.to[1]-e.source.from[1])/e.source.sprite.size;
     for(int front=0;front<2;front++){bool reverse=bool(front)==e.source.sprite.mirrored;QJsonObject face;face["texture"]="#"+key;face["uv"]=QJsonArray{reverse?w:0.0,0.0,reverse?0.0:w,h};int angle=((e.source.sprite.rotation%360)+360)%360;face["rotation"]=front?angle:(360-angle)%360;
      unsigned c=e.source.color; face["mineimator_color"]=int(((c>>16)&255) | (c&0xff00) | ((c&255)<<16));
      faceMap[front?"north":"south"]=face;}
    }
    if(!faceMap.isEmpty()){item["faces"]=faceMap;elements.append(item);}n++;
   }
   QJsonObject json;json["textures"]=textureMap;json["elements"]=elements;
   good=writeJson(base+"/models/block/"+hash+"_"+QString::number(state)+".json",json);
  }
  if(!good){report<<hash+": cannot save model";continue;}
  QJsonObject variants;
  const char* facing[]={"north","east","south","west"};const int rotations[]={0,90,180,270};
  for(int state=0;state<2;state++)for(int i=0;i<4;i++){
   QJsonObject v;v["model"]=ns+":block/"+hash+"_"+QString::number(state);v["y"]=rotations[i];
   variants[QString("active=%1,facing=%2").arg(state?"true":"false",facing[i])]=v;
  }
  QJsonObject stateFile;stateFile["variants"]=variants;
  if(!writeJson(base+"/blockstates/"+hash+".json",stateFile)){report<<hash+": cannot save blockstate";continue;}
  QJsonObject entry,states;entry["name"]=id;entry["id"]=id;entry["file"]=id;states["active"]=QJsonArray{"false","true"};states["facing"]=QJsonArray{"north","east","south","west"};entry["states"]=states;blocks.append(entry);
  for(const QString& t:localTextures)textures.insert(t);sprites.unite(localSprites);
  report<<hash+": "+QString::fromStdString(model.name)+", missing baked faces: "+QString::number(baked.missingFaces)+", ignored elements: "+QString::number(baked.ignoredElements);
 }
}
}
