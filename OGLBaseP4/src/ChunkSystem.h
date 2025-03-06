#pragma once
#include <unordered_map>
#include <cmath>

struct ChunkCoord {
    int x, z;
    
    bool operator==(const ChunkCoord& other) const {
        return x == other.x && z == other.z;
    }
};

namespace std {
    template<> struct hash<ChunkCoord> {
        size_t operator()(const ChunkCoord& k) const {
            size_t h1 = hash<int>()(k.x);
            size_t h2 = hash<int>()(k.z);
            return h1 ^ (h2 + 0x9e3779b9 + (h2 << 6) + (h2 >> 2));
        }
    };
}