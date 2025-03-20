#pragma once
#ifndef _SHAPE_H_
#define _SHAPE_H_

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <tiny_obj_loader/tiny_obj_loader.h>

class Program;



class Shape
{
public:
	Shape(bool textured);
	virtual ~Shape();
	void createShape(tinyobj::shape_t & shape);
	void init();
	void measure();
	void draw(const std::shared_ptr<Program> prog) const;

	// Getters
    glm::mat4 getModelMatrix() const { return modelMatrix; }
    glm::vec3 getScale() const { return scale; }
    glm::vec3 getTranslation() const { return translation; }

    // Setters
    void setScale(const glm::vec3 &s) { scale = s; updateModelMatrix(); }
    void setTranslation(const glm::vec3 &t) { translation = t; updateModelMatrix(); }

	glm::vec3 min;
	glm::vec3 max;

	// Bounding sphere
    struct BoundingSphere {
        glm::vec3 center;
        float radius;
    };

    BoundingSphere boundingSphere;
	
private:
	std::vector<unsigned int> eleBuf;
	std::vector<float> posBuf;
	std::vector<float> norBuf;
	std::vector<float> texBuf;
	unsigned eleBufID;
	unsigned posBufID;
	unsigned norBufID;
	unsigned texBufID;
    unsigned vaoID;
	bool texOff;

	// Transformations
    glm::vec3 scale;
    glm::vec3 translation;
    glm::mat4 modelMatrix;

    // Updates the model matrix after transformations
    void updateModelMatrix();
	
	void calculateBoundingSphere();
};

#endif
