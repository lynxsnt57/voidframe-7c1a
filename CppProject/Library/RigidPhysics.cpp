#include "Generated/Scripts.hpp"
#include "RigidCore.hpp"
#include <string>
#include <unordered_map>
namespace CppProject {
namespace {
using namespace Rigid;
struct Entity { IntType id=0,parent=0;int joint=0;V limits{},scale{1,1,1};Body body;Pose pivot{},relative{};std::size_t index=0; };
World rigidWorld;
std::map<IntType,Entity> entities;
std::map<IntType,std::map<IntType,Pose>> cache;
std::string lastError;
bool built=false;
Q rotation(V a,V b,V c){double trace=a.x+b.y+c.z;Q q;
 if(trace>0){double s=std::sqrt(trace+1)*2;q={.25*s,(b.z-c.y)/s,(c.x-a.z)/s,(a.y-b.x)/s};}
 else if(a.x>b.y&&a.x>c.z){double s=std::sqrt(1+a.x-b.y-c.z)*2;q={(b.z-c.y)/s,.25*s,(b.x+a.y)/s,(c.x+a.z)/s};}
 else if(b.y>c.z){double s=std::sqrt(1+b.y-a.x-c.z)*2;q={(c.x-a.z)/s,(b.x+a.y)/s,.25*s,(c.y+b.z)/s};}
 else{double s=std::sqrt(1+c.z-a.x-b.y)*2;q={(a.y-b.x)/s,(c.x+a.z)/s,(c.y+b.z)/s,.25*s};}return q.normalized();
}
Pose decode(MatrixType matrix,V& scale){auto& m=matrix.matrix.m;V a{m[0],m[1],m[2]},b{m[4],m[5],m[6]},c{m[8],m[9],m[10]};scale={length(a),length(b),length(c)};
 if(std::min({scale.x,scale.y,scale.z})<1e-6||!finite(scale))throw std::runtime_error("Zero/non-finite scale is not supported.");
 a=a/scale.x;b=b/scale.y;c=c/scale.z;
 if(std::abs(dot(a,b))>.001||std::abs(dot(a,c))>.001||std::abs(dot(b,c))>.001||dot(cross(a,b),c)<.99)throw std::runtime_error("Sheared or mirrored transforms are not supported. Apply/fix the model scale first.");
 Pose p{{m[12],m[13],m[14]},rotation(a,b,c)};if(!finite(p.p))throw std::runtime_error("Invalid object position.");return p;
}
MatrixType encode(Pose p,V scale,MatrixType original){auto out=original.matrix;auto a=axes(p.q);double scales[3]={scale.x,scale.y,scale.z};for(int i=0;i<3;i++){out.m[i*4]=a[i].x*scales[i];out.m[i*4+1]=a[i].y*scales[i];out.m[i*4+2]=a[i].z*scales[i];}out.m[12]=p.p.x;out.m[13]=p.p.y;out.m[14]=p.p.z;return MatrixType(out);}
Pose entityPose(const Entity& e){return compose(rigidWorld.bodies[e.index].pose,e.relative);}
IntType rootOf(IntType id){IntType root=id;for(std::size_t n=0;n<=entities.size();n++){auto& e=entities.at(root);if(e.joint!=3||e.parent==root||!entities.count(e.parent))return root;root=e.parent;}throw std::runtime_error("Cyclic weld links. Choose an unlinked root.");}
}
void rigid_clear(){rigidWorld=Rigid::World{};entities.clear();cache.clear();lastError.clear();built=false;}
StringType rigid_error(){return StringType(lastError.c_str());}
RealType rigid_count(){return double(entities.size());}
RealType rigid_add(IntType id,MatrixType base,ArrType params){try{
 if(built||entities.size()>=128||params.Size()!=26)throw std::runtime_error("Invalid body registration or more than 128 bodies.");
 auto get=[&](int i){double v=params.Value(i).ToReal();if(!std::isfinite(v))throw std::runtime_error("Non-finite modifier value.");return v;};Entity e;e.id=id;e.pivot=decode(base,e.scale);e.body.id=id;e.body.mode=std::clamp(int(get(0)),0,2);e.body.mass=safe(get(2),.001,1e6,1);e.body.friction=get(3);e.body.bounce=get(4);e.body.linearDrag=get(5);e.body.angularDrag=get(6);
 V offset{get(10)*e.scale.x,get(11)*e.scale.y,get(12)*e.scale.z};e.body.pose=compose(e.pivot,{offset,{}});e.body.v={get(17),get(18),get(19)};e.body.w=V{get(20),get(21),get(22)}*(Pi/180);e.body.layer=1u<<std::clamp(int(get(24)),0,7);e.body.mask=unsigned(std::clamp(int(get(25)),0,255));
 Shape shape;shape.kind=get(1)>=.5?1:0;shape.half={std::max(.001,get(7)*e.scale.x*.5),std::max(.001,get(8)*e.scale.y*.5),std::max(.001,get(9)*e.scale.z*.5)};if(shape.kind==1)shape.half={std::max({shape.half.x,shape.half.y,shape.half.z}),0,0};e.body.shapes.push_back(shape);e.joint=std::clamp(int(get(13)),0,3);e.limits={safe(get(14),0,175,60)*Pi/180,safe(get(15),0,175,60)*Pi/180,safe(get(16),0,175,45)*Pi/180};e.parent=IntType(get(23));entities[id]=e;return 1;
 }catch(const std::exception& e){lastError=e.what();return 0;}}
RealType rigid_build(){try{
 if(entities.empty())throw std::runtime_error("No enabled rigid bodies. Enable the modifier on objects and static colliders first.");
 rigidWorld=World{};
 // Scene scale: 16 editor units per Minecraft block (one metre).
 rigidWorld.gravity={0,0,-9.81*16.0};
 std::map<IntType,std::vector<IntType>> groups;for(auto& item:entities)groups[rootOf(item.first)].push_back(item.first);
 for(auto& group:groups){auto& root=entities.at(group.first);Body body=root.body;double total=0;V center{};
  for(IntType id:group.second){auto& e=entities.at(id);if(e.body.mode!=body.mode)throw std::runtime_error("Welded parts must use the same motion type.");total+=e.body.mass;center+=e.body.pose.p*e.body.mass;}
  center=center/total;body.mass=total;body.pose.p=center;body.shapes.clear();V inertia{};
  for(IntType id:group.second){auto& e=entities.at(id);Pose rel=compose(inverse(body.pose),e.body.pose);Shape sh=e.body.shapes[0];sh.local=rel;body.shapes.push_back(sh);e.relative=compose(inverse(body.pose),e.pivot);V h=sh.half;if(sh.kind==1)h={h.x,h.x,h.x};double mass=e.body.mass;V diagonal{mass*(h.y*h.y+h.z*h.z)/3,mass*(h.x*h.x+h.z*h.z)/3,mass*(h.x*h.x+h.y*h.y)/3};if(sh.kind==1)diagonal={.4*mass*h.x*h.x,.4*mass*h.x*h.x,.4*mass*h.x*h.x};auto a=axes(rel.q);V r=rel.p;inertia.x+=a[0].x*a[0].x*diagonal.x+a[1].x*a[1].x*diagonal.y+a[2].x*a[2].x*diagonal.z+mass*(r.y*r.y+r.z*r.z);inertia.y+=a[0].y*a[0].y*diagonal.x+a[1].y*a[1].y*diagonal.y+a[2].y*a[2].y*diagonal.z+mass*(r.x*r.x+r.z*r.z);inertia.z+=a[0].z*a[0].z*diagonal.x+a[1].z*a[1].z*diagonal.y+a[2].z*a[2].z*diagonal.z+mass*(r.x*r.x+r.y*r.y);}
  auto index=rigidWorld.add(body);rigidWorld.bodies[index].inverseInertia={1/std::max(.000001,inertia.x),1/std::max(.000001,inertia.y),1/std::max(.000001,inertia.z)};for(IntType id:group.second)entities.at(id).index=index;
 }
 for(auto& pair:entities){auto& e=pair.second;if(e.joint>0&&e.joint<3&&entities.count(e.parent)){auto& p=entities.at(e.parent);rigidWorld.joint(p.index,e.index,e.pivot.p,e.joint,e.limits);}}
 built=true;return 1;
 }catch(const std::exception& e){lastError=e.what();built=false;return 0;}}
void rigid_target(IntType id,MatrixType target,RealType active){try{if(!built||!entities.count(id))return;auto& e=entities.at(id);if(rootOf(id)!=id)return;V scale;Pose p=decode(target,scale);if(length(scale-e.scale)>.001)throw std::runtime_error("Animated scale is not supported. Apply/fix scale before simulation.");auto& body=rigidWorld.bodies[e.index];body.target=compose(p,inverse(e.relative));body.enabled=active!=0;}catch(const std::exception& e){lastError=e.what();}}
RealType rigid_step(RealType seconds){try{if(!built)throw std::runtime_error("Simulation is not initialized.");rigidWorld.advance(seconds);return 1;}catch(const std::exception& e){lastError=e.what();return 0;}}
RealType rigid_ground(RealType enabled,RealType height,RealType friction){try{if(!built)throw std::runtime_error("Simulation is not initialized.");rigidWorld.setGround(enabled!=0,height,friction);return 1;}catch(const std::exception& e){lastError=e.what();return 0;}}
RealType rigid_store(IntType frame){try{if(!built||cache.size()*entities.size()>=100000)throw std::runtime_error("Cache limit reached: 100,000 object/frame samples.");auto& sample=cache[frame];for(auto& e:entities)sample[e.first]=entityPose(e.second);return 1;}catch(const std::exception& e){lastError=e.what();return 0;}}
MatrixType rigid_result(IntType id,MatrixType base,RealType frame){if(!built||!entities.count(id))return base;auto& e=entities.at(id);Pose p=entityPose(e);if(frame>=0){auto lo=cache.find(IntType(std::floor(frame))),hi=cache.find(IntType(std::ceil(frame)));if(lo==cache.end()||hi==cache.end()||!lo->second.count(id)||!hi->second.count(id))return base;p=lo->second.at(id);Pose q=hi->second.at(id);double u=frame-std::floor(frame);p.p=p.p*(1-u)+q.p*u;Q a=p.q,b=q.q;if(a.w*b.w+a.x*b.x+a.y*b.y+a.z*b.z<0)b={-b.w,-b.x,-b.y,-b.z};p.q=Q{a.w*(1-u)+b.w*u,a.x*(1-u)+b.x*u,a.y*(1-u)+b.y*u,a.z*(1-u)+b.z*u}.normalized();}return encode(p,e.scale,base);}
ArrType rigid_debug(IntType id,MatrixType base){ArrType out;try{auto it=entities.find(id);if(it==entities.end()||!built)return out;auto& e=it->second;V scale;Pose pivot=decode(base,scale);Shape shape=e.body.shapes[0];Pose relative=compose(inverse(e.pivot),e.body.pose);Box box{compose(pivot,relative),shape.half,shape.kind};if(box.kind==1)box.h={box.h.x,box.h.x,box.h.x};for(V p:corners(box)){out.Append(p.x);out.Append(p.y);out.Append(p.z);}}catch(const std::exception& ex){lastError=ex.what();}return out;}
// Decompose through the application's own matrix_build convention. A damped
// three-variable solve avoids assuming a foreign Euler order and chooses angles
// near the previous baked sample. Failure returns no channels, never bad keys.
ArrType rigid_euler(MatrixType local,ArrType reference){try{
 V scale;Pose desired=decode(local,scale);V original{};if(reference.Size()==3)original={reference.Value(0).ToReal(),reference.Value(1).ToReal(),reference.Value(2).ToReal()};
 auto residual=[&](V e){V dummy;Pose guess=decode(matrix_build(0,0,0,e.x,e.y,e.z,1,1,1),dummy);return angle(desired.q*guess.q.inverse());};
 V best=original;double bestError=1e100;
 for(V seed:{original,V{},V{90,90,90},V{-90,90,0}}){V e=seed;
  for(int it=0;it<90;it++){V r=residual(e);double error=length(r);if(error<bestError){bestError=error;best=e;}if(error<1e-8)break;
   const double eps=.01;V j[3];for(int col=0;col<3;col++){V next=e;if(col==0)next.x+=eps;else if(col==1)next.y+=eps;else next.z+=eps;j[col]=(residual(next)-r)/eps;}
   double a[3][4]{};for(int row=0;row<3;row++){for(int col=0;col<3;col++)a[row][col]=dot(j[row],j[col])+(row==col?1e-9:0);a[row][3]=-dot(j[row],r);}
   for(int col=0;col<3;col++){int pivot=col;for(int row=col+1;row<3;row++)if(std::abs(a[row][col])>std::abs(a[pivot][col]))pivot=row;for(int k=col;k<4;k++)std::swap(a[col][k],a[pivot][k]);double denom=a[col][col];if(std::abs(denom)<1e-15)continue;for(int k=col;k<4;k++)a[col][k]/=denom;for(int row=0;row<3;row++)if(row!=col){double factor=a[row][col];for(int k=col;k<4;k++)a[row][k]-=factor*a[col][k];}}
   V change{std::clamp(a[0][3],-45.,45.),std::clamp(a[1][3],-45.,45.),std::clamp(a[2][3],-45.,45.)};if(length(change)<1e-8)break;e+=change;
  }
  if(bestError<1e-8)break;
 }
 if(bestError>.00001)throw std::runtime_error("Cannot decompose local rotation safely for Bake.");
 best.x=original.x+std::remainder(best.x-original.x,360.);best.y=original.y+std::remainder(best.y-original.y,360.);best.z=original.z+std::remainder(best.z-original.z,360.);
 return ArrType::From({best.x,best.y,best.z});
 }catch(const std::exception& ex){lastError=ex.what();return ArrType();}}

}
