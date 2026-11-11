#include "Generated/Scripts.hpp"
#include "SecondaryMotionCore.hpp"
#include <unordered_map>

namespace CppProject {
namespace {
std::unordered_map<IntType,SecondaryMotion::State> secondaryStates;
}
// Render-thread only, keyed by live timeline ID; destroy explicitly releases it.
MatrixType secondary_motion_update(IntType id, MatrixType base, ArrType params, RealType time) {
    if(params.Size()!=16 && params.Size()!=26) return base;
    auto get=[&](int i){return double(params.Value(i).ToReal());};
    SecondaryMotion::Settings s;
    s.enabled=get(0)!=0;s.blend=get(1);s.frequency=get(2);s.damping=get(3);
    s.inertia=get(4);s.gravity=get(5);s.limit=get(6);
    s.anchor={get(7),get(8),get(9)};s.tip={get(10),get(11),get(12)};
    s.wind={get(13),get(14),get(15)};
    if(params.Size()==26){s.mode=get(16)>=.5?1:0;s.stiffness={get(17),get(18),get(19)};s.drag={get(20),get(21),get(22)};s.travel={get(23),get(24),get(25)};}
    if(!s.enabled||!std::isfinite(s.blend)||s.blend<=0){secondaryStates.erase(id);return base;}
    SecondaryMotion::Mat input;
    for(int i=0;i<16;i++)input[i]=base.matrix.m[i];
    auto& state=secondaryStates[id];
    auto result=SecondaryMotion::evaluate(state,input,s,time);
    Matrix out=base.matrix;for(int i=0;i<16;i++)out.m[i]=result[i];
    return MatrixType(out);
}
ArrType secondary_motion_tip(IntType id){
    auto it=secondaryStates.find(id);if(it==secondaryStates.end()||!it->second.valid)return ArrType();
    auto& s=it->second;return ArrType::From({s.displayAnchor.x,s.displayAnchor.y,s.displayAnchor.z,s.displayTip.x,s.displayTip.y,s.displayTip.z});
}
void secondary_motion_grab(IntType id,ArrType target,RealType active){
    auto it=secondaryStates.find(id);if(it==secondaryStates.end())return;
    auto& s=it->second;s.grabbing=active!=0&&target.Size()==3;
    if(s.grabbing){s.grabTarget={target.Value(0).ToReal(),target.Value(1).ToReal(),target.Value(2).ToReal()};if(!SecondaryMotion::finite(s.grabTarget))s.grabbing=false;}
}
void secondary_motion_release(IntType id){secondaryStates.erase(id);}
} // namespace CppProject
