#pragma once
#include <vector>
#include "GLSL.h"
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

class World;

const int CHUNK_SIZE = 16;
const int CHUNK_HEIGHT = 256;

class ChunkData {
public:
    ChunkData(int chunkX, int chunkZ, World* world);
    glm::vec2 getChunkCoords();
    bool isSolid(int x, int y, int z) const;
    void setBlock(int x, int y, int z, int block);
    int getBlock(int x, int y, int z) const;
    void generateTrees();
    glm::vec3 origin;
    
private:
    World* world;
    int chunkX, chunkZ;
    std::vector<std::vector<std::vector<int>>> voxels;
    void generateTerrain(); 
    void generateTree(int x, int y, int z);
};
