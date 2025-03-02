/*
 * Example two meshes and two shaders (could also be used for Program 2)
 * includes modifications to shape and initGeom in preparation to load
 * multi shape objects 
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn
 */

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vector>
#include "ChunkData.h"
#include "ChunkMesh.h"

#include "Bezier.h"
#include "Spline.h"

using namespace std;
using namespace glm;

// Hash function for glm::vec3
struct Vec3Hash {
	size_t operator()(const vec3& v) const {
		return hash<float>()(v.x) ^ (hash<float>()(v.y) << 1) ^ (hash<float>()(v.z) << 2);
	}
};

// Equality function for unordered_map
struct Vec3Equal {
	bool operator()(const vec3& v1, const vec3& v2) const {
		return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
	}
};

class Application : public EventCallbacks {

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog;

	//Our shader program for textures
	std::shared_ptr<Program> texProg;

	// shader program for terrain
	std::shared_ptr<Program> voxelProg;

	// images
	shared_ptr<Texture> texture0;
	shared_ptr<Texture> texture1;
	shared_ptr<Texture> texture2; // block texture

	std::vector<std::shared_ptr<Shape>> meshes;

	double theta = -M_PI / 2;
	double phi = 0.0;

	// camera movement
	int radius = 1;
	vec3 eye = vec3(0,5,10);
	vec3 lookAt = vec3(0,0,-1);
	mat4 View;
	vec3 up = vec3(0,1,0);
	vec3 forward = normalize(lookAt);
    vec3 right = normalize(cross(forward, up));
	float gX = 0;
	float gZ = 0;
	float speed = 1.5f;
	// tour variables
	Spline splinepath[2];
	bool tour = false;

	// variables for creeper walking and explosion
	float timeElapsed = 0.0f;
	float creeperStartX = -20.0f;
	float creeperEndX = -18.1f;
	float creeperScale = 1.0f;
	bool exploding = false;
	float walkDuration = 4.0f;
    float explodeDuration = 0.7f;
	float delayDuration = 1.0f; 
    float creeperX = creeperStartX;

	// light data
	float lightTrans = 0;

	// randomly generated terrain data
	static const int GRID_SIZE = 3; 

	// Store chunks
	unordered_map<vec3, ChunkData*, Vec3Hash, Vec3Equal> chunks;
	unordered_map<vec3, ChunkMesh*, Vec3Hash, Vec3Equal> chunkMeshes;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		// movement
		if (key == GLFW_KEY_W && action == GLFW_REPEAT) {
			eye += forward * speed;
		}
		if (key == GLFW_KEY_A && action == GLFW_REPEAT) {
			eye -= right * speed;
		}
		if (key == GLFW_KEY_S && action == GLFW_REPEAT){
			eye -= forward * speed;
		}
		if (key == GLFW_KEY_D && action == GLFW_REPEAT){
			eye += right * speed;
		}
		if (key == GLFW_KEY_G && action == GLFW_PRESS){
			tour = !tour;
		}
		// light movement
		if (key == GLFW_KEY_Q && action == GLFW_PRESS){
			lightTrans -= 0.25;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS){
			lightTrans += 0.25;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_Z && action == GLFW_RELEASE) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
		}
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
		int width, height;

		glfwGetFramebufferSize(window, &width, &height);
		//code for pitch and yaw variablge updates

		theta -= (deltaX / width) * M_PI * 2;
		phi += (deltaY / height) * M_PI * 2;
		phi = clamp(phi, -80.0 * M_PI / 180, 80 * M_PI / 180);
		double x = radius*cos(phi)*cos(theta);
		double y = radius*sin(phi);
		double z = radius*cos(phi)*cos((M_PI/2)-theta);
		
		lookAt = vec3(x, y, z);
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.12f, .34f, .56f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		// Initialize the GLSL program.
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatShine");
		prog->addUniform("MatSpec");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");

		// Initialize the GLSL program.
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("flip");
		texProg->addUniform("Texture0");
		texProg->addUniform("MatShine");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		// voxel shaders
		voxelProg = make_shared<Program>();
		voxelProg->setVerbose(true);
		voxelProg->setShaderNames(resourceDirectory + "/voxel_vert.glsl", resourceDirectory + "/voxel_frag.glsl");
		voxelProg->init();
		voxelProg->addUniform("P");
		voxelProg->addUniform("V");
		voxelProg->addUniform("M");
		voxelProg->addUniform("flip");
		voxelProg->addUniform("Texture0");
		voxelProg->addUniform("MatShine");
		voxelProg->addUniform("lightPos");
		voxelProg->addAttribute("vertPos");
		voxelProg->addAttribute("vertNor");
		voxelProg->addAttribute("vertTex");


		//read in a load the textures
		// grass texture
		texture0 = make_shared<Texture>();
  		texture0->setFilename(resourceDirectory + "/minecraft_grass.jpg");
  		texture0->init();
  		texture0->setUnit(0);
  		texture0->setWrapModes(GL_REPEAT, GL_REPEAT);

		// sky texture
		texture1 = make_shared<Texture>();
		texture1->setFilename(resourceDirectory + "/cartoonSky.png");
		texture1->init();
		texture1->setUnit(1);
		texture1->setWrapModes(GL_REPEAT, GL_REPEAT);

		splinepath[0] = Spline(glm::vec3(-24,5,10), glm::vec3(-22,0,10), glm::vec3(-22,-5, 0), 5);
        splinepath[1] = Spline(glm::vec3(-22,-5, 0), glm::vec3(-22,-5, 0), glm::vec3(-24,-5, -10), 5);
	}

	void initGeom(const std::string& resourceDirectory) {
		std::vector<std::string> objFiles = {"cartoon_flower.obj", "steve.obj", "creeper.obj", "cube.obj", "bunnyNoNorm.obj", "minecraft_tree.obj", "sphereWTex.obj"};
	
		int count = 0;
		for (const auto& file : objFiles) {
			std::vector<tinyobj::shape_t> TOshapes;
			std::vector<tinyobj::material_t> objMaterials;
			std::string errStr;
	
			bool rc = tinyobj::LoadObj(TOshapes, objMaterials, errStr, (resourceDirectory + "/" + file).c_str());
	
			if (!rc) {
				std::cerr << "Error loading " << file << ": " << errStr << std::endl;
				continue;
			}
	
			// Process each shape
			for (auto& toShape : TOshapes) { 
				count += 1;
				shared_ptr<Shape> shape;
				if(count == 15){
					shape = std::make_shared<Shape>(true);
					cout << "Loaded textured shape!" << endl;
				} else {
					shape = std::make_shared<Shape>(false);
				}
				
				tinyobj::shape_t mutableShape = toShape; 
				shape->createShape(mutableShape);
				shape->measure();  // Computes min and max per shape
				shape->init();
	
				meshes.push_back(shape);
			}
		}
	
		// now compute global bounds for each of the objects in the scene. So if its a multi object, must do it for all objects.
		// flower 0 - 2
		float minX = meshes[0]->min.x;
		float minY = meshes[0]->min.y;
		float minZ = meshes[0]->min.z;
		float maxX = meshes[0]->max.x;
		float maxY = meshes[0]->max.y;
		float maxZ = meshes[0]->max.z;
		
		for(int i = 0; i <= 2; i++){
			minX = glm::min(meshes[i]->min.x, minX);
			minY = glm::min(meshes[i]->min.y, minY);
			minZ = glm::min(meshes[i]->min.z, minZ);
			
			maxX = glm::max(meshes[i]->max.x, maxX);
			maxY = glm::max(meshes[i]->max.y, maxY);
			maxZ = glm::max(meshes[i]->max.z, maxZ);
		}
		for(int i = 0; i <= 2; i++){
			normalizeMesh(meshes[i], vec3(minX, minY, minZ), vec3(maxX, maxY, maxZ));
		}

		// steve 3 - 8
		minX = meshes[3]->min.x;
		minY = meshes[3]->min.y;
		minZ = meshes[3]->min.z;
		maxX = meshes[3]->max.x;
		maxY = meshes[3]->max.y;
		maxZ = meshes[3]->max.z;
		
		for(int i = 3; i <= 8; i++){
			minX = glm::min(meshes[i]->min.x, minX);
			minY = glm::min(meshes[i]->min.y, minY);
			minZ = glm::min(meshes[i]->min.z, minZ);
			
			maxX = glm::max(meshes[i]->max.x, maxX);
			maxY = glm::max(meshes[i]->max.y, maxY);
			maxZ = glm::max(meshes[i]->max.z, maxZ);
		}
		
		for(int i = 3; i <= 8; i++){
			normalizeMesh(meshes[i], vec3(minX, minY, minZ), vec3(maxX, maxY, maxZ));
		}

		// creeper 9
		normalizeMesh(meshes[9], meshes[9]->min, meshes[9]->max);

		// floor 10
		normalizeMesh(meshes[10], meshes[10]->min, meshes[10]->max);

		// bunny 11
		normalizeMesh(meshes[11], meshes[11]->min, meshes[11]->max);

		// tree 12 - 13
		normalizeMesh(meshes[12], meshes[12]->min, meshes[12]->max);
		normalizeMesh(meshes[13], meshes[13]->min, meshes[13]->max);

		// sphere 14
		normalizeMesh(meshes[14], meshes[14]->min, meshes[14]->max);

		initChunks();

		std::cout << "Total shapes loaded: " << count << std::endl;
	}

	void normalizeMesh(std::shared_ptr<Shape>& shape, const glm::vec3& globalMin, const glm::vec3& globalMax) {

		glm::vec3 center = (globalMin + globalMax) * 0.5f;

		float largest_extent = glm::max(glm::max(globalMax.x - globalMin.x, 
												 globalMax.y - globalMin.y), 
												 globalMax.z - globalMin.z);
	
		float scale_factor = 2.0f / largest_extent;

		shape->setTranslation(-1.0f * center);
		shape->setScale(vec3(scale_factor));

	}

	void initChunks() {
		for (int x = -GRID_SIZE; x < GRID_SIZE; x++) {
			for (int y = -GRID_SIZE; y < GRID_SIZE; y++) {
				for (int z = -GRID_SIZE; z < GRID_SIZE; z++) {
					vec3 pos = vec3(x, y, z);
					chunks[pos] = new ChunkData(x, y, z); // Store chunk data
					chunkMeshes[pos] = new ChunkMesh(*chunks[pos]); // Store chunk mesh
					chunkMeshes[pos]->generateMesh(); // Generate mesh
				}
			}
		}
	}
	
	// Render the chunks
	void renderChunks() {
		for (const auto& pair : chunkMeshes) {
			vec3 chunkCoords = pair.first;
			ChunkMesh* mesh = pair.second;
			mat4 Model = glm::translate(mat4(1.0f), mesh->chunkData.getChunkCoords()); // Offset by chunk size
			glUniformMatrix4fv(voxelProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model));
	
			mesh->render();
		}
	}

	void updateUsingCameraPath(float frametime)  {
		if (tour) {
			if(!splinepath[0].isDone()){
					splinepath[0].update(frametime);
				eye = splinepath[0].getPosition();
			} else {
				splinepath[1].update(frametime);
				eye = splinepath[1].getPosition();
			}
			lookAt = vec3(-17.0f,-6.8,0);
			forward = normalize(lookAt - eye);
			right = normalize(cross(forward, up));
			View = glm::lookAt(eye, eye + forward, up);
		}
	}
	
	/* helper for animating steve to bend over the flower */
	void animateSteve(shared_ptr<Program> curS, std::shared_ptr<Shape> shape, float steveBendAngle) {
		mat4 hipPivot = glm::translate(glm::mat4(1.0f), vec3(0, -0.3, 0));
		mat4 hipRotation = glm::rotate(glm::mat4(1.0f), radians(steveBendAngle), vec3(0, 0, 1));
		mat4 hipTranslation = glm::translate(glm::mat4(1.0f), vec3(-17.0, -6.8, 0));
		mat4 hipTransform =  hipTranslation * hipPivot * hipRotation * glm::inverse(hipPivot) * shape->getModelMatrix();

		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(hipTransform));
	}

	/* helper function to set model trasnforms */
  	void setModel(shared_ptr<Program> curS, std::shared_ptr<Shape> shape, vec3 trans, float rotY, float rotX, float rotZ, vec3 sc) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
		mat4 RotZ = glm::rotate(glm::mat4(1.0f), rotZ, vec3(0, 0, 1));

  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), sc);

		mat4 normTransform = shape->getModelMatrix();
    
  		mat4 ctm = Trans*RotX*RotY*ScaleS * normTransform;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void SetModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}

	 //helper function to pass material data to the GPU
	 void setMaterial(shared_ptr<Program> curS, int i) {
		switch (i) {
			case 0: // Light Green
				glUniform3f(curS->getUniform("MatAmb"), 0.1f, 0.25f, 0.1f);  // Soft green ambient
				glUniform3f(curS->getUniform("MatDif"), 0.4f, 0.9f, 0.4f);   // Bright green diffuse
				glUniform3f(curS->getUniform("MatSpec"), 0.2f, 0.5f, 0.2f);  // Moderate green specular
				glUniform1f(curS->getUniform("MatShine"), 50.0f);            // Medium shininess
				break;
	
			case 1: // Light Purple
				glUniform3f(curS->getUniform("MatAmb"), 0.063, 0.038, 0.1);
				glUniform3f(curS->getUniform("MatDif"), 0.63, 0.38, 1.0);
				glUniform3f(curS->getUniform("MatSpec"), 0.3, 0.2, 0.5);
				glUniform1f(curS->getUniform("MatShine"), 4.0);
				break;
	
			case 2: // Light Blue
				glUniform3f(curS->getUniform("MatAmb"), 0.05f, 0.1f, 0.2f);   // Soft blue ambient
				glUniform3f(curS->getUniform("MatDif"), 0.4f, 0.6f, 0.9f);   // Sky blue diffuse
				glUniform3f(curS->getUniform("MatSpec"), 0.2f, 0.4f, 0.6f);  // Moderate blue specular
				glUniform1f(curS->getUniform("MatShine"), 60.0f);            // Medium shininess
				break;
	
			case 3: // Dark Blue
				glUniform3f(curS->getUniform("MatAmb"), 0.02f, 0.02f, 0.1f);  // Deep blue ambient
				glUniform3f(curS->getUniform("MatDif"), 0.1f, 0.2f, 0.8f);    // Strong blue diffuse
				glUniform3f(curS->getUniform("MatSpec"), 0.1f, 0.2f, 0.9f);   // High specular highlight
				glUniform1f(curS->getUniform("MatShine"), 100.0f);            // Higher shininess for a sharper highlight
				break;
	
			case 4: // Light Skin Color
				glUniform3f(curS->getUniform("MatAmb"), 0.2f, 0.15f, 0.1f);   // Warm skin-tone ambient
				glUniform3f(curS->getUniform("MatDif"), 0.9f, 0.7f, 0.6f);    // Soft peach diffuse
				glUniform3f(curS->getUniform("MatSpec"), 0.3f, 0.2f, 0.2f);   // Subtle specular highlight
				glUniform1f(curS->getUniform("MatShine"), 20.0f);             // Lower shininess for soft skin appearance
				break;
	
			case 5: // Brown Bunny
				glUniform3f(curS->getUniform("MatAmb"), 0.15f, 0.1f, 0.05f);  // Warm brown ambient
				glUniform3f(curS->getUniform("MatDif"), 0.5f, 0.3f, 0.1f);    // Earthy brown diffuse
				glUniform3f(curS->getUniform("MatSpec"), 0.2f, 0.1f, 0.05f);  // Soft brown specular highlight
				glUniform1f(curS->getUniform("MatShine"), 25.0f);             // Moderate shininess for fur appearance
				break;
		}
	}	

	void render(float frametime) {
		timeElapsed += 0.02f;
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width / (float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();
		forward = normalize(lookAt);
    	right = normalize(cross(forward, up));
		View = glm::lookAt(eye, eye + forward, up);
		
		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		View = glm::translate(View, vec3(gZ, 0, gX));
		updateUsingCameraPath(frametime);

		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniform3f(prog->getUniform("lightPos"), -2.0 + lightTrans, 30.0, 2.0);

		// draw flower
		setMaterial(prog, 1); // purple for flower
		setModel(prog, meshes[0], vec3(-15.8,-7.3f,0), 0, 0, 0, vec3(0.5,0.5f,0.5));
		meshes[0]->draw(prog);
		meshes[1]->draw(prog);
		meshes[2]->draw(prog);
		
		// draw bottom half of steve
		setMaterial(prog, 3); // dark blue for pants
		setModel(prog, meshes[3], vec3(-17.0f,-6.8,0), 0, 0, 0, vec3(1,1,1));
		meshes[3]->draw(prog);
		meshes[4]->draw(prog);

		// draw animated top half of steve
		float steveBendAngle = (sin(glfwGetTime()) - 1.0f) * 25.0f;
		animateSteve(prog, meshes[5], steveBendAngle);
		setMaterial(prog, 4); // skin color
		meshes[5]->draw(prog);
		meshes[6]->draw(prog);
		meshes[7]->draw(prog);
		setMaterial(prog, 2); // shirt color
		meshes[8]->draw(prog);
		
		setMaterial(prog, 0);
		// Draw animated creeper
		if (timeElapsed < walkDuration) {
			// Move creeper towards Steve
			creeperScale = 1.0f;
			float t = timeElapsed / walkDuration; 
			creeperX = (1 - t) * creeperStartX + t * creeperEndX;
		} else if (timeElapsed < walkDuration + explodeDuration) {
			// Start explosion
			exploding = true;
			float t = (timeElapsed - walkDuration) / explodeDuration;
			creeperScale = 1.0f + 0.5f * t;
		} else if (timeElapsed < walkDuration + explodeDuration + delayDuration) {
			// Delay after explosion before resetting
			exploding = false;
			creeperScale = 0;
		} else {
			// Reset after explosion and delay
			timeElapsed = 0.0f;
			exploding = false;
			creeperScale = 1.0f;
			creeperX = creeperStartX;
		}
	
		setModel(prog, meshes[9], vec3(creeperX, -6.8f, 0), 33, 0, 0, vec3(creeperScale, 1, creeperScale));
		meshes[9]->draw(prog);

		// draw bunny
		setMaterial(prog, 5);
		setModel(prog, meshes[11], vec3(-14.8,-7.3f,0), 0, 0, 0, vec3(0.5f,0.5f,0.5f));
		meshes[11]->draw(prog);

		// draw first tree
		setMaterial(prog, 5);
		setModel(prog, meshes[12], vec3(-15.0,-5.5f,-5.5), 0, 0, 0, vec3(2.5f,2.5f,2.5f));
		meshes[12]->draw(prog);
		setMaterial(prog, 0);
		meshes[13]->draw(prog);

		// draw second tree
		setMaterial(prog, 5);
		setModel(prog, meshes[12], vec3(-25.5,-4.5f,-5.5), 0, 0, 0, vec3(2.5f,2.5f,2.5f));
		meshes[12]->draw(prog);
		setMaterial(prog, 0);
		meshes[13]->draw(prog);

		// draw third tree
		setMaterial(prog, 5);
		setModel(prog, meshes[12], vec3(-14.0,-4.5f,-20.5), 0, 0, 0, vec3(2.5f,2.5f,2.5f));
		meshes[12]->draw(prog);
		setMaterial(prog, 0);
		meshes[13]->draw(prog);

		// draw fourth tree
		setMaterial(prog, 5);
		setModel(prog, meshes[12], vec3(-16.0,-4.5f,10.5), 0, 0, 0, vec3(2.5f,2.5f,2.5f));
		meshes[12]->draw(prog);
		setMaterial(prog, 0);
		meshes[13]->draw(prog);
		
		prog->unbind();

		//switch shaders to the texture mapping shader and draw big background sphere
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(texProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniformMatrix4fv(texProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model->topMatrix()));
		glUniform3f(texProg->getUniform("lightPos"), -2.0+lightTrans, 30.0, 2.0);
		glUniform1f(texProg->getUniform("MatShine"), 27.9);
		glUniform1i(texProg->getUniform("flip"), 0);
		texture1->bind(texProg->getUniform("Texture0"));
		setModel(texProg, meshes[14], vec3(0,0,0), 0, 0, 0, vec3(50.0f,50.0f,50.0f));
		meshes[14]->draw(texProg);
		texProg->unbind();

		voxelProg->bind();
		glUniformMatrix4fv(voxelProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(voxelProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniform3f(voxelProg->getUniform("lightPos"), -2.0+lightTrans, 30.0, 2.0);
		glUniform1f(voxelProg->getUniform("MatShine"), 27.9);
		glUniform1i(voxelProg->getUniform("flip"), 0);
		texture0->bind(voxelProg->getUniform("Texture0"));
		renderChunks();
		voxelProg->unbind();

		// Pop matrix stacks.
		Projection->popMatrix();
	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
				.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime);
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
