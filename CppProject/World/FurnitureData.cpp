#include "FurnitureData.hpp"
#include <zlib.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
#include <utility>

namespace ImmersiveFurniture {
namespace {
constexpr std::size_t MAX_IN=16u*1024*1024, MAX_OUT=64u*1024*1024, MAX_ITEMS=1000000, MAX_DEPTH=64;
struct Tag {
    int type=0; double num=0; std::string str;
    std::vector<std::int32_t> ints;
    std::vector<Tag> list;
    std::map<std::string,Tag> children;
    const Tag* field(const char* k,int t) const {
        auto it=children.find(k);
        return it!=children.end() && it->second.type==t ? &it->second : nullptr;
    }
};
struct Reader {
    const std::vector<unsigned char>& bytes; std::size_t pos=0, items=0;
    [[noreturn]] void bad(const char* e) { throw std::runtime_error(e); }
    void require(std::size_t n) { if(n>bytes.size()-pos) bad("truncated NBT"); }
    std::uint64_t number(int n) { require(n); std::uint64_t v=0; while(n--)v=(v<<8)|bytes[pos++]; return v; }
    std::string text() { auto n=number(2); require(n); std::string out(reinterpret_cast<const char*>(&bytes[pos]),static_cast<std::size_t>(n));pos+=n;return out; }
    std::size_t count() { auto raw=number(4); if(raw>MAX_ITEMS || items+raw>MAX_ITEMS)bad("too many NBT items");items+=static_cast<std::size_t>(raw);return static_cast<std::size_t>(raw); }
    Tag read(int type,std::size_t depth) {
        if(depth>MAX_DEPTH || ++items>MAX_ITEMS)bad("NBT nesting or item limit exceeded");
        Tag out;out.type=type;
        switch(type) {
        case 1: out.num=static_cast<std::int8_t>(number(1));break;
        case 2: out.num=static_cast<std::int16_t>(number(2));break;
        case 3: out.num=static_cast<std::int32_t>(number(4));break;
        case 4: out.num=static_cast<double>(static_cast<std::int64_t>(number(8)));break;
        case 5: { std::uint32_t b=static_cast<std::uint32_t>(number(4));float f;std::memcpy(&f,&b,4);out.num=f;break; }
        case 6: { std::uint64_t b=number(8);double d;std::memcpy(&d,&b,8);out.num=d;break; }
        case 7: { auto n=count();require(n);pos+=n;break; }
        case 8: out.str=text();break;
        case 9: {
            int elem=static_cast<int>(number(1)); auto n=count();
            if((elem<1 || elem>12) && !(elem==0 && n==0))bad("invalid NBT list type");
            out.list.reserve(n);for(std::size_t i=0;i<n;++i)out.list.push_back(read(elem,depth+1));break;
        }
        case 10: {
            while(true){int t=static_cast<int>(number(1));if(!t)break;if(t>12)bad("invalid NBT type");std::string key=text();
                Tag val=read(t,depth+1);if(!out.children.emplace(std::move(key),std::move(val)).second)bad("duplicate NBT key");}break;
        }
        case 11: {auto n=count();if(n>(bytes.size()-pos)/4)bad("truncated int array");out.ints.reserve(n);for(std::size_t i=0;i<n;++i)out.ints.push_back(static_cast<std::int32_t>(number(4)));break;}
        case 12: {auto n=count();if(n>(bytes.size()-pos)/8)bad("truncated long array");pos+=n*8;break;}
        default:bad("invalid NBT type");
        }
        return out;
    }
};
std::vector<unsigned char> inflateBounded(const void* data,std::size_t size) {
    if(!data || !size || size>MAX_IN)throw std::runtime_error("invalid or oversized gzip");
    z_stream z{};z.next_in=const_cast<Bytef*>(static_cast<const Bytef*>(data));z.avail_in=static_cast<uInt>(size);
    if(inflateInit2(&z,15+32)!=Z_OK)throw std::runtime_error("cannot initialise gzip");
    std::vector<unsigned char> out;unsigned char buf[16384];int status=Z_OK;
    while(status==Z_OK){z.next_out=buf;z.avail_out=sizeof(buf);status=inflate(&z,Z_NO_FLUSH);
      auto n=sizeof(buf)-z.avail_out;if(n>MAX_OUT-out.size()){inflateEnd(&z);throw std::runtime_error("decompressed data too large");}
      out.insert(out.end(),buf,buf+n);
    }
    inflateEnd(&z);if(status!=Z_STREAM_END)throw std::runtime_error("invalid or truncated gzip");return out;
}
int integer(const Tag& t,const char* k,int def){auto p=t.field(k,3);return p?static_cast<int>(p->num):def;}
float number(const Tag& t,const char* k,float def){auto p=t.field(k,5);return p?static_cast<float>(p->num):def;}
std::string string(const Tag& t,const char* k,const char* def=""){auto p=t.field(k,8);return p?p->str:def;}
bool boolean(const Tag& t,const char* k){auto p=t.field(k,1);return p && p->num!=0;}
std::array<float,3> vector3(const Tag& t,const char* k){
    auto p=t.field(k,9);if(!p || p->list.size()!=3)throw std::runtime_error("invalid element coordinates");
    std::array<float,3> out;for(int i=0;i<3;i++){
      if(p->list[i].type!=5 || !std::isfinite(p->list[i].num) || std::abs(p->list[i].num)>16384)throw std::runtime_error("invalid element coordinate");
      out[i]=static_cast<float>(p->list[i].num);
    }return out;
}
void faces(const Tag* map, int defaultState, Element& e) {
 if(!map)return;
 for(const auto& kv:map->children){if(kv.second.type!=11)continue;
  auto sep=kv.first.find(':');std::string dir=kv.first.substr(0,sep);int state=defaultState;
  if(dir!="north" && dir!="south" && dir!="east" && dir!="west" && dir!="up" && dir!="down")continue;
  if(sep!=std::string::npos){auto tail=kv.first.substr(sep+1);if(tail!="0" && tail!="1")continue;state=tail=="1"?1:0;}
  TextureFace face;face.direction=dir;face.state=state;face.pixels.reserve(kv.second.ints.size());
  for(auto n:kv.second.ints) face.pixels.push_back(static_cast<std::uint32_t>(n));
  e.faces.push_back(std::move(face));
 }
}
Model decode(const Tag& root) {
 if(root.type!=10)throw std::runtime_error("NBT root is not a compound");
 Model m;m.name=string(root,"Name");m.size={{integer(root,"SizeX",1),integer(root,"SizeY",1),integer(root,"SizeZ",1)}};
 auto list=root.field("Elements",9);if(!list || list->list.size()>8192)throw std::runtime_error("invalid element list");
 for(const Tag& t:list->list){if(t.type!=10)throw std::runtime_error("invalid element");
   Element e;auto type=string(t,"Type");e.kind=type=="element"?Element::Kind::Cube:type=="sprite"?Element::Kind::Sprite:Element::Kind::Other;
   e.from=vector3(t,"From");e.to=vector3(t,"To");e.rotation=number(t,"Rotation",0);auto axis=string(t,"Axis","y");
   if(!std::isfinite(e.rotation) || (axis!="x" && axis!="y" && axis!="z"))throw std::runtime_error("invalid rotation");
   e.axis=axis[0];e.mask=integer(t,"Mask",3);e.emission=integer(t,"Emission",0);e.color=static_cast<std::uint32_t>(integer(t,"Color",-1));
   if(auto material=t.field("Material",10))e.transparency=string(*material,"Transparency","SOLID");
   if(auto s=t.field("Sprite",10)){e.sprite.texture=string(*s,"Sprite");e.sprite.size=number(*s,"Size",1);e.sprite.rotation=integer(*s,"Rotation",0);e.sprite.tiled=boolean(*s,"Tiled");e.sprite.item=boolean(*s,"Item");e.sprite.mirrored=boolean(*s,"Mirrored");}
   faces(t.field("BakedTexture",10),0,e);faces(t.field("BakedTextures",10),0,e);m.elements.push_back(std::move(e));
 }
 return m;
}
}
bool ReadGzip(const void* data,std::size_t size,Model& result,std::string& error){
 try{auto bytes=inflateBounded(data,size);Reader r{bytes};if(r.number(1)!=10)throw std::runtime_error("invalid root");r.text();auto root=r.read(10,0);if(r.pos!=bytes.size())throw std::runtime_error("extra NBT data");Model parsed=decode(root);result=std::move(parsed);error.clear();return true;}
 catch(const std::exception& ex){error=ex.what();return false;}
}
}
