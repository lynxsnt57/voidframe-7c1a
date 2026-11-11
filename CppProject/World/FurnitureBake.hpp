#pragma once
#include "FurnitureData.hpp"
#include <array>
#include <string>
#include <vector>

namespace ImmersiveFurniture {
constexpr int AtlasSize=128;
// Minecraft NativeImage stores packed pixels as AABBGGRR; Qt expects AARRGGBB.
constexpr std::uint32_t NativeAbgrToArgb(std::uint32_t p) {
  return (p & 0xff00ff00u) | ((p & 0x00ff0000u) >> 16) | ((p & 0x000000ffu) << 16);
}
struct Rect { int atlas=0,x=0,y=0,width=0,height=0; };
struct Face { std::string direction; Rect rect; };
struct BakedElement {
  Element source;
  std::vector<Face> faces;
};
struct Atlas { std::vector<std::uint32_t> pixels; Atlas():pixels(AtlasSize*AtlasSize,0){} };
struct BakedModel { std::array<std::vector<BakedElement>,2> states; std::vector<Atlas> atlases; int missingFaces=0, ignoredElements=0; };
// Visual compatibility workaround for the known Lab Tube furniture hash only.
void MakeLabTubeGlassOpaque(Model& model);
// Makes stable fixed-size atlas pages; leaves 1px extruded margins to prevent bleeding.
// Returns false without modifying result if model exceeds limits.
bool Bake(const Model& model, BakedModel& result, std::string& error);
}
