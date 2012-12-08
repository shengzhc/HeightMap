/*
  15-462 Computer Graphics I
  Assignment 1: Height Fields
  C sample solution
  Author: fp, loosely based on C++ code (jsk2)
  Feb 2002
*/

#include <stdlib.h>
#include <GL/glut.h>
#include <pic.h>

const int WIDTH = 640;		/* initial window width */
const int HEIGHT = 480;		/* initial window height */
const GLdouble FOV = 40.0;	/* perspective field of view */

/* most recent mouse position in screen coordinates */
/* only differences are relevant, not absolute positions */
int xMousePos = 0;
int yMousePos = 0;

/* mouse button state, 1 if down, 0 if up */
int leftMouseButton = 0;
int middleMouseButton = 0;
int rightMouseButton = 0;

/* control state derived from mouse button state */
typedef enum { ROTATE, TRANSLATE, SCALE } CONTROLSTATE;
CONTROLSTATE controlState = ROTATE;

/* state of the world */
float landRotate[3] = {0.0, 0.0, 0.0};
float landTranslate[3] = {0.0, 0.0, 0.0};
float landScale[3] = {1.0, 1.0, 1.0};

/* see pic.h for type Pic */
Pic *heightData;

/* initializing GL */
void init()
{
  glClearColor(0.0, 0.0, 0.0, 0.0);
  glEnable(GL_DEPTH_TEST);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

/* draw floor in medium green */
void drawFloor()
{
  glColor3f(0.0, 0.75, 0.0);
  glBegin(GL_QUADS);
    glVertex3f(-0.5, -0.5, 0.0);
    glVertex3f(-0.5, 0.5, 0.0);
    glVertex3f(0.5, 0.5, 0.0);
    glVertex3f(0.5, -0.5, 0.0);
  glEnd();
}

/* drax x and y axes in medium red */
void drawXYAxes()
{
  glColor3f(0.75, 0.0, 0.0);
  glBegin(GL_LINES);
    glVertex3f(-0.5, 0.0, 0.0);
    glVertex3f(0.5, 0.0, 0.0);
    glVertex3f(0.0, -0.5, 0.0);
    glVertex3f(0.0, 0.5, 0.0);
  glEnd();
}

/* draw z axis in medium red */
void drawZAxis()
{
  glColor3f(0.75, 0.0, 0.0);
  glBegin(GL_LINES);
    glVertex3f(0.0, 0.0, 0.0);
    glVertex3f(0.0, 0.0, 1.0);
  glEnd();
}

/* draw height field in blue to white */
/* into cube -0.5 <= x,y < 0.5, 0.0 <= z <= 1.0 */
/* based on blue component of picture heightData */
/* if polygonmode = GL_POINT every point will be drawn twice */
/* this could be fixed with a small amount of additional complexity */
void drawHeightField()
{
  int i, j;
  GLfloat x, y, z;
  int nx = heightData->nx;  /* number of x-pixels */
  int ny = heightData->ny;  /* number of y-pixels */
  GLfloat s = (nx >= ny) ? nx : ny;  /* scale to preserve aspect ratio */
  for (i = 0; i < nx-1; i++)
  {
    x = (GLfloat) i;
    glBegin(GL_TRIANGLE_STRIP);
    for (j = 0; j < ny; j++)
    {
      y = (GLfloat) j;
      z = (GLfloat) PIC_PIXEL(heightData, i, j, 2);
      glColor3f(z/255.0, z/255.0, 1.0);
      glVertex3f(x/s-0.5, y/s-0.5, z/255.0);

      z = (GLfloat) PIC_PIXEL(heightData, i+1, j, 2);
      glColor3f(z/255.0, z/255.0, 1.0);
      glVertex3f((x+1.0)/s-0.5, y/s-0.5, z/255.0);
    }
    glEnd();
  }
}

/* display callback */
/* draw height field, floor and coordinate axes */
/* scaled, rotated, and translated */
void display()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity();
  gluLookAt(-1.0,-2.0,1.0, 0.0,0.0,0.5, 0.0,0.0,1.0);

  glTranslatef(landTranslate[0], landTranslate[1], landTranslate[2]);

  glRotatef(landRotate[0], 1.0, 0.0, 0.0);
  glRotatef(landRotate[1], 0.0, 1.0, 0.0);
  glRotatef(landRotate[2], 0.0, 0.0, 1.0);

  glScalef(landScale[0], landScale[1], landScale[2]);

  /* draw x-y-axes on top of floor */
  /* use just depth buffer (instead of stencil buffer) */
  glDepthMask(GL_FALSE);	/* set depth buffer to read-only */
  drawFloor();			/* draw floor into color buffer (only) */
  glDepthMask(GL_TRUE);		/* set depth buffer to read-write */
  drawXYAxes();			/* draw x-y-axes normally */
  glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
				/* set color buffer to read-only */
  drawFloor();			/* draw floor into depth-buffer (only) */
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
				/* set color buffer to read-write */

  glTranslatef(0.0, 0.0, 0.01);	/* move up slightly to prevent floor overlap */
  drawZAxis();			/* draw z-axis normally */
  drawHeightField();		/* draw height field normally */

  glFlush();
  glutSwapBuffers();
}

/* mouse drag callback */
/* converts mouse drags to scaling, rotations, and translations */
/* left mouse button controls x and y axes */
/* right mouse button controls z axes (height) */
void mousedrag(int x, int y)
{
  int dx = x-xMousePos;
  int dy = -(y-yMousePos); /* invert y-movement from screen coordinates */

  switch (controlState)
  {
    case TRANSLATE:  
      if (leftMouseButton)
      {
        landTranslate[0] += dx*0.01;
        landTranslate[1] += dy*0.01;
      }
      if (middleMouseButton)
      {
        landTranslate[2] += dy*0.01;
      }
      break;
    case ROTATE:
      if (leftMouseButton)
      {
        landRotate[0] -= dy; /* drag up = clockwise around x-axis */
        landRotate[1] += dx; /* drag right = counter-clockwise around y-axis */
      }
      if (middleMouseButton)
      {
        landRotate[2] += dx; /* drag right = counter-clockwise around z-axis */
      }
      break;
    case SCALE:
      if (leftMouseButton)
      {
        landScale[0] *= 1.0+dx*0.01; /* drag right = scale up in x-direction */
        landScale[1] *= 1.0+dy*0.01; /* drag up = scale up in y-direction */
      }
      if (middleMouseButton)
      {
        landScale[2] *= 1.0+dy*0.01; /* drag up = scale up in z-direction */
      }
      break;
  }
  xMousePos = x;
  yMousePos = y;

  glutPostRedisplay();
}

/* mouse callback */
/* set state based on modifier key */
/* ctrl = translate, shift = scale, otherwise rotate */
void mousebutton(int button, int state, int x, int y)
{
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state==GLUT_DOWN);
      break;
    case GLUT_RIGHT_BUTTON:	/* right button not used */
      rightMouseButton = (state==GLUT_DOWN);
      break;
  }
  switch(glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
      break;
    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
      break;
    default:
      controlState = ROTATE;
      break;
  }
  xMousePos = x;
  yMousePos = y;
}

/* reshape callback */
/* set projection to aspect ratio of window */
void reshape(int w, int h)
{
    GLfloat aspect = (GLfloat) w / (GLfloat) h;
    /* scale viewport with window */
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(FOV, aspect, 1.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

/* keyboard callback */
/* switch polygon mode or exit */
void keyboard(unsigned char key, int x, int y)
{
  switch (key) {
  case 'q': case 'Q':
    exit(0);
    break;
  case 'f': case 'F':
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case 'l': case 'L':
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    break;
  case 'p': case 'P':
    glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    break;
  }
  glutPostRedisplay();
}

/* main function */
int main (int argc, char **argv)
{
  if (argc<2)
  {  
    printf ("usage: %s heightfield.jpg\n", argv[0]);
    exit(1);
  }

  heightData = jpeg_read(argv[1], NULL);
  if (!heightData)
  {
    printf ("error reading %s.\n", argv[1]);
    exit(1);
  }

  /* create window */
  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutInitWindowSize(WIDTH, HEIGHT);
  glutCreateWindow("Assignment 1");

  /* set callbacks */
  glutDisplayFunc(display);	/* display callback */
  glutMotionFunc(mousedrag);	/* mouse drag callback */
  glutMouseFunc(mousebutton);	/* mouse button callback */
  glutReshapeFunc(reshape);	/* reshape callback */
  glutKeyboardFunc(keyboard);	/* keyboard callback */

  /* initialize GL */
  init();

  /* enter event loop */
  glutMainLoop();
  return(0);
}
