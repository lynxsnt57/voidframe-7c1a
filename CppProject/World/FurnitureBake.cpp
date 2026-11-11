#include "FurnitureBake.hpp"
#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <utility>
namespace ImmersiveFurniture {
namespace {
struct Shelf { int y,height,next; };
struct Page { Atlas image; std::vector<Shelf> shelves; };
struct Key {
 int width,height; std::vector<std::uint32_t> pixels;
 bool operator<(const Key& x) const {return std::tie(width,height,pixels)<std::tie(x.width,x.height,x.pixels);}
};
std::pair<int,int> dimensions(const Element& e,const std::string& face) {
 int x=static_cast<int>(e.to[0]-e.from[0]);
 int y=static_cast<int>(e.to[1]-e.from[1]);
 int z=static_cast<int>(e.to[2]-e.from[2]);
 if(face=="north"||face=="south")return {x,y};
 if(face=="east"||face=="west")return {z,y};
 return {x,z};
}
const TextureFace* findFace(const Element& e,const char* name,int state) {
 const TextureFace* fallback=nullptr;
 for(const auto& f:e.faces) if(f.direction==name) {if(f.state==state)return &f;if(f.state==0)fallback=&f;}
 return fallback;
}
Rect place(const Key& key, std::vector<Page>& pages) {
 int w=key.width+2,h=key.height+2;int page=-1,x=0,y=0;
 for(std::size_t i=0;i<pages.size() && page<0;i++) {
   auto& p=pages[i];
   for(auto& shelf:p.shelves)if(h<=shelf.height && shelf.next+w<=AtlasSize){page=static_cast<int>(i);x=shelf.next;y=shelf.y;shelf.next+=w;break;}
   if(page<0){int top=0;for(const auto& s:p.shelves)top+=s.height;if(top+h<=AtlasSize){page=static_cast<int>(i);x=0;y=top;p.shelves.push_back({top,h,w});}}
 }
 if(page<0){if(pages.size()>=512)throw std::runtime_error("furniture atlas page limit reached");page=static_cast<int>(pages.size());pages.emplace_back();pages.back().shelves.push_back({0,h,w});}
 Atlas& dest=pages[page].image;
 for(int j=-1;j<=key.height;j++)for(int i=-1;i<=key.width;i++){
   int sx=std::max(0,std::min(key.width-1,i)),sy=std::max(0,std::min(key.height-1,j));
   dest.pixels[(y+j+1)*AtlasSize+x+i+1]=key.pixels[sy*key.width+sx];
 }
 return {page,x+1,y+1,key.width,key.height};
}
}
void MakeLabTubeGlassOpaque(Model& model) {
 // The three translucent elements of this specific saved model are the blue
 // glass walls. Keep fully transparent pixels invisible, including cutouts.
 for (auto& elem:model.elements) {
  if (elem.kind!=Element::Kind::Cube || elem.transparency!="TRANSLUCENT") continue;
  bool hasTransparentPixel=false;
  for (auto& face:elem.faces) for (auto& pixel:face.pixels) {
   if ((pixel & 0xff000000u)==0) hasTransparentPixel=true;
   else pixel |= 0xff000000u;
  }
  elem.transparency=hasTransparentPixel ? "CUTOUT_MIPPED" : "SOLID";
 }
}
bool Bake(const Model& model,BakedModel& result,std::string& error){
 try{
  BakedModel temp;std::vector<Page> pages;std::map<Key,Rect> cache;
  static const char* dirs[]={"north","south","east","west","up","down"};
  for(int state=0;state<2;state++)for(const auto& elem:model.elements){
   if((elem.mask&(1<<state))==0)continue;
   if(elem.kind==Element::Kind::Other){temp.ignoredElements++;continue;}
   BakedElement out;out.source=elem;out.source.faces.clear();
   if(elem.kind==Element::Kind::Cube)for(const char* dir:dirs){
    const auto* f=findFace(elem,dir,state);if(!f){temp.missingFaces++;continue;}
    auto [w,h]=dimensions(elem,dir);
    if(w<=0 || h<=0 || static_cast<std::size_t>(w)*h!=f->pixels.size())throw std::runtime_error("invalid baked face dimensions");
    // Downsample only when the face cannot fit a single atlas slot.
    int ow=std::min(w,AtlasSize-2),oh=std::min(h,AtlasSize-2);Key key{ow,oh,{}};key.pixels.reserve(ow*oh);
    for(int y=0;y<oh;y++)for(int x=0;x<ow;x++)key.pixels.push_back(f->pixels[static_cast<std::size_t>(y)*h/oh*w+static_cast<std::size_t>(x)*w/ow]);
    auto it=cache.find(key);if(it==cache.end()){Rect r=place(key,pages);it=cache.emplace(std::move(key),r).first;}
    out.faces.push_back({dir,it->second});
   }
   if(elem.kind==Element::Kind::Sprite || !out.faces.empty())temp.states[state].push_back(std::move(out));
  }
  for(auto& page:pages)temp.atlases.push_back(std::move(page.image));
  result=std::move(temp);error.clear();return true;
 }catch(const std::exception& ex){error=ex.what();return false;}
}
}
