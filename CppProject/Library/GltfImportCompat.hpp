// Internal glTF compatibility helpers. Included after cgltf implementation.
#pragma once
#include <QUrl>
#include <QMessageBox>
#include <cstring>
#include <limits>
namespace {
bool localResource(const QString& root, const char* uri, QByteArray& bytes, QString& error) {
    const QString text=QString::fromUtf8(uri ? uri : "");
    if(text.startsWith("data:")) {
        int comma=text.indexOf(',');
        if(comma<0 || !text.left(comma).endsWith(";base64") || text.size()>180*1024*1024) {
            error="Invalid or oversized base64 resource."; return false;
        }
        auto decoded=QByteArray::fromBase64Encoding(text.mid(comma+1).toLatin1(),QByteArray::AbortOnBase64DecodingErrors);
        if(!decoded || decoded.decoded.size()>128*1024*1024) { error="Invalid or oversized base64 resource."; return false; }
        bytes=decoded.decoded; return true;
    }
    QUrl url(text);
    QString relative=QUrl::fromPercentEncoding(text.toUtf8());
    if(!url.isRelative() || url.hasQuery() || url.hasFragment() || relative.contains(':') || relative.startsWith('/') || relative.startsWith('\\')) {
        error="Only relative local resource paths are supported."; return false;
    }
    QString base=QFileInfo(root).canonicalFilePath();
    QString path=QFileInfo(QDir(root).filePath(relative)).canonicalFilePath();
    if(base.isEmpty() || path.isEmpty() || !path.startsWith(base+"/",Qt::CaseInsensitive)) {
        error="Resource is missing or outside the model folder: "+text.left(160); return false;
    }
    QFile f(path);
    if(!f.open(QIODevice::ReadOnly) || f.size()>128LL*1024*1024) { error="Cannot read resource or resource exceeds 128 MiB."; return false; }
    bytes=f.readAll();
    if(bytes.size()!=f.size()) { error="Incomplete resource read."; return false; }
    return true;
}
// Materialize sparse data in its ORIGINAL component format, preserving integer
// indices and normalized weights. Sparse values are tightly packed, not strided
// like the optional base accessor (cgltf 1.15 unpack_floats does not handle that).
bool expandSparse(cgltf_data* data, std::vector<QByteArray>& storage,
                  std::vector<cgltf_buffer_view>& views, QString& error) {
    storage.resize(data->accessors_count); views.resize(data->accessors_count);
    size_t budget=0;
    for(size_t ai=0;ai<data->accessors_count;ai++) {
        auto& a=data->accessors[ai]; if(!a.is_sparse) continue;
        size_t element=cgltf_calc_size(a.type,a.component_type);
        if(!element || a.count>1000000 || a.count>size_t(128*1024*1024)/element || budget+a.count*element>128*1024*1024) {
            error="Sparse data exceeds the 128 MiB expansion budget."; return false;
        }
        auto& s=a.sparse;
        if(s.count>a.count || !s.indices_buffer_view || !s.values_buffer_view) { error="Invalid sparse accessor."; return false; }
        auto fits=[](size_t offset,size_t count,size_t stride,size_t width,size_t size) {
            return offset<=size && (!count || (width<=size-offset && (count-1)<=(size-offset-width)/stride));
        };
        if(a.buffer_view && (!a.stride || a.stride<element || !fits(a.offset,a.count,a.stride,element,a.buffer_view->size))) { error="Invalid sparse base range."; return false; }
        size_t iw=cgltf_component_size(s.indices_component_type);
        if((s.indices_component_type!=cgltf_component_type_r_8u && s.indices_component_type!=cgltf_component_type_r_16u && s.indices_component_type!=cgltf_component_type_r_32u) ||
           !fits(s.indices_byte_offset,s.count,iw,iw,s.indices_buffer_view->size) ||
           !fits(s.values_byte_offset,s.count,element,element,s.values_buffer_view->size)) { error="Invalid sparse values range."; return false; }
        const auto* indices=cgltf_buffer_view_data(s.indices_buffer_view);
        const auto* values=cgltf_buffer_view_data(s.values_buffer_view);
        const auto* base=a.buffer_view ? cgltf_buffer_view_data(a.buffer_view) : nullptr;
        if(!indices || !values || (a.buffer_view && !base)) { error="Missing sparse buffer."; return false; }
        auto& out=storage[ai]; out=QByteArray(int(a.count*element),char(0)); budget+=size_t(out.size());
        if(base) for(size_t i=0;i<a.count;i++) std::memcpy(out.data()+i*element,base+a.offset+i*a.stride,element);
        size_t previous=0;
        for(size_t i=0;i<s.count;i++) {
            size_t index=cgltf_component_read_index(indices+s.indices_byte_offset+i*iw,s.indices_component_type);
            if(index>=a.count || (i && index<=previous)) { error="Sparse indices must be increasing and inside the accessor."; return false; }
            previous=index; std::memcpy(out.data()+index*element,values+s.values_byte_offset+i*element,element);
        }
        auto& view=views[ai]; view={}; view.data=out.data(); view.size=size_t(out.size());
        a.buffer_view=&view; a.offset=0; a.stride=element; a.is_sparse=0;
    }
    return true;
}
const cgltf_texture_view* sharedUV(const cgltf_material* m, QString& error) {
    if(!m) return nullptr;
    const cgltf_texture_view* views[]={m->has_pbr_specular_glossiness ? &m->pbr_specular_glossiness.diffuse_texture : &m->pbr_metallic_roughness.base_color_texture,
        m->has_pbr_specular_glossiness ? &m->pbr_specular_glossiness.specular_glossiness_texture : &m->pbr_metallic_roughness.metallic_roughness_texture,&m->emissive_texture, m->has_transmission ? &m->transmission.transmission_texture : &m->emissive_texture};
    const cgltf_texture_view* first=nullptr;
    auto channel=[](const cgltf_texture_view* v) { return v->has_transform && v->transform.has_texcoord ? v->transform.texcoord : v->texcoord; };
    auto transform=[](const cgltf_texture_view* v) {
        cgltf_texture_transform t={}; t.scale[0]=t.scale[1]=1;
        return v->has_transform ? v->transform : t;
    };
    for(auto v:views) if(v->texture) {
        auto t=transform(v);
        if(channel(v)<0 || !std::isfinite(t.rotation) || !std::isfinite(t.offset[0]) || !std::isfinite(t.offset[1]) || !std::isfinite(t.scale[0]) || !std::isfinite(t.scale[1])) { error="Invalid texture coordinates or transform."; return nullptr; }
        if(first) {
            auto f=transform(first);
            if(channel(v)!=channel(first) || t.rotation!=f.rotation || t.offset[0]!=f.offset[0] || t.offset[1]!=f.offset[1] || t.scale[0]!=f.scale[0] || t.scale[1]!=f.scale[1]) {
                error="Material maps use different UV layouts; bake them to a shared UV set before import."; return nullptr;
            }
        } else first=v;
    }
    return first;
}
void transformUV(const cgltf_texture_view* view,float* uv) {
    if(!view || !view->has_transform) return;
    const auto& t=view->transform;
    float x=uv[0]*t.scale[0], y=uv[1]*t.scale[1];
    uv[0]=t.offset[0]+std::cos(t.rotation)*x-std::sin(t.rotation)*y;
    uv[1]=t.offset[1]+std::sin(t.rotation)*x+std::cos(t.rotation)*y;
}
bool confirmImport(const QStringList& warnings) {
#ifndef MINEIMATOR_GLTF_TESTING
    if(!warnings.isEmpty()) return QMessageBox::question(QApplication::activeWindow(),"glTF import report",
        warnings.join("\n\n")+"\n\nImport with these limitations?",QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel)==QMessageBox::Yes;
#endif
    return true;
}
}
