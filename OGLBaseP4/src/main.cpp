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
#include "World.h"
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

class Application : public EventCallbacks {

public:

	WindowManager * windowManager = nullptr;

	// Our shader program
	std::shared_ptr<Program> prog;

	//Our shader program for textures
	std::shared_ptr<Program> texProg;

	// shader program for terrain
	std::shared_ptr<Program> voxelProg;

	// shader program for skybox
	std::shared_ptr<Program> skyboxProg;

	// images
	shared_ptr<Texture> texture0; // texture atlas
	shared_ptr<Texture> skyboxTexture; // skybox texture

	std::vector<std::shared_ptr<Shape>> meshes;

	double theta = - M_PI / 2;
	double phi = 0.0;

	// camera movement
	int radius = 1;
	vec3 eye = vec3(0,0,0);
	vec3 lookAt = vec3(0,0,-1);
	mat4 View;
	vec3 up = vec3(0,1,0);
	vec3 forward = normalize(lookAt);
    vec3 right = normalize(cross(forward, up));
	float gX = 0;
	float gZ = 0;
	float speed = 20.0f;
	std::unordered_map<int, bool> pressedKeys;

	// tour variables
	Spline splinepath[2];
	bool tour = false;

	// Steve position
	vec3 stevePosition = vec3(0,0,0);

	// light data
	float lightTrans = 0;

	// randomly generated terrain data
	static const int GRID_SIZE = 3; 

	// world gen
	int seed;
	World world;
	unordered_map<ChunkCoord, ChunkMesh*> chunkMeshes;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		
		// Track key states (true if pressed, false if released)
		if (action == GLFW_PRESS) {
			pressedKeys[key] = true;
		} else if (action == GLFW_RELEASE) {
			pressedKeys[key] = false;
		}

		// steve movment
		if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS){
			moveSteve(0);
		}
		if (key == GLFW_KEY_LEFT && action == GLFW_PRESS){
			moveSteve(1);
		}
		if (key == GLFW_KEY_UP && action == GLFW_PRESS){
			moveSteve(2);
		}
		if (key == GLFW_KEY_DOWN && action == GLFW_PRESS){
			moveSteve(3);
		}
		

		// tour
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

		theta -= (deltaX / width) * M_PI * 4;
		phi += (deltaY / height) * M_PI * 4;
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

	void updateMovement(float deltaTime) {
		float moveSpeed = speed * deltaTime; // Scale speed by frame time
	
		if (pressedKeys[GLFW_KEY_W]) {
			eye += forward * moveSpeed;
		}
		if (pressedKeys[GLFW_KEY_A]) {
			eye -= right * moveSpeed;
		}
		if (pressedKeys[GLFW_KEY_S]) {
			eye -= forward * moveSpeed;
		}
		if (pressedKeys[GLFW_KEY_D]) {
			eye += right * moveSpeed;
		}
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
  		texture0->setFilename(resourceDirectory + "/texture_atlas.jpg");
  		texture0->init();
  		texture0->setUnit(0);
  		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
		// change filtering to nearest
		glBindTexture(GL_TEXTURE_2D, texture0->getID());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);

		splinepath[0] = Spline(glm::vec3(-24,5,10), glm::vec3(-22,0,10), glm::vec3(-22,-5, 0), 5);
        splinepath[1] = Spline(glm::vec3(-22,-5, 0), glm::vec3(-22,-5, 0), glm::vec3(-24,-5, -10), 5);

		skyboxProg = make_shared<Program>();
		skyboxProg->setVerbose(true);
		skyboxProg->setShaderNames(resourceDirectory + "/skybox_vert.glsl", resourceDirectory + "/skybox_frag.glsl");
		skyboxProg->init();
		skyboxProg->addUniform("P");
		skyboxProg->addUniform("V");
		skyboxProg->addUniform("M");
		skyboxProg->addUniform("skybox");
		skyboxProg->addAttribute("vertPos");
		skyboxProg->addAttribute("vertNor");
		
		// Load cubemap
		vector<string> cubemapFaces = {
			resourceDirectory + "/skybox/Daylight Box_Right.bmp",
			resourceDirectory + "/skybox/Daylight Box_Left.bmp",
			resourceDirectory + "/skybox/Daylight Box_Top.bmp",
			resourceDirectory + "/skybox/Daylight Box_Bottom.bmp",
			resourceDirectory + "/skybox/Daylight Box_Front.bmp",
			resourceDirectory + "/skybox/Daylight Box_Back.bmp"
		};
		skyboxTexture = make_shared<Texture>();
		skyboxTexture->loadCubeMap(cubemapFaces);
		skyboxTexture->setUnit(0);


	}

	void initGeom(const std::string& resourceDirectory) {
		std::vector<std::string> objFiles = {"cartoon_flower.obj", "steve.obj", "creeper.obj", "cube.obj"};
	
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

		// cube 10
		normalizeMesh(meshes[10], meshes[10]->min, meshes[10]->max);

		initChunkData();
		generateTrees();
		initChunkMeshes();

		// set camera and steve at correct position
		initCameraAndSteve();

		std::cout << "Total shapes loaded: " << count << std::endl;
	}

	void initCameraAndSteve(){
		ChunkCoord origin = {0,0};
		ChunkData* chunk = world.getChunk(origin);
		eye = vec3(chunk->origin) + vec3(0, 15, 20);
		//lookAt = vec3(0,-1,0);
		//theta = 
		stevePosition = chunk->origin + vec3(0.5,2,0.5);
	}

	void initChunkData() {
		for (int x = -GRID_SIZE; x < GRID_SIZE; x++) {
			for (int z = -GRID_SIZE; z < GRID_SIZE; z++) {
				ChunkCoord pos = {x, z};
				world.addChunk(pos); // Store chunk data
			}
		}
	}

	void generateTrees(){
		for (int x = -GRID_SIZE; x < GRID_SIZE; x++) {
			for (int z = -GRID_SIZE; z < GRID_SIZE; z++) {
				ChunkCoord pos = {x, z};
				world.getChunk(pos)->generateTrees();
			}
		}
	}

	void initChunkMeshes(){
		for (int x = -GRID_SIZE; x < GRID_SIZE; x++) {
			for (int z = -GRID_SIZE; z < GRID_SIZE; z++) {
				ChunkCoord pos = {x, z};
				chunkMeshes[pos] = new ChunkMesh(*world.getChunk(pos)); // Store chunk mesh
				chunkMeshes[pos]->generateMesh(); // Generate mesh
			}
		}
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
	
	// Render the chunks
	void renderChunks() {
		for (const auto& pair : chunkMeshes) {
			ChunkCoord chunkCoords = pair.first;
			ChunkMesh* mesh = pair.second;
			mat4 Model = glm::translate(mat4(1.0f), vec3(mesh->chunkData.getChunkCoords().x, 0, mesh->chunkData.getChunkCoords().y)); // Offset by chunk size
			glUniformMatrix4fv(voxelProg->getUniform("M"), 1, GL_FALSE, value_ptr(Model));
	
			mesh->render();
		}
	}

	void moveSteve(int direction){
		int block1 = 0;
		int block2 = 0;
		int block3 = 0;
		switch (direction){
			case 0: // right
				block1 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(1,1,0)); // block to right
				block2 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(1,2,0)); // block above that one
				block3 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(1,0,0)); // block below that one
				if(block1 == 0) { // if block to the right is empty
					if(block2 == 0 && block3 > 0){ // if block above that is empty and block below is not empty
						stevePosition += vec3(1, 0, 0); // move right
					} else if (block3 == 0) { // if the block below is empty
						int counter = -1;
						while (world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(1,counter,0)) == 0){
							counter--;
						}
						stevePosition += vec3(1, counter, 0); // move right one and down as many as we need to go
					}
				} else if(block1 != 0 && block2 == 0){
					stevePosition += vec3(1, 1, 0);
				}
				return;
			case 1: // left
				block1 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(-1,1,0)); // block to left
				block2 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(-1,2,0)); // block above that one
				block3 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(-1,0,0)); // block below that one
				if(block1 == 0) { // if block to the left is empty
					if(block2 == 0 && block3 > 0){ // if block above that is empty and block below is not empty
						stevePosition += vec3(-1, 0, 0); // move left
					} else if (block3 == 0) { // if the block below is empty
						int counter = -1;
						while (world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(-1,counter,0)) == 0){
							counter--;
						}
						stevePosition += vec3(-1, counter, 0); // move left one and down as many as we need to go
					}
				} else if(block1 != 0 && block2 == 0){
					stevePosition += vec3(-1, 1, 0);
				}
			return;
			case 2: // up
				block1 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(0,1,-1)); // block to up
				block2 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(0,2,-1)); // block above that one
				block3 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(0,0,-1)); // block below that one
				if(block1 == 0) { // if block to the up is empty
					if(block2 == 0 && block3 > 0){ // if block above that is empty and block below is not empty
						stevePosition += vec3(0, 0, -1); // move up
					} else if (block3 == 0) { // if the block below is empty
						int counter = -1;
						while (world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(0,counter,-1)) == 0){
							counter--;
						}
						stevePosition += vec3(0, counter, 1); // move up one and down as many as we need to go
					}
				} else if(block1 != 0 && block2 == 0){
					stevePosition += vec3(0, 1, -1);
				}
			return;
			case 3: // down
				block1 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(0,1,1)); // block to down
				block2 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(0,2,1)); // block above that one
				block3 = world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(0,0,1)); // block below that one
				if(block1 == 0) { // if block to the down is empty
					if(block2 == 0 && block3 > 0){ // if block above that is empty and block below is not empty
						stevePosition += vec3(0, 0, 1); // move right
					} else if (block3 == 0) { // if the block below is empty
						int counter = -1;
						while (world.getBlock(stevePosition - vec3(0.5,2,0.5) + vec3(0,counter,1)) == 0){
							counter--;
						}
						stevePosition += vec3(0, counter, 1); // move right one and down as many as we need to go
					}
				} else if(block1 != 0 && block2 == 0){
					stevePosition += vec3(0, 1, 1);
				}
				return;
			default:
				return;
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

		updateMovement(frametime);
		updateUsingCameraPath(frametime);

		prog->bind();

		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(prog->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniform3f(prog->getUniform("lightPos"), -2.0 + lightTrans, 60.0, 2.0);

		// draw flower
		setMaterial(prog, 1); // purple for flower
		setModel(prog, meshes[0], vec3(-15.8,-7.3f,0), 0, 0, 0, vec3(0.5,0.5f,0.5));
		meshes[0]->draw(prog);
		meshes[1]->draw(prog);
		meshes[2]->draw(prog);
		
		// draw steve
		setMaterial(prog, 3); // dark blue for pants
		setModel(prog, meshes[3], stevePosition, 0, 0, 0, vec3(1,1,1));
		meshes[3]->draw(prog);
		meshes[4]->draw(prog);
		setMaterial(prog, 4); // skin color
		meshes[5]->draw(prog);
		meshes[6]->draw(prog);
		meshes[7]->draw(prog);
		setMaterial(prog, 2); // shirt color
		meshes[8]->draw(prog);
		
		prog->unbind();

		// draw chunks
		voxelProg->bind();
		glUniformMatrix4fv(voxelProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(voxelProg->getUniform("V"), 1, GL_FALSE, value_ptr(View));
		glUniform3f(voxelProg->getUniform("lightPos"), -2.0+lightTrans, 60.0, 2.0);
		glUniform1f(voxelProg->getUniform("MatShine"), 27.9);
		glUniform1i(voxelProg->getUniform("flip"), 0);
		texture0->bind(voxelProg->getUniform("Texture0"));
		renderChunks();
		voxelProg->unbind();
		
		// draw skybox
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);
		skyboxProg->bind();
		// Remove translation from the view matrix
		mat4 view = mat4(mat3(View)); 
		glUniformMatrix4fv(skyboxProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		glUniformMatrix4fv(skyboxProg->getUniform("V"), 1, GL_FALSE, value_ptr(view));

		setModel(skyboxProg, meshes[10], vec3(0), 0, 0, 0, vec3(1.0f)); // Scale up
		meshes[10]->draw(skyboxProg);

		skyboxProg->unbind();

		// Restore depth settings
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);

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
		World::seed = atoi(argv[1]);
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
