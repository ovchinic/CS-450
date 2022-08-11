#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <ctime>

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
//	Date:		12/01/2021

// NOTE: There are a lot of good reasons to use const variables instead
// of #define's.  However, Visual C++ does not allow a const variable
// to be used as an array size or as the case in a switch( ) statement.  So in
// the following, all constants are const variables except those which need to
// be array sizes or cases in switch( ) statements.  Those are #defines.


// title of these windows:

const char* WINDOWTITLE = { "Minecraft in OpenGL/GLUT -- Christian Ovchinikov" };
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
int				TextureOn;								// != 0 means to draw the axes
int				DebugOn;								// != 0 means to print debugging info
int				DepthBufferOn;							// != 0 means to use the z-buffer
int				DepthFightingOn;						// != 0 means to force the creation of z-fighting
int				MainWindow;								// window id for main graphics window
float			Scale;									// scaling factor
float			Time;								    // Time factor
int				ShadowsOn;								// != 0 means to turn shadows on
int				WhichColor;								// index into Colors[ ]
int				WhichProjection;						// ORTHO or PERSP
int				Xmouse, Ymouse;							// mouse values
float			Xrot, Yrot;								// rotation angles in degrees
bool			Light0On, Light1On = true;				// keeps track of light statuses
bool			Frozen;									// current freeze status of animations
float			PigPosX, PigPosZ;						// Pig Head position
bool			Day = false;							// Keeps track if it is Day or Night, starts in Night
GLuint			BoxList;								// Block Object
GLuint			PlaneList;								// Flat Ground Object
GLuint			SlabList;								// Slab Object
GLuint			DoorList;								// Door Object
GLuint			WindowList;								// Window Object
GLuint			TorchList;								// Torch Object
GLuint			PigList;								// Pig Body Object
GLuint			PigLegList;								// Pig Leg Object
GLuint			PigFaceList;							// Pig Head/Face Object
GLuint			PigNoseList;							// Pig Nose Object
GLuint			dark_oak_planks;						// Dark Oak Texture
GLuint			grass;									// Grass Texture
GLuint			stonebrick;								// Stone Brick Texture
GLuint			door;									// Door Texture
GLuint			pigface;								// Pig Face Texture
GLuint			pigbody;								// Pig Body Texture
GLuint			pignose;								// Pig Nose Texture
GLuint			moon;									// Moon Texture
int				WhichDirection = 0;						// Keeps track of the direction of the eye


// initiliaze eye position and look at so that key commands can update it in glutlookat()

float			eyex = -1., eyey = 3.5, eyez = 10., lookx = -1., looky = 3.4, lookz = 0., upx = 0., upy = 1., upz = 0.;


// function prototypes:

void	Animate();
void	Display();
void	DoDebugMenu(int);
void	DoLightsMenu(int);
void	DoMainMenu(int);
void	DoTimeMenu(int);
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

unsigned char* BmpToTexture(char*, int*, int*);
void			HsvRgb(float[3], float[3]);
int				ReadInt(FILE*);
short			ReadShort(FILE*);

void			Cross(float[3], float[3], float[3]);
float			Dot(float[3], float[3]);
float			Unit(float[3], float[3]);


// Referenced from slide 21 of Lighting Material from lecture

float White[] = { 1., 1., 1., 1. };

float* Array3(float a, float b, float c)
{
	static float array[4];

	array[0] = a;
	array[1] = b;
	array[2] = c;
	array[3] = 1.;

	return array;
}

float* MulArray3(float factor, float array0[3])
{
	static float array[4];

	array[0] = factor * array0[0];
	array[1] = factor * array0[1];
	array[2] = factor * array0[2];
	array[3] = 1.;

	return array;
}


// Referenced from slide 27 of Lighting Material from lecture

void SetMaterial(float r, float g, float b, float shininess)
{
	glMaterialfv(GL_BACK, GL_EMISSION, Array3(0., 0., 0.));
	glMaterialfv(GL_BACK, GL_AMBIENT, MulArray3(0.4f, White));
	glMaterialfv(GL_BACK, GL_DIFFUSE, MulArray3(1., White));
	glMaterialfv(GL_BACK, GL_SPECULAR, Array3(0., 0., 0.));
	glMaterialf(GL_BACK, GL_SHININESS, 2.f);

	glMaterialfv(GL_FRONT, GL_EMISSION, Array3(0., 0., 0.));
	glMaterialfv(GL_FRONT, GL_AMBIENT, Array3(r, g, b));
	glMaterialfv(GL_FRONT, GL_DIFFUSE, Array3(r, g, b));
	glMaterialfv(GL_FRONT, GL_SPECULAR, MulArray3(0.8f, White));
	glMaterialf(GL_FRONT, GL_SHININESS, shininess);
}


// Referenced from slide 35 of Lighting Material from lecture

void SetPointLight(int ilight, float x, float y, float z, float r, float g, float b, float consatten, float quadatten)
{
	glLightfv(ilight, GL_POSITION, Array3(x, y, z));
	glLightfv(ilight, GL_AMBIENT, Array3(0., 0., 0.));
	glLightfv(ilight, GL_DIFFUSE, Array3(r, g, b));
	glLightfv(ilight, GL_SPECULAR, Array3(r, g, b));
	glLightf(ilight, GL_CONSTANT_ATTENUATION, consatten);
	glLightf(ilight, GL_LINEAR_ATTENUATION, 0.);
	glLightf(ilight, GL_QUADRATIC_ATTENUATION, quadatten);
	glEnable(ilight);
}


// OsuSphere.cpp provided by Mike Bailey for this assignment

int		NumLngs, NumLats;
struct point* Pts;

struct point
{
	float x, y, z;		// coordinates
	float nx, ny, nz;	// surface normal
	float s, t;		// texture coords
};

inline
struct point*
	PtsPointer(int lat, int lng)
{
	if (lat < 0)	lat += (NumLats - 1);
	if (lng < 0)	lng += (NumLngs - 0);
	if (lat > NumLats - 1)	lat -= (NumLats - 1);
	if (lng > NumLngs - 1)	lng -= (NumLngs - 0);
	return &Pts[NumLngs * lat + lng];
}

inline
void
DrawPoint(struct point* p)
{
	glNormal3fv(&p->nx);
	glTexCoord2fv(&p->s);
	glVertex3fv(&p->x);
}

void
OsuSphere(float radius, int slices, int stacks)
{
	// set the globals:

	NumLngs = slices;
	NumLats = stacks;
	if (NumLngs < 3)
		NumLngs = 3;
	if (NumLats < 3)
		NumLats = 3;

	// allocate the point data structure:

	Pts = new struct point[NumLngs * NumLats];

	// fill the Pts structure:

	for (int ilat = 0; ilat < NumLats; ilat++)
	{
		float lat = -M_PI / 2. + M_PI * (float)ilat / (float)(NumLats - 1);	// ilat=0/lat=0. is the south pole
											// ilat=NumLats-1, lat=+M_PI/2. is the north pole
		float xz = cosf(lat);
		float  y = sinf(lat);
		for (int ilng = 0; ilng < NumLngs; ilng++)				// ilng=0, lng=-M_PI and
											// ilng=NumLngs-1, lng=+M_PI are the same meridian
		{
			float lng = -M_PI + 2. * M_PI * (float)ilng / (float)(NumLngs - 1);
			float x = xz * cosf(lng);
			float z = -xz * sinf(lng);
			struct point* p = PtsPointer(ilat, ilng);
			p->x = radius * x;
			p->y = radius * y;
			p->z = radius * z;
			p->nx = x;
			p->ny = y;
			p->nz = z;
			p->s = (lng + M_PI) / (2. * M_PI);
			p->t = (lat + M_PI / 2.) / M_PI;
		}
	}

	struct point top, bot;		// top, bottom points

	top.x = 0.;		top.y = radius;	top.z = 0.;
	top.nx = 0.;		top.ny = 1.;		top.nz = 0.;
	top.s = 0.;		top.t = 1.;

	bot.x = 0.;		bot.y = -radius;	bot.z = 0.;
	bot.nx = 0.;		bot.ny = -1.;		bot.nz = 0.;
	bot.s = 0.;		bot.t = 0.;

	// connect the north pole to the latitude NumLats-2:


	glBegin(GL_TRIANGLE_STRIP);
	for (int ilng = 0; ilng < NumLngs; ilng++)
	{
		glTexCoord2f(bot.s, bot.t);
		float lng = -M_PI + 2. * M_PI * (float)ilng / (float)(NumLngs - 1);
		top.s = (lng + M_PI) / (2. * M_PI);
		DrawPoint(&top);
		struct point* p = PtsPointer(NumLats - 2, ilng);	// ilat=NumLats-1 is the north pole
		DrawPoint(p);
	}
	glEnd();

	// connect the south pole to the latitude 1:

	glBegin(GL_TRIANGLE_STRIP);
	for (int ilng = NumLngs - 1; ilng >= 0; ilng--)
	{
		glTexCoord2f(bot.s, bot.t);
		float lng = -M_PI + 2. * M_PI * (float)ilng / (float)(NumLngs - 1);
		bot.s = (lng + M_PI) / (2. * M_PI);
		DrawPoint(&bot);
		struct point* p = PtsPointer(1, ilng);					// ilat=0 is the south pole
		DrawPoint(p);
	}
	glEnd();

	// connect the horizontal strips:

	for (int ilat = 2; ilat < NumLats - 1; ilat++)
	{
		struct point* p;
		glBegin(GL_TRIANGLE_STRIP);
		for (int ilng = 0; ilng < NumLngs; ilng++)
		{
			p = PtsPointer(ilat, ilng);
			DrawPoint(p);
			p = PtsPointer(ilat - 1, ilng);
			DrawPoint(p);
		}
		glEnd();
	}
	// clean-up:
	delete[] Pts;
	Pts = NULL;
}


// main program:

int
main(int argc, char* argv[])
{
	// turn on the glut package:
	// (do this before checking argc and argv since it might
	// pull some command line arguments out)

	glutInit(&argc, argv);

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
	srand(time(0));
	const int MS_IN_THE_ANIMATION_CYCLE = 10000;	// milliseconds in the animation loop
	int ms = glutGet(GLUT_ELAPSED_TIME);			// milliseconds since the program started
	ms %= MS_IN_THE_ANIMATION_CYCLE;				// milliseconds in the range 0 to MS_IN_THE_ANIMATION_CYCLE-1
	Time = (float)ms / (float)MS_IN_THE_ANIMATION_CYCLE;        // [ 0., 1. )
	float timer = 0.5 + 0.5 * sin(2. * M_PI * Time);

	int randNum = rand() % 2;
	if (randNum >= 0.7)
		PigPosX = timer;
	else if (randNum <= 0.2)
		PigPosZ = timer;
	else if (randNum > 0.5 && randNum < 0.7)
		PigPosZ = timer;
	else if (randNum <= 0.5 && randNum < 0.2)
		PigPosX = timer;

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
		gluPerspective(10., 1., 0.1, 1000.);


	// place the objects into the scene:
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// set the eye position, look-at position, and up-vector:
	gluLookAt(eyex, eyey, eyez, lookx, looky, lookz, upx, upy, upz);



	// rotate the scene:

	glRotatef((GLfloat)Yrot, 0., 1., 0.);
	glRotatef((GLfloat)Xrot, 1., 0., 0.);


	// uniformly scale the scene:

	if (Scale < MINSCALE)
		Scale = MINSCALE;
	glScalef((GLfloat)Scale, (GLfloat)Scale, (GLfloat)Scale);


	// Turn the lights on

	if (Light0On)
		glEnable(GL_LIGHT0);
	else
		glDisable(GL_LIGHT0);

	if (Light1On)
		glEnable(GL_LIGHT1);
	else
		glDisable(GL_LIGHT1);


	// since we are using glScalef( ), be sure normals get unitized:

	glEnable(GL_NORMALIZE);

	
	// Draw Sky

	if (!Day) {
		glColor3f(0.06, 0.06, 0.12);
		OsuSphere(400., 10., 10.);
	}
	else {
		glColor3f(0.33, 0.62, 0.98);
		OsuSphere(400., 10., 10.);
	}

	glEnable(GL_TEXTURE_2D);


	// Draw Moon/Sun

	glPushMatrix();
	if (!Day) {
		glBindTexture(GL_TEXTURE_2D, moon);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glMatrixMode(GL_TEXTURE_2D);
	}
	else {
		glDisable(GL_TEXTURE_2D);
		glColor3f(1., 0.9, 0.65);
	}
	glRotatef(5, 0., 0., 1.);
	glTranslatef(300., 0., 0.);
	glScalef(2., 2., 2.);
	glCallList(BoxList);
	glPopMatrix();

	glEnable(GL_TEXTURE_2D);


	// Draw grass on ground

	glEnable(GL_LIGHTING);

	glBindTexture(GL_TEXTURE_2D, grass);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glMatrixMode(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glCallList(PlaneList);

	glDisable(GL_LIGHTING);


	// Draw door

	glBindTexture(GL_TEXTURE_2D, door);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glCallList(DoorList);


	// Draw stone brick walls

	glEnable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, stonebrick);
	if (!Day)
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	else
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glMatrixMode(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glPushMatrix();
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-8, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);


	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);

	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);

	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glPopMatrix();
	glDisable(GL_LIGHTING);


	// Draw dark oak plank walls

	glEnable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, dark_oak_planks);
	if (!Day)
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	else
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glMatrixMode(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glPushMatrix();
	glTranslatef(0, 2, 0);
	glCallList(BoxList);
	glTranslatef(-4, 0, 0);
	glCallList(BoxList);
	glTranslatef(-4, 0, 0);
	glCallList(BoxList);

	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);

	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);

	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);


	glTranslatef(-2, 2, 0);
	glCallList(BoxList);

	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);

	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);
	glTranslatef(0, 0, -2);
	glCallList(BoxList);

	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);

	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);
	glTranslatef(0, 0, 2);
	glCallList(BoxList);

	glTranslatef(0, 2, 0);
	glCallList(SlabList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(SlabList);

	glTranslatef(4, 2, 0);
	glCallList(SlabList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(SlabList);

	glTranslatef(4, -2, -10);
	glCallList(SlabList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(BoxList);
	glTranslatef(-2, 0, 0);
	glCallList(SlabList);

	glTranslatef(4, 2, 0);
	glCallList(SlabList);
	glTranslatef(2, 0, 0);
	glCallList(BoxList);
	glTranslatef(2, 0, 0);
	glCallList(SlabList);
	glPopMatrix();
	glDisable(GL_LIGHTING);


	// Draw stone brick roof

	glEnable(GL_LIGHTING);
	glBindTexture(GL_TEXTURE_2D, stonebrick);
	if (!Day)
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	else
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glMatrixMode(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);

	glPushMatrix();
	glTranslatef(6, 6, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);

	glTranslatef(-2, 1, 0);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);

	glTranslatef(-2, 1, 0);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);

	glTranslatef(-2, 1, 0);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);

	glTranslatef(-2, 1, 0);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);

	glTranslatef(-2, -1, 0);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);

	glTranslatef(-2, -1, 0);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);

	glTranslatef(-2, -1, 0);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);
	glTranslatef(0, 0, 2);
	glCallList(SlabList);

	glTranslatef(-2, -1, 0);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glTranslatef(0, 0, -2);
	glCallList(SlabList);
	glPopMatrix();
	glDisable(GL_LIGHTING);

	// Draw Pig
	glPushMatrix();


	glTranslatef(7., 0., 0.);
	glBindTexture(GL_TEXTURE_2D, pigbody);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glPushMatrix();
	glTranslatef(0., 0.25, 5.);
	glCallList(PigList);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.1, 0., 5.);
	glCallList(PigLegList);
	glTranslatef(0., 0., 0.5);
	glCallList(PigLegList);
	glTranslatef(0.75, 0., 0.);
	glCallList(PigLegList);
	glTranslatef(0., 0., -0.5);
	glCallList(PigLegList);
	glPopMatrix();


	glRotatef(PigPosX*2, 1., 0., 0.);
	glRotatef(PigPosZ*4, 0., 0., 1.);


	glBindTexture(GL_TEXTURE_2D, pigface);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glPushMatrix();
	glTranslatef(-0.75, 0.25, 4.875);
	glCallList(PigFaceList);
	glPopMatrix();

	glBindTexture(GL_TEXTURE_2D, pignose);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

	glPushMatrix();
	glTranslatef(-0.90, 0.25, 5.125);
	glScalef(0.5, 0.35, 0.525);
	glCallList(PigFaceList);
	glPopMatrix();

	glDisable(GL_TEXTURE_2D);

	glPopMatrix();


	// Draw windows

	glPushMatrix();
	glCallList(WindowList);
	glTranslatef(-8., 0., 0.);
	glCallList(WindowList);
	glPopMatrix();


	// Draw torches

	glPushMatrix();

	glEnable(GL_LIGHTING);
	SetPointLight(GL_LIGHT0, -3., 3., 4, 1., 1., 0.75, 1., 0.);
	if (Light0On)
		glEnable(GL_LIGHT0);
	else
		glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	glTranslatef(-3., 3., 2.2);
	glRotatef(25., 1., 0., 0.);
	glCallList(TorchList);
	glPopMatrix();

	glEnable(GL_LIGHTING);
	SetPointLight(GL_LIGHT1, 1., 3., 4., 1., 1., 0.75, 1., 0.);
	if (Light1On)
		glEnable(GL_LIGHT1);
	else
		glDisable(GL_LIGHT1);
	glDisable(GL_LIGHTING);

	glPushMatrix();

	glTranslatef(1., 3., 2.2);
	glRotatef(25., 1., 0., 0.);
	glCallList(TorchList);
	glPopMatrix();


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
	DoRasterString(2., 95., 0., (char*)"L");

	glColor3f(1., 1., 1.);
	DoRasterString(2., 2., 0., (char*)"Minecraft - C++");




	// swap the double-buffered framebuffers:

	glutSwapBuffers();


	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !

	glFlush();
}


void
DoLightsMenu(int id)
{
	if (id == 0)
		Light0On = !Light0On;
	else if (id == 1)
		Light1On = !Light1On;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}


void
DoTimeMenu(int id)
{
	if (id == 0)
		Day = true;
	else if (id == 1)
		Day = false;

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

	int lightsmenu = glutCreateMenu(DoLightsMenu);
	glutAddMenuEntry("Left Torch - On/Off", 0);
	glutAddMenuEntry("Right Torch - On/Off", 1);

	int timemenu = glutCreateMenu(DoTimeMenu);
	glutAddMenuEntry("Daytime", 0);
	glutAddMenuEntry("Nighttime", 1);

	int debugmenu = glutCreateMenu(DoDebugMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int mainmenu = glutCreateMenu(DoMainMenu);
	glutAddSubMenu("Torches", lightsmenu);
	glutAddSubMenu("Time of Day", timemenu);
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

	int width1, height1, width2, height2, width3, height3, width4, height4, 
		width5, height5, width6, height6, width7, height7, width8, height8;

	unsigned char * TextureArray1 = BmpToTexture("acacia.bmp", &width1, &height1);
	unsigned char* TextureArray2 = BmpToTexture("grass.bmp", &width2, &height2);
	unsigned char* TextureArray3 = BmpToTexture("stonebrick.bmp", &width3, &height3);
	unsigned char* TextureArray4 = BmpToTexture("door.bmp", &width4, &height4);
	unsigned char* TextureArray5 = BmpToTexture("pigbody.bmp", &width5, &height5);
	unsigned char* TextureArray6 = BmpToTexture("pigface.bmp", &width6, &height6);
	unsigned char* TextureArray7 = BmpToTexture("pignose.bmp", &width7, &height7);
	unsigned char* TextureArray8 = BmpToTexture("moon.bmp", &width8, &height8);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	glGenTextures(1, &dark_oak_planks);
	glGenTextures(1, &grass);
	glGenTextures(1, &stonebrick);
	glGenTextures(1, &door);
	glGenTextures(1, &pigbody);
	glGenTextures(1, &pigface);
	glGenTextures(1, &pignose);
	glGenTextures(1, &moon);

	glBindTexture(GL_TEXTURE_2D, dark_oak_planks);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width1, height1, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureArray1);

	glBindTexture(GL_TEXTURE_2D, grass);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width2, height2, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureArray2);

	glBindTexture(GL_TEXTURE_2D, stonebrick);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width3, height3, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureArray3);

	glBindTexture(GL_TEXTURE_2D, door);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width4, height4, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureArray4);

	glBindTexture(GL_TEXTURE_2D, pigbody);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width5, height5, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureArray5);

	glBindTexture(GL_TEXTURE_2D, pigface);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width6, height6, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureArray6);
	
	glBindTexture(GL_TEXTURE_2D, pignose);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width7, height7, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureArray7);

	glBindTexture(GL_TEXTURE_2D, moon);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width8, height8, 0, GL_RGB, GL_UNSIGNED_BYTE, TextureArray8);

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

	// create the grass:
	PlaneList = glGenLists(1);
	glNewList(PlaneList, GL_COMPILE);

	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex3f(-200, 0, -200);
	glTexCoord2f(200, 0);
	glVertex3f(-200, 0, 200);
	glTexCoord2f(200, 201);
	glVertex3f(202, 0, 200);
	glTexCoord2f(0, 201);
	glVertex3f(202, 0, -200);
	glEnd();

	glEndList();


	// create the door:
	DoorList = glGenLists(1);
	glNewList(DoorList, GL_COMPILE);

	glBegin(GL_QUADS);
	glTexCoord2f(1, 0);
	glVertex3f(-1.85, 0, 0);
	glTexCoord2f(1, 1);
	glVertex3f(-1.85, 4, 0);
	glTexCoord2f(0, 1);
	glVertex3f(-2, 4, 2);
	glTexCoord2f(0, 0);
	glVertex3f(-2, 0, 2);
	glEnd();

	glEndList();

	// create the blocks:
	BoxList = glGenLists(1);
	glNewList(BoxList, GL_COMPILE);
	GLdouble a[] = { 0., 0., 2 };
	GLdouble b[] = { 2, 0., 2 };
	GLdouble c[] = { 2, 2, 2 };
	GLdouble d[] = { 0., 2, 2 };
	GLdouble e[] = { 0., 0., 0. };
	GLdouble f[] = { 2, 0., 0. };
	GLdouble g[] = { 2, 2, 0. };
	GLdouble h[] = { 0., 2, 0. };

	// Front
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3dv(a);
	glTexCoord2f(1, 0);
	glVertex3dv(b);
	glTexCoord2f(1, 1);
	glVertex3dv(c);
	glTexCoord2f(0, 1);
	glVertex3dv(d);
	glEnd();

	// Right
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glTexCoord2f(0, 1);
	glVertex3dv(c);
	glTexCoord2f(0, 0);
	glVertex3dv(b);
	glTexCoord2f(1, 0);
	glVertex3dv(f);
	glTexCoord2f(1, 1);
	glVertex3dv(g);
	glEnd();

	// Back
	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glTexCoord2f(1, 1);
	glVertex3dv(h);
	glTexCoord2f(0, 1);
	glVertex3dv(g);
	glTexCoord2f(0, 0);
	glVertex3dv(f);
	glTexCoord2f(1, 0);
	glVertex3dv(e);
	glEnd();

	// Left
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glTexCoord2f(1, 1);
	glVertex3dv(d);
	glTexCoord2f(0, 1);
	glVertex3dv(h);
	glTexCoord2f(0, 0);
	glVertex3dv(e);
	glTexCoord2f(1, 0);
	glVertex3dv(a);
	glEnd();

	// Top
	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glTexCoord2f(1, 0);
	glVertex3dv(d);
	glTexCoord2f(1, 1);
	glVertex3dv(c);
	glTexCoord2f(0, 1);
	glVertex3dv(g);
	glTexCoord2f(0, 0);
	glVertex3dv(h);
	glEnd();

	// Bottom
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glTexCoord2f(0, 1);
	glVertex3dv(e);
	glTexCoord2f(0, 0);
	glVertex3dv(f);
	glTexCoord2f(1, 0);
	glVertex3dv(b);
	glTexCoord2f(1, 1);
	glVertex3dv(a);
	glEnd();
	glEndList();


	// create the Slab Blocks:
	SlabList = glGenLists(1);
	glNewList(SlabList, GL_COMPILE);
	GLdouble c1[] = { 2, 1, 2 };
	GLdouble d1[] = { 0., 1, 2 };
	GLdouble g1[] = { 2, 1, 0. };
	GLdouble h1[] = { 0., 1, 0. };

	// Front
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glTexCoord2f(0, 0);
	glVertex3dv(a);
	glTexCoord2f(1, 0);
	glVertex3dv(b);
	glTexCoord2f(1, 0.5);
	glVertex3dv(c1);
	glTexCoord2f(0, 0.5);
	glVertex3dv(d1);
	glEnd();

	// Right
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glTexCoord2f(0, 0.5);
	glVertex3dv(c1);
	glTexCoord2f(0, 0);
	glVertex3dv(b);
	glTexCoord2f(1, 0);
	glVertex3dv(f);
	glTexCoord2f(1, 0.5);
	glVertex3dv(g1);
	glEnd();

	// Back
	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glTexCoord2f(1, 0.5);
	glVertex3dv(h1);
	glTexCoord2f(0, 0.5);
	glVertex3dv(g1);
	glTexCoord2f(0, 0);
	glVertex3dv(f);
	glTexCoord2f(1, 0);
	glVertex3dv(e);
	glEnd();

	// Left
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glTexCoord2f(1, 0.5);
	glVertex3dv(d1);
	glTexCoord2f(0, 0.5);
	glVertex3dv(h1);
	glTexCoord2f(0, 0);
	glVertex3dv(e);
	glTexCoord2f(1, 0);
	glVertex3dv(a);
	glEnd();

	// Top
	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glTexCoord2f(1, 0);
	glVertex3dv(d1);
	glTexCoord2f(1, 1);
	glVertex3dv(c1);
	glTexCoord2f(0, 1);
	glVertex3dv(g1);
	glTexCoord2f(0, 0);
	glVertex3dv(h1);
	glEnd();

	// Bottom
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glTexCoord2f(0, 1);
	glVertex3dv(e);
	glTexCoord2f(0, 0);
	glVertex3dv(f);
	glTexCoord2f(1, 0);
	glVertex3dv(b);
	glTexCoord2f(1, 1);
	glVertex3dv(a);
	glEnd();
	glEndList();

	// create the Window:
	WindowList = glGenLists(1);
	glNewList(WindowList, GL_COMPILE);
	glColor3f(0.9, 0.9, 1.);
	glLineWidth(20.);
	glBegin(GL_LINE_STRIP);
	glVertex3f(2.1, 2.1, 1.);
	glVertex3f(3.9, 2.1, 1.);
	glVertex3f(3.9, 3.9, 1.);
	glVertex3f(2.1, 3.9, 1.);
	glVertex3f(2.1, 2.1, 1.);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(2.5, 2.5, 1.);
	glVertex3f(2.8, 2.8, 1.);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(3.4, 3.1, 1.);
	glVertex3f(3.9, 3.6, 1.);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(2.1, 3.1, 1.);
	glVertex3f(2.6, 3.6, 1.);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(3.1, 2.2, 1.);
	glVertex3f(3.7, 2.8, 1.);
	glEnd();
	glBegin(GL_LINE_STRIP);
	glVertex3f(3.0, 3.7, 1.);
	glVertex3f(3.2, 3.9, 1.);
	glEnd();
	glEndList();


	// create the torches:
	TorchList = glGenLists(1);
	glNewList(TorchList, GL_COMPILE);
	glPushMatrix();
	glTranslatef(0., 0.5, 0.);
	glColor3f(1., 1., 0.5);
	glutSolidCube(0.2);
	glPopMatrix();

	glColor3f(0.4, 0.2, 0.05);
	glScalef(0.2, 1., 0.2);
	glutSolidCube(1.);
	glEndList();


	// create the pig body:
	PigList = glGenLists(1);
	glNewList(PigList, GL_COMPILE);
	GLdouble aPIG[] = { 0., 0., 0.75 };
	GLdouble bPIG[] = { 1, 0., 0.75 };
	GLdouble cPIG[] = { 1, 0.5, 0.75 };
	GLdouble dPIG[] = { 0., 0.5, 0.75 };
	GLdouble ePIG[] = { 0., 0., 0. };
	GLdouble fPIG[] = { 1, 0., 0. };
	GLdouble gPIG[] = { 1, 0.5, 0. };
	GLdouble hPIG[] = { 0., 0.5, 0. };

	// Front
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glTexCoord2f(0, 1);
	glVertex3dv(aPIG);
	glTexCoord2f(0, 0);
	glVertex3dv(bPIG);
	glTexCoord2f(1, 0);
	glVertex3dv(cPIG);
	glTexCoord2f(1, 1);
	glVertex3dv(dPIG);
	glEnd();

	// Right
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glTexCoord2f(1, 1);
	glVertex3dv(cPIG);
	glTexCoord2f(0, 1);
	glVertex3dv(bPIG);
	glTexCoord2f(0, 0);
	glVertex3dv(fPIG);
	glTexCoord2f(1, 0);
	glVertex3dv(gPIG);
	glEnd();

	// Back
	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glTexCoord2f(1, 0);
	glVertex3dv(hPIG);
	glTexCoord2f(1, 1);
	glVertex3dv(gPIG);
	glTexCoord2f(0, 1);
	glVertex3dv(fPIG);
	glTexCoord2f(0, 0);
	glVertex3dv(ePIG);
	glEnd();

	// Left
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glTexCoord2f(1, 0);
	glVertex3dv(dPIG);
	glTexCoord2f(1, 1);
	glVertex3dv(hPIG);
	glTexCoord2f(0, 1);
	glVertex3dv(ePIG);
	glTexCoord2f(0, 0);
	glVertex3dv(aPIG);
	glEnd();

	// Top
	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glTexCoord2f(1, 0);
	glVertex3dv(dPIG);
	glTexCoord2f(1, 1);
	glVertex3dv(cPIG);
	glTexCoord2f(0, 1);
	glVertex3dv(gPIG);
	glTexCoord2f(0, 0);
	glVertex3dv(hPIG);
	glEnd();

	// Bottom
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glTexCoord2f(1, 0);
	glVertex3dv(ePIG);
	glTexCoord2f(1, 1);
	glVertex3dv(fPIG);
	glTexCoord2f(0, 1);
	glVertex3dv(bPIG);
	glTexCoord2f(0, 0);
	glVertex3dv(aPIG);
	glEnd();
	glEndList();

	PigLegList = glGenLists(1);
	glNewList(PigLegList, GL_COMPILE);
	GLdouble aPIG1[] = { 0., 0., 0.25 };
	GLdouble bPIG1[] = { 0.25, 0., 0.25 };
	GLdouble cPIG1[] = { 0.25, 0.25, 0.25 };
	GLdouble dPIG1[] = { 0., 0.25, 0.25 };
	GLdouble ePIG1[] = { 0., 0., 0. };
	GLdouble fPIG1[] = { 0.25, 0., 0. };
	GLdouble gPIG1[] = { 0.25, 0.25, 0. };
	GLdouble hPIG1[] = { 0., 0.25, 0. };

	// Front
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glTexCoord2f(0, 1);
	glVertex3dv(aPIG1);
	glTexCoord2f(0, 0);
	glVertex3dv(bPIG1);
	glTexCoord2f(1, 0);
	glVertex3dv(cPIG1);
	glTexCoord2f(1, 1);
	glVertex3dv(dPIG1);
	glEnd();

	// Right
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glTexCoord2f(1, 1);
	glVertex3dv(cPIG1);
	glTexCoord2f(0, 1);
	glVertex3dv(bPIG1);
	glTexCoord2f(0, 0);
	glVertex3dv(fPIG1);
	glTexCoord2f(1, 0);
	glVertex3dv(gPIG1);
	glEnd();

	// Back
	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glTexCoord2f(1, 0);
	glVertex3dv(hPIG1);
	glTexCoord2f(1, 1);
	glVertex3dv(gPIG1);
	glTexCoord2f(0, 1);
	glVertex3dv(fPIG1);
	glTexCoord2f(0, 0);
	glVertex3dv(ePIG1);
	glEnd();

	// Left
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glTexCoord2f(1, 0);
	glVertex3dv(dPIG1);
	glTexCoord2f(1, 1);
	glVertex3dv(hPIG1);
	glTexCoord2f(0, 1);
	glVertex3dv(ePIG1);
	glTexCoord2f(0, 0);
	glVertex3dv(aPIG1);
	glEnd();

	// Top
	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glTexCoord2f(1, 0);
	glVertex3dv(dPIG1);
	glTexCoord2f(1, 1);
	glVertex3dv(cPIG1);
	glTexCoord2f(0, 1);
	glVertex3dv(gPIG1);
	glTexCoord2f(0, 0);
	glVertex3dv(hPIG1);
	glEnd();

	// Bottom
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glTexCoord2f(1, 0);
	glVertex3dv(ePIG1);
	glTexCoord2f(1, 1);
	glVertex3dv(fPIG1);
	glTexCoord2f(0, 1);
	glVertex3dv(bPIG1);
	glTexCoord2f(0, 0);
	glVertex3dv(aPIG1);
	glEnd();
	glEndList();


	// Pig Face

	PigFaceList = glGenLists(1);
	glNewList(PigFaceList, GL_COMPILE);
	GLdouble aPIG2[] = { 0., 0., 1 };
	GLdouble bPIG2[] = { 1, 0., 1 };
	GLdouble cPIG2[] = { 1, 1, 1 };
	GLdouble dPIG2[] = { 0., 1, 1 };
	GLdouble ePIG2[] = { 0., 0., 0. };
	GLdouble fPIG2[] = { 1, 0., 0. };
	GLdouble gPIG2[] = { 1, 1, 0. };
	GLdouble hPIG2[] = { 0., 1, 0. };

	// Front
	glBegin(GL_QUADS);
	glNormal3d(0, 0, 1);
	glTexCoord2f(0, 0.3);
	glVertex3dv(aPIG2);
	glTexCoord2f(0, 0);
	glVertex3dv(bPIG2);
	glTexCoord2f(0.3, 0);
	glVertex3dv(cPIG2);
	glTexCoord2f(0.3, 0.3);
	glVertex3dv(dPIG2);
	glEnd();

	// Right
	glBegin(GL_QUADS);
	glNormal3d(1, 0, 0);
	glTexCoord2f(0.5, 0.5);
	glVertex3dv(cPIG2);
	glTexCoord2f(0, 0.5);
	glVertex3dv(bPIG2);
	glTexCoord2f(0, 0);
	glVertex3dv(fPIG2);
	glTexCoord2f(0.5, 0);
	glVertex3dv(gPIG2);
	glEnd();

	// Back
	glBegin(GL_QUADS);
	glNormal3d(0, 0, -1);
	glTexCoord2f(0.5, 0);
	glVertex3dv(hPIG2);
	glTexCoord2f(0.5, 0.5);
	glVertex3dv(gPIG2);
	glTexCoord2f(0, 0.5);
	glVertex3dv(fPIG2);
	glTexCoord2f(0, 0);
	glVertex3dv(ePIG2);
	glEnd();

	// Left
	glBegin(GL_QUADS);
	glNormal3d(-1, 0, 0);
	glTexCoord2f(1, 1);
	glVertex3dv(dPIG2);
	glTexCoord2f(0, 1);
	glVertex3dv(hPIG2);
	glTexCoord2f(0, 0);
	glVertex3dv(ePIG2);
	glTexCoord2f(1, 0);
	glVertex3dv(aPIG2);
	glEnd();

	// Top
	glBegin(GL_QUADS);
	glNormal3d(0, 1, 0);
	glTexCoord2f(0.5, 0);
	glVertex3dv(dPIG2);
	glTexCoord2f(0.5, 0.5);
	glVertex3dv(cPIG2);
	glTexCoord2f(0, 0.5);
	glVertex3dv(gPIG2);
	glTexCoord2f(0, 0);
	glVertex3dv(hPIG2);
	glEnd();

	// Bottom
	glBegin(GL_QUADS);
	glNormal3d(0, -1, 0);
	glTexCoord2f(0.5, 0);
	glVertex3dv(ePIG2);
	glTexCoord2f(0.5, 0.5);
	glVertex3dv(fPIG2);
	glTexCoord2f(0, 0.5);
	glVertex3dv(bPIG2);
	glTexCoord2f(0, 0);
	glVertex3dv(aPIG2);
	glEnd();
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

	case 'w':
	case 'W':
		if (WhichDirection == 0) {
			eyez--;
			lookz--;
		}
		else if (WhichDirection == 1) {
			eyex--;
			lookx--;
		}
		else if (WhichDirection == 2) {
			eyez++;
			lookz++;
		}
		else if (WhichDirection == 3) {
			eyex++;
			lookx++;
		}
		break;

	case 's':
	case 'S':
		if (WhichDirection == 0) {
			eyez++;		
			lookz++;
		}
		else if (WhichDirection == 1) {
			eyex++;
			lookx++;
		}
		else if (WhichDirection == 2) {
			eyez--;
			lookz--;
		}
		else if (WhichDirection == 3) {
			eyex--;
			lookx--;
		}
		break;

	case 'a':
	case 'A':
		WhichDirection++;
		if (WhichDirection == 4)
			WhichDirection = 0;


		if (WhichDirection == 0) {
			lookx = lookx - 10;
			lookz = lookz - 10;
		}
		else if (WhichDirection == 1) {
			lookx = lookx - 10;
			lookz = lookz + 10;
		}
		else if (WhichDirection == 2) {
			lookx = lookx + 10;
			lookz = lookz + 10;
		}
		else if (WhichDirection == 3) {
			lookx = lookx + 10;
			lookz = lookz - 10;
		}
		break;

	case 'd':
	case 'D':
		WhichDirection--;
		if (WhichDirection == -1)
			WhichDirection = 3;


		if (WhichDirection == 0) {
			lookx = lookx + 10;
			lookz = lookz - 10;
		}
		else if (WhichDirection == 1) {
			lookx = lookx - 10;
			lookz = lookz - 10;
		}
		else if (WhichDirection == 2) {
			lookx = lookx - 10;
			lookz = lookz + 10;
		}
		else if (WhichDirection == 3) {
			lookx = lookx + 10;
			lookz = lookz + 10;
		}
		break;
		break;


	case 'r':
	case 'R':
		Reset();
		break;

	case '1':
		Light0On = !Light0On;
		break;

	case '2':
		Light1On = !Light1On;
		break;

	case '3':
		Day = !Day;
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
	TextureOn = 1;
	DebugOn = 0;
	DepthBufferOn = 1;
	DepthFightingOn = 0;
	Scale = 1.0;
	ShadowsOn = 0;
	WhichProjection = PERSP;
	Xrot = Yrot = 0.;
	Light0On = Light1On = true;
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


// read a BMP file into a Texture:

#define VERBOSE		false
#define BMP_MAGIC_NUMBER	0x4d42
#ifndef BI_RGB
#define BI_RGB			0
#define BI_RLE8			1
#define BI_RLE4			2
#endif


// bmp file header:
struct bmfh
{
	short bfType;		// BMP_MAGIC_NUMBER = "BM"
	int bfSize;		// size of this file in bytes
	short bfReserved1;
	short bfReserved2;
	int bfOffBytes;		// # bytes to get to the start of the per-pixel data
} FileHeader;

// bmp info header:
struct bmih
{
	int biSize;		// info header size, should be 40
	int biWidth;		// image width
	int biHeight;		// image height
	short biPlanes;		// #color planes, should be 1
	short biBitCount;	// #bits/pixel, should be 1, 4, 8, 16, 24, 32
	int biCompression;	// BI_RGB, BI_RLE4, BI_RLE8
	int biSizeImage;
	int biXPixelsPerMeter;
	int biYPixelsPerMeter;
	int biClrUsed;		// # colors in the palette
	int biClrImportant;
} InfoHeader;



// read a BMP file into a Texture:

unsigned char*
BmpToTexture(char* filename, int* width, int* height)
{
	FILE* fp;
#ifdef _WIN32
	errno_t err = fopen_s(&fp, filename, "rb");
	if (err != 0)
	{
		fprintf(stderr, "Cannot open Bmp file '%s'\n", filename);
		return NULL;
	}
#else
	fp = fopen(filename, "rb");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open Bmp file '%s'\n", filename);
		return NULL;
	}
#endif

	FileHeader.bfType = ReadShort(fp);


	// if bfType is not BMP_MAGIC_NUMBER, the file is not a bmp:

	if (VERBOSE) fprintf(stderr, "FileHeader.bfType = 0x%0x = \"%c%c\"\n",
		FileHeader.bfType, FileHeader.bfType & 0xff, (FileHeader.bfType >> 8) & 0xff);
	if (FileHeader.bfType != BMP_MAGIC_NUMBER)
	{
		fprintf(stderr, "Wrong type of file: 0x%0x\n", FileHeader.bfType);
		fclose(fp);
		return NULL;
	}


	FileHeader.bfSize = ReadInt(fp);
	if (VERBOSE)	fprintf(stderr, "FileHeader.bfSize = %d\n", FileHeader.bfSize);

	FileHeader.bfReserved1 = ReadShort(fp);
	FileHeader.bfReserved2 = ReadShort(fp);

	FileHeader.bfOffBytes = ReadInt(fp);
	if (VERBOSE)	fprintf(stderr, "FileHeader.bfOffBytes = %d\n", FileHeader.bfOffBytes);


	InfoHeader.biSize = ReadInt(fp);
	if (VERBOSE)	fprintf(stderr, "InfoHeader.biSize = %d\n", InfoHeader.biSize);
	InfoHeader.biWidth = ReadInt(fp);
	if (VERBOSE)	fprintf(stderr, "InfoHeader.biWidth = %d\n", InfoHeader.biWidth);
	InfoHeader.biHeight = ReadInt(fp);
	if (VERBOSE)	fprintf(stderr, "InfoHeader.biHeight = %d\n", InfoHeader.biHeight);

	const int nums = InfoHeader.biWidth;
	const int numt = InfoHeader.biHeight;

	InfoHeader.biPlanes = ReadShort(fp);
	if (VERBOSE)	fprintf(stderr, "InfoHeader.biPlanes = %d\n", InfoHeader.biPlanes);

	InfoHeader.biBitCount = ReadShort(fp);
	if (VERBOSE)	fprintf(stderr, "InfoHeader.biBitCount = %d\n", InfoHeader.biBitCount);

	InfoHeader.biCompression = ReadInt(fp);
	if (VERBOSE)	fprintf(stderr, "InfoHeader.biCompression = %d\n", InfoHeader.biCompression);

	InfoHeader.biSizeImage = ReadInt(fp);
	if (VERBOSE)	fprintf(stderr, "InfoHeader.biSizeImage = %d\n", InfoHeader.biSizeImage);

	InfoHeader.biXPixelsPerMeter = ReadInt(fp);
	InfoHeader.biYPixelsPerMeter = ReadInt(fp);

	InfoHeader.biClrUsed = ReadInt(fp);
	if (VERBOSE)	fprintf(stderr, "InfoHeader.biClrUsed = %d\n", InfoHeader.biClrUsed);

	InfoHeader.biClrImportant = ReadInt(fp);


	// fprintf( stderr, "Image size found: %d x %d\n", ImageWidth, ImageHeight );


	// pixels will be stored bottom-to-top, left-to-right:
	unsigned char* texture = new unsigned char[3 * nums * numt];
	if (texture == NULL)
	{
		fprintf(stderr, "Cannot allocate the texture array!\n");
		return NULL;
	}

	// extra padding bytes:

	int requiredRowSizeInBytes = 4 * ((InfoHeader.biBitCount * InfoHeader.biWidth + 31) / 32);
	if (VERBOSE)	fprintf(stderr, "requiredRowSizeInBytes = %d\n", requiredRowSizeInBytes);

	int myRowSizeInBytes = (InfoHeader.biBitCount * InfoHeader.biWidth + 7) / 8;
	if (VERBOSE)	fprintf(stderr, "myRowSizeInBytes = %d\n", myRowSizeInBytes);

	int oldNumExtra = 4 * (((3 * InfoHeader.biWidth) + 3) / 4) - 3 * InfoHeader.biWidth;
	if (VERBOSE)	fprintf(stderr, "Old NumExtra padding = %d\n", oldNumExtra);

	int numExtra = requiredRowSizeInBytes - myRowSizeInBytes;
	if (VERBOSE)	fprintf(stderr, "New NumExtra padding = %d\n", numExtra);


	// this function does not support compression:

	if (InfoHeader.biCompression != 0)
	{
		fprintf(stderr, "Wrong type of image compression: %d\n", InfoHeader.biCompression);
		fclose(fp);
		return NULL;
	}


	// we can handle 24 bits of direct color:
	if (InfoHeader.biBitCount == 24)
	{
		rewind(fp);
		fseek(fp, FileHeader.bfOffBytes, SEEK_SET);
		int t;
		unsigned char* tp;
		for (t = 0, tp = texture; t < numt; t++)
		{
			for (int s = 0; s < nums; s++, tp += 3)
			{
				*(tp + 2) = fgetc(fp);		// b
				*(tp + 1) = fgetc(fp);		// g
				*(tp + 0) = fgetc(fp);		// r
			}

			for (int e = 0; e < numExtra; e++)
			{
				fgetc(fp);
			}
		}
	}

	// we can also handle 8 bits of indirect color:
	if (InfoHeader.biBitCount == 8 && InfoHeader.biClrUsed == 256)
	{
		struct rgba32
		{
			unsigned char r, g, b, a;
		};
		struct rgba32* colorTable = new struct rgba32[InfoHeader.biClrUsed];

		rewind(fp);
		fseek(fp, sizeof(struct bmfh) + InfoHeader.biSize - 2, SEEK_SET);
		for (int c = 0; c < InfoHeader.biClrUsed; c++)
		{
			colorTable[c].r = fgetc(fp);
			colorTable[c].g = fgetc(fp);
			colorTable[c].b = fgetc(fp);
			colorTable[c].a = fgetc(fp);
			if (VERBOSE)	fprintf(stderr, "%4d:\t0x%02x\t0x%02x\t0x%02x\t0x%02x\n",
				c, colorTable[c].r, colorTable[c].g, colorTable[c].b, colorTable[c].a);
		}

		rewind(fp);
		fseek(fp, FileHeader.bfOffBytes, SEEK_SET);
		int t;
		unsigned char* tp;
		for (t = 0, tp = texture; t < numt; t++)
		{
			for (int s = 0; s < nums; s++, tp += 3)
			{
				int index = fgetc(fp);
				*(tp + 0) = colorTable[index].r;	// r
				*(tp + 1) = colorTable[index].g;	// g
				*(tp + 2) = colorTable[index].b;	// b
			}

			for (int e = 0; e < numExtra; e++)
			{
				fgetc(fp);
			}
		}

		delete[] colorTable;
	}

	fclose(fp);

	*width = nums;
	*height = numt;
	return texture;
}

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

