// Last updated: 3/31/2022 7:51am Patrick Farrell
// My file reverted to its old self at 5:54ish and removed all A5 changes I am now logging backups because of this ;-;
// Last updated 1:28am 4/14/2022
// Last updated 1:29am 4/26/2022 A7

#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glm/glm.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"
#include "glm/gtc/type_ptr.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// A7 Metalic and Roughness
float metallic = 0.0;
float roughness = 0.1;

// A4 farrelp variables
float rotAngle = 0;

// A5 farrelp variables
glm::vec3 eye = glm::vec3(0,0,1);
glm::vec3 lookAt = glm::vec3(0,0,0);
glm::vec2 mousePos;

// Struct for holding vertex data
struct Vertex {
	glm::vec3 position;
	glm::vec4 color;
	// A6
	glm::vec3 normal;
	// A8
	glm::vec2 texcoords;
	glm::vec3 tangent;
};

struct PointLight {
    glm::vec4 pos;
    glm::vec4 color;
};

PointLight light;


// Struct for holding mesh data
struct Mesh {
	vector<Vertex> vertices;
	vector<unsigned int> indices;
};

// Struct for holding OpenGL mesh
struct MeshGL {
	GLuint VBO = 0;
	GLuint EBO = 0;
	GLuint VAO = 0;
	int indexCnt = 0;
};

// Read from file and dump in string
string readFileToString(string filename) {
	// Open file
	ifstream file(filename);
	// Could we open file?
	if(!file || file.fail()) {
		cerr << "ERROR: Could not open file: " << filename << endl;
		const char *m = ("ERROR: Could not open file: " + filename).c_str();
		throw runtime_error(m);
	}

	// Create output stream to receive file data
	ostringstream outS;
	outS << file.rdbuf();
	// Get actual string of file contents
	string allS = outS.str();
	// Close file
	file.close();
	// Return string
	return allS;
}

// Print out shader code
void printShaderCode(string &vertexCode, string &fragCode) {
	cout << "***********************" << endl;
	cout <<"** VERTEX SHADER CODE **" << endl;
	cout << "***********************" << endl;
	cout << vertexCode << endl;
	cout << "*************************" << endl;
	cout <<"** FRAGMENT SHADER CODE **" << endl;
	cout << "*************************" << endl;
	cout << fragCode << endl;
	cout << "*************************" << endl;
}

// GLFW error callback
static void error_callback(int error, const char* description) {
	cerr << "ERROR " << error << ": " << description << endl;
}

// GLFW setup
GLFWwindow* setupGLFW(int major, int minor, int windowWidth, int windowHeight, bool debugging) {

	// Set GLFW error callback
	glfwSetErrorCallback(error_callback);

	// (Try to) initialize GLFW
	if (!glfwInit()) {
		exit(EXIT_FAILURE);
	}

	// Force specific OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Request debugging context?
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debugging);

	// Create our GLFW window
	GLFWwindow* window = glfwCreateWindow(	windowWidth, windowHeight,
											"Patricks Object Window",
											NULL, NULL);

	// Were we able to make it?
	if (!window) {
		// Kill GLFW and exit program
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// We want to draw to this window, so make the OpenGL context associated with this window our current context.
	glfwMakeContextCurrent(window);

	// Basically, turning VSync on (so we will wait until the screen is updated once before swapping the back and front buffers
	glfwSwapInterval(1);

	// Return window
	return window;
}

// Cleanup GLFW
void cleanupGLFW(GLFWwindow* window) {
	glfwDestroyWindow(window);
	glfwTerminate();
}

// GLEW setup
void setupGLEW(GLFWwindow* window) {

	// MAC-SPECIFIC: Some issues occur with using OpenGL core and GLEW; so, we'll use the experimental version of GLEW
	glewExperimental = true;

	// (Try to) initalize GLEW
	GLenum err = glewInit();

	if (GLEW_OK != err) {
		// We couldn't start GLEW, so we've got to go.
		// Kill GLFW and get out of here
		cout << "ERROR: GLEW could not start: " << glewGetErrorString(err) << endl;
		cleanupGLFW(window);
		exit(EXIT_FAILURE);
	}

	cout << "GLEW initialized; version ";
	cout << glewGetString(GLEW_VERSION) << endl;
}

// Check OpenGL version
void checkOpenGLVersion() {
	GLint glMajor, glMinor;
	glGetIntegerv(GL_MAJOR_VERSION, &glMajor);
	glGetIntegerv(GL_MINOR_VERSION, &glMinor);
	cout << "OpenGL context version: ";
	cout << glMajor << "." << glMinor << endl;
	cout << "Supported GLSL version is ";
	cout << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

// OpenGL debugging callback
// Taken from: https://learnopengl.com/In-Practice/Debugging
void APIENTRY openGLDebugCallback(	GLenum source,
									GLenum type,
									unsigned int id,
									GLenum severity,
									GLsizei length,
									const char *message,
									const void *userParam) {
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    cout << "---------------" << endl;
    cout << "Debug message (" << id << "): " <<  message << endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: cout << "Severity: notification"; break;
    } cout << endl;
    cout << endl;
}

// Check and setup debugging
void checkAndSetupOpenGLDebugging() {
	// If we have a debug context, we can connect a callback function for OpenGL errors...
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if(flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		// Enable debug output
		glEnable(GL_DEBUG_OUTPUT);
		// Call debug output function when error occurs
    	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		// Attach error callback
    	glDebugMessageCallback(openGLDebugCallback, nullptr);
		// Control output
		// * ALL messages...
    	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		// * Only high severity errors from the OpenGL API...
		// glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DEBUG_TYPE_ERROR, GL_DEBUG_SEVERITY_HIGH, 0, nullptr, GL_TRUE);
	}
}

// GLSL Compiling/Linking Error Check
// Returns GL_TRUE if compile was successful; GL_FALSE otherwise.
GLint checkGLSLError(GLuint ID, bool isCompile) {

	GLint resultGL = GL_FALSE;
	int infoLogLength;
	char *errorMessage = nullptr;

	if(isCompile) {
		// Get the compilation status and message length
		glGetShaderiv(ID, GL_COMPILE_STATUS, &resultGL);
		glGetShaderiv(ID, GL_INFO_LOG_LENGTH, &infoLogLength);
	}
	else {
		// Get linking status and message length
		glGetProgramiv(ID, GL_LINK_STATUS, &resultGL);
		glGetProgramiv(ID, GL_INFO_LOG_LENGTH, &infoLogLength);
	}

	// Make sure length is at least one and allocate space for message
	infoLogLength = (infoLogLength > 1) ? infoLogLength : 1;
	errorMessage = new char[infoLogLength];

	// Get actual message
	if(isCompile)
		glGetShaderInfoLog(ID, infoLogLength, NULL, errorMessage);
	else
		glGetProgramInfoLog(ID, infoLogLength, NULL, errorMessage);

	// Print error message
	if(infoLogLength > 1)
		cout << errorMessage << endl;

	// Cleanup
	if(errorMessage) delete [] errorMessage;

	// Return OpenGL error
	return resultGL;
}

// Creates and compiles GLSL shader from code string; returns shader ID
GLuint createAndCompileShader(const char *shaderCode, GLenum shaderType) {
	// Create the shader ID
	GLuint shaderID = glCreateShader(shaderType);

	// Compile the vertex shader...
	cout << "Compiling shader..." << endl;
	glShaderSource(shaderID, 1, &shaderCode, NULL);
	glCompileShader(shaderID);

	// Checking result of compilation...
	GLint compileOK = checkGLSLError(shaderID, true);
	if (!compileOK || shaderID == 0) {
		glDeleteShader(shaderID);
		cout << "Error compiling shader." << endl;
		throw runtime_error("Error compiling shader.");
	}

	// Return shader ID
	return shaderID;
}

// Given a list of compiled shaders, create and link a shader program (ID returned).
GLuint createAndLinkShaderProgram(std::vector<GLuint> allShaderIDs) {

	// Create program ID and attach shaders
	cout << "Linking program..." << endl;
	GLuint programID = glCreateProgram();
	for (GLuint &shaderID : allShaderIDs) {
		glAttachShader(programID, shaderID);
	}

	// Actually link the program
	glLinkProgram(programID);

	// Detach shaders (program already linked, successful or not)
	for (GLuint &shaderID : allShaderIDs) {
		glDetachShader(programID, shaderID);
	}

	// Check linking
	GLint linkOK = checkGLSLError(programID, false);
	if (!linkOK || programID == 0) {
		glDeleteProgram(programID);
		cout << "Error linking shaders." << endl;
		throw runtime_error("Error linking shaders.");
	}

	// Return program ID
	return programID;
}

// Does the following:
// - Creates and compiles vertex and fragment shaders (from provided code strings)
// - Creates and links shader program
// - Deletes vertex and fragment shaders
GLuint initShaderProgramFromSource(string vertexShaderCode, string fragmentShaderCode) {
	GLuint vertID = 0;
	GLuint fragID = 0;
	GLuint programID = 0;

	try {
		// Create and compile shaders
		cout << "Vertex shader: ";
		vertID = createAndCompileShader(vertexShaderCode.c_str(), GL_VERTEX_SHADER);
		cout << "Fragment shader: ";
		fragID = createAndCompileShader(fragmentShaderCode.c_str(), GL_FRAGMENT_SHADER);

		// Create and link program
		programID = createAndLinkShaderProgram({ vertID, fragID });

		// Delete individual shaders
		glDeleteShader(vertID);
		glDeleteShader(fragID);

		// Success!
		cout << "Program successfully compiled and linked!" << endl;
	}
	catch (exception e) {
		// Cleanup shaders and shader program, just in case
		if (vertID) glDeleteShader(vertID);
		if (fragID) glDeleteShader(fragID);
		// Rethrow exception
		throw e;
	}

	return programID;
}

// Create very simple mesh: a quad (4 vertices, 6 indices, 2 triangles)
void createSimpleQuad(Mesh &m) {
	// Clear out vertices and elements
	m.vertices.clear();
	m.indices.clear();

	// Create four corners
	Vertex upperLeft, upperRight;
	Vertex lowerLeft, lowerRight;
	Vertex thirdPoint; // Third point for triangle farrelpA2

	// Set positions of vertices
	// Note: glm::vec3(x, y, z)
	upperLeft.position = glm::vec3(-0.5, 0.5, 0.0);
	upperRight.position = glm::vec3(0.5, 0.5, 0.0);
	lowerLeft.position = glm::vec3(-0.5, -0.5, 0.0);
	lowerRight.position = glm::vec3(0.5, -0.5, 0.0);
	thirdPoint.position = glm::vec3(-0.7, 0.2, 0.0);

	// Set vertex colors (red, green, blue, white)
	// Note: glm::vec4(red, green, blue, alpha)
	upperLeft.color = glm::vec4(1.0, 0.0, 0.0, 1.0);
	upperRight.color = glm::vec4(0.0, 1.0, 0.0, 1.0);
	lowerLeft.color = glm::vec4(0.0, 0.0, 1.0, 1.0);
	lowerRight.color = glm::vec4(1.0, 1.0, 1.0, 1.0);
	thirdPoint.color = glm::vec4(0.2, 0.2, 0.6, 1.0);

	// Add to mesh's list of vertices
	m.vertices.push_back(upperLeft);
	m.vertices.push_back(upperRight);
	m.vertices.push_back(lowerLeft);
	m.vertices.push_back(lowerRight);
	m.vertices.push_back(thirdPoint);

	// Add indices for two triangles
	m.indices.push_back(0);
	m.indices.push_back(3);
	m.indices.push_back(1);

	m.indices.push_back(0);
	m.indices.push_back(2);
	m.indices.push_back(3);

	m.indices.push_back(0);
	m.indices.push_back(4);
	m.indices.push_back(2);
}

// Create OpenGL mesh (VAO) from mesh data
void createMeshGL(Mesh &m, MeshGL &mgl) {
	// Create Vertex Buffer Object (VBO)
	glGenBuffers(1, &(mgl.VBO));
	glBindBuffer(GL_ARRAY_BUFFER, mgl.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*m.vertices.size(), m.vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// Create Vertex Array Object (VAO)
	glGenVertexArrays(1, &(mgl.VAO));

	// Enable VAO
	glBindVertexArray(mgl.VAO);

	// Enable the first two vertex attribute arrays
	glEnableVertexAttribArray(0);	// position
	glEnableVertexAttribArray(1);	// color
	glEnableVertexAttribArray(2);   // A6
	glEnableVertexAttribArray(3);   // A8
	glEnableVertexAttribArray(4);   // A8


	// Bind the VBO and set up data mappings so that VAO knows how to read it
	// 0 = pos (3 elements)
	// 1 = color (4 elements)
	glBindBuffer(GL_ARRAY_BUFFER, mgl.VBO);

	// Attribute, # of components, type, normalized?, stride, array buffer offset
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texcoords));
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));




	// Create Element Buffer Object (EBO)
	glGenBuffers(1, &(mgl.EBO));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mgl.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		m.indices.size() * sizeof(GLuint),
		m.indices.data(),
		GL_STATIC_DRAW);

	// Set index count
	mgl.indexCnt = (int)m.indices.size();

	// Unbind vertex array for now
	glBindVertexArray(0);
}

// Draw OpenGL mesh
void drawMesh(MeshGL &mgl) {
	glBindVertexArray(mgl.VAO);
	glDrawElements(GL_TRIANGLES, mgl.indexCnt, GL_UNSIGNED_INT, (void*)0);
	glBindVertexArray(0);
}

// Cleanup OpenGL mesh
void cleanupMesh(MeshGL &mgl) {

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &(mgl.VBO));
	mgl.VBO = 0;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glDeleteBuffers(1, &(mgl.EBO));
	mgl.EBO = 0;

	glBindVertexArray(0);
	glDeleteVertexArrays(1, &(mgl.VAO));
	mgl.VAO = 0;

	mgl.indexCnt = 0;
}
/*
** !ASSIGNMENTS START HERE! **
$$$$$$\ $$$$$$$$\  $$$$$$\  $$$$$$$\ $$$$$$$$\
$$  __$$\\__$$  __|$$  __$$\ $$  __$$\\__$$  __|
$$ /  \__|  $$ |   $$ /  $$ |$$ |  $$ |  $$ |
\$$$$$$\    $$ |   $$$$$$$$ |$$$$$$$  |  $$ |
\____$$\   $$ |   $$  __$$ |$$  __$$<   $$ |
$$\   $$ |  $$ |   $$ |  $$ |$$ |  $$ |  $$ |
\$$$$$$  |  $$ |   $$ |  $$ |$$ |  $$ |  $$ |
\______/   \__|   \__|  \__|\__|  \__|  \__|
*/

// A8
unsigned int loadAndCreateTexture(string filename) {
    int twidth, theight, tnumc;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* tex_image = stbi_load(filename.c_str(), &twidth, &theight, &tnumc, 0);

    if(!tex_image) {
        cout << "COULD NOT LOAD TEXTURE: " << filename << endl;
        glfwTerminate();
        exit(1);
    }

    GLenum format;
    if(tnumc == 3) {
        format = GL_RGB;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    else if(tnumc == 4) {
        format = GL_RGBA;
    }
    else {
        cout << "UNKNOWN NUMBER OF CHANNELS: " << tnumc << endl;
        glfwTerminate();
        exit(1);
    }

    unsigned int textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, twidth, theight, 0, format, 
                    GL_UNSIGNED_BYTE, tex_image);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    stbi_image_free(tex_image);

    return textureID;
}

// Patrick Farrell A3 Extract Mesh data for objects
// A6 changed
void extractMeshData(aiMesh *mesh, Mesh &m){

	m.vertices.clear();
	m.indices.clear();

	unsigned int i, j;
	for(i = 0; i < mesh -> mNumVertices; i++){
		Vertex vert;
	    aiVector3D vert_holder = mesh -> mVertices[i];
		aiVector3D vh2_val = mesh->mNormals[i];
		vert.position = glm::vec3(vert_holder.x,vert_holder.y,vert_holder.z);
		vert.color = glm::vec4(1,1,0,1);
		vert.normal = glm::vec3(vh2_val.x,vh2_val.y,vh2_val.z);
		vert.texcoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		vert.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y,mesh->mTangents[i].z);
		m.vertices.push_back(vert);
	}

	for(i = 0; i < mesh -> mNumFaces; i++){
		aiFace face = mesh -> mFaces[i];

		for(j = 0; j < face.mNumIndices; j++){
			m.indices.push_back(face.mIndices[j]);
		}
	}
}

// Patrick Farrell A4

// A4 farrelp
void aiMatToGLM4(aiMatrix4x4 &a, glm::mat4 &m) {
	for(int i = 0; i < 4; i++) {
		for(int j = 0; j < 4; j++) {
		m[j][i] = a[i][j];
		}
	}
}

glm::mat4 makeRotateZ(glm::vec3 offset){
	glm::mat4 modelz_rot(1.0);
	modelz_rot = glm::translate(-offset) * modelz_rot;
	glm::mat4 R = glm::rotate(glm::radians(rotAngle), glm::vec3(0,0,1));
	modelz_rot = R * modelz_rot;
	modelz_rot = glm::translate(offset)*modelz_rot;

	return modelz_rot;
}

void renderScene(vector<MeshGL> &allMeshes, aiNode *node, glm::mat4 parentMat, GLint modelMatLocation, int level, GLint normMatLoc,glm::mat4 viewMat){
	glm::mat4 transformation_node;
	aiMatToGLM4(node->mTransformation, transformation_node);
	glm::mat4 mat_render = parentMat * transformation_node;
	glm::mat4 R = makeRotateZ(mat_render[3]);
	glm::mat4 tmpModel = R * mat_render;
	// A6 addition
	glm::mat3 normal = glm::transpose(glm::inverse(glm::mat3(viewMat * tmpModel)));
	glUniformMatrix4fv(modelMatLocation, 1,false,glm::value_ptr(tmpModel));
	glUniformMatrix3fv(normMatLoc,1,false,glm::value_ptr(normal));
	for(int i = 0; i < node->mNumMeshes; i++){
		int index = node->mMeshes[i];
		drawMesh(allMeshes.at(index));
	}

	for(int i = 0; i < node->mNumChildren; i++){
		renderScene(allMeshes, node->mChildren[i], mat_render, modelMatLocation, level+1, normMatLoc, viewMat);
	}
}

// https://www.glfw.org/docs/3.3/input_guide.html
static void keyPressedByUser(GLFWwindow* window, int key, int scancode, int action, int mods){
	glm::vec3 camera_direction = lookAt - eye;
	float speed = 0.1;
	if(action == GLFW_PRESS || action == GLFW_REPEAT) {
		if(key == GLFW_KEY_ESCAPE){
			glfwSetWindowShouldClose(window, true);
		}
		else if(key == GLFW_KEY_J){
			rotAngle++;
		}
		else if(key == GLFW_KEY_K){
			rotAngle--;
		}

		// A5 Keys
		else if(key == GLFW_KEY_W){
			lookAt += camera_direction * speed;
			eye += camera_direction * speed;
		}
		else if(key == GLFW_KEY_S){
			lookAt -= camera_direction * speed;
			eye -= camera_direction * speed;
		}
		else if(key == GLFW_KEY_D){
			glm::vec3 move_right = glm::cross(glm::vec3(0,1,0), -camera_direction);
			lookAt += move_right * speed;
			eye += move_right * speed;

		}
		else if(key == GLFW_KEY_A){
			glm::vec3 move_left = glm::cross(glm::vec3(0,1,0), -camera_direction);
			lookAt -= move_left * speed;
			eye -= move_left * speed;
		}
		// A5 **
		// A6
		else if(key == GLFW_KEY_1){
			light.color = glm::vec4(1,1,1,1);
		}
		else if(key == GLFW_KEY_2){
			light.color = glm::vec4(1,0,0,1);
		}
		else if(key == GLFW_KEY_3){
			light.color = glm::vec4(0,1,0,1);
		}
		else if(key == GLFW_KEY_4){
			light.color = glm::vec4(0,0,1,1);
		}
		// A7
		else if(key == GLFW_KEY_V){
			metallic -= 0.1;
			metallic = max(0.0f, metallic);
		}
		else if(key == GLFW_KEY_B){
			metallic += 0.1;
			metallic = min(1.0f, metallic);
		}
		else if(key == GLFW_KEY_N){
			roughness -= 0.1;
			roughness = max(0.1f, roughness);

		}
		else if(key == GLFW_KEY_M){
			roughness += 0.1;
			roughness = min(0.7f, roughness);
		}
	}
}
// A4 ***

// A5 start farrelp

glm::mat4 makeLocalRotate(glm::vec3 offset, glm::vec3 axis, float angle){
	glm::mat4 mat(1.0);
	mat = glm::translate(-offset) * mat;
	glm::mat4 R = glm::rotate(glm::radians(angle), axis);
	mat = R * mat;
	mat = glm::translate(offset) * mat;
	return mat;
}

static void mouse_position_callback(GLFWwindow* window, double xpos, double ypos){
	glm::vec2 mouse_current_position = glm::vec2(xpos, ypos);
  glm::vec2 mouse_motion = mousePos - mouse_current_position;
  int frame_w, frame_h;
  glfwGetFramebufferSize(window, &frame_w, &frame_h);
  if(frame_w > 0 && frame_h > 0) {
    mouse_motion.x /= frame_w;
    mouse_motion.y /= frame_h;
		float xang = 30.0f * mouse_motion.x;
		float yang = 30.0f * mouse_motion.y;
		glm::vec3 camera_direction = lookAt - eye;
		glm::vec3 lx = glm::cross(glm::vec3(0,1,0), -camera_direction);
		glm::mat4 RX = makeLocalRotate(eye, glm::vec3(0,1,0), xang);
		glm::mat4 RY = makeLocalRotate(eye, lx, yang);
		glm::vec4 lookAtV = glm::vec4(lookAt, 1.0);
		lookAtV = RY * RX * lookAtV;
		lookAt = glm::vec3(lookAtV);
	}
	mousePos = glm::vec2(xpos, ypos);
}

// A5 end farrelp **


// Main
int main(int argc, char **argv) {

	// A3 Patrick Farrell MAIN SETUP Load Model
	if(argc <= 1){
		perror("Error");
		exit(EXIT_FAILURE);
	}

	Assimp::Importer object_importer;
	const aiScene* scene = object_importer.ReadFile( argv[1],
										 aiProcess_Triangulate ||
										 aiProcess_CalcTangentSpace ||
										 aiProcess_GenNormals ||
										 aiProcess_JoinIdenticalVertices );

	if(scene == NULL || scene -> mFlags & AI_SCENE_FLAGS_INCOMPLETE || scene -> mRootNode == NULL){
		perror("Error, check back 3 scene flags");
		exit(EXIT_FAILURE);
	}
	// A3 Patrick Farrell MAIN SETUP

	// A3 draw after glew setup --*--

	// Are we in debugging mode?
	bool DEBUG_MODE = true;

	// GLFW setup
	GLFWwindow* window = setupGLFW(4, 3, 800, 800, DEBUG_MODE);

	// GLEW setup
	setupGLEW(window);

	// A3 continue --*--

	// A3 Patrick Farrell
	vector<MeshGL> object_vector_s3;

	unsigned int obj_index;
	for(obj_index = 0; obj_index < scene -> mNumMeshes; obj_index++){
		Mesh obj_mesh;
		MeshGL obj_mgl;
		extractMeshData(scene -> mMeshes[obj_index], obj_mesh);
		createMeshGL(obj_mesh, obj_mgl);
		object_vector_s3.push_back(obj_mgl);
	}
	// A3

	// A4 farrelp
	glfwSetKeyCallback(window, keyPressedByUser);
	// A4

	// A5 farrelp
	double mx, my;
	glfwGetCursorPos(window, &mx, &my);
	mousePos = glm::vec2(mx, my);
	glfwSetCursorPosCallback(window, mouse_position_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// A5

	// Check OpenGL version
	checkOpenGLVersion();

	// Set up debugging (if requested)
	if(DEBUG_MODE) checkAndSetupOpenGLDebugging();

	// Set the background color to a shade of blue - Dark Green now pf !BACKGROUND
	glClearColor(0.3f, 0.5f, 0.0f, 1.0f);

	// Create and load shader
	GLuint programID = 0;
	try {
		// Load vertex shader code and fragment shader code
		string vertexCode = readFileToString("./Basic.vs");
		string fragCode = readFileToString("./Basic.fs");

		// Print out shader code, just to check
		if(DEBUG_MODE) printShaderCode(vertexCode, fragCode);

		// Create shader program from code
		programID = initShaderProgramFromSource(vertexCode, fragCode);
	}
	catch (exception e) {
		// Close program
		cleanupGLFW(window);
		exit(EXIT_FAILURE);
	}

	// A4 farrelp
	GLint modelMatLocation = glGetUniformLocation(programID, "modelMat");
	// A4

	// A5 farrelp
	GLint viewMatLocation = glGetUniformLocation(programID, "viewMat");
	GLint projMatLocation = glGetUniformLocation(programID, "projMat");
	// A5

	// A6
	GLint normMatLoc = glGetUniformLocation(programID, "normMat");

	light.pos = glm::vec4(0.5, 0.5, 0.5, 1);
	light.color = glm::vec4(1,1,1,1);
	GLint light_var_pos = glGetUniformLocation(programID, "light.pos");
    GLint light_var_col = glGetUniformLocation(programID, "light.color");
	// A6

	// A7
	GLint roughnessLocation = glGetUniformLocation(programID, "roughness");
	GLint metallicLocation = glGetUniformLocation(programID, "metallic");
	// A7

	// A8
	GLint diffuseTexLocation = glGetUniformLocation(programID, "diffuseTexture");
	GLint normalTexLocation = glGetUniformLocation(programID, "normalTexture");
	// A8

	unsigned int diffuseID = loadAndCreateTexture("./sampleModels/sky.jpg");
	unsigned int normalID = loadAndCreateTexture("./sampleModels/NormalMap.png");



	// Create simple quad
	Mesh m;
	createSimpleQuad(m);

	// Create OpenGL mesh (VAO) from data
	MeshGL mgl;
	createMeshGL(m, mgl);

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window)) {
		// Set viewport size
		int fwidth, fheight;
		glfwGetFramebufferSize(window, &fwidth, &fheight);
		glViewport(0, 0, fwidth, fheight);

		// Clear the framebuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use shader program
		glUseProgram(programID);

		// A8
		glActiveTexture(GL_TEXTURE0);
   	 	glBindTexture(GL_TEXTURE_2D, diffuseID);
		glUniform1i(diffuseTexLocation, 0);

    	glActiveTexture(GL_TEXTURE1);
    	glBindTexture(GL_TEXTURE_2D, normalID);
		glUniform1i(normalTexLocation, 1);
		// A8

		// Draw object
		// drawMesh(mgl); !REMOVED

		// Patrick Farrell A3 draw object
		/*for(obj_index = 0; obj_index < object_vector_s3.size(); obj_index++){
			drawMesh(object_vector_s3[obj_index]);
		}*/

		// A5 farrelp
		glm::mat4 viewMat = glm::lookAt(glm::vec3(eye), glm::vec3(lookAt), glm::vec3(0,1,0));
		glUniformMatrix4fv(viewMatLocation, 1, false, glm::value_ptr(viewMat));
		int fw, fh;
		glfwGetFramebufferSize(window, &fw, &fh);
		float aspectRatio = 1.0;
		if(fh > 0 && fw > 0){
			aspectRatio = fw / fh;
		}

		float fov = glm::radians(90.0);
		float near = 0.01;
		float far = 50.0;
		glm::mat4 projMat = glm::perspective(fov, aspectRatio, near, far);
		glUniformMatrix4fv(projMatLocation, 1, false, glm::value_ptr(projMat));
		// A5

		// A6
		glm::vec4 lightPosition = viewMat * light.pos;
    glUniform4fv(light_var_pos, 1, glm::value_ptr(lightPosition));
    glUniform4fv(light_var_col, 1, glm::value_ptr(light.color));
		// A6

		// A7
		glUniform1f(metallicLocation, metallic);
		glUniform1f(roughnessLocation, roughness);
		// A7

		// A4 farrelp
		renderScene(object_vector_s3, scene->mRootNode, glm::mat4(1.0), modelMatLocation, 0, normMatLoc, viewMat);
		// A4 ***

		// Swap buffers and poll for window events
		glfwSwapBuffers(window);
		glfwPollEvents();

		// Sleep for 15 ms
		this_thread::sleep_for(chrono::milliseconds(15));
	}

	// Clean up mesh
	// cleanupMesh(mgl); !REMOVED

	// Patrick Farrell A3 cleanup object
	for(obj_index = 0; obj_index < object_vector_s3.size(); obj_index++){
			drawMesh(object_vector_s3[obj_index]);
		}

	// Clean up shader programs
	glUseProgram(0);
	glDeleteProgram(programID);

	// Destroy window and stop GLFW
	cleanupGLFW(window);

	return 0;
}
