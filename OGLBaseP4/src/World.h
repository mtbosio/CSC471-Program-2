#pragma once
#include <vector>
#include "GLSL.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ChunkSystem.h"
#include "ChunkData.h"

class World {
    static std::unordered_map<ChunkCoord, ChunkData> chunks;
public:
    ChunkCoord worldToChunk(int worldX, int worldZ);
    int getBlock(int x, int y, int z);
    void setBlock(int x, int y, int z, int blockType);
    ChunkData* getChunk(const ChunkCoord& coord);
    void addChunk(const ChunkCoord& coord);
};
