#pragma once
// Secondary motion v1. Pure C++17, no renderer/Qt access; Z is world up.
// This is a constrained tip particle, not cloth, rigid bodies or a hair solver.
#include <algorithm>
#include <array>
#include <cmath>

namespace SecondaryMotion {
constexpr double Pi=3.14159265358979323846;
constexpr double Step=1.0/240.0;
struct Vec {
    double x=0,y=0,z=0;
    Vec operator+(Vec b) const {return {x+b.x,y+b.y,z+b.z};}
    Vec operator-(Vec b) const {return {x-b.x,y-b.y,z-b.z};}
    Vec operator*(double s) const {return {x*s,y*s,z*s};}
    Vec operator/(double s) const {return *this*(1.0/s);}
};
inline double dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec cross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline double length(Vec a){return std::sqrt(dot(a,a));}
inline bool finite(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline Vec unit(Vec a,Vec fallback={0,0,-1}){double n=length(a);return n>1e-10?a/n:fallback;}
inline Vec lerp(Vec a,Vec b,double u){return a*(1-u)+b*u;}
inline double safe(double v,double lo,double hi,double fallback=0){return std::isfinite(v)?std::clamp(v,lo,hi):fallback;}
using Mat=std::array<double,16>;
inline Vec transform(const Mat& m,Vec p){return {m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};}
inline bool finite(const Mat& m){for(double x:m)if(!std::isfinite(x))return false;return true;}
struct Settings {
    bool enabled=false;
    double blend=1,frequency=2.5,damping=.7,inertia=1,gravity=32,limit=60;
    Vec anchor{},tip{0,0,-8},wind{};
    int mode=0; // 0 legacy constrained swing, 1 local-axis spring translation
    Vec stiffness{1,1,1},drag{1,1,1},travel{.5,.5,.5};
    std::array<Vec,3> basis{{{1,0,0},{0,1,0},{0,0,1}}};
    bool grabbing=false; Vec grabTarget{};
    void sanitize(){
        mode=mode==1?1:0;
        stiffness={safe(stiffness.x,.1,4,1),safe(stiffness.y,.1,4,1),safe(stiffness.z,.1,4,1)};
        drag={safe(drag.x,.1,4,1),safe(drag.y,.1,4,1),safe(drag.z,.1,4,1)};
        travel={safe(travel.x,.001,100,.5),safe(travel.y,.001,100,.5),safe(travel.z,.001,100,.5)};
        blend=safe(blend,0,1);frequency=safe(frequency,.1,12,2.5);
        damping=safe(damping,0,3,.7);inertia=safe(inertia,0,2,1);
        gravity=safe(gravity,-1000,1000,32);limit=safe(limit,0,175,60);
        anchor={safe(anchor.x,-10000,10000),safe(anchor.y,-10000,10000),safe(anchor.z,-10000,10000)};
        tip={safe(tip.x,-10000,10000),safe(tip.y,-10000,10000),safe(tip.z,-10000,10000)};
        wind={safe(wind.x,-1000,1000),safe(wind.y,-1000,1000),safe(wind.z,-1000,1000)};
    }
};
inline Vec rotate(Vec p,Vec axis,double angle){double c=std::cos(angle),s=std::sin(angle);return p*c+cross(axis,p)*s+axis*(dot(axis,p)*(1-c));}
inline Vec limited(Vec direction,Vec rest,double maxAngle){
    direction=unit(direction,rest);rest=unit(rest);
    double c=std::clamp(dot(direction,rest),-1.0,1.0);
    if(c>=std::cos(maxAngle))return direction;
    Vec tangent=direction-rest*c;
    if(length(tangent)<1e-8)tangent=cross(rest,std::abs(rest.z)<.9?Vec{0,0,1}:Vec{0,1,0});
    return rest*std::cos(maxAngle)+unit(tangent)*std::sin(maxAngle);
}
struct Particle {Vec p{},v{},anchor{};double time=0;};
struct State {
    bool valid=false;
    double sampleTime=0;
    Particle grid{},beforeGrid{};
    Vec sampleAnchor{},sampleTip{},beforeAnchor{},beforeTip{};
    double beforeTime=0;
    Settings geometry{};
    bool grabbing=false;Vec grabTarget{},displayTip{},displayAnchor{};
    void clear(){valid=false;}
    void reset(double time,Vec anchor,Vec tip,const Settings& s){
        valid=true;sampleTime=beforeTime=time;grid={tip,{},anchor,time};beforeGrid=grid;displayTip=tip;displayAnchor=anchor;
        sampleAnchor=beforeAnchor=anchor;sampleTip=beforeTip=tip;geometry=s;
    }
};
inline void integrate(Particle& p,Vec anchor,Vec restTip,const Settings& s,double dt){
    const double len=length(restTip-anchor);if(len<1e-8)return;
    Vec old=p.p+(anchor-p.anchor)*(1-s.inertia);p.p=old;
    double omega=2*Pi*s.frequency;
    Vec acceleration=(restTip-p.p)*(omega*omega)+s.wind+Vec{0,0,-s.gravity};
    if(s.grabbing && finite(s.grabTarget)) acceleration=acceleration+(s.grabTarget-p.p)*(omega*omega*4);
    if(s.mode==1){
        Vec error=restTip-p.p,force=s.wind+Vec{0,0,-s.gravity},velocity{};
        const double k[3]={s.stiffness.x,s.stiffness.y,s.stiffness.z};
        const double damp[3]={s.drag.x,s.drag.y,s.drag.z};
        const double limits[3]={s.travel.x,s.travel.y,s.travel.z};
        for(int axis=0;axis<3;axis++){
            Vec b=s.basis[axis];double w=omega*std::sqrt(k[axis]);
            double a=dot(error,b)*w*w+dot(force,b);
            if(s.grabbing&&finite(s.grabTarget))a+=dot(s.grabTarget-p.p,b)*omega*omega*4;
            double v=(dot(p.v,b)+a*dt)/(1+2*s.damping*damp[axis]*w*dt);
            velocity=velocity+b*v;
        }
        Vec proposed=p.p+velocity*dt,delta=proposed-restTip;double radius2=0;
        for(int axis=0;axis<3;axis++){double q=dot(delta,s.basis[axis])/limits[axis];radius2+=q*q;}
        if(radius2>1){
            delta=delta/std::sqrt(radius2);proposed=restTip+delta;
            Vec normal{};for(int axis=0;axis<3;axis++)normal=normal+s.basis[axis]*(dot(delta,s.basis[axis])/(limits[axis]*limits[axis]));
            normal=unit(normal);double outward=dot(velocity,normal);if(outward>0)velocity=velocity-normal*outward;
        }
        p.p=proposed;p.v=velocity;p.anchor=anchor;p.time+=dt;return;
    }
    p.v=(p.v+acceleration*dt)/(1+2*s.damping*omega*dt);
    Vec d=limited(p.p+p.v*dt-anchor,unit(restTip-anchor),s.limit*Pi/180);
    p.p=anchor+d*len;
    p.v=(p.p-old)/dt;
    // Drop radial speed rather than feeding constraint correction back as energy.
    p.v=p.v-d*dot(p.v,d);p.anchor=anchor;p.time+=dt;
}
inline Mat applyRotation(const Mat& base,Vec anchor,Vec restTip,Vec simulatedTip,double blend,double maxAngle){
    Vec from=unit(restTip-anchor),to=limited(simulatedTip-anchor,from,maxAngle);
    double angle=std::acos(std::clamp(dot(from,to),-1.0,1.0))*blend;
    if(angle<1e-10)return base;
    Vec axis=unit(cross(from,to));Mat out=base;
    for(int col=0;col<3;col++){
        Vec v=rotate({base[col*4],base[col*4+1],base[col*4+2]},axis,angle);
        out[col*4]=v.x;out[col*4+1]=v.y;out[col*4+2]=v.z;
    }
    Vec translation=anchor+rotate(Vec{base[12],base[13],base[14]}-anchor,axis,angle);
    out[12]=translation.x;out[13]=translation.y;out[14]=translation.z;return out;
}
inline Mat evaluate(State& st,const Mat& base,Settings s,double time){
    s.sanitize();
    for(int axis=0;axis<3;axis++)s.basis[axis]=unit({base[axis*4],base[axis*4+1],base[axis*4+2]},axis==0?Vec{1,0,0}:(axis==1?Vec{0,1,0}:Vec{0,0,1}));
    s.grabbing=st.grabbing;s.grabTarget=st.grabTarget;
    if(!s.enabled||s.blend<=0||!finite(base)||!std::isfinite(time)){st.clear();return base;}
    Vec anchor=transform(base,s.anchor),restTip=transform(base,s.anchor+s.tip);
    double len=length(restTip-anchor);
    if(len<1e-6||len>1e6||!finite(anchor)||!finite(restTip)){st.clear();return base;}
    bool changed=st.geometry.mode!=s.mode||length(st.geometry.anchor-s.anchor)>1e-7||length(st.geometry.tip-s.tip)>1e-7;
    if(!st.valid||changed||time<st.sampleTime-1e-8||time-st.sampleTime>.5||
       length(anchor-st.sampleAnchor)>std::max(64.0,len*8)){
        st.reset(time,anchor,restTip,s);return base;
    }
    // Re-entering the matrix pass (IK, inherit pose, copy transforms, F5) at
    // the same timeline instant replays the SAME interval, never an extra step.
    if(std::abs(time-st.sampleTime)<1e-8){
        st.grid=st.beforeGrid;st.sampleTime=st.beforeTime;
        st.sampleAnchor=st.beforeAnchor;st.sampleTip=st.beforeTip;
    }else{
        st.beforeGrid=st.grid;st.beforeTime=st.sampleTime;
        st.beforeAnchor=st.sampleAnchor;st.beforeTip=st.sampleTip;
    }
    const double interval=time-st.sampleTime;
    const Vec oldAnchor=st.sampleAnchor,oldTip=st.sampleTip;
    auto target=[&](double at,bool tip){
        double u=interval>1e-10?std::clamp((at-st.sampleTime)/interval,0.0,1.0):1;
        return lerp(tip?oldTip:oldAnchor,tip?restTip:anchor,u);
    };
    // Full 240 Hz ticks are committed. Fractional preview is NOT committed,
    // so renderer passes and varying output sample rates don't add time steps.
    int steps=0;
    while(st.grid.time+Step<=time+1e-10&&steps++<122){
        double at=st.grid.time+Step;integrate(st.grid,target(at,false),target(at,true),s,Step);
    }
    Particle preview=st.grid;double fraction=time-preview.time;
    if(fraction>1e-10)integrate(preview,anchor,restTip,s,fraction);
    else if(fraction<=1e-10){
        if(s.mode==0){Vec dir=limited(preview.p-anchor,unit(restTip-anchor),s.limit*Pi/180);preview.p=anchor+dir*len;}
    }
    st.sampleTime=time;st.sampleAnchor=anchor;st.sampleTip=restTip;
    if(!finite(preview.p)||!finite(preview.v)){st.reset(time,anchor,restTip,s);return base;}
    st.displayTip=preview.p;st.displayAnchor=anchor;
    if(s.mode==1){Mat out=base;Vec shift=(preview.p-restTip)*s.blend;out[12]+=shift.x;out[13]+=shift.y;out[14]+=shift.z;return out;}
    return applyRotation(base,anchor,restTip,preview.p,s.blend,s.limit*Pi/180);
}
} // namespace SecondaryMotion
