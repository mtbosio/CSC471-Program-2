#include "Shape.h"
#include <iostream>
#include <assert.h>

#include "GLSL.h"
#include "Program.h"

using namespace std;

Shape::Shape(bool textured) :
	eleBufID(0),
	posBufID(0),
	norBufID(0),
	texBufID(0), 
    vaoID(0),
    scale(glm::vec3(1.0f)),      // Default scale is 1 (no scaling)
    translation(glm::vec3(0.0f)) // Default translation is at origin
{
	min = glm::vec3(0);
	max = glm::vec3(0);
	texOff = !textured;
	updateModelMatrix(); // Ensure modelMatrix is set correctly
}

Shape::~Shape()
{
}

/* Copy the data from the shape to this object */
void Shape::createShape(tinyobj::shape_t &shape)
{
	posBuf = shape.mesh.positions;
	norBuf = shape.mesh.normals;
	texBuf = shape.mesh.texcoords;
	eleBuf = shape.mesh.indices;
}

/* Compute the bounding box for the shape */
void Shape::measure() {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;

    minX = minY = minZ = 1.1754E+38F;
    maxX = maxY = maxZ = -1.1754E+38F;

    // Go through all vertices to determine min and max of each dimension
    for (size_t v = 0; v < posBuf.size() / 3; v++) {
        if (posBuf[3 * v + 0] < minX) minX = posBuf[3 * v + 0];
        if (posBuf[3 * v + 0] > maxX) maxX = posBuf[3 * v + 0];

        if (posBuf[3 * v + 1] < minY) minY = posBuf[3 * v + 1];
        if (posBuf[3 * v + 1] > maxY) maxY = posBuf[3 * v + 1];

        if (posBuf[3 * v + 2] < minZ) minZ = posBuf[3 * v + 2];
        if (posBuf[3 * v + 2] > maxZ) maxZ = posBuf[3 * v + 2];
    }

    min = glm::vec3(minX, minY, minZ);
    max = glm::vec3(maxX, maxY, maxZ);
}

/* Initialize OpenGL buffers */
void Shape::init()
{
    // Initialize the vertex array object
    glGenVertexArrays(1, &vaoID);
    glBindVertexArray(vaoID);

    // Send the position array to the GPU
    glGenBuffers(1, &posBufID);
    glBindBuffer(GL_ARRAY_BUFFER, posBufID);
    glBufferData(GL_ARRAY_BUFFER, posBuf.size() * sizeof(float), &posBuf[0], GL_STATIC_DRAW);

    // Send the normal array to the GPU
    if (norBuf.empty()) {
        norBuf.resize(posBuf.size(), 0.0f); // Same size as posBuf, initialized to 0

        for (size_t i = 0; i < eleBuf.size(); i += 3) {
            // vertex indices
            unsigned int idx0 = eleBuf[i];
            unsigned int idx1 = eleBuf[i + 1];
            unsigned int idx2 = eleBuf[i + 2];

            // vertex positions
            glm::vec3 v0(posBuf[3 * idx0], posBuf[3 * idx0 + 1], posBuf[3 * idx0 + 2]);
            glm::vec3 v1(posBuf[3 * idx1], posBuf[3 * idx1 + 1], posBuf[3 * idx1 + 2]);
            glm::vec3 v2(posBuf[3 * idx2], posBuf[3 * idx2 + 1], posBuf[3 * idx2 + 2]);

            // compute face normal
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

            // add face normal to each vertex normal
            for (int j = 0; j < 3; j++) {
                norBuf[3 * idx0 + j] += faceNormal[j];
                norBuf[3 * idx1 + j] += faceNormal[j];
                norBuf[3 * idx2 + j] += faceNormal[j];
            }
        }

        // normalize
        for (size_t i = 0; i < norBuf.size(); i += 3) {
            glm::vec3 normal(norBuf[i], norBuf[i + 1], norBuf[i + 2]);
            normal = glm::normalize(normal);
            norBuf[i] = normal.x;
            norBuf[i + 1] = normal.y;
            norBuf[i + 2] = normal.z;
        }
    }
    
    glGenBuffers(1, &norBufID);
    glBindBuffer(GL_ARRAY_BUFFER, norBufID);
    glBufferData(GL_ARRAY_BUFFER, norBuf.size() * sizeof(float), &norBuf[0], GL_STATIC_DRAW);
    

    // Send the texture array to the GPU - for now no textures
    if (texBuf.empty() || texOff) {
        texBufID = 0;
        cout << "warning no textures!" << endl;
    } else {
        glGenBuffers(1, &texBufID);
        glBindBuffer(GL_ARRAY_BUFFER, texBufID);
        glBufferData(GL_ARRAY_BUFFER, texBuf.size() * sizeof(float), &texBuf[0], GL_STATIC_DRAW);
    }

    // Send the element array to the GPU
    glGenBuffers(1, &eleBufID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, eleBuf.size() * sizeof(unsigned int), &eleBuf[0], GL_STATIC_DRAW);

    // Unbind the arrays
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //assert(glGetError() == GL_NO_ERROR);
}

/* Draw the shape */
void Shape::draw(const shared_ptr<Program> prog) const
{
    int h_pos, h_nor, h_tex;
    h_pos = h_nor = h_tex = -1;

    glBindVertexArray(vaoID);

    // Bind position buffer
    h_pos = prog->getAttribute("vertPos");
    GLSL::enableVertexAttribArray(h_pos);
    glBindBuffer(GL_ARRAY_BUFFER, posBufID);
    glVertexAttribPointer(h_pos, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);

    // Bind normal buffer
    h_nor = prog->getAttribute("vertNor");
    if (h_nor != -1 && norBufID != 0) {
        GLSL::enableVertexAttribArray(h_nor);
        glBindBuffer(GL_ARRAY_BUFFER, norBufID);
        glVertexAttribPointer(h_nor, 3, GL_FLOAT, GL_FALSE, 0, (const void *)0);
    }

    if (texBufID != 0) {
        // Bind texcoords buffer
        h_tex = prog->getAttribute("vertTex");
        if (h_tex != -1 && texBufID != 0) {
            GLSL::enableVertexAttribArray(h_tex);
            glBindBuffer(GL_ARRAY_BUFFER, texBufID);
            glVertexAttribPointer(h_tex, 2, GL_FLOAT, GL_FALSE, 0, (const void *)0);
        }
    }

    // Bind element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eleBufID);

    // Draw
    glDrawElements(GL_TRIANGLES, (int)eleBuf.size(), GL_UNSIGNED_INT, (const void *)0);

    // Disable and unbind
    if (h_tex != -1) {
        GLSL::disableVertexAttribArray(h_tex);
    }
    if (h_nor != -1) {
        GLSL::disableVertexAttribArray(h_nor);
    }
    GLSL::disableVertexAttribArray(h_pos);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/* Update the model matrix based on scale and translation */
void Shape::updateModelMatrix() {
    modelMatrix =  glm::scale(glm::mat4(1.0f), scale) * glm::translate(glm::mat4(1.0f), translation);
}
