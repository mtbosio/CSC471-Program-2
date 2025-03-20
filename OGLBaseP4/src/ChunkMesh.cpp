#include "ChunkMesh.h"
#include <glad/glad.h>

ChunkMesh::ChunkMesh(ChunkData& chunkData) : chunkData(chunkData) {
}

ChunkMesh::~ChunkMesh() {
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteVertexArrays(1, &VAO);
}

void ChunkMesh::generateMesh() {
    // go through voxels and add faces for visible blocks
    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_HEIGHT; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                int blockType = chunkData.getBlock(x,y,z);
                if(chunkData.isSolid(x,y,z)){
                    if (!chunkData.isSolid(x + 1, y, z)) addFace(x, y, z, 0, blockType); // Right
                    if (!chunkData.isSolid(x - 1, y, z)) addFace(x, y, z, 1, blockType); // Left
                    if (!chunkData.isSolid(x, y + 1, z)) addFace(x, y, z, 2, blockType); // Top
                    if (!chunkData.isSolid(x, y - 1, z)) addFace(x, y, z, 3, blockType); // Bottom
                    if (!chunkData.isSolid(x, y, z + 1)) addFace(x, y, z, 4, blockType); // Front
                    if (!chunkData.isSolid(x, y, z - 1)) addFace(x, y, z, 5, blockType); // Back
                }
                
            }
        }
    }

    // Generate VAO, VBO, EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    //glEnable(GL_CULL_FACE);    // Enable face culling
    //glCullFace(GL_BACK);       // Cull back faces
    //glFrontFace(GL_CCW);

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

void ChunkMesh::addFace(int x, int y, int z, int faceIndex, int blockType) {
    static const glm::vec3 positions[6][4] = {
        // Right (+X) - Adjusted vertex order
        { {1, 0, 0}, {1, 0, 1}, {1, 1, 1}, {1, 1, 0} },
        // Left (-X) - Adjusted vertex order
        { {0, 0, 1}, {0, 0, 0}, {0, 1, 0}, {0, 1, 1} },
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

    int atlasSize = 16; // Assume 16x16 grid in the texture atlas
    float texSize = 1.0f / atlasSize; // Size of one tile (e.g., 1/16 = 0.0625)

    glm::vec2 columnRow = getColumnRowForBlockType(blockType, faceIndex);


    glm::vec2 texOffset(columnRow.x * texSize, columnRow.y * texSize);

    int baseIndex = vertices.size();
    for (int i = 0; i < 4; i++) {
        glm::vec2 adjustedTexCoord = texCoords[i] * texSize + texOffset;
        
        vertices.push_back(Vertex(
            glm::vec3(x, y, z) + positions[faceIndex][i],
            normals[faceIndex],
            adjustedTexCoord
        ));
    }
    
    // Reverse winding order for +X and -X faces to fix backface culling
    if (faceIndex == 0 || faceIndex == 1) {
        // Triangle 1: 0 -> 2 -> 1
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 1);
        
        // Triangle 2: 0 -> 3 -> 2
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 3);
        indices.push_back(baseIndex + 2);
    } else {
        // Original winding order for other faces
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 1);
        indices.push_back(baseIndex + 2);
        
        indices.push_back(baseIndex);
        indices.push_back(baseIndex + 2);
        indices.push_back(baseIndex + 3);
    }
}

glm::vec2 ChunkMesh::getColumnRowForBlockType(int blockType, int normal) {
    switch (blockType) {
        case 1: // Grass
            switch (normal) {
                case 2: return glm::vec2(8, 13); // Top
                case 3: return glm::vec2(2, 15); // Bottom
                default: return glm::vec2(3, 15); // Sides
            }

        case 2: // Dirt
            return glm::vec2(2, 15);

        case 3: // Tree trunk
            switch (normal) {
                case 2:
                case 3: return glm::vec2(5, 14); // Top or Bottom
                default: return glm::vec2(4, 14); // Sides
            }

        case 4: // Leaves
            return glm::vec2(4, 7);

        default:
            return glm::vec2(0, 0);
    }
}
