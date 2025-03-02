#include "ChunkMesh.h"
#include <glad/glad.h>

ChunkMesh::ChunkMesh(ChunkData& chunkData) : chunkData(chunkData){
    generateMesh();
}

ChunkMesh::~ChunkMesh() {
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

void ChunkMesh::generateMesh() {
    // go through voxels and add faces for visible blocks
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                if(chunkData.isSolid(x,y,z)){
                    if (!chunkData.isSolid(x + 1, y, z)) addFace(x, y, z, 0); // Right
                    if (!chunkData.isSolid(x - 1, y, z)) addFace(x, y, z, 1); // Left
                    if (!chunkData.isSolid(x, y + 1, z)) addFace(x, y, z, 2); // Top
                    if (!chunkData.isSolid(x, y - 1, z)) addFace(x, y, z, 3); // Bottom
                    if (!chunkData.isSolid(x, y, z + 1)) addFace(x, y, z, 4); // Front
                    if (!chunkData.isSolid(x, y, z - 1)) addFace(x, y, z, 5); // Back
                }
                
            }
        }
    }

    // Generate VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    

    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glEnableVertexAttribArray(0);

    // Normal attribute (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);

    // Texture coordinate attribute (location = 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

}


void ChunkMesh::render() {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void ChunkMesh::addFace(int x, int y, int z, int faceIndex) {
    static const glm::vec3 positions[6][4] = {
        // Right (+X)
        { {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} },
        // Left (-X)
        { {0, 0, 1}, {0, 1, 1}, {0, 1, 0}, {0, 0, 0} },
        // Top (+Y)
        { {0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0} },
        // Bottom (-Y)
        { {0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1} },
        // Front (+Z)
        { {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} },
        // Back (-Z)
        { {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0} }
    };

    static const glm::vec3 normals[6] = {
        {1, 0, 0}, {-1, 0, 0}, {0, 1, 0},
        {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
    };

    static const glm::vec2 texCoords[4] = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}
    };

    int baseIndex = vertices.size();

    // Add vertices for the face
    for (int i = 0; i < 4; i++) {
        vertices.push_back(Vertex(glm::vec3(x, y, z) + positions[faceIndex][i], normals[faceIndex], texCoords[i]));
    }

    // Add indices for two triangles (quad)
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 1);
    indices.push_back(baseIndex + 2);
    
    indices.push_back(baseIndex);
    indices.push_back(baseIndex + 2);
    indices.push_back(baseIndex + 3);
}
