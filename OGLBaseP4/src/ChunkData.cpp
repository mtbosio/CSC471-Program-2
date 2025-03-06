#include "FastNoiseLite.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cstdlib>
#include "ChunkData.h"
#include "World.h"

ChunkData::ChunkData(int x, int z, World* world, int seed) : chunkX(x), chunkZ(z), world(world), seed(seed) {
    voxels.resize(CHUNK_SIZE, std::vector<std::vector<int>>(CHUNK_HEIGHT, std::vector<int>(CHUNK_SIZE, 0)));
    generateTerrain();
}

glm::vec2 ChunkData::getChunkCoords()
{
    return glm::vec2(chunkX * CHUNK_SIZE, chunkZ * CHUNK_SIZE);
}

void ChunkData::generateTerrain() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    noise.SetSeed(seed);
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            float worldX = (chunkX * CHUNK_SIZE + x);
            float worldZ = (chunkZ * CHUNK_SIZE + z);
            float value = noise.GetNoise(worldX, worldZ);
            int height = (value + 1) * 30; 
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                
                int voxelY = y;
                if (voxelY == height) {
                    if(x == 0 && z == 0){
                        origin = glm::vec3(x,y,z);
                    }
                    voxels[x][y][z] = 1; // grass top block
                }
                if (voxelY < height) {
                    voxels[x][y][z] = 2; // dirt lower block
                } 
            }
        }
    }
}
void ChunkData::generateTrees() {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(3);
    noise.SetSeed(seed);
    
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int z = 0; z < CHUNK_SIZE; z++) {
            float worldX = (chunkX * CHUNK_SIZE + x);
            float worldZ = (chunkZ * CHUNK_SIZE + z);
            float value = noise.GetNoise(worldX, worldZ);
            int height = (value + 1) * 30; 
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                
                int voxelY = y;
                if (voxelY == height) {
                    if (rand() % 150 < 1) {
                        generateTree(x, y + 1, z); // Place tree on top of grass
                    }
                }
            }
        }
    }
}
void ChunkData::generateTree(int x, int y, int z) {
    int trunkHeight = rand() % 3 + 4; // Trunk height: 4-6 blocks

    // Generate trunk
    for (int i = 0; i < trunkHeight; ++i) {
        setBlock(x, y + i, z, 3);
    }

    int leafStartY = y + trunkHeight; 
    int leafRadius = 2;

    // Generate leaf layers
    for (int layer = 0; layer < 3; ++layer) {
        int currentY = leafStartY + layer;
        int currentRadius = leafRadius - layer;

        for (int dx = -currentRadius; dx <= currentRadius; ++dx) {
            for (int dz = -currentRadius; dz <= currentRadius; ++dz) {
                // Check if within a circular radius
                if (dx*dx + dz*dz <= currentRadius*currentRadius + 1) {
                    int leafX = x + dx;
                    int leafZ = z + dz;
                    // Only replace air blocks to avoid overwriting terrain
                    if (!isSolid(leafX, currentY, leafZ)) {
                        setBlock(leafX, currentY, leafZ, 4);
                    }
                
                }
            }
        }
    }
}

int ChunkData::getBlock(int x, int y, int z) const {
    if(x < 0 || x >= CHUNK_SIZE || 
       z < 0 || z >= CHUNK_SIZE ||
       y < 0 || y >= CHUNK_HEIGHT)
       return world->getBlock(chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z);
    
    return voxels[x][y][z];
}
void ChunkData::setBlock(int x, int y, int z, int type) {
    if(x < 0 || x >= CHUNK_SIZE || 
       z < 0 || z >= CHUNK_SIZE ||
       y < 0 || y >= CHUNK_HEIGHT) return world->setBlock(chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z, type);
    
    voxels[x][y][z] = type;
}

bool ChunkData::isSolid(int x, int y, int z) const {
    // If out of bounds, return false
    if (x < 0 || x >= CHUNK_SIZE || 
        y < 0 || y >= CHUNK_HEIGHT || 
        z < 0 || z >= CHUNK_SIZE) {
        return world->getBlock(chunkX * CHUNK_SIZE + x, y, chunkZ * CHUNK_SIZE + z) > 0;
    }

    return voxels[x][y][z] > 0;
}
