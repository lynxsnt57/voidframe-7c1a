#include "Generated/Scripts.hpp"
#include "Asset/VertexBuffer.hpp"
#include "Render/GraphicsApiHandler.hpp"
#include <QDataStream>
#include <QFile>
#include <QMatrix4x4>
#include <QVector3D>
#include <algorithm>
#include <cmath>
#include <vector>
#include <memory>

namespace CppProject {
namespace {
struct Influence { quint32 joint; float weight; QVector3D local; };
struct SkinVertex { QVector3D normal; float u, v; Influence influences[8]; };
struct SkinBuffer final : VertexBuffer {
    std::vector<SkinVertex> source;
    std::vector<QMatrix4x4> last;
    QString meshPath, skinPath;
    size_t joints=0;
    SkinBuffer() { dynamicSkin=true; saveData=true; }
};
bool finite(float a) { return std::isfinite(a); }
bool finite3(const QVector3D& a) {return finite(a.x()) && finite(a.y()) && finite(a.z());}
bool matrixOk(const QMatrix4x4& m) {
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) if(!finite(m(r,c))) return false;
    return true;
}
// Keep world coordinates in double until the owner transform has been removed.
// Casting the world matrices to float first makes rigid movement look like a
// changing bone pose, needlessly rebuilding the whole skinned mesh.
bool inversePrecise(const Matrix& input, Matrix& result) {
    double a[4][8];
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) {
        a[r][c]=input.m[c*4+r]; a[r][4+c]=(r==c ? 1.0 : 0.0);
        if(!std::isfinite(a[r][c])) return false;
    }
    for(int c=0;c<4;c++) {
        int pivot=c;
        for(int r=c+1;r<4;r++) if(std::abs(a[r][c])>std::abs(a[pivot][c])) pivot=r;
        if(!std::isfinite(a[pivot][c]) || a[pivot][c]==0.0) return false;
        for(int k=0;k<8;k++) std::swap(a[c][k],a[pivot][k]);
        double divisor=a[c][c];
        for(int k=0;k<8;k++) a[c][k]/=divisor;
        for(int r=0;r<4;r++) if(r!=c) {
            double factor=a[r][c];
            for(int k=0;k<8;k++) a[r][k]-=factor*a[c][k];
        }
    }
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) {
        double value=a[r][4+c];
        if(!std::isfinite(value)) return false;
        result.m[c*4+r]=value;
    }
    return true;
}
bool relativePrecise(const Matrix& inverse, const Matrix& joint, QMatrix4x4& out) {
    // Matrix::operator* uses the reverse of the usual column-vector order.
    Matrix relative=joint*inverse;
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) {
        double value=relative.m[c*4+r];
        if(!std::isfinite(value)) return false;
        // Discard numerical noise around zero, not ordinary pose changes.
        if(std::abs(value)<1e-12) value=0.0;
        out(r,c)=float(value);
    }
    return matrixOk(out);
}
QVector3D normalMap(const QMatrix3x3& m, const QVector3D& n) {
    return {m(0,0)*n.x()+m(0,1)*n.y()+m(0,2)*n.z(),m(1,0)*n.x()+m(1,1)*n.y()+m(1,2)*n.z(),m(2,0)*n.x()+m(2,1)*n.y()+m(2,2)*n.z()};
}
std::unique_ptr<SkinBuffer> loadSkin(const QString& meshPath,const QString& skinPath) {
    QFile mf(meshPath),sf(skinPath);
    if(!mf.open(QIODevice::ReadOnly) || !sf.open(QIODevice::ReadOnly) || mf.size()<12 || sf.size()<16) return {};
    QDataStream m(&mf), s(&sf);m.setByteOrder(QDataStream::LittleEndian);s.setByteOrder(QDataStream::LittleEndian);m.setFloatingPointPrecision(QDataStream::SinglePrecision);s.setFloatingPointPrecision(QDataStream::SinglePrecision);
    quint32 mm,mv,mc,sm,sv,sc,jc;m>>mm>>mv>>mc;s>>sm>>sv>>sc>>jc;
    if(mm!=0x3148534d || mv!=1 || sm!=0x314e4b53 || sv!=1 || !mc || mc%3 || mc>1000000 || sc!=mc || !jc || jc>256 || mf.size()!=12+qint64(mc)*32 || sf.size()!=16+qint64(mc)*160) return {};
    std::unique_ptr<SkinBuffer> result(new SkinBuffer);result->source.resize(mc);result->joints=jc;result->meshPath=meshPath;result->skinPath=skinPath;
    for(quint32 i=0;i<mc;i++) {
        float x,y,z,nx,ny,nz,u,v;m>>x>>y>>z>>nx>>ny>>nz>>u>>v;
        SkinVertex& out=result->source[i];out.normal={nx,ny,nz};out.u=u;out.v=v;
        if(!finite(x)||!finite(y)||!finite(z)||!finite3(out.normal)||!finite(u)||!finite(v)) return {};
        float sum=0;
        for(auto& inf:out.influences) {float px,py,pz;s>>inf.joint>>inf.weight>>px>>py>>pz;inf.local={px,py,pz};
            if(inf.joint>=jc || !finite(inf.weight) || inf.weight<0 || inf.weight>1 || !finite3(inf.local)) return {};
            sum+=inf.weight;
        }
        if(!finite(sum)||std::abs(sum-1.f)>0.001f) return {};
    }
    if(m.status()!=QDataStream::Ok || s.status()!=QDataStream::Ok) return {};
    return result;
}
bool deformOne(const SkinVertex& src,const std::vector<QMatrix4x4>& transforms,
               const std::vector<QMatrix3x3>& normalTransforms,QVector3D& p,QVector3D& n) {
    p=QVector3D(); n=QVector3D();
    for(const auto& inf:src.influences) if(inf.weight>0) {
        p+=transforms[inf.joint].map(inf.local)*inf.weight;
        n+=normalMap(normalTransforms[inf.joint],src.normal)*inf.weight;
    }
    if(!finite3(p)||!finite3(n)) return false;
    if(n.lengthSquared()>1e-20f) n.normalize(); else n=src.normal;
    return true;
}
bool buildSkinVertices(const std::vector<SkinVertex>& source, const std::vector<QMatrix4x4>& transforms,
                       const std::vector<QMatrix3x3>& normalTransforms, std::vector<Vertex>& verts) {
    // Vertices are independent. Use a small bounded worker team for large
    // meshes; graphics operations and buffer uploads stay on the render thread.
    const IntType count=IntType(source.size());
    verts.resize(static_cast<size_t>(count));
    int invalid=0;
    const int workers=std::min(4,omp_get_max_threads());
    #pragma omp parallel for schedule(static) if(count>=30000 && workers>1 && !omp_in_parallel()) num_threads(workers) reduction(|:invalid)
    for(IntType i=0;i<count;i++) {
        const SkinVertex& src=source[size_t(i)];
        QVector3D p,n;
        if(!deformOne(src,transforms,normalTransforms,p,n)) { invalid=1; continue; }
        verts[size_t(i)]=Vertex(p.x(),p.y(),p.z(),n.x(),n.y(),n.z(),0xffffff,1.0,src.u,src.v,false,false,0,false);
    }
    if(invalid) return false;
    return true;
}
bool updateSkin(SkinBuffer* b,const std::vector<QMatrix4x4>& transforms) {
    if(b->last==transforms && !b->meshes.empty()) return true;
    std::vector<QMatrix3x3> normalTransforms;normalTransforms.reserve(transforms.size());
    for(const auto& m:transforms) normalTransforms.push_back(m.normalMatrix());
    std::vector<Vertex> verts;
    if(!buildSkinVertices(b->source,transforms,normalTransforms,verts)) return false;
    GFX->SubmitBatch();
    if(b->meshes.empty()) {
        for(size_t i=0;i<verts.size();i+=3) b->AddTriangle(verts[i],verts[i+1],verts[i+2]);
        b->Complete();
    } else {
        size_t at=0;
        for(Mesh<>* mesh:b->meshes) {
            if(at+size_t(mesh->numVertices)>verts.size() || mesh->vertexData.Size()!=mesh->numVertices) return false;
            for(IntType i=0;i<mesh->numVertices;i++) mesh->vertexData[i]=verts[at++];
            for(IntType i=0;i<mesh->numIndices;i+=3) {
                Vertex& a=mesh->vertexData[mesh->indexData.Value(i)]; Vertex& c=mesh->vertexData[mesh->indexData.Value(i+1)]; Vertex& d=mesh->vertexData[mesh->indexData.Value(i+2)];
                QVector3D e1(c.x-a.x,c.y-a.y,c.z-a.z),e2(d.x-a.x,d.y-a.y,d.z-a.z);float du1=c.u-a.u,dv1=c.v-a.v,du2=d.u-a.u,dv2=d.v-a.v,det=du1*dv2-dv1*du2;
                QVector3D t=std::abs(det)>1e-12f ? (e1*dv2-e2*dv1)/det : e1;
                if(!finite3(t)||t.lengthSquared()<1e-20f) t={1,0,0}; else t.normalize();
                a.SetTangent(t.x(),t.y(),t.z());c.SetTangent(t.x(),t.y(),t.z());d.SetTangent(t.x(),t.y(),t.z());
            }
            mesh->bounds=Bounds(mesh->vertexData);mesh->FreeBuffers(false);mesh->CreateBuffers(false);
        }
        if(at!=verts.size()) return false;
    }
    b->last=transforms;return !b->meshes.empty();
}
}
IntType glb_smooth_update(StringType meshPath,StringType skinPath,ArrType joints,MatrixType owner,IntType previous) {
    VertexBuffer* existing=FindVertexBuffer(previous);
    SkinBuffer* skin=existing && existing->dynamicSkin ? static_cast<SkinBuffer*>(existing) : nullptr;
    std::unique_ptr<SkinBuffer> fresh;
    if(!skin) {fresh=loadSkin((QString)meshPath,(QString)skinPath);skin=fresh.get();}
    if(!skin || skin->meshPath!=(QString)meshPath || skin->skinPath!=(QString)skinPath || joints.Size()!=IntType(skin->joints)) return 0;
    Matrix inverse;
    if(!inversePrecise(owner.matrix,inverse)) return 0;
    std::vector<QMatrix4x4> transforms;transforms.reserve(skin->joints);
    for(IntType i=0;i<joints.Size();i++) {
        VarType item=joints.Value(i);
        if(!item.IsMatrix()) return 0;
        QMatrix4x4 m;
        if(!relativePrecise(inverse,item.ToMat().matrix,m)) return 0;
        transforms.push_back(m);
    }
    if(!updateSkin(skin,transforms)) return 0;
    IntType id=skin->id;if(fresh) fresh.release();return id;
}
}
