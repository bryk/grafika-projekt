// Piotr Bryk, Grafika komputerowa, projekt
#include <string>
#include <vector>
#include <stack>
#include <math.h>
#include <stdio.h>
#include <glload/gl_3_3.h>
#include <glutil/glutil.h>
#include <GL/freeglut.h>
#include "../framework/framework.h"
#include "../framework/Mesh.h"
#include "../framework/MousePole.h"
#include "../framework/Timer.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define ARRAY_COUNT( array ) (sizeof( array ) / (sizeof( array[0] ) * (sizeof( array ) != sizeof(void*) || sizeof( array[0] ) <= sizeof(void*))))
void onKeyUp(unsigned char key, int x, int y);
#define SLOW_CAR_SPEED 0.1f
#define FAST_CAR_SPEED 0.5f

struct ProgramData {
	GLuint theProgram;
	
	GLuint position;
	GLuint position2;
	GLuint intensity;
	GLuint Ka;
	GLuint Kd;
	GLuint Ks;
	GLuint Shininess;
	GLuint direction;
	GLuint cutoff;
	GLuint exponent;

	GLuint modelToCameraMatrixUnif;
};

struct UnlitProgData {
	GLuint theProgram;

	GLuint objectColorUnif;
	GLuint modelToCameraMatrixUnif;
};

float fzNear = 1.0f;
float fzFar = 4000.0f;
ProgramData adsShader;
UnlitProgData flatShader;
const int projectionBlockIndex = 2;

// Wczytuje shader bez oœwietlenia
UnlitProgData loadFlatShader(const std::string &strVertexShader, const std::string &strFragmentShader) {
	std::vector<GLuint> shaderList;

	shaderList.push_back(Framework::LoadShader(GL_VERTEX_SHADER, strVertexShader));
	shaderList.push_back(Framework::LoadShader(GL_FRAGMENT_SHADER, strFragmentShader));

	UnlitProgData data;
	data.theProgram = Framework::CreateProgram(shaderList);
	data.modelToCameraMatrixUnif = glGetUniformLocation(data.theProgram, "modelToCameraMatrix");
	data.objectColorUnif = glGetUniformLocation(data.theProgram, "objectColor");

	GLuint projectionBlock = glGetUniformBlockIndex(data.theProgram, "Projection");
	glUniformBlockBinding(data.theProgram, projectionBlock, projectionBlockIndex);

	return data;
}

// Wczutyje shader z oœwietleniem
ProgramData loadLightedShader(const std::string &strVertexShader, const std::string &strFragmentShader) {
	std::vector<GLuint> shaderList;

	shaderList.push_back(Framework::LoadShader(GL_VERTEX_SHADER, strVertexShader));
	shaderList.push_back(Framework::LoadShader(GL_FRAGMENT_SHADER, strFragmentShader));

	ProgramData data;
	data.theProgram = Framework::CreateProgram(shaderList);
	data.modelToCameraMatrixUnif = glGetUniformLocation(data.theProgram, "modelToCameraMatrix");
	
	data.Kd = glGetUniformLocation(data.theProgram, "Kd");
	data.Ka = glGetUniformLocation(data.theProgram, "Ka");
	data.Ks = glGetUniformLocation(data.theProgram, "Ks");
	data.position = glGetUniformLocation(data.theProgram, "position");
	data.position2 = glGetUniformLocation(data.theProgram, "position2");
	data.intensity = glGetUniformLocation(data.theProgram, "intensity");
	data.Shininess = glGetUniformLocation(data.theProgram, "Shininess");
	data.direction = glGetUniformLocation(data.theProgram, "direction");
	data.exponent = glGetUniformLocation(data.theProgram, "exponent");
	data.cutoff = glGetUniformLocation(data.theProgram, "cutoff");
	
	GLuint projectionBlock = glGetUniformBlockIndex(data.theProgram, "Projection");

	glUniformBlockBinding(data.theProgram, projectionBlock, projectionBlockIndex);

	return data;
}

// Wczytuje shadery
void initializeShaders() {
	adsShader = loadLightedShader("FragmentLighting_PCN.vert", "FragmentLighting.frag");
	flatShader = loadFlatShader("PosTransform.vert", "UniformColor.frag");
}

// Siatki poszczególnych obiektów
Framework::Mesh *g_pCylinderMesh = NULL;
Framework::Mesh *g_pShpere1Mesh = NULL;
Framework::Mesh *g_pTetrahedron1Mesh = NULL;
Framework::Mesh *g_pShpere2Mesh = NULL;
Framework::Mesh *g_pPlaneMesh = NULL;
Framework::Mesh *g_pCubeMesh = NULL;

///////////////////////////////////////////////
// View/Object Setup
glutil::ViewData g_initialViewData = {
	glm::vec3(1.0f, 0.5f, 0.0f),
	glm::fquat(0.92387953f, 0.3826834f, 0.0f, 0.0f),
	15.0f,
	0.0f
};

glutil::ViewScale g_viewScale = {
	2.0f, 50.0f,
	1.5f, 0.5f,
	0.0f, 10.0f,
	90.0f/250.0f
};

glutil::ObjectData g_initialObjectData = {
	glm::vec3(0.0f, 0.5f, 0.0f),
	glm::fquat(1.0f, 0.0f, 0.0f, 0.0f),
};

glutil::ViewPole g_viewPole = glutil::ViewPole(g_initialViewData, g_viewScale, glutil::MB_LEFT_BTN);

namespace {
	void MouseMotion(int x, int y) {
		Framework::ForwardMouseMotion(g_viewPole, x, y);
		glutPostRedisplay();
	}

	void MouseButton(int button, int state, int x, int y) {
		Framework::ForwardMouseButton(g_viewPole, button, state, x, y);
		glutPostRedisplay();
	}

	void MouseWheel(int wheel, int direction, int x, int y) {
		Framework::ForwardMouseWheel(g_viewPole, wheel, direction, x, y);
		glutPostRedisplay();
	}
}

GLuint g_projectionUniformBuffer = 0;

struct ProjectionBlock {
	glm::mat4 cameraToClipMatrix;
};

// Inicjalizuje meshe i parametry opengl
void init() {
	initializeShaders();

	try {
		g_pCylinderMesh = new Framework::Mesh("UnitCylinder.xml");
		g_pShpere1Mesh = new Framework::Mesh("Sphere1.xml");
		g_pShpere2Mesh = new Framework::Mesh("Sphere2.xml");
		g_pPlaneMesh = new Framework::Mesh("LargePlane.xml");
		g_pCubeMesh = new Framework::Mesh("UnitCube.xml");
		g_pTetrahedron1Mesh = new Framework::Mesh("UnitTetrahedron1.xml");
	} catch(std::exception &except) {
		printf("%s\n", except.what());
		throw;
	}

 	glutMouseFunc(MouseButton);
 	glutMotionFunc(MouseMotion);
	glutMouseWheelFunc(MouseWheel);
	glutKeyboardUpFunc(onKeyUp);

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0.0f, 1.0f);
	glEnable(GL_DEPTH_CLAMP);

	glGenBuffers(1, &g_projectionUniformBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, g_projectionUniformBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ProjectionBlock), NULL, GL_DYNAMIC_DRAW);

	glBindBufferRange(GL_UNIFORM_BUFFER, projectionBlockIndex, g_projectionUniformBuffer,
		0, sizeof(ProjectionBlock));

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

int oldTime, currentTime; // aktualny i poprzedni czas. Wykorzystywane podczas suchu samochodu do obliczania kroku.
static glm::vec4 carPosition(0.0f, 0.4f, 0.0f, 1.0f); // pozycja samochodu - w praktyce chodzi o jego prawy reflektor.
bool aPressed, dPressed, wPressed, sPressed; // czy naciœniête s¹ klawse WSAD?
float carSpeed; // szybkoœæ samochodu 
float nextLight = 1.5f; // odleg³oœæ pomiêdzy œwiat³ami
float strength = 1.0f; // si³a œwiat³a
float cutoff = 70; // odciêcie sto¿ka œwiat³a
glm::vec4 lightPosCameraSpace; // posycja œwiat³a w camera space
glm::vec3 direction(-10, 0, -1); // kierunek œwiate³ samochodu

// Oblicza now¹ pozycjê samochodu na podstawie up³yniêtego czasu i szybkoœci
void CalcCarPosition() {
	float diff = ((float)(oldTime - currentTime))/100.0f;
	if (aPressed) {
		carPosition[0] += carSpeed * diff;
	}
	if (dPressed) {
		carPosition[0] -= carSpeed * diff;
	}
	if (wPressed) {
		carPosition[2] += carSpeed * diff;
	}
	if (sPressed) {
		carPosition[2] -= carSpeed * diff;
	}
}

// Ustawia domyœlne parametry oœwietlania dla danego shadera
void setDefaultLightParams(ProgramData *data, glm::vec4 &worldLightPos, glm::vec3 &direction, float nextLight) {
	glUseProgram(data->theProgram);
	glUniform3f(data->position, worldLightPos[0], worldLightPos[1], worldLightPos[2]);
	glUniform3f(data->position2, worldLightPos[0], worldLightPos[1], worldLightPos[2] + nextLight);
	glUniform3f(data->intensity, 1.8f, 1.4f, 1.2f);
	glUniform3f(data->Ka, 0.2f, 0.2f, 0.2f);
	glUniform3f(data->Kd, strength*1.0f, strength*1.0f, strength*1.0f);
	glUniform3f(data->Ks, strength*0.1f, strength*0.1f, strength*0.1f);
	glUniform3f(data->direction, direction[0], direction[1], direction[2]);
	glUniform1f(data->cutoff, cutoff);
	glUniform1f(data->exponent, 5.0f);
	glUniform1f(data->Shininess, 10.5f);
	glUseProgram(0);
}

// Rysuje jedno œwiat³o samochodu
void drawLight(glutil::MatrixStack &modelMatrix, glm::vec4 lightPos) {
	glutil::PushStack push(modelMatrix);

	modelMatrix.Translate(glm::vec3(lightPos));
	modelMatrix.Scale(0.1f, 0.1f, 0.1f);

	glUseProgram(flatShader.theProgram);
	glUniformMatrix4fv(flatShader.modelToCameraMatrixUnif, 1, GL_FALSE,
		glm::value_ptr(modelMatrix.Top()));
	glUniform4f(flatShader.objectColorUnif, 0.9078f, 0.9706f, 0.1922f, 1.0f);
	g_pCubeMesh->Render("flat");
}

// Rysuje samochód, zaczynaj¹c od œwiate³
void drawCar(glutil::MatrixStack &modelMatrix, glm::vec4 &lightPos) {
	drawLight(modelMatrix, lightPos);
	drawLight(modelMatrix, lightPos + glm::vec4(0.0, 0.0, nextLight, 0.0));
	glutil::PushStack push(modelMatrix);
	
	modelMatrix.Translate(glm::vec3(carPosition) + glm::vec3(1.55f, 0.0, 0.75));
	modelMatrix.Scale(3.0f, 0.8f, 2.1f);

	glm::mat4 invTransform = glm::inverse(modelMatrix.Top());
	glm::vec4 lightPosModelSpace = invTransform * lightPosCameraSpace;
	glm::vec3 directionModelSpace = direction * glm::vec3(lightPosCameraSpace);

	glUseProgram(adsShader.theProgram);
	glUniformMatrix4fv(adsShader.modelToCameraMatrixUnif, 1, GL_FALSE,
		glm::value_ptr(modelMatrix.Top()));
				
	glUniform3fv(adsShader.position, 1, glm::value_ptr(lightPosModelSpace));
	glUniform3f(adsShader.position2, lightPosModelSpace[0], lightPosModelSpace[1], lightPosModelSpace[2] + nextLight);

	g_pCubeMesh->Render("color");
}

// Rysuje kolorow¹ siatkê na danej pozycji i z danym parametrem shine
void drawColoredMesh(glutil::MatrixStack &modelMatrix, glm::vec4 &lightPos, Framework::Mesh *mesh, glm::vec3 position, float shine) {
	glutil::PushStack push(modelMatrix);
	
	modelMatrix.Translate(position);

	glm::mat4 invTransform = glm::inverse(modelMatrix.Top());
	glm::vec4 lightPosModelSpace = invTransform * lightPosCameraSpace;
	glm::vec3 directionModelSpace = direction * glm::vec3(lightPosCameraSpace);

	glUseProgram(adsShader.theProgram);
	glUniformMatrix4fv(adsShader.modelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
				
	glUniform3fv(adsShader.position, 1, glm::value_ptr(lightPosModelSpace));
	glUniform3f(adsShader.position2, lightPosModelSpace[0], lightPosModelSpace[1], lightPosModelSpace[2] + nextLight);
	glUniform1f(adsShader.Shininess, shine);
	glUniform3f(adsShader.Kd, strength*0.3f, strength*0.3f, strength*0.3f);
	glUniform3f(adsShader.Ks, 0.1f, 0.1f, 0.1f);
	mesh->Render("lit-color");

	glUseProgram(0);
}

// Rysuje czerwonawy cylinder, s³abo lœni¹cy
void drawCylinder(glutil::MatrixStack &modelMatrix, glm::vec4 &lightPos) {
	drawColoredMesh(modelMatrix, lightPos, g_pCylinderMesh, glm::vec3(-3, 0.5f, 3), 0.9f);
}

// Rysuje kolorow¹ sferê, s³abo lœni¹c¹
void drawSphere1(glutil::MatrixStack &modelMatrix, glm::vec4 &lightPos) {
	drawColoredMesh(modelMatrix, lightPos, g_pShpere1Mesh, glm::vec3(-3, 1, 2), 1.9f);
}

// Rysuje dwukolorow¹ sferê, s³abo lœni¹c¹
void drawSphere2(glutil::MatrixStack &modelMatrix, glm::vec4 &lightPos) {
	drawColoredMesh(modelMatrix, lightPos, g_pShpere2Mesh, glm::vec3(-4, 0.2f, -2), 0.9f);
}

// Rysuje kolorowy czworoœcia, lœni¹cy
void drawTetrahedron1(glutil::MatrixStack &modelMatrix, glm::vec4 &lightPos) {
	drawColoredMesh(modelMatrix, lightPos, g_pTetrahedron1Mesh, glm::vec3(-4, 1.0f, 0), 20.9f);
}

// G³ówna funkcja rysowania. Tutaj wywo³ujemy wszystkie obiekty do narysowania.
void display() {
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	CalcCarPosition();
	glm::vec4 &worldLightPos = carPosition;
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	{
		glutil::MatrixStack modelMatrix;
		modelMatrix.SetMatrix(g_viewPole.CalcMatrix());
		

		lightPosCameraSpace = modelMatrix.Top() * worldLightPos;
		
		setDefaultLightParams(&adsShader, worldLightPos, direction, nextLight);

		{
			glutil::PushStack push(modelMatrix);

			// Pod³oga
			{
				glutil::PushStack push(modelMatrix);
				glm::mat4 invTransform = glm::inverse(modelMatrix.Top());
				glUseProgram(adsShader.theProgram);
				glUniformMatrix4fv(adsShader.modelToCameraMatrixUnif, 1, GL_FALSE, glm::value_ptr(modelMatrix.Top()));
				glUniform3f(adsShader.Ks, 0.0f, 0.0f, 0.0f); // niech pod³oga nie ma odbicia
				g_pPlaneMesh->Render("lit-color");
				glUseProgram(0);
			}
			
			drawCylinder(modelMatrix, worldLightPos);
			drawCar(modelMatrix, worldLightPos);
			drawSphere1(modelMatrix, worldLightPos);
			drawSphere2(modelMatrix, worldLightPos);
			drawTetrahedron1(modelMatrix, worldLightPos);
		}
	}

	glutPostRedisplay();
	glutSwapBuffers();
	oldTime = currentTime;
}

//Called whenever the window is resized. The new window size is given, in pixels.
//This is an opportunity to call glViewport or glScissor to keep up with the change in size.
void reshape (int w, int h) {
	glutil::MatrixStack persMatrix;
	persMatrix.Perspective(45.0f, (w / (float)h), fzNear, fzFar);

	ProjectionBlock projData;
	projData.cameraToClipMatrix = persMatrix.Top();

	glBindBuffer(GL_UNIFORM_BUFFER, g_projectionUniformBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ProjectionBlock), &projData);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glutPostRedisplay();
}

// Wywo³ywana przy naciœniêciu klawisza
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
	case 27:
		glutLeaveMainLoop();
		return;
		
	case 'w': wPressed = true; carSpeed = FAST_CAR_SPEED; break;
	case 's': sPressed = true; carSpeed = FAST_CAR_SPEED; break;
	case 'd': dPressed = true; carSpeed = FAST_CAR_SPEED; break;
	case 'a': aPressed = true; carSpeed = FAST_CAR_SPEED; break;
	case 'W': wPressed = true; carSpeed = SLOW_CAR_SPEED; break;
	case 'S': sPressed = true; carSpeed = SLOW_CAR_SPEED; break;
	case 'D': dPressed = true; carSpeed = SLOW_CAR_SPEED; break;
	case 'A': aPressed = true; carSpeed = SLOW_CAR_SPEED; break;
		
	case '+': strength += 0.2f; break;
	case '-': strength -= 0.2f; break;
	case '8': cutoff += 1; break;
	case '9': cutoff -= 1; break;
	case '5': direction[1] += 0.1f; break;
	case '6': direction[1] -= 0.1f; break;
	}
	if (strength < 0.2f) {
		strength = 0.2f;
	} else if (strength > 10) {
		strength = 10;
	}
	if (cutoff < 5) {
		cutoff = 5;
	} else if (cutoff > 85) {
		cutoff = 85;
	}

	glutPostRedisplay();
}

// Wywo³ywana przy podniesieniu klawisza
void onKeyUp(unsigned char key, int x, int y) {
	switch (key) {
	case 'W':
	case 'w': wPressed = false; break;
	case 'S':
	case 's': sPressed = false; break;
	case 'D':
	case 'd': dPressed = false; break;
	case 'A':
	case 'a': aPressed = false; break;
	}
}


unsigned int defaults(unsigned int displayMode, int &width, int &height) {return displayMode;}
