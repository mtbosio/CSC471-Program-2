#include "ChunkData.h"
#include "FastNoiseLite.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

ChunkData::ChunkData(int x, int y, int z) : chunkX(x), chunkY(y), chunkZ(z) {
    voxels.resize(CHUNK_SIZE, std::vector<std::vector<bool>>(CHUNK_SIZE, std::vector<bool>(CHUNK_SIZE, false)));
    generateTerrain();
}

glm::vec3 ChunkData::getChunkCoords()
{
    return glm::vec3(chunkX * CHUNK_SIZE, chunkY * CHUNK_SIZE, chunkZ * CHUNK_SIZE);
}

void ChunkData::generateTerrain() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            float worldX = (chunkX * CHUNK_SIZE + x);
            float worldZ = (chunkZ * CHUNK_SIZE + z);
            float value = noise.GetNoise(worldX, worldZ);
            int height = (value + 1) * 30 - 30; 
            for (int y = 0; y < CHUNK_SIZE; y++) {
                int voxelY = chunkY * CHUNK_SIZE + y;
                if (voxelY < height) {
                    voxels[x][y][z] = true;
                } 
            }
        }
    }
}

bool ChunkData::isSolid(int x, int y, int z) const {
    // If out of bounds, return false
    if (x < 0 || x >= CHUNK_SIZE || 
        y < 0 || y >= CHUNK_SIZE || 
        z < 0 || z >= CHUNK_SIZE) {
        return false;
    }

    return voxels[x][y][z];
}
