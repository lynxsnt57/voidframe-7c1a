// Bounded local glTF resources. Included after GltfImportCompat.hpp.
#pragma once
namespace {
struct GltfResources {
    std::vector<QByteArray> buffers, images, sparse;
    std::vector<cgltf_buffer_view> imageViews, sparseViews;
    std::vector<cgltf_buffer> sparseBuffers;
    bool load(cgltf_data* d,const QString& folder,size_t sourceBytes,QString& error) {
        if(d->buffers_count>1024 || d->images_count>1024 || d->accessors_count>100000) { error="Too many resources."; return false; }
        buffers.resize(d->buffers_count); images.resize(d->images_count); imageViews.resize(d->images_count);
        size_t budget=sourceBytes;
        for(size_t i=0;i<d->buffers_count;i++) {
            auto& b=d->buffers[i];
            if(b.size>128u*1024u*1024u) { error="Buffer exceeds 128 MiB."; return false; }
            if(b.uri) {
                if(!localResource(folder,b.uri,buffers[i],error)) return false;
                budget+=size_t(buffers[i].size());
                if(budget>256u*1024u*1024u || size_t(buffers[i].size())<b.size) { error="Short buffer or resource budget exceeded (256 MiB)."; return false; }
                b.data=buffers[i].data();
            } else if(i==0 && d->file_type==cgltf_file_type_glb && d->bin && b.size<=d->bin_size) b.data=const_cast<void*>(d->bin);
            else { error="Missing buffer URI or embedded buffer."; return false; }
        }
        for(size_t i=0;i<d->buffer_views_count;i++) {
            const auto& v=d->buffer_views[i];
            if(v.has_meshopt_compression) { error="EXT_meshopt_compression needs decoding; export uncompressed glTF/GLB."; return false; }
            if(!v.buffer || !v.buffer->data || v.offset>v.buffer->size || v.size>v.buffer->size-v.offset) { error="Buffer view is out of range."; return false; }
        }
        if(!expandSparse(d,sparse,sparseViews,error)) return false;
        sparseBuffers.resize(sparseViews.size());
        for(size_t i=0;i<sparseViews.size();i++) if(sparseViews[i].data) {
            auto& b=sparseBuffers[i]; b={}; b.data=sparseViews[i].data; b.size=sparseViews[i].size;
            sparseViews[i].buffer=&b;
        }
        for(size_t i=0;i<d->accessors_count;i++) {
            const auto& a=d->accessors[i]; size_t width=cgltf_calc_size(a.type,a.component_type);
            if(!width || !a.count || a.count>1000000 || !a.stride || a.stride<width || (a.buffer_view &&
               (a.offset>a.buffer_view->size || width>a.buffer_view->size-a.offset || (a.count-1)>(a.buffer_view->size-a.offset-width)/a.stride))) {
                error="Invalid accessor range or size."; return false;
            }
        }
        if(cgltf_validate(d)!=cgltf_result_success) { error="Invalid glTF structure."; return false; }
        for(size_t i=0;i<d->images_count;i++) if(d->images[i].uri) {
            if(!localResource(folder,d->images[i].uri,images[i],error)) return false;
            budget+=size_t(images[i].size());
            if(budget>256u*1024u*1024u) { error="Resource budget exceeded (256 MiB)."; return false; }
            auto& v=imageViews[i]; v={}; v.data=images[i].data(); v.size=size_t(images[i].size());
            d->images[i].buffer_view=&v;
        }
        return true;
    }
};
}
