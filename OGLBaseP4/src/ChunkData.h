#pragma once
#include <vector>
#include "GLSL.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

const int CHUNK_SIZE = 16;

class ChunkData {
public:
    ChunkData(int chunkX, int chunkY, int chunkZ);
    glm::vec3 getChunkCoords();
    bool isSolid(int x, int y, int z) const;
    int getVoxel(int x, int y, int z);
    
private:
    int chunkX, chunkY, chunkZ;
    std::vector<std::vector<std::vector<int>>> voxels;
    void generateTerrain(); 
};
