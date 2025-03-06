#include "World.h"
#include <cstdlib>
#include <iostream>

std::unordered_map<ChunkCoord, ChunkData> World::chunks;
int World::seed = 0;
// Convert world position to chunk coordinates
ChunkCoord World::worldToChunk(int worldX, int worldZ) {
    return {
        static_cast<int>(floor(static_cast<float>(worldX) / CHUNK_SIZE)),
        static_cast<int>(floor(static_cast<float>(worldZ) / CHUNK_SIZE))
    };
}

int World::getBlock(int x, int y, int z) {
    if(y < 0 || y >= CHUNK_HEIGHT) return 0;
    
    int localX = x % CHUNK_SIZE;  
    int localZ = z % CHUNK_SIZE;

    // Fix for negative world coordinates
    if (localX < 0) localX += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;

    ChunkCoord cc = worldToChunk(x, z);
    
    ChunkData* chunk = getChunk(cc);
    if(chunk != nullptr)
    {return chunk->getBlock(localX, y, localZ);}
    return 0;
}

int World::getBlock(glm::vec3 pos) {
    return getBlock(pos.x, pos.y, pos.z);
}

void World::setBlock(int x, int y, int z, int blockType) {
    if(y < 0 || y >= CHUNK_HEIGHT) return;
    
    int localX = x % CHUNK_SIZE;  
    int localZ = z % CHUNK_SIZE;

    // Fix for negative world coordinates
    if (localX < 0) localX += CHUNK_SIZE;
    if (localZ < 0) localZ += CHUNK_SIZE;

    ChunkCoord cc = worldToChunk(x, z);
    
    ChunkData* chunk = getChunk(cc);
    if(chunk != nullptr)
    {
        chunk->setBlock(localX, y, localZ, blockType);
    }
}

ChunkData* World::getChunk(const ChunkCoord& coord) {
    auto it = chunks.find(coord);
    if(it != chunks.end()) return &it->second;
    return nullptr;
}

// Add a chunk with specified coordinates
void World::addChunk(const ChunkCoord& coord) {
    // Check if the chunk already exists
    if(chunks.find(coord) == chunks.end()) {
        // Add the chunk with appropriate initialization
        chunks.emplace(coord, ChunkData(coord.x, coord.z, this, World::seed));
    }
}
