#include "GltfMeshImport.hpp"
#define CGLTF_IMPLEMENTATION
#include "External/cgltf.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QInputDialog>
#include <QApplication>
#include <QMatrix4x4>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QVector3D>
#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <vector>
#include <utility>
#include <QMessageBox>
#include "GltfImportCompat.hpp"
#include "GltfResourceLoad.hpp"

namespace {
constexpr qint64 MaxFile = 128LL * 1024 * 1024;
constexpr size_t MaxVertices = 1000000;
constexpr size_t MaxGroups = 64;
constexpr size_t MaxNodes = 256;
constexpr size_t MaxPartGroups = 512;
struct Influence { unsigned joint=0; float weight=0; QVector3D p; };
struct SkinPoint { Influence influence[8]; };
struct Vertex { QVector3D p, n; float u, v; };
struct Material { const cgltf_material* material; QImage image, materialImage; const cgltf_primitive* bakedPrimitive=nullptr; };
struct Group { size_t node, material; std::vector<Vertex> verts; const cgltf_skin* skin=nullptr; std::vector<SkinPoint> skinPoints; };
struct Node { int parent; QString name; QVector3D pivot; std::vector<int> children; };
QString err(const QString& e) { return "ERROR: " + e; }
bool finite3(const QVector3D& v) { return std::isfinite(v.x()) && std::isfinite(v.y()) && std::isfinite(v.z()); }
QVector3D axis(const QVector3D& v) { return QVector3D(v.x(), -v.z(), v.y()); }
QVector3D transformNormal(const QMatrix3x3& m, const QVector3D& n) {
    return QVector3D(m(0,0)*n.x()+m(0,1)*n.y()+m(0,2)*n.z(), m(1,0)*n.x()+m(1,1)*n.y()+m(1,2)*n.z(), m(2,0)*n.x()+m(2,1)*n.y()+m(2,2)*n.z());
}
const cgltf_accessor* attr(const cgltf_primitive& p, cgltf_attribute_type type, int index=0) {
    for (size_t i=0; i<p.attributes_count; ++i)
        if (p.attributes[i].type==type && p.attributes[i].index==index) return p.attributes[i].data;
    return nullptr;
}
// Every currently rendered material map must use REPEAT on the axis that wraps.
bool repeatSupported(const cgltf_material* m, bool sAxis) {
    if (!m) return true;
    if(m->has_pbr_specular_glossiness) {
        const cgltf_texture_view* sgViews[]={&m->pbr_specular_glossiness.diffuse_texture,&m->pbr_specular_glossiness.specular_glossiness_texture};
        for(auto v:sgViews) if(v->texture && v->texture->sampler && (sAxis?v->texture->sampler->wrap_s:v->texture->sampler->wrap_t)!=cgltf_wrap_mode_repeat) return false;
    }
    if(m->has_transmission && m->transmission.transmission_texture.texture && m->transmission.transmission_texture.texture->sampler &&
       (sAxis?m->transmission.transmission_texture.texture->sampler->wrap_s:m->transmission.transmission_texture.texture->sampler->wrap_t)!=cgltf_wrap_mode_repeat) return false;
    const cgltf_texture_view* views[]={&m->pbr_metallic_roughness.base_color_texture,
        &m->pbr_metallic_roughness.metallic_roughness_texture, &m->emissive_texture};
    for (auto view:views) if(view->texture && view->texture->sampler) {
        auto sampler=view->texture->sampler;
        if((sAxis ? sampler->wrap_s : sampler->wrap_t)!=cgltf_wrap_mode_repeat) return false;
    }
    return true;
}
QMatrix4x4 gltfMatrix(const float* m) {
    QMatrix4x4 result;
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) result(r,c)=m[c*4+r];
    return result;
}
bool validMatrix(const QMatrix4x4& m) {
    for(int r=0;r<4;r++) for(int c=0;c<4;c++) if(!std::isfinite(m(r,c))) return false;
    return true;
}
// External/data-URI images are staged as buffer views by the bounded loader.
QImage readImage(const cgltf_texture_view* view, QString& error) {
    if (!view || !view->texture) return {};
    if (!view->texture->image || !view->texture->image->buffer_view) {
        error="Texture has no decodable image (convert KTX2/BasisU to PNG)."; return {};
    }
    auto bv=view->texture->image->buffer_view;
    if (bv->size>MaxFile || !cgltf_buffer_view_data(bv)) {
        error="Invalid image data."; return {};
    }
    QByteArray bytes((const char*)cgltf_buffer_view_data(bv), int(bv->size));
    QBuffer input(&bytes); input.open(QIODevice::ReadOnly);
    QImageReader reader(&input); QSize size=reader.size();
    if (!size.isValid() || size.width()>4096 || size.height()>4096 || qint64(size.width())*size.height()>16777216) {
        error="Image dimensions are invalid or exceed 4096 pixels."; return {};
    }
    QImage img=reader.read();
    if (img.isNull()) { error="Cannot decode embedded image."; return {}; }
    return img.convertToFormat(QImage::Format_ARGB32);
}
bool unit(float v) { return std::isfinite(v) && v>=0 && v<=1; }
QImage materialImage(const cgltf_material* m, QString& error) {
    QImage img=readImage(m ? (m->has_pbr_specular_glossiness ? &m->pbr_specular_glossiness.diffuse_texture : &m->pbr_metallic_roughness.base_color_texture) : nullptr,error);
    if (!error.isEmpty()) return {};
    if (img.isNull()) { img=QImage(1,1,QImage::Format_ARGB32); img.fill(Qt::white); }
    const float* f=m ? (m->has_pbr_specular_glossiness ? m->pbr_specular_glossiness.diffuse_factor : m->pbr_metallic_roughness.base_color_factor) : nullptr;
    if (f) for (int i=0;i<4;i++) if (!unit(f[i])) { error="Invalid base colour factor."; return {}; }
    bool mask=m && m->alpha_mode==cgltf_alpha_mode_mask;
    if (mask && !unit(m->alpha_cutoff)) { error="Invalid alpha cutoff."; return {}; }
    const bool blend=m && m->alpha_mode==cgltf_alpha_mode_blend;
    QImage transmission;
    if(m && m->has_transmission) {
        if(!unit(m->transmission.transmission_factor)) { error="Invalid transmission factor."; return {}; }
        transmission=readImage(&m->transmission.transmission_texture,error);
        if(!error.isEmpty()) return {};
        int w=std::max(img.width(),transmission.width()), h=std::max(img.height(),transmission.height());
        img=img.scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        if(!transmission.isNull()) transmission=transmission.scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    }
    // Coverage only: transmission is approximated without refraction or volume absorption.
    for(int y=0;y<img.height();++y) {
        QRgb* line=(QRgb*)img.scanLine(y);
        for(int x=0;x<img.width();++x) {
            QRgb c=line[x];
            float coverage=blend ? qAlpha(c)/255.f*(f?f[3]:1.f) :
                (mask && qAlpha(c)/255.f*(f?f[3]:1.f)<m->alpha_cutoff ? 0.f : 1.f);
            if(m && m->has_transmission) coverage*=1.f-m->transmission.transmission_factor*
                (transmission.isNull()?1.f:qRed(transmission.pixel(x,y))/255.f);
            int a=qBound(0,int(coverage*255.f+0.5f),255);
            line[x]=qRgba(qBound(0,int(qRed(c)*(f?f[0]:1.f)+0.5f),255),qBound(0,int(qGreen(c)*(f?f[1]:1.f)+0.5f),255),qBound(0,int(qBlue(c)*(f?f[2]:1.f)+0.5f),255),a);
        }
    }
    int side=std::max(img.width(),img.height());
    return img.scaled(side,side,Qt::IgnoreAspectRatio,mask?Qt::FastTransformation:Qt::SmoothTransformation);
}
// Convert glTF metallic-roughness into the engine's SEUS material channels.
// The engine has one scalar emission channel; coloured emission is approximated.
QImage pbrImage(const cgltf_material* m, QString& error) {
    bool sg=m && m->has_pbr_specular_glossiness;
    float metal=sg?0.f:m?m->pbr_metallic_roughness.metallic_factor:1.f;
    float rough=sg?m->pbr_specular_glossiness.glossiness_factor:m?m->pbr_metallic_roughness.roughness_factor:1.f;
    if(!unit(metal)||!unit(rough)) { error="Invalid metallic or roughness factor."; return {}; }
    if(m) for(int i=0;i<3;i++) if(!unit(m->emissive_factor[i])) { error="Invalid emissive factor."; return {}; }
    QImage mr=readImage(m?(sg?&m->pbr_specular_glossiness.specular_glossiness_texture:&m->pbr_metallic_roughness.metallic_roughness_texture):nullptr,error);
    if(!error.isEmpty()) return {};
    QImage em=readImage(m?&m->emissive_texture:nullptr,error);
    if(!error.isEmpty()) return {};
    int side=1;
    if(!mr.isNull()) side=std::max(side,std::max(mr.width(),mr.height()));
    if(!em.isNull()) side=std::max(side,std::max(em.width(),em.height()));
    if(!mr.isNull()) mr=mr.scaled(side,side,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    if(!em.isNull()) em=em.scaled(side,side,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    QImage out(side,side,QImage::Format_ARGB32);
    if(out.isNull()) { error="Cannot allocate material texture."; return {}; }
    for(int y=0;y<side;y++) {
        QRgb* dst=(QRgb*)out.scanLine(y);
        for(int x=0;x<side;x++) {
            QRgb a=mr.isNull()?qRgb(255,255,255):mr.pixel(x,y);
            QRgb e=em.isNull()?qRgb(255,255,255):em.pixel(x,y);
            float emission=m?std::max(qRed(e)*m->emissive_factor[0],std::max(qGreen(e)*m->emissive_factor[1],qBlue(e)*m->emissive_factor[2])):0.f;
            dst[x]=qRgb(qBound(0,int((sg?qAlpha(a)*rough:255.f-qGreen(a)*rough)+0.5f),255),qBound(0,int(qBlue(a)*metal+0.5f),255),qBound(0,int(emission+0.5f),255));
        }
    }
    return out;
}
// Uses the helpers/types above; deliberately private to this translation unit.
#include "GltfUvBake.hpp"
}

QString importStaticGlb(const QString& fileName, int filterMode, int filterNode, bool rigidBones, bool smoothBones) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly) || file.size()<20 || file.size()>MaxFile) return err("Cannot read GLB or file exceeds 128 MiB.");
    QByteArray bytes=file.readAll(); if (bytes.size()!=file.size()) return err("Cannot read the whole GLB.");
    cgltf_options opts={}; cgltf_data* raw=nullptr;
    if (cgltf_parse(&opts,bytes.constData(),size_t(bytes.size()),&raw)!=cgltf_result_success) return err("Invalid glTF/GLB file.");
    std::unique_ptr<cgltf_data,decltype(&cgltf_free)> data(raw,cgltf_free);
    for(size_t i=0;i<data->extensions_required_count;i++) {
        QString ext=QString::fromUtf8(data->extensions_required[i]);
        if(ext!="KHR_mesh_quantization" && ext!="KHR_texture_transform" && ext!="KHR_materials_pbrSpecularGlossiness" && ext!="KHR_materials_transmission")
            return err("Required extension needs a decoder or renderer not included in this build: "+ext);
    }
    if(data->nodes_count>MaxNodes || data->meshes_count>10000 || data->materials_count>MaxGroups)
        return err("Model exceeds node, mesh or material limits.");
    QString loadError;
    GltfResources resources;
    if(!resources.load(data.get(),QFileInfo(fileName).absolutePath(),size_t(bytes.size()),loadError)) return err(loadError);
    QStringList warnings;
    if(data->animations_count) warnings << "Animation clips are NOT imported. Geometry uses the default node pose, not animation frame zero.";
    bool morphs=false;
    for(size_t i=0;i<data->meshes_count;i++) for(size_t j=0;j<data->meshes[i].primitives_count;j++) morphs|=data->meshes[i].primitives[j].targets_count!=0;
    if(morphs) warnings << "Morph targets are baked at their default weights; editable morph channels are NOT imported.";
    for(size_t i=0;i<data->materials_count;i++) {
        const auto& m=data->materials[i];
        if(m.alpha_mode==cgltf_alpha_mode_blend) warnings << "BLEND alpha is preserved. Rendering uses the engine's existing alpha mode; intersecting surfaces may need depth ordering or alpha hashing.";
        if(m.has_transmission) warnings << "Transmission is a non-refractive opacity approximation (factor times red texture channel); refraction, IOR and volume absorption are NOT reproduced.";
        if(m.has_pbr_specular_glossiness) warnings << "Specular-glossiness is approximated as diffuse colour, glossiness and non-metallic SEUS material; coloured specular is not reproduced.";
        if(m.normal_texture.texture || m.occlusion_texture.texture || m.unlit || m.has_clearcoat || m.has_volume || m.has_ior || m.has_specular || m.has_sheen || m.has_emissive_strength || m.has_iridescence || m.has_anisotropy || m.has_diffuse_transmission || m.has_dispersion)
            warnings << "Advanced material properties (normal/AO maps, unlit, coat and other extensions) are NOT reproduced by this converter.";
    }
    warnings << "Different material UV channels/transforms and non-repeat samplers are automatically baked to a finite-resolution triangle atlas. This can reduce texture detail; mipmaps/anisotropic filtering are not reproduced. Normal/AO maps remain unsupported.";
    warnings << "Materials use SEUS channels; coloured emission is approximated. Model is fitted to 16 units, converting Y-up to Z-up.";
    warnings.removeDuplicates();
    if(!confirmImport(warnings)) return QString();
    const cgltf_scene* scene=data->scene ? data->scene : (data->scenes_count ? &data->scenes[0] : nullptr);
    if(!scene) return err("The GLB has no scene.");
    // This exact sample has an out-of-scale tail mesh in its source data.
    // Offer an explicit, isolated workaround; other GLBs are never altered.
    const bool knownKit=QCryptographicHash::hash(bytes,QCryptographicHash::Sha256).toHex()==
        QByteArray("0fd2e016541fb949e1381ca455d891be4ccd3c962bb70359498e25b9d26743b8");
    bool repairKitTail=false;
    bool pruneUnusedBones=false;
    // Explicit, opt-in mesh filtering. No automatic changes to model coordinates.
    smoothBones=(smoothBones || filterMode==-3);
    if(filterMode==-1 || filterMode==-2 || filterMode==-3) {
        rigidBones=(filterMode==-2);
        QStringList modes;
        if(rigidBones || smoothBones) modes << "Import all meshes";
        modes << "Import all except selected mesh" << "Import only selected mesh";
        if(smoothBones && knownKit) modes << "Import all meshes (Kit tail repair, experimental)";
        if(smoothBones) modes << "Import all meshes (prune unused bones, experimental)";
        bool ok=false;
        QString mode=QInputDialog::getItem(QApplication::activeWindow(), "GLB mesh selection",
            smoothBones ? "Smooth skinning (experimental): choose meshes:" : rigidBones ? "Rigid bones: parts move but joints may split. Choose meshes:" :
                         "Choose how to import (the original GLB will not be changed):", modes, 0, false, &ok);
        if(!ok) return QString();
        repairKitTail=smoothBones && knownKit && mode=="Import all meshes (Kit tail repair, experimental)";
        pruneUnusedBones=smoothBones && mode=="Import all meshes (prune unused bones, experimental)";
        if(repairKitTail || pruneUnusedBones || ((rigidBones || smoothBones) && mode==modes[0])) filterMode=0;
        else filterMode=(mode=="Import all except selected mesh")?2:1;
        if(filterMode!=0) {
            QStringList choices; std::vector<int> indexes;
            std::vector<bool> listed(data->nodes_count,false);
            std::function<bool(const cgltf_node*,size_t)> listMeshes;
            listMeshes=[&](const cgltf_node* n,size_t depth)->bool {
                if(!n || n<data->nodes || n>=data->nodes+data->nodes_count || depth>128) return false;
                size_t i=size_t(n-data->nodes); if(listed[i]) return false; listed[i]=true;
                if(n->mesh) {
                    QString nodeName=QString::fromUtf8(n->name?n->name:"Node");
                    QString meshName=QString::fromUtf8(n->mesh->name?n->mesh->name:"Mesh");
                    choices << nodeName.left(120)+" ["+QString::number(i)+"] - "+meshName.left(120);
                    indexes.push_back(int(i));
                }
                for(size_t k=0;k<n->children_count;k++) if(!listMeshes(n->children[k],depth+1)) return false;
                return true;
            };
            for(size_t i=0;i<scene->nodes_count;i++) if(!listMeshes(scene->nodes[i],0)) return err("Invalid or too-deep hierarchy.");
            if(choices.isEmpty()) return err("No mesh nodes to select.");
            QString choice=QInputDialog::getItem(QApplication::activeWindow(), "GLB mesh selection",
                "Select a mesh node:", choices, 0, false, &ok);
            if(!ok) return QString();
            int selected=choices.indexOf(choice);
            if(selected<0) return err("Invalid mesh selection.");
            filterNode=indexes[size_t(selected)];
        }
    }
    if(filterMode<0 || filterMode>2 || (filterMode && (filterNode<0 || size_t(filterNode)>=data->nodes_count || !data->nodes[filterNode].mesh)))
        return err("Invalid mesh selection.");
    std::vector<Group> groups; std::vector<Material> materials; std::vector<Node> nodes; size_t total=0, texturePixels=0; QString error;
    QVector3D lo, hi; bool haveBounds=false, haveRiggedMesh=false; std::vector<bool> visited(data->nodes_count,false);
    std::vector<int> nodeMap(data->nodes_count,-1);
    auto materialIndex=[&](const cgltf_material* m)->int {
        for(size_t i=0;i<materials.size();++i) if(materials[i].material==m && !materials[i].bakedPrimitive) return int(i);
        if(materials.size()>=MaxGroups) { error="Too many materials."; return -1; }
        QImage image=materialImage(m,error);
        if (!error.isEmpty()) return -1;
        QImage matImage=pbrImage(m,error);
        if (!error.isEmpty()) return -1;
        texturePixels += size_t(image.width()) * size_t(image.height()) + size_t(matImage.width()) * size_t(matImage.height());
        if (texturePixels > 32u * 1024u * 1024u) { error="Total texture size exceeds 32 megapixels."; return -1; }
        materials.push_back(Material{m,std::move(image),std::move(matImage)});
        return int(materials.size()-1);
    };
    auto nodeGroup=[&](size_t node, size_t material)->Group* {
        for(auto& group:groups) if(group.node==node && group.material==material) return &group;
        if(groups.size()>=MaxPartGroups) { error="Too many mesh groups."; return nullptr; }
        groups.push_back(Group{node,material,{}});
        return &groups.back();
    };
    std::function<bool(const cgltf_node*,const QMatrix4x4&,int,int)> visit;
    visit=[&](const cgltf_node* node,const QMatrix4x4& parent,int depth,int parentIndex)->bool {
        if(!node || node<data->nodes || node>=data->nodes+data->nodes_count || depth>128) { error="Invalid or too-deep hierarchy."; return false; }
        size_t ni=size_t(node-data->nodes); if(visited[ni]) { error="Cycle or repeated node in scene."; return false; } visited[ni]=true;
        if(node->has_mesh_gpu_instancing) { error="GPU instancing is not supported."; return false; }
        float m[16]; cgltf_node_transform_local(node,m); QMatrix4x4 local;
        for(int r=0;r<4;r++) for(int c=0;c<4;c++) local(r,c)=m[c*4+r];
        QMatrix4x4 world=parent*local; float determinant=world.determinant();
        if(!std::isfinite(determinant) || std::abs(determinant)<1e-12f) { error="Invalid node transform."; return false; }
        QVector3D pivot=axis(world.map(QVector3D(0,0,0)));
        if(!finite3(pivot)) { error="Invalid node position."; return false; }
        int nodeIndex=int(nodes.size()); nodeMap[ni]=nodeIndex;
        QString name=node->name ? QString::fromUtf8(node->name) : QString("Node");
        if(name.isEmpty()) name="Node";
        // Stable indices disambiguate repeated/empty names for Mine-imator.
        name=name.left(160)+" ["+QString::number(ni)+"]";
        bool useNodeMesh=node->mesh && (filterMode==0 || (filterMode==1 && int(ni)==filterNode) || (filterMode==2 && int(ni)!=filterNode));
        if(node->skin && useNodeMesh && !rigidBones && !smoothBones) name += " (skin frozen)";
        if(node->skin && useNodeMesh && (rigidBones || smoothBones)) haveRiggedMesh=true;
        nodes.push_back(Node{parentIndex,name,pivot,{}});
        if(parentIndex>=0) nodes[size_t(parentIndex)].children.push_back(nodeIndex);
        QMatrix3x3 normalMatrix=world.normalMatrix();
        std::vector<QMatrix4x4> skinMatrices;
        if(node->skin && useNodeMesh) {
            const cgltf_skin* skin=node->skin;
            if(!skin->joints_count || skin->joints_count>MaxNodes ||
               (skin->inverse_bind_matrices && (skin->inverse_bind_matrices->is_sparse ||
                skin->inverse_bind_matrices->type!=cgltf_type_mat4 ||
                skin->inverse_bind_matrices->component_type!=cgltf_component_type_r_32f ||
                skin->inverse_bind_matrices->count<skin->joints_count))) {
                error="Invalid or unsupported skin matrices."; return false;
            }
            for(size_t j=0;j<skin->joints_count;j++) {
                if(!skin->joints[j] || skin->joints[j]<data->nodes || skin->joints[j]>=data->nodes+data->nodes_count) {
                    error="Invalid skin joint."; return false;
                }
                QMatrix4x4 joint, inverseBind;
                const cgltf_node* ancestor=skin->joints[j];
                size_t depth=0;
                while(ancestor) {
                    if(ancestor<data->nodes || ancestor>=data->nodes+data->nodes_count || ++depth>129) {
                        error="Invalid or too-deep skin hierarchy."; return false;
                    }
                    float jm[16]; cgltf_node_transform_local(ancestor,jm);
                    joint=gltfMatrix(jm)*joint;
                    ancestor=ancestor->parent;
                }
                if(skin->inverse_bind_matrices) {
                    float bm[16];
                    if(!cgltf_accessor_read_float(skin->inverse_bind_matrices,j,bm,16)) { error="Cannot read skin matrices."; return false; }
                    inverseBind=gltfMatrix(bm);
                }
                QMatrix4x4 combined=joint*inverseBind;
                if(!validMatrix(combined)) { error="Invalid skin transform."; return false; }
                skinMatrices.push_back(combined);
            }
        }
        if(useNodeMesh) for(size_t pi=0;pi<node->mesh->primitives_count;pi++) {
            const auto& p=node->mesh->primitives[pi];
            if(p.has_draco_mesh_compression) { error="KHR_draco_mesh_compression needs decoding; export uncompressed glTF/GLB."; return false; }
            if(p.type!=cgltf_primitive_type_triangles && p.type!=cgltf_primitive_type_triangle_strip && p.type!=cgltf_primitive_type_triangle_fan) { error="Point and line primitives cannot be represented as model surfaces."; return false; }
            const auto* pos=attr(p,cgltf_attribute_type_position);
            const auto* norm=attr(p,cgltf_attribute_type_normal);
            GltfUvBake bake;
            if(!gltfPrepareUv(p,pos,bake,error)) return false;
            const auto* uvView=bake.common;
            const auto* uv=uvView ? attr(p,cgltf_attribute_type_texcoord,gltfUvChannel(uvView)) : nullptr;
            if(!pos || pos->type!=cgltf_type_vec3 || (norm && (norm->type!=cgltf_type_vec3 || norm->count!=pos->count)) || (uv && (uv->type!=cgltf_type_vec2 || uv->count!=pos->count))) { error="Unsupported vertex attributes."; return false; }
            const cgltf_accessor* joints[2]={attr(p,cgltf_attribute_type_joints,0),attr(p,cgltf_attribute_type_joints,1)};
            const cgltf_accessor* weights[2]={attr(p,cgltf_attribute_type_weights,0),attr(p,cgltf_attribute_type_weights,1)};
            if(node->skin) {
                for(size_t ai=0;ai<p.attributes_count;ai++)
                    if((p.attributes[ai].type==cgltf_attribute_type_joints || p.attributes[ai].type==cgltf_attribute_type_weights) && p.attributes[ai].index>1) {
                        error="More than eight skin influences are not supported."; return false;
                    }
                for(int set=0;set<2;set++) {
                    if((set==0 && (!joints[set] || !weights[set])) || ((joints[set]!=nullptr)!=(weights[set]!=nullptr))) { error="Missing skin weights or joints."; return false; }
                    if(!joints[set]) continue;
                    const auto* ja=joints[set]; const auto* wa=weights[set];
                    if(ja->is_sparse || wa->is_sparse || ja->type!=cgltf_type_vec4 || wa->type!=cgltf_type_vec4 ||
                       ja->count!=pos->count || wa->count!=pos->count || ja->normalized ||
                       (ja->component_type!=cgltf_component_type_r_8u && ja->component_type!=cgltf_component_type_r_16u) ||
                       !(wa->component_type==cgltf_component_type_r_32f || (wa->normalized &&
                         (wa->component_type==cgltf_component_type_r_8u || wa->component_type==cgltf_component_type_r_16u)))) {
                        error="Unsupported skin attribute format."; return false;
                    }
                }
            }
            if(uvView && !uv) { error="Textured mesh is missing its selected TEXCOORD channel."; return false; }
            size_t sourceCount=p.indices?p.indices->count:pos->count; bool two=p.material && p.material->double_sided;
            if(sourceCount<3 || sourceCount>MaxVertices || (p.type==cgltf_primitive_type_triangles && sourceCount%3)) { error="Invalid primitive size."; return false; }
            size_t count=p.type==cgltf_primitive_type_triangles ? sourceCount : (sourceCount-2)*3;
            if(total+count*(two?2:1)>MaxVertices) { error="Expanded triangles exceed the one million vertex limit; no parts were removed."; return false; }
            int mi=-1;
            if(bake.needed) {
                // The same primitive can be instanced by many nodes. Bake it once;
                // geometry, morphs and skin weights are still evaluated per node.
                for(size_t j=0;j<materials.size();j++) if(materials[j].bakedPrimitive==&p) { mi=int(j); break; }
                if(mi<0) {
                    if(materials.size()>=MaxGroups) { error="UV baking exceeds the 64 output-material limit."; return false; }
                    QImage colour, properties;
                    if(!gltfBakePrimitive(p,bake,count,texturePixels,colour,properties,error)) return false;
                    materials.push_back(Material{p.material,std::move(colour),std::move(properties),&p});
                    mi=int(materials.size()-1);
                } else {
                    bake.resolution=materials[size_t(mi)].image.width();
                    if(!gltfAtlasLayout(count/3,bake,error)) return false;
                }
            } else mi=materialIndex(p.material);
            if(mi<0) return false;
            Group* fixedGroup=nullptr;
            if(!(rigidBones && node->skin)) {
                fixedGroup=nodeGroup(size_t(nodeIndex),size_t(mi)); if(!fixedGroup) return false;
            }
            for(size_t i=0;i<count;i+=3) {
                Vertex v[3]; SkinPoint skinPoint[3]; bool mirrored[3]={};
                std::vector<float> votes;
                if(rigidBones && node->skin) votes.resize(skinMatrices.size(),0.f);
                for(size_t k=0;k<3;k++) {
                    size_t source=i+k;
                    if(p.type==cgltf_primitive_type_triangle_strip) {
                        size_t triangle=i/3; source=triangle+k;
                        if((triangle&1) && k<2) source=triangle+1-k;
                    } else if(p.type==cgltf_primitive_type_triangle_fan) source=k==0 ? 0 : i/3+k;
                    size_t ix=p.indices ? cgltf_accessor_read_index(p.indices,source) : source;
                    if(ix>=pos->count) { error="Index outside vertex array."; return false; }
                    float xyz[3],n[3]={0,0,0},t[2]={0,0};
                    if(!cgltf_accessor_read_float(pos,ix,xyz,3) || (norm && !cgltf_accessor_read_float(norm,ix,n,3)) || (uv && !cgltf_accessor_read_float(uv,ix,t,2))) { error="Cannot read vertex data."; return false; }
                    for(size_t ti=0;ti<p.targets_count;ti++) {
                        float weight=node->weights && ti<node->weights_count ? node->weights[ti] : node->mesh->weights && ti<node->mesh->weights_count ? node->mesh->weights[ti] : 0.f;
                        if(!std::isfinite(weight)) { error="Invalid morph weight."; return false; }
                        if(weight==0) continue;
                        const auto& target=p.targets[ti];
                        for(size_t ai=0;ai<target.attributes_count;ai++) {
                            const auto& a=target.attributes[ai];
                            if(a.type!=cgltf_attribute_type_position && a.type!=cgltf_attribute_type_normal) continue;
                            float delta[3];
                            if(a.data->type!=cgltf_type_vec3 || a.data->count!=pos->count || !cgltf_accessor_read_float(a.data,ix,delta,3)) { error="Invalid morph delta."; return false; }
                            for(int c=0;c<3;c++) {
                                if(!std::isfinite(delta[c])) { error="Non-finite morph delta."; return false; }
                                if(a.type==cgltf_attribute_type_position) xyz[c]+=weight*delta[c];
                                else if(norm) n[c]+=weight*delta[c];
                            }
                        }
                    }
                    if(!bake.needed) transformUV(uvView,t);
                    else gltfAtlasUv(i/3,k,bake,t);
                    // Exact known Kit file only, and only when explicitly selected:
                    // tail positions are ~100x larger and face the opposite way.
                    // Transform before skinning so weights and bone animation agree.
                    if(repairKitTail && ni==194) {
                        xyz[0]*=0.01f; xyz[1]*=-0.01f; xyz[2]*=-0.01f;
                        n[1]=-n[1]; n[2]=-n[2];
                    }
                    QMatrix4x4 vertexMatrix=world;
                    if(node->skin) {
                        cgltf_uint js[2][4]={{0},{0}}; float ws[2][4]={{0},{0}}; float sum=0;
                        for(int set=0;set<2;set++) if(joints[set]) {
                            if(!cgltf_accessor_read_uint(joints[set],ix,js[set],4) || !cgltf_accessor_read_float(weights[set],ix,ws[set],4)) { error="Cannot read skin influences."; return false; }
                            for(int q=0;q<4;q++) {
                                float w=ws[set][q];
                                if(!unit(w) || (w>0 && js[set][q]>=skinMatrices.size())) { error="Invalid skin influence."; return false; }
                                sum+=w;
                            }
                        }
                        if(!std::isfinite(sum) || sum<1e-8f) { error="Skin vertex has no weights."; return false; }
                        vertexMatrix.fill(0.f);
                        for(int set=0;set<2;set++) if(joints[set]) for(int q=0;q<4;q++)
                            if(ws[set][q]>0) {
                                const float weight=ws[set][q]/sum;
                                vertexMatrix+=skinMatrices[js[set][q]]*weight;
                                if(rigidBones) votes[js[set][q]]+=weight;
                                if(smoothBones) {
                                    Influence& inf=skinPoint[k].influence[set*4+q];
                                    inf.joint=js[set][q]; inf.weight=weight;
                                    inf.p=axis(skinMatrices[inf.joint].map(QVector3D(xyz[0],xyz[1],xyz[2])));
                                    if(!finite3(inf.p)) { error="Invalid weighted vertex."; return false; }
                                }
                            }
                        float det=vertexMatrix.determinant();
                        if(!validMatrix(vertexMatrix) || !std::isfinite(det) || std::abs(det)<1e-12f) { error="Invalid blended skin transform."; return false; }
                        mirrored[k]=det<0;
                    } else mirrored[k]=determinant<0;
                    QVector3D q=vertexMatrix.map(QVector3D(xyz[0],xyz[1],xyz[2])); v[k].p=axis(q);
                    QMatrix3x3 vertexNormal=node->skin?vertexMatrix.normalMatrix():normalMatrix;
                    v[k].n=norm?axis(transformNormal(vertexNormal,QVector3D(n[0],n[1],n[2]))):QVector3D();
                    if(!finite3(v[k].p)||!finite3(v[k].n)||!std::isfinite(t[0])||!std::isfinite(t[1]) ||
                       std::abs(t[0])>10000 || std::abs(t[1])>10000 ||
                       (!bake.needed && (t[0]<0.f || t[0]>1.f) && !repeatSupported(p.material,true)) ||
                       (!bake.needed && (t[1]<0.f || t[1]>1.f) && !repeatSupported(p.material,false))) {
                        error="Invalid values or unsupported UV wrapping mode."; return false;
                    }
                    // Preserve wrapped UVs; the renderer repeats at sample time, not per vertex.
                    v[k].u=t[0]; v[k].v=t[1];
                }
                if(mirrored[0]!=mirrored[1] || mirrored[0]!=mirrored[2]) { error="Skin triangle has inconsistent mirrored transforms."; return false; }
                if(mirrored[0]) { std::swap(v[1],v[2]); std::swap(skinPoint[1],skinPoint[2]); }
                QVector3D face=QVector3D::crossProduct(v[1].p-v[0].p,v[2].p-v[0].p);
                const float faceLength2=face.lengthSquared();
                if(!std::isfinite(faceLength2)) { error="Invalid triangle normal."; return false; }
                if(faceLength2<=1e-30f) continue;
                face/=std::sqrt(faceLength2);
                size_t groupNode=size_t(nodeIndex);
                if(!votes.empty()) {
                    size_t winner=size_t(std::max_element(votes.begin(),votes.end())-votes.begin());
                    // Tagged source-node ID; resolved to the model part after the entire scene is traversed.
                    groupNode=data->nodes_count+size_t(node->skin->joints[winner]-data->nodes);
                }
                Group* g=fixedGroup ? fixedGroup : nodeGroup(groupNode,size_t(mi)); if(!g) return false;
                if(smoothBones && node->skin) g->skin=node->skin;
                for(int vi=0;vi<3;vi++) {
                    auto& x=v[vi];
                    // Discarded degenerate triangles must not affect fitting.
                    if(!haveBounds) { lo=hi=x.p; haveBounds=true; }
                    else { lo.setX(std::min(lo.x(),x.p.x())); lo.setY(std::min(lo.y(),x.p.y())); lo.setZ(std::min(lo.z(),x.p.z())); hi.setX(std::max(hi.x(),x.p.x())); hi.setY(std::max(hi.y(),x.p.y())); hi.setZ(std::max(hi.z(),x.p.z())); }
                    const float normalLength2=x.n.lengthSquared();
                    if(!std::isfinite(normalLength2)) { error="Invalid vertex normal."; return false; }
                    if(normalLength2<=1e-30f) x.n=face; else x.n/=std::sqrt(normalLength2);
                    g->verts.push_back(x);
                    if(smoothBones && node->skin) g->skinPoints.push_back(skinPoint[vi]);
                }
                if(two) for(int k=2;k>=0;--k) { Vertex back=v[k]; back.n=-back.n; g->verts.push_back(back); if(smoothBones && node->skin) g->skinPoints.push_back(skinPoint[k]); }
                total+=two?6:3;
            }
        }
        for(size_t i=0;i<node->children_count;i++) if(!visit(node->children[i],world,depth+1,nodeIndex)) return false;
        return true;
    };
    QMatrix4x4 identity;
    for(size_t i=0;i<scene->nodes_count;i++) if(!visit(scene->nodes[i],identity,0,-1)) return err(error);
    if(!haveBounds || !total) return err("The scene has no usable triangles.");
    if((rigidBones || smoothBones) && !haveRiggedMesh) {
        // An unskinned mesh is valid input, not a broken skin. Do not invent
        // weights/bones or drop the model; explicitly offer hierarchy import.
        const QString fallbackWarning="Selected meshes contain no skin or vertex weights. Import as static meshes with the existing node hierarchy instead? No skeleton, automatic rig or smooth deformation will be created.";
        if(!confirmImport(QStringList() << fallbackWarning)) return QString();
        warnings << fallbackWarning;
        rigidBones=false; smoothBones=false;
        repairKitTail=false; pruneUnusedBones=false;
    }
    for(auto& group:groups) if(group.node>=data->nodes_count) {
        size_t source=group.node-data->nodes_count;
        if(source>=nodeMap.size() || nodeMap[source]<0) return err("A skin joint is outside the active scene.");
        group.node=size_t(nodeMap[source]);
    }
    groups.erase(std::remove_if(groups.begin(),groups.end(),[](const Group& g){ return g.verts.empty(); }),groups.end());
    QVector3D extent=hi-lo; float span=std::max(extent.x(),std::max(extent.y(),extent.z()));
    if(!std::isfinite(span)||span<1e-9f) return err("Model bounds are invalid.");
    float scale=16.f/span; QVector3D center((lo.x()+hi.x())/2,(lo.y()+hi.y())/2,lo.z());
    QTemporaryDir temp(QDir::tempPath()+"/MineimatorGLB-XXXXXX"); if(!temp.isValid()) return err("Cannot create temporary directory.");
    QString base=QFileInfo(temp.path()).fileName();
    if(!QDir(temp.path()).mkdir(base)) return err("Cannot create mesh directory.");
    for(size_t i=0;i<materials.size();++i) {
        const auto& m=materials[i];
        if(!m.image.save(temp.filePath(base+"/texture"+QString::number(i)+".png"),"PNG") ||
           !m.materialImage.save(temp.filePath(base+"/material"+QString::number(i)+".png"),"PNG")) return err("Cannot write texture.");
    }
    std::vector<bool> skinJoint(nodes.size(),false);
    if(smoothBones) for(const auto& group:groups) if(group.skin) {
        // Preserve every joint by default. Pruning is opt-in: retain weighted
        // joints, and containsGeometry below retains all their ancestors.
        std::vector<bool> needed(group.skin->joints_count,!pruneUnusedBones);
        if(pruneUnusedBones) for(const SkinPoint& point:group.skinPoints)
            for(const Influence& inf:point.influence) if(inf.weight>0) {
                if(inf.joint>=needed.size()) return err("Invalid joint index.");
                needed[inf.joint]=true;
            }
        for(size_t ji=0;ji<needed.size();ji++) if(needed[ji]) {
            size_t source=size_t(group.skin->joints[ji]-data->nodes);
            if(source>=nodeMap.size() || nodeMap[source]<0) return err("A skin joint is outside the active scene.");
            skinJoint[size_t(nodeMap[source])]=true;
        }
    }
    std::vector<QJsonArray> shapes(nodes.size());
    for(size_t i=0;i<groups.size();i++) {
        const auto& g=groups[i]; const auto& node=nodes[g.node];
        QString relative=base+"/group"+QString::number(i)+".ctmesh";
        QFile out(temp.filePath(relative));
        if(!out.open(QIODevice::WriteOnly|QIODevice::NewOnly)) return err("Cannot write mesh.");
        QDataStream stream(&out); stream.setByteOrder(QDataStream::LittleEndian); stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream<<quint32(0x3148534d)<<quint32(1)<<quint32(g.verts.size());
        QVector3D blo, bhi; bool has=false;
        for(const auto& v:g.verts) {
            QVector3D p=(v.p-node.pivot)*scale;
            if(!finite3(p)) return err("Invalid transformed position.");
            if(!has) { blo=bhi=p;has=true; } else {
                blo.setX(std::min(blo.x(),p.x())); blo.setY(std::min(blo.y(),p.y())); blo.setZ(std::min(blo.z(),p.z()));
                bhi.setX(std::max(bhi.x(),p.x())); bhi.setY(std::max(bhi.y(),p.y())); bhi.setZ(std::max(bhi.z(),p.z()));
            }
            stream<<p.x()<<p.y()<<p.z()<<v.n.x()<<v.n.y()<<v.n.z()<<v.u<<v.v;
        }
        if(stream.status()!=QDataStream::Ok || !out.flush()) return err("Cannot finish mesh file.");
        QJsonObject shape;
        shape["type"]="glb_mesh"; shape["description"]="Mesh "+QString::number(i);
        const auto* sourceMaterial=materials[g.material].material;
        shape["gltf_transparent"]=bool(sourceMaterial && (sourceMaterial->alpha_mode==cgltf_alpha_mode_blend ||
            (sourceMaterial->has_transmission && sourceMaterial->transmission.transmission_factor>0)));
        shape["gltf_transmission_approximation"]=bool(sourceMaterial && sourceMaterial->has_transmission);
        shape["mesh_file"]=relative;
        if(smoothBones && g.skin) {
            if(g.skinPoints.size()!=g.verts.size()) return err("Invalid skin geometry.");
            // Each group needs only joints that influence its own vertices.
            // Keep joint order stable and remap indices in the existing v1 format.
            std::vector<int> remap(g.skin->joints_count,-1);
            for(const SkinPoint& point:g.skinPoints) for(const Influence& inf:point.influence) if(inf.weight>0) {
                if(inf.joint>=remap.size()) return err("Invalid joint index.");
                remap[inf.joint]=0;
            }
            QJsonArray names;
            for(size_t ji=0;ji<remap.size();ji++) if(remap[ji]==0) {
                remap[ji]=names.size();
                names.append(nodes[size_t(nodeMap[size_t(g.skin->joints[ji]-data->nodes)])].name);
            }
            if(names.isEmpty()) return err("Skin has no weighted joints.");
            QString skinRelative=base+"/group"+QString::number(i)+".ctskin";
            QFile skinOut(temp.filePath(skinRelative));
            if(!skinOut.open(QIODevice::WriteOnly|QIODevice::NewOnly)) return err("Cannot write skin weights.");
            QDataStream sk(&skinOut); sk.setByteOrder(QDataStream::LittleEndian); sk.setFloatingPointPrecision(QDataStream::SinglePrecision);
            sk<<quint32(0x314e4b53)<<quint32(1)<<quint32(g.verts.size())<<quint32(names.size());
            for(const SkinPoint& point:g.skinPoints) for(const Influence& inf:point.influence) {
                QVector3D q;
                if(inf.weight>0) {
                    size_t source=size_t(g.skin->joints[inf.joint]-data->nodes);
                    q=(inf.p-nodes[size_t(nodeMap[source])].pivot)*scale;
                    if(!finite3(q)) return err("Invalid skin position.");
                }
                // Zero-weight entries still need a valid index for the reader.
                sk<<quint32(inf.weight>0 ? remap[inf.joint] : 0)<<inf.weight<<q.x()<<q.y()<<q.z();
            }
            if(sk.status()!=QDataStream::Ok || !skinOut.flush()) return err("Cannot finish skin weights.");
            shape["skin_file"]=skinRelative; shape["skin_joints"]=names;
        }
        // .mimodel vectors are stored X,Z,Y; value_get_point3D swaps Y/Z on load.
        shape["from"]=QJsonArray{blo.x(),blo.z(),blo.y()}; shape["to"]=QJsonArray{bhi.x(),bhi.z(),bhi.y()}; shape["uv"]=QJsonArray{0,0};
        shape["texture"]=base+"/texture"+QString::number(g.material)+".png";
        shape["texture_material"]=base+"/material"+QString::number(g.material)+".png";
        shape["texture_size"]=QJsonArray{16,16};
        shapes[g.node].append(shape);
    }
    std::function<bool(int)> containsGeometry;
    containsGeometry=[&](int i)->bool {
        if(!shapes[size_t(i)].isEmpty() || skinJoint[size_t(i)]) return true;
        for(int child:nodes[size_t(i)].children) if(containsGeometry(child)) return true;
        return false;
    };
    std::function<QJsonObject(int)> makePart;
    makePart=[&](int i)->QJsonObject {
        const auto& n=nodes[size_t(i)];
        QVector3D p=n.parent<0 ? (n.pivot-center)*scale+QVector3D(8,8,0) : (n.pivot-nodes[size_t(n.parent)].pivot)*scale;
        QJsonObject obj; obj["name"]=n.name; obj["position"]=QJsonArray{p.x(),p.z(),p.y()}; obj["show_position"]=true;
        if(!shapes[size_t(i)].isEmpty()) obj["shapes"]=shapes[size_t(i)];
        QJsonArray children; for(int child:n.children) if((!data->skins_count && !filterMode) || containsGeometry(child)) children.append(makePart(child));
        if(!children.isEmpty()) obj["parts"]=children;
        return obj;
    };
    QJsonArray roots; for(size_t i=0;i<nodes.size();i++) if(nodes[i].parent<0 && ((!data->skins_count && !filterMode) || containsGeometry(int(i)))) roots.append(makePart(int(i)));
    QJsonObject model; model["name"]=base; model["texture"]=""; model["texture_size"]=QJsonArray{16,16}; model["parts"]=roots;
    model["glb_hierarchy_version"]=1; model["glb_skin_frozen"]=bool(data->skins_count && !rigidBones && !smoothBones); model["glb_rigid_bones"]=rigidBones; model["glb_smooth_bones"]=smoothBones;
    QJsonArray report; for(const auto& warning:warnings) report.append(warning);
    model["gltf_import_report"]=QJsonObject{{"warnings",report},{"source",QFileInfo(fileName).fileName()},{"expanded_vertices",double(total)},{"scale",double(scale)},
        {"uv_baked_materials",int(std::count_if(materials.begin(),materials.end(),[](const Material& m){ return m.bakedPrimitive!=nullptr; }))}};
    QString result=temp.filePath(base+".mimodel");
    QFile out(result); if(!out.open(QIODevice::WriteOnly|QIODevice::NewOnly)) return err("Cannot write model file.");
    QByteArray json=QJsonDocument(model).toJson(QJsonDocument::Compact);
    if(out.write(json)!=json.size() || !out.flush()) return err("Cannot finish model file.");
    out.close(); temp.setAutoRemove(false); return result;
}
