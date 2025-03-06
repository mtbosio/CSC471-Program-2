#pragma once
#include <glad/glad.h>
#include "ChunkData.h"
#include "Vertex.h"
#include <vector>

class ChunkMesh {
public:
    ChunkMesh(ChunkData& chunkData);
    ~ChunkMesh();
    
    void generateMesh();
    void render();
    ChunkData chunkData;
private:
    GLuint VAO, VBO, EBO;
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    void addFace(int x, int y, int z, int face, int blockType);
    glm::vec2 getColumnRowForBlockType(int blockType, int normal);
};
