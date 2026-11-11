#pragma once
// Experimental, self-contained rigid-body solver. Z-up; double precision.
// Oriented boxes and spheres; sequential impulses; limited parent joints.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>
namespace Rigid {
constexpr double Pi=3.14159265358979323846;
struct V {
 double x=0,y=0,z=0;
 V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};} V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};}
 V operator-()const{return{-x,-y,-z};} V operator*(double s)const{return{x*s,y*s,z*s};} V operator/(double s)const{return *this*(1/s);}
 V& operator+=(V b){*this=*this+b;return *this;} V& operator-=(V b){*this=*this-b;return *this;}
};
inline double dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V cross(V a,V b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(V a){return std::sqrt(dot(a,a));}
inline bool finite(V a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline V unit(V a,V fallback={1,0,0}){double l=length(a);return l>1e-12?a/l:fallback;}
inline V cap(V a,double max){double l=length(a);return l>max?a*(max/l):a;}
inline double safe(double x,double lo,double hi,double fallback){return std::isfinite(x)?std::clamp(x,lo,hi):fallback;}
struct Q {
 double w=1,x=0,y=0,z=0;
 Q operator*(Q b)const{return{w*b.w-x*b.x-y*b.y-z*b.z,w*b.x+x*b.w+y*b.z-z*b.y,w*b.y-x*b.z+y*b.w+z*b.x,w*b.z+x*b.y-y*b.x+z*b.w};}
 Q inverse()const{return{w,-x,-y,-z};}
 Q normalized()const{double n=std::sqrt(w*w+x*x+y*y+z*z);return n>1e-12?Q{w/n,x/n,y/n,z/n}:Q{};}
 V rotate(V v)const{V u{x,y,z};return v+cross(u,v)*(2*w)+cross(u,cross(u,v))*2;}
};
inline bool finite(Q q){return std::isfinite(q.w)&&std::isfinite(q.x)&&std::isfinite(q.y)&&std::isfinite(q.z);}
inline Q turn(V a){double l=length(a);if(l<1e-12)return{};double s=std::sin(l/2)/l;return{std::cos(l/2),a.x*s,a.y*s,a.z*s};}
inline V angle(Q q){q=q.normalized();if(q.w<0)q={-q.w,-q.x,-q.y,-q.z};V v{q.x,q.y,q.z};double l=length(v);return l<1e-12?V{}:v*(2*std::atan2(l,q.w)/l);}
struct Pose{V p{};Q q{};};
inline Pose compose(Pose a,Pose b){return{a.p+a.q.rotate(b.p),(a.q*b.q).normalized()};}
inline Pose inverse(Pose a){Q q=a.q.inverse();return{q.rotate(-a.p),q};}
struct Shape{int kind=0;V half{.5,.5,.5};Pose local{};}; // 0 box, 1 sphere (half.x radius)
struct Body{
 std::int64_t id=0;int mode=0; // 0 dynamic, 1 static, 2 kinematic
 Pose pose{},previous{},target{};V v{},w{},inverseInertia{};double mass=1,invMass=1,friction=.6,bounce=.05,linearDrag=.05,angularDrag=.1;
 bool enabled=true;unsigned mask=255,layer=1;std::vector<Shape> shapes;
 V inertia(V torque)const{if(mode!=0||!enabled)return{};V local=pose.q.inverse().rotate(torque);return pose.q.rotate({local.x*inverseInertia.x,local.y*inverseInertia.y,local.z*inverseInertia.z});}
 double inverseMass()const{return mode==0&&enabled?invMass:0;}
 void impulse(V j,V r){if(inverseMass()==0)return;v+=j*invMass;w+=inertia(cross(r,j));}
};
struct Joint{std::size_t a=0,b=0;V anchorA{},anchorB{};Q rest{};V limit{Pi/3,Pi/3,Pi/4};int kind=1;}; // 1 ball/limited, 2 hinge X, 3 fixed
struct Contact{std::size_t a=0,b=0;V point{},normal{};double depth=0,normalImpulse=0;V frictionImpulse{};double bounce=0;};
struct Box{Pose pose;V h;int kind;};
inline std::array<V,3> axes(Q q){return{{q.rotate({1,0,0}),q.rotate({0,1,0}),q.rotate({0,0,1})}};}
inline double radius(Box b,V n){if(b.kind==1)return b.h.x;auto a=axes(b.pose.q);return std::abs(dot(n,a[0]))*b.h.x+std::abs(dot(n,a[1]))*b.h.y+std::abs(dot(n,a[2]))*b.h.z;}
inline V support(Box b,V n){if(b.kind==1)return b.pose.p+unit(n)*b.h.x;auto a=axes(b.pose.q);return b.pose.p+a[0]*(dot(n,a[0])>=0?b.h.x:-b.h.x)+a[1]*(dot(n,a[1])>=0?b.h.y:-b.h.y)+a[2]*(dot(n,a[2])>=0?b.h.z:-b.h.z);}
inline bool inside(Box b,V p,double eps=.002){V l=b.pose.q.inverse().rotate(p-b.pose.p);return std::abs(l.x)<=b.h.x+eps&&std::abs(l.y)<=b.h.y+eps&&std::abs(l.z)<=b.h.z+eps;}
inline std::vector<V> corners(Box b){std::vector<V> v;for(int i=0;i<8;i++)v.push_back(b.pose.p+b.pose.q.rotate({(i&1?1:-1)*b.h.x,(i&2?1:-1)*b.h.y,(i&4?1:-1)*b.h.z}));return v;}
inline bool collision(Box a,Box b,V& n,double& depth,std::vector<V>& points){
 V d=b.pose.p-a.pose.p;depth=1e100;
 if(a.kind==1&&b.kind==1){double l=length(d);depth=a.h.x+b.h.x-l;if(depth<=0)return false;n=unit(d,{0,0,1});points.push_back(a.pose.p+n*(a.h.x-depth*.5));return true;}
 if(a.kind==1||b.kind==1){bool flip=a.kind!=1;Box sphere=flip?b:a,box=flip?a:b;V p=box.pose.q.inverse().rotate(sphere.pose.p-box.pose.p);V nearest{std::clamp(p.x,-box.h.x,box.h.x),std::clamp(p.y,-box.h.y,box.h.y),std::clamp(p.z,-box.h.z,box.h.z)};V delta=nearest-p;double l=length(delta);V local;
  if(l>1e-10){depth=sphere.h.x-l;if(depth<=0)return false;local=delta/l;}else{double dx=box.h.x-std::abs(p.x),dy=box.h.y-std::abs(p.y),dz=box.h.z-std::abs(p.z);if(dx<=dy&&dx<=dz){local={p.x>=0?-1.:1.,0,0};depth=sphere.h.x+dx;}else if(dy<=dz){local={0,p.y>=0?-1.:1.,0};depth=sphere.h.x+dy;}else{local={0,0,p.z>=0?-1.:1.};depth=sphere.h.x+dz;}}
  n=box.pose.q.rotate(local);V point=sphere.pose.p+n*(sphere.h.x-depth*.5);if(flip)n=-n;points.push_back(point);return true;
 }
 auto aa=axes(a.pose.q),bb=axes(b.pose.q);std::vector<V> directions(aa.begin(),aa.end());directions.insert(directions.end(),bb.begin(),bb.end());for(V x:aa)for(V y:bb)if(length(cross(x,y))>1e-7)directions.push_back(unit(cross(x,y)));
 for(V axis:directions){double overlap=radius(a,axis)+radius(b,axis)-std::abs(dot(d,axis));if(overlap<=0)return false;if(overlap<depth){depth=overlap;n=dot(d,axis)>=0?axis:-axis;}}
 double faceA=dot(support(a,n),n),faceB=dot(support(b,-n),n);
 for(V v:corners(a))if(inside(b,v)&&dot(v,n)>faceA-depth-.003)points.push_back(v-n*(depth*.5));
 for(V v:corners(b))if(inside(a,v)&&dot(v,n)<faceB+depth+.003)points.push_back(v+n*(depth*.5));
 if(points.empty())points.push_back((support(a,n)+support(b,-n))*.5);
 if(points.size()>4){std::vector<V> compact;for(std::size_t i=0;i<4;i++)compact.push_back(points[i*points.size()/4]);points=compact;}return true;
}
class World{
 public: std::vector<Body> bodies;std::vector<Joint> joints;V gravity{0,0,-32};int iterations=12;double step=1./240.;std::size_t maxBodies=128;double elapsed=0;
 std::size_t groundIndex=std::size_t(-1);bool groundEnabled=false;double groundHeight=-.01;
 void setGround(bool enabled,double height,double friction=.8){if(!std::isfinite(height))throw std::runtime_error("Invalid ground height.");groundEnabled=enabled;groundHeight=height;if(groundIndex==std::size_t(-1)){if(!enabled)return;maxBodies=std::max(maxBodies,bodies.size()+1);Body b;b.mode=1;b.shapes.push_back({});b.shapes[0].kind=2;groundIndex=add(b);}auto& b=bodies[groundIndex];b.enabled=enabled;b.pose.p=b.target.p={0,0,height};b.friction=safe(friction,0,2,.8);b.bounce=0;}
 std::size_t add(Body b){if(bodies.size()>=maxBodies)throw std::runtime_error("Body limit reached (128). Reduce the simulation selection.");b.mass=safe(b.mass,.001,1e6,1);b.invMass=1/b.mass;b.friction=safe(b.friction,0,2,.6);b.bounce=safe(b.bounce,0,1,.05);b.linearDrag=safe(b.linearDrag,0,10,.05);b.angularDrag=safe(b.angularDrag,0,10,.1);if(!finite(b.pose.p)||!finite(b.pose.q))throw std::runtime_error("Non-finite body pose");if(b.shapes.empty())b.shapes.push_back({});V h=b.shapes[0].half;if(b.shapes[0].kind==1)h={h.x,h.x,h.x};double m=b.mass/3; b.inverseInertia={1/std::max(.00001,m*(h.y*h.y+h.z*h.z)),1/std::max(.00001,m*(h.x*h.x+h.z*h.z)),1/std::max(.00001,m*(h.x*h.x+h.y*h.y))};b.previous=b.target=b.pose;bodies.push_back(b);return bodies.size()-1;}
 void joint(std::size_t a,std::size_t b,V anchor,int kind,V limits){if(a==b||a>=bodies.size()||b>=bodies.size())return;auto& A=bodies[a];auto& B=bodies[b];joints.push_back({a,b,A.pose.q.inverse().rotate(anchor-A.pose.p),B.pose.q.inverse().rotate(anchor-B.pose.p),(A.pose.q.inverse()*B.pose.q).normalized(),limits,kind});}
 bool linked(std::size_t a,std::size_t b)const{for(auto& j:joints)if((j.a==a&&j.b==b)||(j.a==b&&j.b==a))return true;return false;}
 std::vector<Contact> contacts(){std::vector<Contact> out;
  for(std::size_t i=0;i<bodies.size();i++)for(std::size_t j=i+1;j<bodies.size();j++){auto& A=bodies[i];auto& B=bodies[j];if(i==groundIndex||j==groundIndex||!A.enabled||!B.enabled||(A.inverseMass()==0&&B.inverseMass()==0)||!(A.mask&B.layer)||!(B.mask&A.layer)||linked(i,j))continue;
   for(auto& sa:A.shapes)for(auto& sb:B.shapes){Box a{compose(A.pose,sa.local),sa.half,sa.kind},b{compose(B.pose,sb.local),sb.half,sb.kind};V delta=b.pose.p-a.pose.p;bool separated=false;for(V axis:{V{1,0,0},V{0,1,0},V{0,0,1}})if(std::abs(dot(delta,axis))>radius(a,axis)+radius(b,axis)){separated=true;break;}if(separated)continue;
    V n;double depth;std::vector<V> points;if(collision(a,b,n,depth,points))for(V p:points){double speed=dot(B.v+cross(B.w,p-B.pose.p)-A.v-cross(A.w,p-A.pose.p),n);out.push_back({i,j,p,n,depth,0,{},speed< -1?-std::max(A.bounce,B.bounce)*speed:0});}
   }
  }
  if(groundEnabled&&groundIndex<bodies.size())for(std::size_t i=0;i<bodies.size();i++){
   auto& a=bodies[i];if(i==groundIndex||!a.enabled||a.inverseMass()==0||!(a.mask&1u))continue;
   for(auto& shape:a.shapes){Box box{compose(a.pose,shape.local),shape.half,shape.kind};std::vector<V> points;
    if(box.kind==1){V bottom=box.pose.p-V{0,0,box.h.x};if(bottom.z<groundHeight)points.push_back(bottom);}
    else{for(V v:corners(box))if(v.z<groundHeight)points.push_back(v);std::sort(points.begin(),points.end(),[](V a,V b){return a.z<b.z;});if(points.size()>4)points.resize(4);}
    for(V p:points){double depth=groundHeight-p.z;p.z+=depth*.5;V n{0,0,-1};double speed=dot(-a.v-cross(a.w,p-a.pose.p),n);out.push_back({i,groundIndex,p,n,depth,0,{},speed< -1?-a.bounce*speed:0});}
   }
  }
  return out;
 }
 double effective(Body& a,Body& b,V ra,V rb,V n){return a.inverseMass()+b.inverseMass()+dot(n,cross(a.inertia(cross(ra,n)),ra)+cross(b.inertia(cross(rb,n)),rb));}
 void solve(Contact& c){auto& a=bodies[c.a];auto& b=bodies[c.b];V ra=c.point-a.pose.p,rb=c.point-b.pose.p,n=c.normal;V relative=b.v+cross(b.w,rb)-a.v-cross(a.w,ra);double denom=effective(a,b,ra,rb,n);if(denom<1e-12)return;double j=-(dot(relative,n)-c.bounce)/denom,old=c.normalImpulse;c.normalImpulse=std::max(0.,old+j);V impulse=n*(c.normalImpulse-old);a.impulse(-impulse,ra);b.impulse(impulse,rb);
  relative=b.v+cross(b.w,rb)-a.v-cross(a.w,ra);V tangent=relative-n*dot(relative,n);double speed=length(tangent);if(speed<1e-10)return;tangent=tangent/speed;double td=effective(a,b,ra,rb,tangent);if(td<1e-12)return;V proposed=c.frictionImpulse-tangent*(speed/td);proposed=cap(proposed,std::sqrt(a.friction*b.friction)*c.normalImpulse);V change=proposed-c.frictionImpulse;c.frictionImpulse=proposed;a.impulse(-change,ra);b.impulse(change,rb);
 }
 void solveJoint(Joint& j,double dt){auto& a=bodies[j.a];auto& b=bodies[j.b];double ia=a.inverseMass(),ib=b.inverseMass(),sum=ia+ib;if(sum<=0)return;V pa=a.pose.p+a.pose.q.rotate(j.anchorA),pb=b.pose.p+b.pose.q.rotate(j.anchorB),error=pb-pa;V change=cap(error*.65,2);a.pose.p+=change*(ia/sum);b.pose.p-=change*(ib/sum);
  V relative=b.v-a.v;V remove=relative*.35;a.v+=remove*(ia/sum);b.v-=remove*(ib/sum);
  Q rel=(a.pose.q.inverse()*b.pose.q*j.rest.inverse()).normalized();V angles=angle(rel),limit=j.limit;if(j.kind==3)limit={};if(j.kind==2)limit={j.limit.x,0,0};V target{std::clamp(angles.x,-limit.x,limit.x),std::clamp(angles.y,-limit.y,limit.y),std::clamp(angles.z,-limit.z,limit.z)};V correction=a.pose.q.rotate(cap(angles-target,.3))*.5;
  if(ia>0){a.pose.q=(turn(correction*(ia/sum))*a.pose.q).normalized();}
  if(ib>0){b.pose.q=(turn(correction*(-ib/sum))*b.pose.q).normalized();}
  V wr=b.w-a.w,axis=unit(correction);if(length(correction)>1e-8){V unwanted=axis*dot(wr,axis)*.5;a.w+=unwanted*(ia/sum);b.w-=unwanted*(ib/sum);} (void)dt;
 }
 void tick(double dt){
  for(auto& b:bodies){b.previous=b.pose;if(b.mode==0&&b.enabled){b.v=(b.v+gravity*dt)*std::exp(-b.linearDrag*dt);b.w=b.w*std::exp(-b.angularDrag*dt);b.v=cap(b.v,2000);b.w=cap(b.w,80);b.pose.p+=b.v*dt;b.pose.q=(turn(b.w*dt)*b.pose.q).normalized();}}
  auto cs=contacts();for(int it=0;it<iterations;it++)for(auto& c:cs)solve(c);
  for(int it=0;it<6;it++){for(auto& j:joints)solveJoint(j,dt);auto pos=contacts();for(auto& c:pos){auto& a=bodies[c.a];auto& b=bodies[c.b];double sum=a.inverseMass()+b.inverseMass();if(sum<=0)continue;double correction=std::min(1.,std::max(0.,c.depth-.001)*.12);a.pose.p-=c.normal*(correction*a.inverseMass()/sum);b.pose.p+=c.normal*(correction*b.inverseMass()/sum);}}
  for(auto& b:bodies){if(!finite(b.pose.p)||!finite(b.pose.q)||!finite(b.v)||!finite(b.w))throw std::runtime_error("Simulation became non-finite. Reset and reduce mass ratios or initial speeds.");}
  elapsed+=dt;
 }
 void advance(double dt){if(!std::isfinite(dt)||dt<0||dt>2)throw std::runtime_error("Invalid simulation timestep");int n=std::max(1,int(std::ceil(dt/step)));if(dt==0)return;
  double minExtent=1e100,maxSpeed=0;
  for(auto& b:bodies){for(auto& sh:b.shapes){double extent=sh.kind==1?sh.half.x:std::min({sh.half.x,sh.half.y,sh.half.z});minExtent=std::min(minExtent,extent);}maxSpeed=std::max(maxSpeed,length(b.v));if(b.mode==2||!b.enabled)maxSpeed=std::max(maxSpeed,length(b.target.p-b.pose.p)/dt);}
  if(minExtent<1e99)n=std::max(n,int(std::ceil(maxSpeed*dt/std::max(.0005,minExtent*.5))));
  if(n>4096)throw std::runtime_error("Adaptive substep limit exceeded. Reduce speed or avoid extremely thin colliders.");
  double h=dt/n;
  std::vector<Pose> old;for(auto& b:bodies)old.push_back(b.pose);
  for(int s=1;s<=n;s++){double u=double(s)/n;for(std::size_t i=0;i<bodies.size();i++){auto& b=bodies[i];if(b.mode==2||!b.enabled){Pose prev=b.pose;b.pose.p=old[i].p*(1-u)+b.target.p*u;Q qa=old[i].q,qb=b.target.q;if(qa.w*qb.w+qa.x*qb.x+qa.y*qb.y+qa.z*qb.z<0)qb={-qb.w,-qb.x,-qb.y,-qb.z};b.pose.q=Q{qa.w*(1-u)+qb.w*u,qa.x*(1-u)+qb.x*u,qa.y*(1-u)+qb.y*u,qa.z*(1-u)+qb.z*u}.normalized();b.v=(b.pose.p-prev.p)/h;b.w=angle(b.pose.q*prev.q.inverse())/h;}}tick(h);}
 }
};
}
