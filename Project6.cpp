#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#include "glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "glut.h"


//	This is a sample OpenGL / GLUT program
//
//	The objective is to draw a 3D Object and Map a Texture that will can be distorted
//
//	The left mouse button does rotation
//	The middle mouse button does scaling
//	The user interface allows:
//		1. Turning the texture on and off
//		2. Distorting the texture on and off
//		3. The projection to be changed
//		4. The transformations to be reset
//		5. The program to quit
//
//	Author:		Christian Ovchinikov
//	Date:		11/23/2021

// NOTE: There are a lot of good reasons to use const variables instead
// of #define's.  However, Visual C++ does not allow a const variable
// to be used as an array size or as the case in a switch( ) statement.  So in
// the following, all constants are const variables except those which need to
// be array sizes or cases in switch( ) statements.  Those are #defines.


// title of these windows:

const char* WINDOWTITLE = { "Geometric Modeling in OpenGL/GLUT -- Christian Ovchinikov" };
const char* GLUITITLE = { "User Interface Window" };

// what the glui package defines as true and false:

const int GLUITRUE = { true };
const int GLUIFALSE = { false };

// time for animation
const int MS_PER_CYCLE = 10;

// the escape key:

#define ESCAPE		0x1b

// initial window size:

const int INIT_WINDOW_SIZE = { 600 };

// size of the 3d box:

const float BOXSIZE = { 2.f };

// multiplication factors for input interaction:
//  (these are known from previous experience)

const float ANGFACT = { 1. };
const float SCLFACT = { 0.005f };

// minimum allowable scale factor:

const float MINSCALE = { 0.05f };

// scroll wheel button values:

const int SCROLL_WHEEL_UP = { 3 };
const int SCROLL_WHEEL_DOWN = { 4 };

// equivalent mouse movement when we click a the scroll wheel:

const float SCROLL_WHEEL_CLICK_FACTOR = { 5. };

// active mouse buttons (or them together):

const int LEFT = { 4 };
const int MIDDLE = { 2 };
const int RIGHT = { 1 };

// which projection:

enum Projections
{
	ORTHO,
	PERSP
};

// which button:

enum ButtonVals
{
	RESET,
	QUIT
};

// line width for the axes:

const GLfloat AXES_WIDTH = { 2. };

const GLfloat White[] = { 1., 1., 1. };


// window background color (rgba):

const GLfloat BACKCOLOR[] = { 0., 0., 0., 1. };


// fog parameters:

const GLfloat FOGCOLOR[4] = { .0, .0, .0, 1. };
const GLenum  FOGMODE = { GL_LINEAR };
const GLfloat FOGDENSITY = { 0.30f };
const GLfloat FOGSTART = { 1.5 };
const GLfloat FOGEND = { 4. };


// non-constant global variables:

int				ActiveButton;							// current button that is down
GLuint			AxesList;								// list to hold the axes
int				AxesOn;									// != 0 means to draw the axes
int				TextureOn;								// != 0 means to draw the axes
int				DebugOn;								// != 0 means to print debugging info
int				DepthBufferOn;							// != 0 means to use the z-buffer
int				DepthFightingOn;						// != 0 means to force the creation of z-fighting
int				MainWindow;								// window id for main graphics window
float			Scale;									// scaling factor
float			Time;								    // Time factor
int				ShadowsOn;								// != 0 means to turn shadows on
int				WhichProjection;						// ORTHO or PERSP
int				Xmouse, Ymouse;							// mouse values
float			Xrot, Yrot;								// rotation angles in degrees
bool			Frozen;									// current freeze status of animations
bool			PointsCheck = true;
bool			LinesCheck = true;

// function prototypes:

void	Animate();
void	Display();
void	DoAxesMenu(int);
void	DoDebugMenu(int);
void	DoLinesMenu(int);
void	DoPointsMenu(int);
void	DoMainMenu(int);
void	DoProjectMenu(int);
void	DoShadowMenu();
void	DoRasterString(float, float, float, char*);
void	DoStrokeString(float, float, float, float, char*);
float	ElapsedSeconds();
void	InitGraphics();
void	InitLists();
void	InitMenus();
void	Keyboard(unsigned char, int, int);
void	MouseButton(int, int, int, int);
void	MouseMotion(int, int);
void	Reset();
void	Resize(int, int);
void	Visibility(int);

void			Axes(float);
unsigned char*	BmpToTexture(char*, int*, int*);
void			HsvRgb(float[3], float[3]);
int				ReadInt(FILE*);
short			ReadShort(FILE*);

void			Cross(float[3], float[3], float[3]);
float			Dot(float[3], float[3]);
float			Unit(float[3], float[3]);


// Referenced from project instructions for organizing the data

struct Point
{
	float x0, y0, z0;       // initial coordinates
	float x, y, z;        // animated coordinates
};

struct Curve
{
	float r, g, b;
	Point p0, p1, p2, p3;
};

const int NUMCURVES = 12;
const int NUMPOINTS = 25;

Curve petals;
Curve stem;
Curve leaf1;
Curve leaf2;
Curve leaf3;
Curve leaf4;


// main program:

int
main(int argc, char* argv[])
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)

	glutInit(&argc, argv);


	// Initialize stem points
	
	stem.p0.x = 0;		stem.p0.y = 0;		stem.p0.z = 0;
	stem.p1.x = -2;		stem.p1.y = -6;		stem.p1.z = -2;
	stem.p2.x = 0;		stem.p2.y = -14;	stem.p2.z = 0;
	stem.p3.x = 1.5;	stem.p3.y = -28;	stem.p3.z = 0;


	// Initialize leaf1 points

	leaf1.p0.x = 1.5;	leaf1.p0.y = -28;	leaf1.p0.z = 0;
	leaf1.p1.x = 2.5;	leaf1.p1.y = -13;	leaf1.p1.z = 0;
	leaf1.p2.x = 8.5;	leaf1.p2.y = -25;	leaf1.p2.z = 0;
	leaf1.p3.x = 1.5;	leaf1.p3.y = -28;	leaf1.p3.z = 0;


	// Initialize leaf2 points

	leaf2.p0.x = 1.5;	leaf2.p0.y = -28;	leaf2.p0.z = 0;
	leaf2.p1.x = -2.5;	leaf2.p1.y = -13;	leaf2.p1.z = 0;
	leaf2.p2.x = -8.5;	leaf2.p2.y = -25;	leaf2.p2.z = 0;
	leaf2.p3.x = 1.5;	leaf2.p3.y = -28;	leaf2.p3.z = 0;


	// Initialize leaf3 points

	leaf3.p0.x = 1.5;	leaf3.p0.y = -28;	leaf3.p0.z = 0;
	leaf3.p1.x = 13.5;	leaf3.p1.y = -19;	leaf3.p1.z = 0;
	leaf3.p2.x = 23.5;	leaf3.p2.y = -30;	leaf3.p2.z = 0;
	leaf3.p3.x = 1.5;	leaf3.p3.y = -28;	leaf3.p3.z = 0;


	// Initialize leaf4 points

	leaf4.p0.x = 1.5;	leaf4.p0.y = -28;	leaf4.p0.z = 0;
	leaf4.p1.x = -13.5; leaf4.p1.y = -19;	leaf4.p1.z = 0;
	leaf4.p2.x = -23.5; leaf4.p2.y = -30;	leaf4.p2.z = 0;
	leaf4.p3.x = 1.5;	leaf4.p3.y = -28;	leaf4.p3.z = 0;

	// setup all the graphics stuff:

	InitGraphics();

	// create the display structures that will not change:

	InitLists();

	// init all the global variables used by Display( ):
	// this will also post a redisplay

	Reset();

	// setup all the user interface stuff:

	InitMenus();

	// draw the scene once and wait for some interaction:
	// (this will never return)


	glutSetWindow(MainWindow);
	glutMainLoop();

	// glutMainLoop( ) never returns
	// this line is here to make the compiler happy:

	return 0;
}


// this is where one would put code that is to be called
// everytime the glut main loop has nothing to do
//
// this is typically where animation parameters are set
//
// do not call Display( ) from here -- let glutMainLoop( ) do it

void
Animate()
{
	const int MS_IN_THE_ANIMATION_CYCLE = 10000;	// milliseconds in the animation loop
	int ms = glutGet(GLUT_ELAPSED_TIME);			// milliseconds since the program started
	ms %= MS_IN_THE_ANIMATION_CYCLE;				// milliseconds in the range 0 to MS_IN_THE_ANIMATION_CYCLE-1
	Time = (float)ms / (float)MS_IN_THE_ANIMATION_CYCLE;        // [ 0., 1. )


	// Stem animation

	stem.p1.x = stem.p1.x + cos(Time * 2 * (2 * M_PI))/100;
	stem.p1.y = stem.p1.y + cos(Time * 2 * (2 * M_PI))/50;
	stem.p1.z = stem.p1.z + cos(Time * 2 * (2 * M_PI))/100;

	stem.p2.x =  stem.p2.x + cos(Time * 2 * (2 * M_PI))/50;
	stem.p2.y =  stem.p2.y + cos(Time * 2 * (2 * M_PI))/100;
	stem.p2.z = stem.p2.z + cos(Time * 2 * (2 * M_PI))/50;


	// leaf1 animation

	leaf1.p1.x = leaf1.p1.x - cos(Time * M_PI) / 50;
	leaf1.p1.y = leaf1.p1.y - cos(Time * M_PI) / 100;
	leaf1.p1.z = leaf1.p1.z - cos(Time * M_PI) / 50;

	leaf1.p2.x = leaf1.p2.x - cos(Time * M_PI) / 100;
	leaf1.p2.y = leaf1.p2.y - cos(Time * M_PI) / 50;
	leaf1.p2.z = leaf1.p2.z - cos(Time * M_PI) / 100;


	// leaf2 animation

	leaf2.p1.x = leaf2.p1.x + cos(Time * M_PI) / 50;
	leaf2.p1.y = leaf2.p1.y + cos(Time * M_PI) / 100;
	leaf2.p1.z = leaf2.p1.z + cos(Time * M_PI) / 50;

	leaf2.p2.x = leaf2.p2.x + cos(Time * M_PI) / 100;
	leaf2.p2.y = leaf2.p2.y + cos(Time * M_PI) / 50;
	leaf2.p2.z = leaf2.p2.z + cos(Time * M_PI) / 100;
	

	// leaf3 animation

	leaf3.p1.x = leaf3.p1.x - cos(Time * M_PI) / 50;
	leaf3.p1.y = leaf3.p1.y - cos(Time * M_PI) / 100;
	leaf3.p1.z = leaf3.p1.z - cos(Time * M_PI) / 50;

	leaf3.p2.x = leaf3.p2.x - cos(Time * M_PI) / 100;
	leaf3.p2.y = leaf3.p2.y - cos(Time * M_PI) / 50;
	leaf3.p2.z = leaf3.p2.z - cos(Time * M_PI) / 100;


	// leaf4 animation

	leaf4.p1.x = leaf4.p1.x + cos(Time * M_PI) / 50;
	leaf4.p1.y = leaf4.p1.y + cos(Time * M_PI) / 100;
	leaf4.p1.z = leaf4.p1.z + cos(Time * M_PI) / 50;

	leaf4.p2.x = leaf4.p2.x + cos(Time * M_PI) / 100;
	leaf4.p2.y = leaf4.p2.y + cos(Time * M_PI) / 50;
	leaf4.p2.z = leaf4.p2.z + cos(Time * M_PI) / 100;


	glFlush();
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


// draw the complete scene:

void
Display()
{
	if (DebugOn != 0)
	{
		fprintf(stderr, "Display\n");
	}


	// set which window we want to do the graphics into:

	glutSetWindow(MainWindow);

	// erase the background:

	glDrawBuffer(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);


	// specify shading to be flat:

	glShadeModel(GL_SMOOTH);


	// set the viewport to a square centered in the window:

	GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
	GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);
	GLsizei v = vx < vy ? vx : vy;			// minimum dimension
	GLint xl = (vx - v) / 2;
	GLint yb = (vy - v) / 2;
	glViewport(xl, yb, v, v);


	// set the viewing volume:
	// remember that the Z clipping  values are actually
	// given as DISTANCES IN FRONT OF THE EYE
	// USE gluOrtho2D( ) IF YOU ARE DOING 2D !

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	if (WhichProjection == ORTHO)
		glOrtho(-55., 55., -55., 55., 0.1, 1000.);
	else
		gluPerspective(90., 1., 0.1, 1000.);


	// place the objects into the scene:

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// set the eye position, look-at position, and up-vector:

	gluLookAt(0., 0., 35., 0., 0., 0., 0., 1., 0.);

	// rotate the scene:

	glRotatef((GLfloat)Yrot, 0., 1., 0.);
	glRotatef((GLfloat)Xrot, 1., 0., 0.);


	// uniformly scale the scene:

	if (Scale < MINSCALE)
		Scale = MINSCALE;
	glScalef((GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale);

	// possibly draw the axes:

	if (AxesOn != 0)
	{
		glColor3fv(White);
		glCallList(AxesList);
	}

	// since we are using glScalef( ), be sure normals get unitized:

	glEnable(GL_NORMALIZE);


	// Drawing Flower Stem

	if (PointsCheck)
	{
		glPointSize(5.);
		glBegin(GL_POINTS);
		glColor3f(1., 1., 1.);
		glVertex3f(stem.p0.x, stem.p0.y, stem.p0.z);
		glVertex3f(stem.p1.x, stem.p1.y, stem.p1.z);
		glVertex3f(stem.p2.x, stem.p2.y, stem.p2.z);
		glVertex3f(stem.p3.x, stem.p3.y, stem.p3.z);
		glEnd();
	}

	if (LinesCheck)
	{
		glBegin(GL_LINE_STRIP);
		glColor3f(1., 1., 1.);
		glVertex3f(stem.p0.x, stem.p0.y, stem.p0.z);
		glVertex3f(stem.p1.x, stem.p1.y, stem.p1.z);
		glVertex3f(stem.p2.x, stem.p2.y, stem.p2.z);
		glVertex3f(stem.p3.x, stem.p3.y, stem.p3.z);
		glEnd();
	}

	// Draw Bezier Curve (referenced from project information page)
	glColor3f(0., 1., 0.);
	glLineWidth(3.);
	glBegin(GL_LINE_STRIP);
	for (int it = 0; it <= NUMPOINTS; it++)
	{
		float t = (float)it / (float)NUMPOINTS;
		float omt = 1.f - t;
		float x = omt * omt * omt * stem.p0.x + 3.f * t * omt * omt * stem.p1.x + 3.f * t * t * omt * stem.p2.x + t * t * t * stem.p3.x;
		float y = omt * omt * omt * stem.p0.y + 3.f * t * omt * omt * stem.p1.y + 3.f * t * t * omt * stem.p2.y + t * t * t * stem.p3.y;
		float z = omt * omt * omt * stem.p0.z + 3.f * t * omt * omt * stem.p1.z + 3.f * t * t * omt * stem.p2.z + t * t * t * stem.p3.z;
		glVertex3f(x, y, z);
	}
	glEnd();
	glLineWidth(1.);


	// Drawing Flower Petals

	for (int i = 0; i < NUMCURVES; i++) {
		petals.p0.x = -0.773;
		petals.p0.y = 2.899;
		petals.p0.z = 0;

		petals.p1.x = -3.092;
		petals.p1.y = 11.595;
		petals.p1.z = 0;

		petals.p2.x = 3.092;
		petals.p2.y = 11.595;
		petals.p2.z = 0;

		petals.p3.x = 0.773;
		petals.p3.y = 2.899;
		petals.p3.z = 0;

		if (PointsCheck)
		{
			glBegin(GL_POINTS);
			glColor3f(1., 1., 1.);
			glVertex3f(petals.p0.x, petals.p0.y, petals.p0.z);
			glVertex3f(petals.p1.x, petals.p1.y, petals.p1.z);
			glVertex3f(petals.p2.x, petals.p2.y, petals.p2.z);
			glVertex3f(petals.p3.x, petals.p3.y, petals.p3.z);
			glEnd();
		}

		if (LinesCheck)
		{
			glBegin(GL_LINE_STRIP);
			glColor3f(1., 1., 1.);
			glVertex3f(petals.p0.x, petals.p0.y, petals.p0.z);
			glVertex3f(petals.p1.x, petals.p1.y, petals.p1.z);
			glVertex3f(petals.p2.x, petals.p2.y, petals.p2.z);
			glVertex3f(petals.p3.x, petals.p3.y, petals.p3.z);
			glEnd();
		}

		// Draw Bezier Curve (referenced from project information page)
		glLineWidth(3.);
		if (i == 0)
			glColor3f(0.5, 1., 0.);
		else if (i == 1)
			glColor3f(1., 1., 0.);
		else if (i == 2)
			glColor3f(1., 0.5, 0.);
		else if (i == 3)
			glColor3f(1., 0., 0.);
		else if (i == 4)
			glColor3f(1., 0., 0.5);
		else if (i == 5)
			glColor3f(1., 0., 1.);
		else if (i == 6)
			glColor3f(0.5, 0., 1.);
		else if (i == 7)
			glColor3f(0., 0., 1.);
		else if (i == 8)
			glColor3f(0., 0.5, 1.);
		else if (i == 9)
			glColor3f(0., 1., 1.);
		else if (i == 10)
			glColor3f(0., 1., 0.5);
		else if (i == 11)
			glColor3f(0., 1., .0);
		glBegin(GL_LINE_STRIP);
		for (int it = 0; it <= NUMPOINTS; it++)
		{
			float t = (float)it / (float)NUMPOINTS;
			float omt = 1.f - t;
			float x = omt * omt * omt * petals.p0.x + 3.f * t * omt * omt * petals.p1.x + 3.f * t * t * omt * petals.p2.x + t * t * t * petals.p3.x;
			float y = omt * omt * omt * petals.p0.y + 3.f * t * omt * omt * petals.p1.y + 3.f * t * t * omt * petals.p2.y + t * t * t * petals.p3.y;
			float z = omt * omt * omt * petals.p0.z + 3.f * t * omt * omt * petals.p1.z + 3.f * t * t * omt * petals.p2.z + t * t * t * petals.p3.z;
			glVertex3f(x, y, z);
		}
		glEnd();
		glLineWidth(1.);
		glRotatef(-30, 0, 0, 1);
	}

	
	// Drawing Leafs

	if (PointsCheck)
	{
		glPointSize(5.);
		glBegin(GL_POINTS);
		glColor3f(1., 1., 1.);
		glVertex3f(leaf1.p0.x, leaf1.p0.y, leaf1.p0.z);
		glVertex3f(leaf1.p1.x, leaf1.p1.y, leaf1.p1.z);
		glVertex3f(leaf1.p2.x, leaf1.p2.y, leaf1.p2.z);
		glVertex3f(leaf1.p3.x, leaf1.p3.y, leaf1.p3.z);

		glVertex3f(leaf2.p0.x, leaf2.p0.y, leaf2.p0.z);
		glVertex3f(leaf2.p1.x, leaf2.p1.y, leaf2.p1.z);
		glVertex3f(leaf2.p2.x, leaf2.p2.y, leaf2.p2.z);
		glVertex3f(leaf2.p3.x, leaf2.p3.y, leaf2.p3.z);

		glVertex3f(leaf3.p0.x, leaf3.p0.y, leaf3.p0.z);
		glVertex3f(leaf3.p1.x, leaf3.p1.y, leaf3.p1.z);
		glVertex3f(leaf3.p2.x, leaf3.p2.y, leaf3.p2.z);
		glVertex3f(leaf3.p3.x, leaf3.p3.y, leaf3.p3.z);

		glVertex3f(leaf4.p0.x, leaf4.p0.y, leaf4.p0.z);
		glVertex3f(leaf4.p1.x, leaf4.p1.y, leaf4.p1.z);
		glVertex3f(leaf4.p2.x, leaf4.p2.y, leaf4.p2.z);
		glVertex3f(leaf4.p3.x, leaf4.p3.y, leaf4.p3.z);
		glEnd();
	}

	if (LinesCheck)
	{
		glBegin(GL_LINE_STRIP);
		glColor3f(1., 1., 1.);
		glVertex3f(leaf1.p0.x, leaf1.p0.y, leaf1.p0.z);
		glVertex3f(leaf1.p1.x, leaf1.p1.y, leaf1.p1.z);
		glVertex3f(leaf1.p2.x, leaf1.p2.y, leaf1.p2.z);
		glVertex3f(leaf1.p3.x, leaf1.p3.y, leaf1.p3.z);
		glEnd();

		glBegin(GL_LINE_STRIP);
		glVertex3f(leaf2.p0.x, leaf2.p0.y, leaf2.p0.z);
		glVertex3f(leaf2.p1.x, leaf2.p1.y, leaf2.p1.z);
		glVertex3f(leaf2.p2.x, leaf2.p2.y, leaf2.p2.z);
		glVertex3f(leaf2.p3.x, leaf2.p3.y, leaf2.p3.z);
		glEnd();

		glBegin(GL_LINE_STRIP);
		glVertex3f(leaf3.p0.x, leaf3.p0.y, leaf3.p0.z);
		glVertex3f(leaf3.p1.x, leaf3.p1.y, leaf3.p1.z);
		glVertex3f(leaf3.p2.x, leaf3.p2.y, leaf3.p2.z);
		glVertex3f(leaf3.p3.x, leaf3.p3.y, leaf3.p3.z);
		glEnd();

		glBegin(GL_LINE_STRIP);
		glVertex3f(leaf4.p0.x, leaf4.p0.y, leaf4.p0.z);
		glVertex3f(leaf4.p1.x, leaf4.p1.y, leaf4.p1.z);
		glVertex3f(leaf4.p2.x, leaf4.p2.y, leaf4.p2.z);
		glVertex3f(leaf4.p3.x, leaf4.p3.y, leaf4.p3.z);
		glEnd();
	}

	// Draw Bezier Curve (referenced from project information page)
	glColor3f(0., 1., 0.);
	glLineWidth(3.);
	glBegin(GL_LINE_STRIP);
	for (int it = 0; it <= NUMPOINTS; it++)
	{
		float t = (float)it / (float)NUMPOINTS;
		float omt = 1.f - t;
		float x = omt * omt * omt * leaf1.p0.x + 3.f * t * omt * omt * leaf1.p1.x + 3.f * t * t * omt * leaf1.p2.x + t * t * t * leaf1.p3.x;
		float y = omt * omt * omt * leaf1.p0.y + 3.f * t * omt * omt * leaf1.p1.y + 3.f * t * t * omt * leaf1.p2.y + t * t * t * leaf1.p3.y;
		float z = omt * omt * omt * leaf1.p0.z + 3.f * t * omt * omt * leaf1.p1.z + 3.f * t * t * omt * leaf1.p2.z + t * t * t * leaf1.p3.z;
		glVertex3f(x, y, z);
	}
	glEnd();

	glBegin(GL_LINE_STRIP);
	for (int it = 0; it <= NUMPOINTS; it++)
	{
		float t = (float)it / (float)NUMPOINTS;
		float omt = 1.f - t;
		float x = omt * omt * omt * leaf2.p0.x + 3.f * t * omt * omt * leaf2.p1.x + 3.f * t * t * omt * leaf2.p2.x + t * t * t * leaf2.p3.x;
		float y = omt * omt * omt * leaf2.p0.y + 3.f * t * omt * omt * leaf2.p1.y + 3.f * t * t * omt * leaf2.p2.y + t * t * t * leaf2.p3.y;
		float z = omt * omt * omt * leaf2.p0.z + 3.f * t * omt * omt * leaf2.p1.z + 3.f * t * t * omt * leaf2.p2.z + t * t * t * leaf2.p3.z;
		glVertex3f(x, y, z);
	}
	glEnd();

	glBegin(GL_LINE_STRIP);
	for (int it = 0; it <= NUMPOINTS; it++)
	{
		float t = (float)it / (float)NUMPOINTS;
		float omt = 1.f - t;
		float x = omt * omt * omt * leaf3.p0.x + 3.f * t * omt * omt * leaf3.p1.x + 3.f * t * t * omt * leaf3.p2.x + t * t * t * leaf3.p3.x;
		float y = omt * omt * omt * leaf3.p0.y + 3.f * t * omt * omt * leaf3.p1.y + 3.f * t * t * omt * leaf3.p2.y + t * t * t * leaf3.p3.y;
		float z = omt * omt * omt * leaf3.p0.z + 3.f * t * omt * omt * leaf3.p1.z + 3.f * t * t * omt * leaf3.p2.z + t * t * t * leaf3.p3.z;
		glVertex3f(x, y, z);
	}
	glEnd();

	glBegin(GL_LINE_STRIP);
	for (int it = 0; it <= NUMPOINTS; it++)
	{
		float t = (float)it / (float)NUMPOINTS;
		float omt = 1.f - t;
		float x = omt * omt * omt * leaf4.p0.x + 3.f * t * omt * omt * leaf4.p1.x + 3.f * t * t * omt * leaf4.p2.x + t * t * t * leaf4.p3.x;
		float y = omt * omt * omt * leaf4.p0.y + 3.f * t * omt * omt * leaf4.p1.y + 3.f * t * t * omt * leaf4.p2.y + t * t * t * leaf4.p3.y;
		float z = omt * omt * omt * leaf4.p0.z + 3.f * t * omt * omt * leaf4.p1.z + 3.f * t * t * omt * leaf4.p2.z + t * t * t * leaf4.p3.z;
		glVertex3f(x, y, z);
	}
	glEnd();

	glLineWidth(1.);


	// draw some gratuitous text that just rotates on top of the scene:

	glDisable(GL_DEPTH_TEST);
	glColor3f(0., 1., 1.);
	DoRasterString(0., 1., 0., (char*)"");


	// draw some gratuitous text that is fixed on the screen:
	//
	// the projection matrix is reset to define a scene whose
	// world coordinate system goes from 0-100 in each axis
	//
	// this is called "percent units", and is just a convenience
	//
	// the modelview matrix is reset to identity as we don't
	// want to transform these coordinates

	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0., 100., 0., 100.);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glColor3f(1., 1., 1.);
	DoRasterString(2., 95., 0., (char*)"Geometric Modeling - Rainbow Flower");

	glColor3f(1., 1., 1.);
	DoRasterString(2., 12., 0., (char*)"(F) Freeze Animation");
	DoRasterString(2., 7., 0., (char*)"(R) Reset");
	DoRasterString(2., 2., 0., (char*)"(Q) Quit");


	// swap the double-buffered framebuffers:

	glutSwapBuffers();


	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !

	glFlush();
}


void
DoAxesMenu(int id)
{
	AxesOn = id;
	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoPointsMenu(int id)
{
	if (id == 1)
		PointsCheck = true;
	else if (id == 0)
		PointsCheck = false;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoLinesMenu(int id)
{
	if (id == 1)
		LinesCheck = true;
	else if (id == 0)
		LinesCheck = false;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoProjectMenu(int id)
{
	WhichProjection = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoDebugMenu(int id)
{
	DebugOn = id;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

// main menu callback:

void
DoMainMenu(int id)
{
	switch (id)
	{
	case RESET:
		Reset();
		break;

	case QUIT:
		// gracefully close out the graphics:
		// gracefully close the graphics window:
		// gracefully exit the program:
		glutSetWindow(MainWindow);
		glFinish();
		glutDestroyWindow(MainWindow);
		exit(0);
		break;

	default:
		fprintf(stderr, "Don't know what to do with Main Menu ID %d\n", id);
	}

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


// use glut to display a string of characters using a raster font:

void
DoRasterString(float x, float y, float z, char* s)
{
	glRasterPos3f((GLfloat)x, (GLfloat)y, (GLfloat)z);

	char c;			// one character to print
	for (; (c = *s) != '\0'; s++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
	}
}


// use glut to display a string of characters using a stroke font:

void
DoStrokeString(float x, float y, float z, float ht, char* s)
{
	glPushMatrix();
	glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z);
	float sf = ht / (119.05f + 33.33f);
	glScalef((GLfloat)sf, (GLfloat)sf, (GLfloat)sf);
	char c;			// one character to print
	for (; (c = *s) != '\0'; s++)
	{
		glutStrokeCharacter(GLUT_STROKE_ROMAN, c);
	}
	glPopMatrix();
}


// return the number of seconds since the start of the program:

float
ElapsedSeconds()
{
	// get # of milliseconds since the start of the program:

	int ms = glutGet(GLUT_ELAPSED_TIME);

	// convert it to seconds:

	return (float)ms / 1000.f;
}


// initialize the glui window:

void
InitMenus()
{
	glutSetWindow(MainWindow);

	int axesmenu = glutCreateMenu(DoAxesMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int pointsmenu = glutCreateMenu(DoPointsMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int linesmenu = glutCreateMenu(DoLinesMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int projmenu = glutCreateMenu(DoProjectMenu);
	glutAddMenuEntry("Orthographic", ORTHO);
	glutAddMenuEntry("Perspective", PERSP);

	int debugmenu = glutCreateMenu(DoDebugMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int mainmenu = glutCreateMenu(DoMainMenu);
	glutAddSubMenu("Axes", axesmenu);
	glutAddSubMenu("Points", pointsmenu);
	glutAddSubMenu("Lines", linesmenu);
	glutAddSubMenu("Projection", projmenu);
	glutAddSubMenu("Debug", debugmenu);
	glutAddMenuEntry("Reset", RESET);
	glutAddMenuEntry("Quit", QUIT);

	// attach the pop-up menu to the right mouse button:

	glutAttachMenu(GLUT_RIGHT_BUTTON);
}



// initialize the glut and OpenGL libraries:
//	also setup display lists and callback functions

void
InitGraphics()
{
	// request the display modes:
	// ask for red-green-blue-alpha color, double-buffering, and z-buffering:

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);

	// set the initial window configuration:

	glutInitWindowPosition(0, 0);
	glutInitWindowSize(INIT_WINDOW_SIZE, INIT_WINDOW_SIZE);

	// open the window and set its title:

	MainWindow = glutCreateWindow(WINDOWTITLE);
	glutSetWindowTitle(WINDOWTITLE);
	glutSetWindowTitle(WINDOWTITLE);

	// set the framebuffer clear values:

	glClearColor(BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3]);

	// setup the callback functions:
	// DisplayFunc -- redraw the window
	// ReshapeFunc -- handle the user resizing the window
	// KeyboardFunc -- handle a keyboard input
	// MouseFunc -- handle the mouse button going down or up
	// MotionFunc -- handle the mouse moving with a button down
	// PassiveMotionFunc -- handle the mouse moving with a button up
	// VisibilityFunc -- handle a change in window visibility
	// EntryFunc	-- handle the cursor entering or leaving the window
	// SpecialFunc -- handle special keys on the keyboard
	// SpaceballMotionFunc -- handle spaceball translation
	// SpaceballRotateFunc -- handle spaceball rotation
	// SpaceballButtonFunc -- handle spaceball button hits
	// ButtonBoxFunc -- handle button box hits
	// DialsFunc -- handle dial rotations
	// TabletMotionFunc -- handle digitizing tablet motion
	// TabletButtonFunc -- handle digitizing tablet button hits
	// MenuStateFunc -- declare when a pop-up menu is in use
	// TimerFunc -- trigger something to happen a certain time from now
	// IdleFunc -- what to do when nothing else is going on

	glutSetWindow(MainWindow);
	glutDisplayFunc(Display);
	glutReshapeFunc(Resize);
	glutKeyboardFunc(Keyboard);
	glutMouseFunc(MouseButton);
	glutMotionFunc(MouseMotion);
	glutPassiveMotionFunc(MouseMotion);
	//glutPassiveMotionFunc( NULL );
	glutVisibilityFunc(Visibility);
	glutEntryFunc(NULL);
	glutSpecialFunc(NULL);
	glutSpaceballMotionFunc(NULL);
	glutSpaceballRotateFunc(NULL);
	glutSpaceballButtonFunc(NULL);
	glutButtonBoxFunc(NULL);
	glutDialsFunc(NULL);
	glutTabletMotionFunc(NULL);
	glutTabletButtonFunc(NULL);
	glutMenuStateFunc(NULL);
	glutTimerFunc(-1, NULL, 0);
	glutIdleFunc(Animate);

	// init glew (a window must be open to do this):

#ifdef WIN32
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		fprintf(stderr, "glewInit Error\n");
	}
	else
		fprintf(stderr, "GLEW initialized OK\n");
	fprintf(stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
#endif

}


// initialize the display lists that will not change:
// (a display list is a way to store opengl commands in
//  memory so that they can be played back efficiently at a later time
//  with a call to glCallList( )

void
InitLists()
{
	glutSetWindow(MainWindow);

	// create the axes:

	AxesList = glGenLists(1);
	glNewList(AxesList, GL_COMPILE);
	glLineWidth(AXES_WIDTH);
	Axes(5);
	glLineWidth(1.);


	glEndList();

}


// the keyboard callback:

void
Keyboard(unsigned char c, int x, int y)
{
	if (DebugOn != 0)
		fprintf(stderr, "Keyboard: '%c' (0x%0x)\n", c, c);

	switch (c)
	{
	case 'o':
	case 'O':
		WhichProjection = ORTHO;
		break;

	case 'p':
	case 'P':
		WhichProjection = PERSP;
		break;

	case 'f':
	case 'F':
		Frozen = !Frozen;
		if (Frozen)
			glutIdleFunc(NULL);
		else
			glutIdleFunc(Animate);
		break;

	case 'r':
	case 'R':
		Reset();
		break;

	case 'q':
	case 'Q':
	case ESCAPE:
		DoMainMenu(QUIT);
		break;

	default:
		fprintf(stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c);
	}

	// force a call to Display( ):

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


// called when the mouse button transitions down or up:

void
MouseButton(int button, int state, int x, int y)
{
	int b = 0;			// LEFT, MIDDLE, or RIGHT

	if (DebugOn != 0)
		fprintf(stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y);


	// get the proper button bit mask:

	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		b = LEFT;		break;

	case GLUT_MIDDLE_BUTTON:
		b = MIDDLE;		break;

	case GLUT_RIGHT_BUTTON:
		b = RIGHT;		break;

	case SCROLL_WHEEL_UP:
		Scale += SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
		// keep object from turning inside-out or disappearing:
		if (Scale < MINSCALE)
			Scale = MINSCALE;
		break;

	case SCROLL_WHEEL_DOWN:
		Scale -= SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
		// keep object from turning inside-out or disappearing:
		if (Scale < MINSCALE)
			Scale = MINSCALE;
		break;

	default:
		b = 0;
		fprintf(stderr, "Unknown mouse button: %d\n", button);
	}

	// button down sets the bit, up clears the bit:

	if (state == GLUT_DOWN)
	{
		Xmouse = x;
		Ymouse = y;
		ActiveButton |= b;		// set the proper bit
	}
	else
	{
		ActiveButton &= ~b;		// clear the proper bit
	}

	glutSetWindow(MainWindow);
	glutPostRedisplay();

}


// called when the mouse moves while a button is down:

void
MouseMotion(int x, int y)
{
	if (true)
		fprintf(stderr, "MouseMotion: %d, %d\n", x, y);


	int dx = x - Xmouse;		// change in mouse coords
	int dy = y - Ymouse;

	if ((ActiveButton & LEFT) != 0)
	{
		Xrot += (ANGFACT * dy);
		Yrot += (ANGFACT * dx);
	}


	if ((ActiveButton & MIDDLE) != 0)
	{
		Scale += SCLFACT * (float)(dx - dy);

		// keep object from turning inside-out or disappearing:

		if (Scale < MINSCALE)
			Scale = MINSCALE;
	}

	Xmouse = x;			// new current position
	Ymouse = y;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


// reset the transformations and the colors:
// this only sets the global variables --
// the glut main loop is responsible for redrawing the scene

void
Reset()
{
	ActiveButton = 0;
	AxesOn = 1;
	DebugOn = 0;
	DepthBufferOn = 1;
	DepthFightingOn = 0;
	Scale = 1.0;
	ShadowsOn = 0;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
	Frozen = false;
	glutIdleFunc(Animate);
	PointsCheck = true;
	LinesCheck = true;
	glFlush();
}


// called when user resizes the window:

void
Resize(int width, int height)
{
	if (DebugOn != 0)
		fprintf(stderr, "ReSize: %d, %d\n", width, height);

	// don't really need to do anything since window size is
	// checked each time in Display( ):

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


// handle a change to the window's visibility:

void
Visibility(int state)
{
	if (DebugOn != 0)
		fprintf(stderr, "Visibility: %d\n", state);

	if (state == GLUT_VISIBLE)
	{
		glutSetWindow(MainWindow);
		glutPostRedisplay();
	}
	else
	{
		// could optimize by keeping track of the fact
		// that the window is not visible and avoid
		// animating or redrawing it ...
	}
}



///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////


// the stroke characters 'X' 'Y' 'Z' :

static float xx[] = {
		0.f, 1.f, 0.f, 1.f
};

static float xy[] = {
		-.5f, .5f, .5f, -.5f
};

static int xorder[] = {
		1, 2, -3, 4
};

static float yx[] = {
		0.f, 0.f, -.5f, .5f
};

static float yy[] = {
		0.f, .6f, 1.f, 1.f
};

static int yorder[] = {
		1, 2, 3, -2, 4
};

static float zx[] = {
		1.f, 0.f, 1.f, 0.f, .25f, .75f
};

static float zy[] = {
		.5f, .5f, -.5f, -.5f, 0.f, 0.f
};

static int zorder[] = {
		1, 2, 3, 4, -5, 6
};

// fraction of the length to use as height of the characters:
const float LENFRAC = 0.10f;

// fraction of length to use as start location of the characters:
const float BASEFRAC = 1.10f;

void
Axes(float length)
{
	glBegin(GL_LINE_STRIP);
	glVertex3f(length, 0., 0.);
	glVertex3f(0., 0., 0.);
	glVertex3f(0., length, 0.);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(0., 0., 0.);
	glVertex3f(0., 0., length);
	glEnd();

	float fact = LENFRAC * length;
	float base = BASEFRAC * length;

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < 4; i++)
	{
		int j = xorder[i];
		if (j < 0)
		{

			glEnd();
			glBegin(GL_LINE_STRIP);
			j = -j;
		}
		j--;
		glVertex3f(base + fact * xx[j], fact * xy[j], 0.0);
	}
	glEnd();

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < 5; i++)
	{
		int j = yorder[i];
		if (j < 0)
		{

			glEnd();
			glBegin(GL_LINE_STRIP);
			j = -j;
		}
		j--;
		glVertex3f(fact * yx[j], base + fact * yy[j], 0.0);
	}
	glEnd();

	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < 6; i++)
	{
		int j = zorder[i];
		if (j < 0)
		{

			glEnd();
			glBegin(GL_LINE_STRIP);
			j = -j;
		}
		j--;
		glVertex3f(0.0, fact * zy[j], base + fact * zx[j]);
	}
	glEnd();

}

// Extra functions from sample.cpp


int
ReadInt(FILE* fp)
{
	const unsigned char b0 = fgetc(fp);
	const unsigned char b1 = fgetc(fp);
	const unsigned char b2 = fgetc(fp);
	const unsigned char b3 = fgetc(fp);
	return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}


short
ReadShort(FILE* fp)
{
	const unsigned char b0 = fgetc(fp);
	const unsigned char b1 = fgetc(fp);
	return (b1 << 8) | b0;
}


// function to convert HSV to RGB
// 0.  <=  s, v, r, g, b  <=  1.
// 0.  <= h  <=  360.
// when this returns, call:
//		glColor3fv( rgb );

void
HsvRgb(float hsv[3], float rgb[3])
{
	// guarantee valid input:

	float h = hsv[0] / 60.f;
	while (h >= 6.)	h -= 6.;
	while (h < 0.) 	h += 6.;

	float s = hsv[1];
	if (s < 0.)
		s = 0.;
	if (s > 1.)
		s = 1.;

	float v = hsv[2];
	if (v < 0.)
		v = 0.;
	if (v > 1.)
		v = 1.;

	// if sat==0, then is a gray:

	if (s == 0.0)
	{
		rgb[0] = rgb[1] = rgb[2] = v;
		return;
	}

	// get an rgb from the hue itself:

	float i = (float)floor(h);
	float f = h - i;
	float p = v * (1.f - s);
	float q = v * (1.f - s * f);
	float t = v * (1.f - (s * (1.f - f)));

	float r = 0., g = 0., b = 0.;			// red, green, blue
	switch ((int)i)
	{
	case 0:
		r = v;	g = t;	b = p;
		break;

	case 1:
		r = q;	g = v;	b = p;
		break;

	case 2:
		r = p;	g = v;	b = t;
		break;

	case 3:
		r = p;	g = q;	b = v;
		break;

	case 4:
		r = t;	g = p;	b = v;
		break;

	case 5:
		r = v;	g = p;	b = q;
		break;
	}


	rgb[0] = r;
	rgb[1] = g;
	rgb[2] = b;
}

void
Cross(float v1[3], float v2[3], float vout[3])
{
	float tmp[3];
	tmp[0] = v1[1] * v2[2] - v2[1] * v1[2];
	tmp[1] = v2[0] * v1[2] - v1[0] * v2[2];
	tmp[2] = v1[0] * v2[1] - v2[0] * v1[1];
	vout[0] = tmp[0];
	vout[1] = tmp[1];
	vout[2] = tmp[2];
}

float
Dot(float v1[3], float v2[3])
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

float
Unit(float vin[3], float vout[3])
{
	float dist = vin[0] * vin[0] + vin[1] * vin[1] + vin[2] * vin[2];
	if (dist > 0.0)
	{
		dist = sqrtf(dist);
		vout[0] = vin[0] / dist;
		vout[1] = vin[1] / dist;
		vout[2] = vin[2] / dist;
	}
	else
	{
		vout[0] = vin[0];
		vout[1] = vin[1];
		vout[2] = vin[2];
	}
	return dist;
}

