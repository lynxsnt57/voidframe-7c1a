#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Reader for the saved data format, independent of Qt and the world importer.
namespace ImmersiveFurniture {
struct TextureFace { std::string direction; int state = 0; std::vector<std::uint32_t> pixels; };
struct Sprite { std::string texture; float size=1; int rotation=0; bool tiled=false, item=false, mirrored=false; };
struct Element {
    enum class Kind { Cube, Sprite, Other } kind=Kind::Other;
    std::array<float,3> from{}, to{};
    float rotation=0;
    char axis='y';
    int mask=3, emission=0;
    std::uint32_t color=0xffffffffu;
    std::string transparency;
    Sprite sprite;
    std::vector<TextureFace> faces;
};
struct Model { std::string name; std::array<int,3> size{{1,1,1}}; std::vector<Element> elements; };
// Returns false on unsupported, malformed, oversized or truncated input. Error is printable.
bool ReadGzip(const void* data, std::size_t size, Model& result, std::string& error);
}
