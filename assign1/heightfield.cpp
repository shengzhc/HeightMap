// assign1.cpp : Defines the entry point for the console application.
//

/*
  CSCI 480 Computer Graphics
  Assignment 1: Height Fields
  C++ starter code
*/

#include "stdafx.h"
#include <pic.h>
#include <windows.h>
#include <stdlib.h>
#include <GL/glu.h>
#include <GL/glut.h>

int g_iMenuId;

int g_vMousePos[2] = {0, 0};
int g_iLeftMouseButton = 0;    /* 1 if pressed, 0 if not */
int g_iMiddleMouseButton = 0;
int g_iRightMouseButton = 0;


#define PIC_GREYINDEX (row*(width) + col)
#define DIMENSION 3
#define SPAN 2.0

#define X 0
#define Y 1
#define Z 2

#define U 0
#define V 1

#define KEY_TAB 0x09
#define KEY_L 0x6c
#define KEY_M 0x6d
#define KEY_V  0x76
#define KEY_A 0x61

#define FOV 54.0f

#define FPS 15
#define TPF ((long int)(1000.0/15 * 0.75))
#define MAX_SCREENSHOTS 300

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
typedef enum  { REND_POINT, REND_WIREFRAME, REND_SOLTRIANGLE} RENDERTYPE;

CONTROLSTATE g_ControlState = ROTATE;
RENDERTYPE g_RenderType = REND_SOLTRIANGLE;
char *folderName = "screenshot/";

/* state of the world */
float g_vLandRotate[3] = {0.0, 0.0, 0.0};
float g_vLandTranslate[3] = {0.0, 0.0, 0.0};
float g_vLandScale[3] = {1.0, 1.0, 1.0};

const char *externPic = "SantaMonicaMountains-256.jpg";
const char *texturePic = "mario.jpg";

GLfloat *vertices;
GLfloat *verticesUV;
bool isLightOn = false;
bool isColorExternal = false;
bool isStartingAnimation = false;
bool isTextureOpen = false;
/* see <your pic directory>/pic.h for type Pic */
Pic * g_pHeightData;


void calculateFrameRate();
void display();
void displayHeightData();
void doIdle();
void init();
void menufunc(int value);
void mousebutton(int button, int state, int x, int y);
void mousedrag(int x, int y);
void mouseidle(int x, int y);
void myKeyboard(unsigned char key, int x, int y);
void reshape(int w, int h);
void scheduleForScreenShot();
void saveScreenshot(char *filename);
void getFileName(int id, char *filename);
void setupLight();
void setupTexture();
void drawTriangles();
void drawTrianglesWithTexture();

void displayHeightData(void)
{
	long int cout = 0;
	int width = g_pHeightData->nx;
	int height = g_pHeightData->ny;

	for(int row = 0; row<g_pHeightData->ny; row++)
	{
		for(int col = 0; col<g_pHeightData->nx; col++)
		{
			unsigned char c = g_pHeightData->pix[PIC_GREYINDEX];
			printf("%d\n", c);
			cout++;
		}
	}
	printf("%ld\n", cout);
}

// calculate current framerate
void calculateFrameRate()
{
	static int framePerSecond = 0.0f;
	static float lastTime = 0.0f;
	float currentTime = GetTickCount() * 0.001f;
	framePerSecond ++;
	if(currentTime - lastTime > 1.0f)
	{
		lastTime = currentTime;
		printf("FPS: %d\n", framePerSecond);
		framePerSecond = 0;
	}
}

// get the proper animation screenshot name
void getFileName(int id, char filename[])
{
	int foldLen = strlen(folderName);
	strncpy(filename, folderName, foldLen);
	foldLen -= 1;
	for(int i=3; i>0; i--, id/=10)
	{
		char ch = '0'+id%10;
		*(filename+i+foldLen) = ch;
	}
	strncpy(filename+foldLen+3+1, ".jpg", sizeof(".jpg")/sizeof(char));
	*(filename+foldLen+3+1+sizeof(".jpg")/sizeof(char)+1)='\0';
}

// save screenshots automatically
void scheduleForScreenShot()
{
	static long int ellipseTime = TPF;
	static long int screenshots = 0;
	static DWORD lastFrameTime = 0;
	DWORD currentTime = GetTickCount();
	//printf("currentTime: %f\n", currentTime * 0.001f);
	ellipseTime -= (currentTime - lastFrameTime);
	lastFrameTime = currentTime;
	//printf("current time: %ld\t ellipseTime: %ld\t \n", currentTime, ellipseTime);
	if(ellipseTime <= 0)
	{
		screenshots++;
		if(screenshots >= MAX_SCREENSHOTS)
		{
			isStartingAnimation = false;
			ellipseTime = TPF;
			screenshots = 0;
			lastFrameTime = 0;
			return;
		}
		char filename[30]={'0'};
		getFileName(screenshots, filename);
		saveScreenshot(filename);
		//printf("Save Screenshot at Time: %f, %d screenshots\n", currentTime * 0.001, screenshots);
		ellipseTime = TPF;
	}
}

/* Write a screenshot to the specified filename */
void saveScreenshot (char *filename)
{
  int i;
  Pic *in = NULL;

  if (filename == NULL)
    return;

  /* Allocate a picture buffer */
  in = pic_alloc(640, 480, 3, NULL);

  printf("File to save to: %s\n", filename);

  for (i=479; i>=0; i--) {
    glReadPixels(0, 479-i, 640, 1, GL_RGB, GL_UNSIGNED_BYTE,
                 &in->pix[i*in->nx*in->bpp]);
  }

  if (jpeg_write(filename, in))
    printf("File saved Successfully\n");
  else
    printf("Error in Saving\n");

  pic_free(in);
}

void init()
{
  /* setup gl view here */

  // Phong shading enable
  glShadeModel(GL_SMOOTH);

  if(vertices)
	  free(vertices);
  
  // parse the height field data into vertices used in program
  int width = g_pHeightData->nx;
  int height = g_pHeightData->ny;
	vertices = (GLfloat *)malloc(sizeof(GLfloat) * width * height * DIMENSION);
 
  GLfloat leftX = -1.0 * SPAN / 2.0;
  GLfloat bottomY = leftX;

  GLfloat xStep = SPAN / width;
  GLfloat yStep = SPAN / height;
	

  for(int row = 0; row<height; row++)
  {
	  for(int col = 0; col<width; col++)
	  {
		  long int verticeIndex = (row * width + col);
		  vertices[verticeIndex * DIMENSION + X] = leftX + col * xStep;
		  vertices[verticeIndex * DIMENSION + Y]  = bottomY + row * yStep;
		  vertices[verticeIndex * DIMENSION + Z] = g_pHeightData->pix[PIC_GREYINDEX]/255.0;
	  }
  }
  // set up lights first
  setupLight();
  setupTexture();
}

// set up remote diretional lights
void setupLight()
{
	GLfloat light0[] = {0.5, 0.5, 0.9, 1.0};
	GLfloat light1[] = {0.9, 0.2, 0.3, 1.0};
	GLfloat light2[] = {0.2, 0.7, 0.3, 1.0};
	GLfloat ambLight[] = {0.3, 0.3, 0.3, 1.0};

	GLfloat light0Pos[] = {-0.7071, 0.7071, 0.5, 0.0};
	GLfloat light1Pos[] = {0, -0.7071, -0.2071, 0.0};
	GLfloat light2Pos[] = {0.7071, 0.0, -0.2071, 0.0};
	GLfloat ambPos[] = {0, 0, 0};

	GLfloat ambCof[] = {0.3, 0.3, 0.3};
	GLfloat difCof[] = {0.1, 0.1, 0.1};
	GLfloat speCof[] = {0.7, 0.7, 0.7};

	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambLight);

	glLightfv(GL_LIGHT0, GL_POSITION, light0Pos);
	glLightfv(GL_LIGHT1, GL_POSITION, light1Pos);
	glLightfv(GL_LIGHT2, GL_POSITION, light2Pos);

	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light0);

	glLightfv(GL_LIGHT1, GL_DIFFUSE, light1);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light1);

	glLightfv(GL_LIGHT2, GL_DIFFUSE, light2);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light2);

	// set up material response co-efficients
	glMaterialfv(GL_FRONT, GL_AMBIENT, ambCof);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, difCof);
	glMaterialfv(GL_FRONT, GL_SPECULAR, speCof);
	
	// front and back normal of a surface should be different 
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
}

void setupTexture()
{
	// setup texture config
	Pic *texture = jpeg_read((char*)texturePic, NULL);

	GLuint texture_id;

	glGenTextures(1, &texture_id);
	glBindTexture(GL_TEXTURE_2D, texture_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	switch (texture->bpp)
	{
	case 1:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, texture->nx, texture->ny, 0, GL_RGB8, GL_UNSIGNED_BYTE, texture->pix);
		break;
	case 3:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture->nx, texture->ny, 0, GL_RGB, GL_UNSIGNED_BYTE, texture->pix);
		break;
	case 4:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture->nx, texture->ny, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture->pix);
		break;
	default:
		break;
	}

	long int t_size = sizeof(GLfloat)*2*texture->nx*texture->ny;
	verticesUV = (GLfloat *)malloc(t_size);
	memset(verticesUV, 0, t_size);

	int width = texture->nx;
	int height = texture->ny;
	// initialize the UV array
	for (int row=0;row<height;row++)
	{
		for(int col=0;col<width;col++)
		{
			verticesUV[PIC_GREYINDEX*2+U] = row/255.0; 
			verticesUV[PIC_GREYINDEX*2+V] = col/255.0;
		}
	}

}

// draw triangles without texture
void drawTriangles()
{
	GLfloat *triangle = (GLfloat *)malloc(sizeof(GLfloat) * 3 * DIMENSION);
	GLfloat *triColor = (GLfloat *)malloc(sizeof(GLfloat) * 3 * 3);
	Pic *g_externHeightData = NULL;
	int width = g_pHeightData->nx;
	int height = g_pHeightData->ny;

	// load external height data if needed
	if (isColorExternal)
	{
		g_externHeightData = jpeg_read((char*)externPic, NULL);
	}
	for(int row = 0; row < height - 1; row++)
	{
		for(int col = 0; col < width - 1; col++)
		{
			// set up vertices positions and colors
			memset(triangle, 0, sizeof(GLfloat)*9);
			memset(triColor, 0, sizeof(GLfloat)*9);
			memcpy(triangle, &vertices[(row*width+col)*DIMENSION], sizeof(GLfloat)*3);
			memcpy(triangle+3, &vertices[(row*width+col+1)*DIMENSION], sizeof(GLfloat)*3);
			memcpy(triangle+6, &vertices[((row+1)*width+col+1)*DIMENSION], sizeof(GLfloat)*3);
			memcpy(triColor, triangle, sizeof(GLfloat)*9);
			for(int colorIndex=0;colorIndex<9;colorIndex++)
			{
				if(colorIndex%3==2)
					triColor[colorIndex] = 1.0;
				else
				{
					triColor[colorIndex] = triColor[colorIndex+2-colorIndex%3];
					if (isColorExternal)
					{
						triColor[colorIndex] = g_externHeightData->pix[PIC_GREYINDEX]/255.0;
					}
				}
			}
			// draw triangles based on the right buttom corner part of a squre of the pic
			for(int verts=0;verts<3;verts++)
			{
				glColor3fv(triColor + verts*3);
				glVertex3fv(triangle + verts*3);
			}

			memcpy(triangle, &vertices[((row+1)*width+col+1)*DIMENSION], sizeof(GLfloat)*3);
			memcpy(triangle+3, &vertices[((row+1)*width+col)*DIMENSION], sizeof(GLfloat)*3);
			memcpy(triangle+6, &vertices[(row*width+col)*DIMENSION], sizeof(GLfloat)*3);;
			memcpy(triColor, triangle, sizeof(GLfloat)*9);

			// draw triangles based on the left top corner part of a squre of the pic 
			for(int colorIndex=0;colorIndex<9;colorIndex++)
			{
				if(colorIndex%3==2)
					triColor[colorIndex] = 1.0;
				else
				{
					triColor[colorIndex] = triColor[colorIndex+2-colorIndex%3];
					if (isColorExternal)
					{
						triColor[colorIndex] = g_externHeightData->pix[PIC_GREYINDEX]/255.0;
					}
				}
			}
			for(int verts=0;verts<3;verts++)
			{
				glColor3fv(triColor + verts*3);
				glVertex3fv(triangle + verts*3);
			}
		}
	}
	free(triangle);
	free(triColor);
}
// importing texture to the terrain
void drawTrianglesWithTexture()
{
	int width = g_pHeightData->nx;
	int height = g_pHeightData->ny;
	
	for (int row=0; row<height-1; row++)
	{
		for (int col=0; col<width-1; col++)
		{
			glTexCoord2fv(&verticesUV[(row*width+col)*2]);
			glVertex3fv(&vertices[(row*width+col)*DIMENSION]);
			glTexCoord2fv(&verticesUV[(row*width+col+1)*2]);
			glVertex3fv(&vertices[(row*width+col+1)*DIMENSION]);
			glTexCoord2fv(&verticesUV[((row+1)*width+col+1)*2]);
			glVertex3fv(&vertices[((row+1)*width+col+1)*DIMENSION]);
			glTexCoord2fv(&verticesUV[((row+1)*width+col+1)*2]);
			glVertex3fv(&vertices[((row+1)*width+col+1)*DIMENSION]);
			glTexCoord2fv(&verticesUV[((row+1)*width+col)*2]);
			glVertex3fv(&vertices[((row+1)*width+col)*DIMENSION]);
			glTexCoord2fv(&verticesUV[(row*width+col)*2]);
			glVertex3fv(&vertices[(row*width+col)*DIMENSION]);
		}
	}
}

void display()
{
  /* draw 1x1 cube about origin */
  /* replace this code with your height field implementation */
  /* you may also want to precede it with your 
rotation/translation/scaling */
  
  // check data successfully loaded
  if (!vertices)
		exit(0);
  // clear buffers
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  // move camera back
  glTranslatef(0.0f, 0.0f, -5.0f);
  // do transformation of objects
  glTranslatef(g_vLandTranslate[X], g_vLandTranslate[Y], g_vLandTranslate[Z]);
  glRotatef(g_vLandRotate[X], 1.0, 0.0, 0.0);
  glRotatef(g_vLandRotate[Y], 0.0, 1.0, 0.0);
  glRotatef(g_vLandRotate[Z], 0.0, 0.0, 1.0);
  glScalef(g_vLandScale[X], g_vLandScale[Y], g_vLandScale[Z]);
  
  // render tri by tri with a alternative render type
  GLenum mode;
  switch(g_RenderType)
  {
  case REND_POINT:
	  mode = GL_POINTS;
	  break;
  case REND_WIREFRAME:
	  mode = GL_LINES;
	  break;
  case REND_SOLTRIANGLE:
	  mode = GL_TRIANGLES;
	  break;
  default:
	  mode = GL_TRIANGLES;
	  break;
  }
  glBegin(mode);
	if (!isTextureOpen)
		drawTriangles();
	else
		drawTrianglesWithTexture();
  glEnd();
  //calculateFrameRate();
  glutSwapBuffers();
}

void reshape(int w, int h)
{
	// ratio of width and height of the display window
	GLfloat aspect = w/h;
	// revise projection matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	/*if(w<=h)
		glOrtho(-2.0, 2.0, -2.0/aspect, 2.0/aspect, -10.0, 10.0);
	else 
		glOrtho(-2.0 * aspect, 2.0 * aspect, -2.0, 2.0, -10.0, 10.0);*/
	// set the FOV of camera and near and far clip
	gluPerspective(FOV, aspect, 0.1f, 100.0f);
	glMatrixMode(GL_MODELVIEW);
	glViewport(0, 0, w, h);
}

void menufunc(int value)
{
  switch (value)
  {
    case 0:
      exit(0);
      break;
	 // save screenshot
	case 1:
		saveScreenshot("a.jpg");
		break;
	default:
		break;
  }
}

void doIdle()
{
  /* do some stuff... */

  /* make the screen update */

	// if animation starts, do saving screenshots
	if (isStartingAnimation)
	{
		scheduleForScreenShot();
	}
	glutPostRedisplay();
}

/* converts mouse drags into information about 
rotation/translation/scaling */
void mousedrag(int x, int y)
{
  int vMouseDelta[2] = {x-g_vMousePos[0], y-g_vMousePos[1]};
  
  switch (g_ControlState)
  {
    case TRANSLATE:  
      if (g_iLeftMouseButton)
      {
        g_vLandTranslate[0] += vMouseDelta[0]*0.01;
        g_vLandTranslate[1] -= vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandTranslate[2] += vMouseDelta[1]*0.01;
      }
      break;
    case ROTATE:
      if (g_iLeftMouseButton)
      {
        g_vLandRotate[0] += vMouseDelta[1];
        g_vLandRotate[1] += vMouseDelta[0];
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandRotate[2] += vMouseDelta[1];
      }
      break;
    case SCALE:
      if (g_iLeftMouseButton)
      {
        g_vLandScale[0] *= 1.0+vMouseDelta[0]*0.01;
        g_vLandScale[1] *= 1.0-vMouseDelta[1]*0.01;
      }
      if (g_iMiddleMouseButton)
      {
        g_vLandScale[2] *= 1.0-vMouseDelta[1]*0.01;
      }
      break;
  }
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mouseidle(int x, int y)
{
  g_vMousePos[0] = x;
  g_vMousePos[1] = y;
}

void mousebutton(int button, int state, int x, int y)
{
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		g_iLeftMouseButton = (state==GLUT_DOWN);
		break;
	case GLUT_MIDDLE_BUTTON:
		g_iMiddleMouseButton = (state==GLUT_DOWN);
		break;
	case GLUT_RIGHT_BUTTON:
		g_iRightMouseButton = (state==GLUT_DOWN);
		break;
	}

	switch(glutGetModifiers())
	{
	case GLUT_ACTIVE_CTRL:
		g_ControlState = TRANSLATE;
		break;
	case GLUT_ACTIVE_SHIFT:
		g_ControlState = SCALE;
		break;
	default:
		g_ControlState = ROTATE;
		break;
	}

	g_vMousePos[0] = x;
	g_vMousePos[1] = y;
}

void myKeyboard(unsigned char key, int x, int y)
{
	switch(key)
	{
	// switch between different rendering method of triangles
	case KEY_TAB:
		g_RenderType = (RENDERTYPE)(((int)g_RenderType + 1)%3);
		break;
	// turn on/off lights
	case KEY_L:
		isLightOn = !isLightOn;
		if (isLightOn)
		{
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			glEnable(GL_LIGHT1);
			glEnable(GL_LIGHT2);
		}
		else
		{
			glDisable(GL_LIGHT0);
			glDisable(GL_LIGHT1);
			glDisable(GL_LIGHT2);
			glDisable(GL_LIGHTING);
		}
		break;
	// use the default height data for coloring or extern heightfield pic (256*256 with 1 byte for each pixel only)
	case KEY_V:
		isColorExternal = !isColorExternal;
		break;
	case KEY_A:
		isStartingAnimation = true;
		break;
	case KEY_M:
		isTextureOpen = !isTextureOpen;
		if (isTextureOpen)
			glEnable(GL_TEXTURE_2D);
		else 
			glDisable(GL_TEXTURE_2D);
		break;
	default:
		break;
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	// I've set the argv[1] to OhioPyle-256.jpg
	// To change it, on the "Solution Explorer",
	// right click "assign1", choose "Properties",
	// go to "Configuration Properties", click "Debugging",
	// then type your texture name for the "Command Arguments"
	if (argc<2)
	{  
		printf ("usage: %s heightfield.jpg\n", argv[0]);
		exit(1);
	}

	g_pHeightData = jpeg_read((char*)argv[1], NULL);
	if (!g_pHeightData)
	{
	    printf ("error reading %s.\n", argv[1]);
	    exit(1);
	}

	// used to display the height data of jpg
	//displayHeightData();

	glutInit(&argc,(char**)argv);
	
	// set up double buffers, rgb color display, and depth buffer
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// set up window size to be 640*480
	glutInitWindowSize(640, 480);
	// set up window position relative to parent window
	glutInitWindowPosition(500, 500);
	// create window with name HeightField
	glutCreateWindow("HeightField");
	// set up rendering function
	glutDisplayFunc(display);

	glutReshapeFunc(reshape);

	g_iMenuId = glutCreateMenu(menufunc);
	glutSetMenu(g_iMenuId);
	glutAddMenuEntry("Quit",0);

	// create a new menu entry to enable you to save screenshot
	glutAddMenuEntry("Save ScreenShot", 1);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	
	/* replace with any animate code */
	glutIdleFunc(doIdle);
	/* callback for mouse drags */
	glutMotionFunc(mousedrag);
	/* callback for idle mouse movement */
	glutPassiveMotionFunc(mouseidle);
	/* callback for mouse button changes */
	glutMouseFunc(mousebutton);
	/* do initialization */

	glutKeyboardFunc(myKeyboard);
	init();
	// enable z-buffer
	glEnable(GL_DEPTH_TEST);
	glutMainLoop();
	return 0;
}