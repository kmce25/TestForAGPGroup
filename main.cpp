#pragma warning(disable: 4996)
#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

#include "rt3d.h"
#include "rt3dObjLoader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stack>
#define DEG_TO_RADIAN 0.017453293

GLuint i = 0;
bool motionBlurBool = true;
//----------------------------------------PBO 
GLuint pboTex[5];
GLuint pboTexSize;
GLuint pboBuffer;

GLuint screenHeight = 600;
GLuint screenWidth = 800;

GLubyte pick_col[800 * 600 * 3] = { 0 };


GLuint loadBitmap(char *fname);
GLuint loadCubeMap(const char *fname[6], GLuint *texID);
glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d);

using namespace std;
stack<glm::mat4> mvStack;
//------------------------------------------Shaders
GLuint phongShader;
GLuint skyboxShader;
GLuint motionBlur;

//------------------------------------------Objects
GLuint meshIndexCount = 0;
GLuint meshObjects[2];
vector<GLfloat> verts, norms, tex_coords;
vector<GLuint> indices, size;
//------------------------------------------Lights & materials
rt3d::lightStruct light =
{
	{0.8, 0.8, 0.8, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0, 1.0},
	{0.0, 0.0, 0.0, 1.0}
};

glm::vec4 lightPos(0.0f, 2.0f, 20.0f, 1.0f); 

rt3d::materialStruct currentMaterial;
rt3d::materialStruct bunnyMaterial;

rt3d::materialStruct material0 = {
	{0.2f, 0.5f, 0.2f, 1.0f}, // ambient
	{1.0f, 1.0f, 1.0f, 1.0f}, // diffuse
	{0.3f, 0.3f, 0.3f, 1.0f}, // specular
	0.2f					 // shininess
};
rt3d::materialStruct material1 = {
	{0.4f, 0.4f, 1.0f, 1.0f}, // ambient
	{0.8f, 0.8f, 0.8f, 1.0f}, // diffuse
	{0.8f, 0.8f, 0.8f, 1.0f}, // specular
	1.0f  // shininess
};


float attConstant = 1.0f;
float attLinear = 0.02f;
float attQuadratic = 0.01f;

//-----------------------------------------------------------------
glm::vec3 eye(0.0f, 3.0f, 0.0f);
glm::vec3 at(0.0f, 1.0f, -1.0f);
glm::vec3 up(0.0f, 1.0f, 0.0f);
GLfloat rotation = 0.0f;
//-----------------------------------------------------------------Textures
GLuint textures[4];
GLuint skybox[5];

//-----------------------------------------------------------------FBO, RBO for mirror

void init(void)
{
	phongShader = rt3d::initShaders("../phong.vert", "../phong.frag");
	rt3d::setLight(phongShader, light);
	rt3d::setMaterial(phongShader, material0);
	GLuint uniformIndex = glGetUniformLocation(phongShader, "attConst");
	glUniform1f(uniformIndex, attConstant);
	uniformIndex = glGetUniformLocation(phongShader, "attLinear");

	glUniform1f(uniformIndex, attLinear);
	uniformIndex = glGetUniformLocation(phongShader, "attQuadratic");
	glUniform1f(uniformIndex, attQuadratic);

	skyboxShader = rt3d::initShaders("../cubeMap.vert", "../cubeMap.frag");

	motionBlur = rt3d::initShaders("../Blur.vert", "../Blur.frag");
	//--------------------------------------------------------------------------------------------Object and bitmap load
	vector<GLfloat> verts, norms, tex_coords;
	vector<GLuint> indices;
	GLuint size;

	rt3d::loadObj("../resources/cube.obj", verts, norms, tex_coords, indices);
	size = indices.size();
	meshIndexCount = size;
	textures[0] = loadBitmap("../resources/fabric.bmp");
	meshObjects[0] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), size, indices.data());

	verts.clear();
	norms.clear();
	tex_coords.clear();
	indices.clear();

	rt3d::loadObj("../resources/bunny-5000.obj", verts, norms, tex_coords, indices);
	size = indices.size();
	meshIndexCount = size;
	meshObjects[1] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), nullptr, size, indices.data());
	   
	textures[1] = loadBitmap("../resources/alienShipWalls.bmp");

	const char *cubeTexFiles[6] = {
		"../resources/skybox/Town_bk.bmp",
		"../resources/skybox/Town_ft.bmp",
		"../resources/skybox/Town_rt.bmp",
		"../resources/skybox/Town_lf.bmp",
		"../resources/skybox/Town_up.bmp",
		"../resources/skybox/Town_dn.bmp"
	};
	loadCubeMap(cubeTexFiles, &skybox[0]);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// ---------------------------------------------------------------------------------Generating textures for PBO/motion blur
	
	glGenTextures(6, pboTex);
	GLuint pboTexSize = screenWidth * screenHeight * 3 * sizeof(GLubyte);
	void* pboData = new GLubyte[pboTexSize];
	memset(pboData, 0, pboTexSize); 
	
	for (int i = 0; i < 6; i++)
	{
		glBindTexture(GL_TEXTURE_2D, pboTex[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pboData);
	}

	uniformIndex = glGetUniformLocation(motionBlur, "textureUnit0");
	glUniform1i(uniformIndex, 0);
	uniformIndex = glGetUniformLocation(motionBlur, "textureUnit1");
	glUniform1i(uniformIndex, 1);
	uniformIndex = glGetUniformLocation(motionBlur, "textureUnit2");
	glUniform1i(uniformIndex, 2);
	uniformIndex = glGetUniformLocation(motionBlur, "textureUnit3");
	glUniform1i(uniformIndex, 3);
	uniformIndex = glGetUniformLocation(motionBlur, "textureUnit4");
	glUniform1i(uniformIndex, 4);
	uniformIndex = glGetUniformLocation(motionBlur, "textureUnit5");
	glUniform1i(uniformIndex, 5);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pboTex[0]);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pboTex[1]);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, pboTex[2]);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, pboTex[3]);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, pboTex[4]);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, pboTex[5]);


	//----------------------------------------------------------------------------------Creating PBO
	glGenBuffers(1, &pboBuffer);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
	glBufferData(GL_PIXEL_PACK_BUFFER, pboTexSize, pboData, GL_DYNAMIC_COPY);			//GLenum target, GLsizeiptr size, const void * data, GLenum usage); Size = size in bytes of the buffer object's new data store. data =  data that will be copied into the data store
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	delete pboData;
}

void draw(SDL_Window * window)
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glClearColor(1.0f, 0.5f, 0.5f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glm::mat4 projection(1.0);
	projection = glm::perspective(float(60.0f*DEG_TO_RADIAN), 800.0f / 600.0f, 1.0f, 150.0f);	

	GLfloat scale(1.0f); 
	glm::mat4 modelview(1.0); 
	mvStack.push(modelview);	

	at = moveForward(eye, rotation, 1.0f);
	mvStack.top() = glm::lookAt(eye, at, up);

	//---------------------------------------------------------------------------------Skybox
	glUseProgram(skyboxShader);
	rt3d::setUniformMatrix4fv(skyboxShader, "projection", glm::value_ptr(projection));
	glDepthMask(GL_FALSE); 
	glm::mat3 mvRotOnlyMat3 = glm::mat3(mvStack.top());
	mvStack.push(glm::mat4(mvRotOnlyMat3));
	glCullFace(GL_FRONT); 
	glBindTexture(GL_TEXTURE_CUBE_MAP, skybox[0]);
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(1.5f, 1.5f, 1.5f));
	rt3d::setUniformMatrix4fv(skyboxShader, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
	glCullFace(GL_BACK); 
	mvStack.pop();
	glDepthMask(GL_TRUE); 

	glUseProgram(phongShader);
	rt3d::setUniformMatrix4fv(phongShader, "projection", glm::value_ptr(projection));
	
	glm::vec4 tmp = mvStack.top()*lightPos;
	light.position[0] = tmp.x;
	light.position[1] = tmp.y;
	light.position[2] = tmp.z;
	rt3d::setLightPos(phongShader, glm::value_ptr(tmp));

	//Bunny
	glBindTexture(GL_TEXTURE_2D, textures[0]);
	mvStack.push(mvStack.top());
	mvStack.top() = glm::translate(mvStack.top(), glm::vec3(0.0, -1.0, -7.0));
	mvStack.top() = glm::scale(mvStack.top(), glm::vec3(30.0, 30.0, 30.0));
	rt3d::setUniformMatrix4fv(phongShader, "modelview", glm::value_ptr(mvStack.top()));
	rt3d::setMaterial(phongShader, material0);
	rt3d::drawIndexedMesh(meshObjects[1], meshIndexCount, GL_TRIANGLES);
	mvStack.pop();
	
	mvStack.pop(); 
	glDepthMask(GL_TRUE);

	//Steps to implement the motion blur:

	//Copy framebuffer to RBO - readPixels
	//pass to texture object
	//render 6 textures to quad

	if (motionBlurBool)
	{
		float max = 2.5;
		float mid = 1.0;
		float min = 0.5;

		glUseProgram(motionBlur);
		rt3d::setUniformMatrix4fv(motionBlur, "projection", glm::value_ptr(projection));

		glBindTexture(GL_TEXTURE_2D, pboTex[i]);															// Needs to increase i each frame, to change out the texture that the pbo is copying to
		
		glBindBuffer(GL_PIXEL_PACK_BUFFER, pboBuffer);
		glReadPixels(0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, NULL);						//FBO to PBO
		glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

		// -----------------------------------------------------------------------------------Copy PBO to the texture
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboBuffer);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		modelview = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, -10.0f));							//Position quad
		modelview = glm::scale(modelview, glm::vec3(7.79, -5.85f, 0.1f));									//Scale quad
		rt3d::setUniformMatrix4fv(motionBlur, "modelview", glm::value_ptr(modelview));

		//---------------------------------------------------------------------------------------bind textures to texture 

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pboTex[i]);
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);																			//To make sure quad is drawn on top of the scene
		rt3d::drawIndexedMesh(meshObjects[0], 6, GL_TRIANGLES);
		glBindTexture(GL_TEXTURE_2D, 0);
		//-----------------------------------------------------------------------Values not needed to test. Only used to smooth effect with skybox
		if (i == 0)
		{
			GLuint uniformIndex = glGetUniformLocation(motionBlur, "val0");
			glUniform1f(uniformIndex, max);
			uniformIndex = glGetUniformLocation(motionBlur, "val1");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val2");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val3");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val4");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val5");
			glUniform1f(uniformIndex, mid);
		}

		if (i == 1)
		{
			GLuint uniformIndex = glGetUniformLocation(motionBlur, "val0");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val1");
			glUniform1f(uniformIndex, max);
			uniformIndex = glGetUniformLocation(motionBlur, "val2");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val3");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val4");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val5");
			glUniform1f(uniformIndex, min);
		}

		if (i == 2)
		{
			GLuint uniformIndex = glGetUniformLocation(motionBlur, "val0");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val1");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val2");
			glUniform1f(uniformIndex, max);
			uniformIndex = glGetUniformLocation(motionBlur, "val3");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val4");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val5");
			glUniform1f(uniformIndex, min);
		}

		if (i == 3)
		{
			GLuint uniformIndex = glGetUniformLocation(motionBlur, "val0");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val1");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val2");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val3");
			glUniform1f(uniformIndex, max);
			uniformIndex = glGetUniformLocation(motionBlur, "val4");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val5");
			glUniform1f(uniformIndex, min);
		}

		if (i == 4)
		{
			GLuint uniformIndex = glGetUniformLocation(motionBlur, "val0");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val1");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val2");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val3");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val4");
			glUniform1f(uniformIndex, max);
			uniformIndex = glGetUniformLocation(motionBlur, "val5");
			glUniform1f(uniformIndex, mid);
		}

		if (i == 5)
		{
			GLuint uniformIndex = glGetUniformLocation(motionBlur, "val0");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val1");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val2");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val3");
			glUniform1f(uniformIndex, min);
			uniformIndex = glGetUniformLocation(motionBlur, "val4");
			glUniform1f(uniformIndex, mid);
			uniformIndex = glGetUniformLocation(motionBlur, "val5");
			glUniform1f(uniformIndex, max);
		}

		i++;
		if (i >= 6) i = 0;
	}
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glReadPixels(0, 0, screenWidth, screenWidth, GL_RGB, GL_UNSIGNED_BYTE, pick_col);
	//cout << " r : " << pick_col[0]/255.0 << "\n";
	cout << " g : " << pick_col[1] / 255.0 << "\n";
	//cout << " b : " << pick_col[2] / 255.0 << "\n";
	
	if (pick_col[i]/255.0 > 0.11)
	{
		cout << "green \n" << pick_col[i] /255.0;
	}
	SDL_GL_SwapWindow(window);
	
	
	
}

glm::vec3 moveForward(glm::vec3 pos, GLfloat angle, GLfloat d)
{
	return glm::vec3(pos.x + d * std::sin(rotation*DEG_TO_RADIAN), pos.y, pos.z - d * std::cos(rotation*DEG_TO_RADIAN));
}

glm::vec3 moveRight(glm::vec3 pos, GLfloat angle, GLfloat d) 
{
	return glm::vec3(pos.x + d * std::cos(rotation*DEG_TO_RADIAN), pos.y, pos.z + d * std::sin(rotation*DEG_TO_RADIAN));
}

void update()
{
	const Uint8 *keys = SDL_GetKeyboardState(NULL);

	
		if (keys[SDL_SCANCODE_W])
		{
			eye = moveForward(eye, rotation, 0.1f);
		}
		if (keys[SDL_SCANCODE_S])
		{
			eye = moveForward(eye, rotation, -0.1f);			
		}
		if (keys[SDL_SCANCODE_A])
		{
			eye = moveRight(eye, rotation, -0.1f);		
		}
		if (keys[SDL_SCANCODE_D])
		{
			eye = moveRight(eye, rotation, 0.1f);			
		}

		if (keys[SDL_SCANCODE_I])
		{
			lightPos[2] -= 0.1;
		}
		if (keys[SDL_SCANCODE_K])
		{
			lightPos[2] += 0.1;
		}
		if (keys[SDL_SCANCODE_J])
		{
			lightPos[0] -= 0.1;
		}
		if (keys[SDL_SCANCODE_L])
		{
			lightPos[0] += 0.1;
		}
		if (keys[SDL_SCANCODE_U])
		{
			lightPos[1] += 0.1;
		}
		if (keys[SDL_SCANCODE_H])
		{
			lightPos[1] -= 0.1;
		}

		if (keys[SDL_SCANCODE_M])
		{
			motionBlurBool = true;
		}
		if (keys[SDL_SCANCODE_N])
		{
			motionBlurBool = false;
		}

	if (keys[SDL_SCANCODE_R]) eye.y += 0.1;
	if (keys[SDL_SCANCODE_F]) eye.y -= 0.1;

	if (keys[SDL_SCANCODE_COMMA]) rotation -= 1.0f;
	if (keys[SDL_SCANCODE_PERIOD]) rotation += 1.0f;

}



SDL_Window* setupRC(SDL_GLContext &context)
{
	SDL_Window* window;
	SDL_Init(SDL_INIT_VIDEO);


	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);  
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8); 
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

	window = SDL_CreateWindow("Class Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,	800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	if (!window)
		rt3d::exitFatalError("winodw not created");

	context = SDL_GL_CreateContext(window);																
	SDL_GL_SetSwapInterval(1);																			
	return window;
}


GLuint loadBitmap(char *fname) 
{
	GLuint texID;
	glGenTextures(1, &texID);																			
	SDL_Surface *tmpSurface;																			
	tmpSurface = SDL_LoadBMP(fname);
	if (!tmpSurface)
	{
		std::cout << "Error loading bimpap" << std::endl;
	}

	glBindTexture(GL_TEXTURE_2D, texID);																
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);									 
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);									
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);								
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	SDL_PixelFormat *format = tmpSurface->format;														

	GLuint externalFormat, internalFormat;																
	if (format->Amask)
	{																				
		internalFormat = GL_RGBA;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGRA;
	}
	else 
	{
		internalFormat = GL_RGB;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGB : GL_BGR;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, tmpSurface->w, tmpSurface->h, 0,	externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);																	

	SDL_FreeSurface(tmpSurface);																		
	return texID;
}

GLuint loadCubeMap(const char *fname[6], GLuint *texID)
{
	glGenTextures(1, texID); 
	GLenum sides[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
						GL_TEXTURE_CUBE_MAP_POSITIVE_X,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
						GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
						GL_TEXTURE_CUBE_MAP_NEGATIVE_Y };
	SDL_Surface *tmpSurface;

	glBindTexture(GL_TEXTURE_CUBE_MAP, *texID); 
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	GLuint externalFormat;
	for (int i = 0; i < 6; i++)
	{		
		tmpSurface = SDL_LoadBMP(fname[i]);
		if (!tmpSurface)
		{
			std::cout << "Error loading bitmap" << std::endl;
			return *texID;
		}
				
		SDL_PixelFormat *format = tmpSurface->format;
		externalFormat = (format->Rmask < format->Bmask) ? GL_RGBA : GL_BGR;

		glTexImage2D(sides[i], 0, GL_RGBA, tmpSurface->w, tmpSurface->h, 0, externalFormat, GL_UNSIGNED_BYTE, tmpSurface->pixels);
		
		SDL_FreeSurface(tmpSurface);
	}
	return *texID;	
}



int main(int argc, char *argv[])
{
	SDL_Window* hWindow; 
	SDL_GLContext glContext; 
	hWindow = setupRC(glContext); 

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{ 
		std::cout << "glew Init failed, aborting." << endl;
		exit(1);
	}
	cout << glGetString(GL_VERSION) << endl;

	init();


	bool running = true; 
	SDL_Event sdlEvent;  
	while (running) 
	{	
		while (SDL_PollEvent(&sdlEvent))
		{
			if (sdlEvent.type == SDL_QUIT)
				running = false;
		}
		update();
		draw(hWindow); 
	}

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(hWindow);
	SDL_Quit();
	return 0;
}

