// Private implementation: include inside GltfMeshImport.cpp's anonymous namespace,
// AFTER readImage(), materialImage() and pbrImage(). No public API changes.
#pragma once
struct GltfBakeMap {
    const cgltf_texture_view* view=nullptr;
    const cgltf_accessor* uv=nullptr;
    QImage image;
};
struct GltfUvBake {
    GltfBakeMap maps[4]; // diffuse/base, MR/spec-gloss, emission, transmission
    const cgltf_texture_view* common=nullptr;
    bool needed=false;
    int grid=0, tile=0, side=0, resolution=64;
};
int gltfUvChannel(const cgltf_texture_view* v) {
    return v->has_transform && v->transform.has_texcoord ? v->transform.texcoord : v->texcoord;
}
bool gltfUvEqual(const cgltf_texture_view* a,const cgltf_texture_view* b) {
    if(gltfUvChannel(a)!=gltfUvChannel(b)) return false;
    const float ar=a->has_transform?a->transform.rotation:0.f, br=b->has_transform?b->transform.rotation:0.f;
    if(ar!=br) return false;
    for(int c=0;c<2;c++) {
        if((a->has_transform?a->transform.offset[c]:0.f)!=(b->has_transform?b->transform.offset[c]:0.f) ||
           (a->has_transform?a->transform.scale[c]:1.f)!=(b->has_transform?b->transform.scale[c]:1.f)) return false;
    }
    return true;
}
bool gltfPrepareUv(const cgltf_primitive& p,const cgltf_accessor* pos,GltfUvBake& b,QString& error) {
    if(!pos || pos->type!=cgltf_type_vec3) { error="Missing or invalid POSITION attribute."; return false; }
    const auto* m=p.material;
    if(!m) return true;
    b.maps[0].view=m->has_pbr_specular_glossiness?&m->pbr_specular_glossiness.diffuse_texture:&m->pbr_metallic_roughness.base_color_texture;
    b.maps[1].view=m->has_pbr_specular_glossiness?&m->pbr_specular_glossiness.specular_glossiness_texture:&m->pbr_metallic_roughness.metallic_roughness_texture;
    b.maps[2].view=&m->emissive_texture;
    b.maps[3].view=m->has_transmission?&m->transmission.transmission_texture:nullptr;
    for(auto& map:b.maps) {
        auto v=map.view;
        if(!v || !v->texture) { map.view=nullptr; continue; }
        int channel=gltfUvChannel(v);
        if(channel<0) { error="Negative texture-coordinate channel."; return false; }
        if(v->has_transform) {
            const auto& t=v->transform;
            if(!std::isfinite(t.rotation) || !std::isfinite(t.offset[0]) || !std::isfinite(t.offset[1]) ||
               !std::isfinite(t.scale[0]) || !std::isfinite(t.scale[1])) { error="Non-finite texture transform."; return false; }
        }
        map.uv=attr(p,cgltf_attribute_type_texcoord,channel);
        if(!map.uv || map.uv->type!=cgltf_type_vec2 || map.uv->count!=pos->count) {
            error="A material texture references a missing or invalid TEXCOORD_"+QString::number(channel)+" attribute."; return false;
        }
        if(!b.common) b.common=v;
        else if(!gltfUvEqual(b.common,v)) b.needed=true;
        auto sampler=v->texture->sampler;
        if(sampler) {
            const int wraps[]={int(sampler->wrap_s),int(sampler->wrap_t)};
            for(int wrap:wraps) {
                if(wrap!=10497 && wrap!=33071 && wrap!=33648) { error="Unsupported texture wrap value."; return false; }
                if(wrap!=10497) b.needed=true; // CLAMP and MIRROR need baking too.
            }
        }
    }
    return true;
}
// Independent islands avoid overlap ambiguities between UV sets. Atlas geometry
// is UV-only: positions/normals, index order and all skin influences are unchanged.
// Bounded, adaptive resolution: normally <=2048, up to4096 for dense primitives.
// Very dense meshes have small tiles; the import report explicitly warns of loss.
bool gltfAtlasLayout(size_t triangles,GltfUvBake& b,QString& error) {
    if(!triangles || triangles>MaxVertices/3) { error="Invalid UV bake triangle count."; return false; }
    b.grid=int(std::ceil(std::sqrt(double(triangles))));
    b.tile=std::max(4,std::min(2048,b.resolution/b.grid));
    b.side=b.grid*b.tile;
    if(b.side>4096) { error="UV atlas exceeds 4096 pixels."; return false; }
    return true;
}
void gltfAtlasUv(size_t triangle,size_t corner,const GltfUvBake& b,float* uv) {
    const int x=int(triangle%size_t(b.grid))*b.tile, y=int(triangle/size_t(b.grid))*b.tile;
    const float low=1.5f, high=float(b.tile)-1.5f;
    uv[0]=(x+(corner==1?high:low))/float(b.side);
    uv[1]=(y+(corner==2?high:low))/float(b.side);
}
size_t gltfSourceVertex(const cgltf_primitive& p,size_t triangle,size_t corner) {
    size_t i=triangle*3+corner;
    if(p.type==cgltf_primitive_type_triangle_strip) {
        i=triangle+corner;
        if((triangle&1) && corner<2) i=triangle+1-corner;
    } else if(p.type==cgltf_primitive_type_triangle_fan) i=corner==0?0:triangle+corner;
    return p.indices?cgltf_accessor_read_index(p.indices,i):i;
}
int gltfWrapPixel(int x,int n,int mode) {
    if(mode==33071) return std::max(0,std::min(n-1,x));
    if(mode==33648) { int t=x%(n*2); if(t<0) t+=n*2; return t<n?t:2*n-1-t; }
    int t=x%n; return t<0?t+n:t;
}
// Normalize BEFORE integer conversion; huge finite UVs must not overflow ints.
double gltfWrapCoord(double x,int mode) {
    if(mode==33071) return std::max(0.,std::min(1.,x));
    const double period=mode==33648?2.:1.;
    double t=std::fmod(x,period); return t<0?t+period:t;
}
QRgb gltfSampleMap(const GltfBakeMap& map,double u,double v) {
    if(!map.view) return qRgba(255,255,255,255);
    const auto* sampler=map.view->texture->sampler;
    const int ws=sampler?int(sampler->wrap_s):10497, wt=sampler?int(sampler->wrap_t):10497;
    const QImage& im=map.image;
    const double x=gltfWrapCoord(u,ws)*im.width()-.5, y=gltfWrapCoord(v,wt)*im.height()-.5;
    // Use magnification filter. Mipmap/anisotropic footprints cannot be preserved
    // by a finite-resolution offline conversion into this renderer's UV stream.
    if(sampler && int(sampler->mag_filter)==9728) return im.pixel(
        gltfWrapPixel(int(std::floor(x+.5)),im.width(),ws),gltfWrapPixel(int(std::floor(y+.5)),im.height(),wt));
    const int ix=int(std::floor(x)), iy=int(std::floor(y));
    const double fx=x-ix, fy=y-iy;
    const QRgb c[]={im.pixel(gltfWrapPixel(ix,im.width(),ws),gltfWrapPixel(iy,im.height(),wt)),
        im.pixel(gltfWrapPixel(ix+1,im.width(),ws),gltfWrapPixel(iy,im.height(),wt)),
        im.pixel(gltfWrapPixel(ix,im.width(),ws),gltfWrapPixel(iy+1,im.height(),wt)),
        im.pixel(gltfWrapPixel(ix+1,im.width(),ws),gltfWrapPixel(iy+1,im.height(),wt))};
    const double w[]={(1-fx)*(1-fy),fx*(1-fy),(1-fx)*fy,fx*fy};
    double r=0,g=0,bl=0,a=0;
    for(int k=0;k<4;k++) { r+=qRed(c[k])*w[k]; g+=qGreen(c[k])*w[k]; bl+=qBlue(c[k])*w[k]; a+=qAlpha(c[k])*w[k]; }
    return qRgba(qBound(0,int(r+.5),255),qBound(0,int(g+.5),255),qBound(0,int(bl+.5),255),qBound(0,int(a+.5),255));
}
bool gltfBakePrimitive(const cgltf_primitive& p,GltfUvBake& b,size_t count,size_t& texturePixels,
                       QImage& colour,QImage& properties,QString& error) {
    const auto* m=p.material;
    const bool sg=m->has_pbr_specular_glossiness;
    const float* f=sg?m->pbr_specular_glossiness.diffuse_factor:m->pbr_metallic_roughness.base_color_factor;
    for(int i=0;i<4;i++) if(!unit(f[i])) { error="Invalid base colour factor."; return false; }
    const float metal=sg?0.f:m->pbr_metallic_roughness.metallic_factor;
    const float rough=sg?m->pbr_specular_glossiness.glossiness_factor:m->pbr_metallic_roughness.roughness_factor;
    if(!unit(metal)||!unit(rough)) { error="Invalid metallic or roughness factor."; return false; }
    for(int i=0;i<3;i++) if(!unit(m->emissive_factor[i])) { error="Invalid emissive factor."; return false; }
    const bool mask=m->alpha_mode==cgltf_alpha_mode_mask, blend=m->alpha_mode==cgltf_alpha_mode_blend;
    if(mask && !unit(m->alpha_cutoff)) { error="Invalid alpha cutoff."; return false; }
    if(m->has_transmission && !unit(m->transmission.transmission_factor)) { error="Invalid transmission factor."; return false; }
    size_t decoded=0;
    for(auto& map:b.maps) if(map.view) {
        map.image=readImage(map.view,error);
        if(!error.isEmpty()) return false;
        if(map.image.isNull()) { error="Cannot decode a UV bake source image."; return false; }
        b.resolution=std::max(b.resolution,std::min(2048,std::max(map.image.width(),map.image.height())));
        decoded+=size_t(map.image.width())*size_t(map.image.height());
        if(decoded>32u*1024u*1024u) { error="UV bake source images exceed 32 megapixels."; return false; }
    }
    if(!gltfAtlasLayout(count/3,b,error)) return false;
    const size_t pixels=2*size_t(b.side)*size_t(b.side);
    if(texturePixels+pixels>32u*1024u*1024u) { error="UV atlases exceed the existing 32-megapixel texture budget; split the model or reduce its materials."; return false; }
    colour=QImage(b.side,b.side,QImage::Format_ARGB32);
    properties=QImage(b.side,b.side,QImage::Format_ARGB32);
    if(colour.isNull()||properties.isNull()) { error="Cannot allocate UV atlas."; return false; }
    colour.fill(Qt::transparent); properties.fill(qRgb(0,0,0));
    const auto* pos=attr(p,cgltf_attribute_type_position);
    for(size_t tri=0;tri<count/3;tri++) {
        double uv[4][3][2]={};
        for(size_t k=0;k<3;k++) {
            const size_t ix=gltfSourceVertex(p,tri,k);
            if(ix>=pos->count) { error="UV bake index outside vertex array."; return false; }
            for(int j=0;j<4;j++) if(b.maps[j].view) {
                float t[2];
                if(!cgltf_accessor_read_float(b.maps[j].uv,ix,t,2)) { error="Cannot read UV bake coordinates."; return false; }
                transformUV(b.maps[j].view,t);
                if(!std::isfinite(t[0])||!std::isfinite(t[1]) || std::abs(t[0])>10000 || std::abs(t[1])>10000) {
                    error="Invalid or excessive UV bake coordinates."; return false;
                }
                uv[j][k][0]=t[0]; uv[j][k][1]=t[1];
            }
        }
        const int ox=int(tri%size_t(b.grid))*b.tile, oy=int(tri/size_t(b.grid))*b.tile;
        for(int y=0;y<b.tile;y++) {
            QRgb* dst=reinterpret_cast<QRgb*>(colour.scanLine(oy+y))+ox;
            QRgb* prop=reinterpret_cast<QRgb*>(properties.scanLine(oy+y))+ox;
            for(int x=0;x<b.tile;x++) {
                // Extend the nearest barycentric edge into the gutter and unused
                // half-tile. This prevents neighbour colours at bilinear edges.
                double w[3]={1.-double(x+y-2)/(b.tile-3),double(x-1)/(b.tile-3),double(y-1)/(b.tile-3)};
                double sum=0; for(double& v:w) { v=std::max(0.,v); sum+=v; }
                for(double& v:w) v/=sum;
                QRgb tex[4];
                for(int j=0;j<4;j++) {
                    double u=0,v=0;
                    for(int k=0;k<3;k++) { u+=uv[j][k][0]*w[k]; v+=uv[j][k][1]*w[k]; }
                    tex[j]=gltfSampleMap(b.maps[j],u,v);
                }
                const QRgb c=tex[0], mr=tex[1], em=tex[2];
                float coverage=blend?qAlpha(c)/255.f*f[3]:(mask && qAlpha(c)/255.f*f[3]<m->alpha_cutoff?0.f:1.f);
                if(m->has_transmission) coverage*=1.f-m->transmission.transmission_factor*qRed(tex[3])/255.f;
                dst[x]=qRgba(qBound(0,int(qRed(c)*f[0]+.5f),255),qBound(0,int(qGreen(c)*f[1]+.5f),255),
                    qBound(0,int(qBlue(c)*f[2]+.5f),255),qBound(0,int(coverage*255.f+.5f),255));
                const float emission=std::max(qRed(em)*m->emissive_factor[0],std::max(qGreen(em)*m->emissive_factor[1],qBlue(em)*m->emissive_factor[2]));
                prop[x]=qRgb(qBound(0,int((sg?qAlpha(mr)*rough:255.f-qGreen(mr)*rough)+.5f),255),
                    qBound(0,int(qBlue(mr)*metal+.5f),255),qBound(0,int(emission+.5f),255));
            }
        }
    }
    texturePixels+=pixels;
    return true;
}
