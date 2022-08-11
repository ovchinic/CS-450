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
//	Date:		10/31/2021

// NOTE: There are a lot of good reasons to use const variables instead
// of #define's.  However, Visual C++ does not allow a const variable
// to be used as an array size or as the case in a switch( ) statement.  So in
// the following, all constants are const variables except those which need to
// be array sizes or cases in switch( ) statements.  Those are #defines.


// title of these windows:

const char* WINDOWTITLE = { "Light Show in OpenGL/GLUT -- Christian Ovchinikov" };
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

// which view:

enum View
{
	InsideRing,
	BesideRing,
	BesideUFO,
	TopView,
	Regular
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
GLuint			Sphere;									// object display list
int				MainWindow;								// window id for main graphics window
float			Scale;									// scaling factor
float			Time;								    // Time factor
int				ShadowsOn;								// != 0 means to turn shadows on
int				WhichColor;								// index into Colors[ ]
int				WhichProjection;						// ORTHO or PERSP
int				Xmouse, Ymouse;							// mouse values
float			Xrot, Yrot;								// rotation angles in degrees
int				width;									// width of texture
int				height;									// height of texture
unsigned char* Texture;									// texture file
GLuint			Tex0;									// texture id
bool			Light0On, Light1On, Light2On = true;	// keeps track of light statuses
bool			Light1Disco;							// keeps track of UFO light disco mode
bool			Frozen;									// current freeze status of animations
float			WorldAngle;								// angle of Earth in degrees for animations
int				WhichView;								// Inside ring, UFO POV, or Regular
float			redufo;									// red color code for ufo disco spotlight
float			blueufo;								// blue color code for ufo disco spotlight
float			greenufo;								// green color code for ufo disco spotlight
int				Distort = 0;							// distortion of texture
bool			MouseLock = false;						// mouse lock status
bool			ScaleOnly = false;						// scroll lock status

// function prototypes:

void	Animate();
void	Display();
void	DoViewMenu(int);
void	DoDebugMenu(int);
void	DoDiscoMenu(int);
void	DoLightsMenu(int);
void	DoDistortMenu(int);
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

void SetSpotLight(int ilight, float x, float y, float z, float xdir, float ydir, float zdir, float r, float g, float b)
{
	glLightfv(ilight, GL_POSITION, Array3(x, y, z));
	glLightfv(ilight, GL_SPOT_DIRECTION, Array3(xdir, ydir, zdir));
	glLightf(ilight, GL_SPOT_EXPONENT, 1.);
	glLightf(ilight, GL_SPOT_CUTOFF, 45.);
	glLightfv(ilight, GL_AMBIENT, Array3(0., 0., 0.));
	glLightfv(ilight, GL_DIFFUSE, Array3(r, g, b));
	glLightfv(ilight, GL_SPECULAR, Array3(r, g, b));
	glLightf(ilight, GL_CONSTANT_ATTENUATION, 1.);
	glLightf(ilight, GL_LINEAR_ATTENUATION, 0.);
	glLightf(ilight, GL_QUADRATIC_ATTENUATION, 0.);
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
			if (Distort == 1)
			{
				p->s = ((lng + M_PI) / (2. * M_PI)) + (cos(rand())) / 100;
				p->t = ((lat + M_PI / 2.) / M_PI) + (cos(rand())) / 100;
			}
			else
			{
				p->s = (lng + M_PI) / (2. * M_PI);
				p->t = (lat + M_PI / 2.) / M_PI;
			}
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
	const int MS_IN_THE_ANIMATION_CYCLE = 10000;	// milliseconds in the animation loop
	int ms = glutGet(GLUT_ELAPSED_TIME);			// milliseconds since the program started
	ms %= MS_IN_THE_ANIMATION_CYCLE;				// milliseconds in the range 0 to MS_IN_THE_ANIMATION_CYCLE-1
	Time = (float)ms / (float)MS_IN_THE_ANIMATION_CYCLE;        // [ 0., 1. )

	WorldAngle += 0.5;
	redufo = (float)rand() / RAND_MAX;
	blueufo = (float)rand() / RAND_MAX;
	greenufo = (float)rand() / RAND_MAX;

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

	if (WhichView == Regular)
	{
		gluLookAt(0., 0., 55., 0., 0., 0., 0., 1., 0.);
		MouseLock = false;
		ScaleOnly = false;
	}

	else if (WhichView == InsideRing)
	{
		WhichProjection = PERSP;
		Scale = 1.0;
		Xrot = Yrot = 0.;
		gluLookAt(-15.25, 0., 0., 0., 0., 0., 0., 1., 0.);
		MouseLock = true;
		ScaleOnly = false;
	}

	else if (WhichView == BesideRing)
	{
		WhichProjection = PERSP;
		Scale = 1;
		Xrot = 0.;
		Yrot = -20.;
		gluLookAt(-7., 0., -10., -10, 0., -7., 0., 1., 0.);
		MouseLock = true;
		ScaleOnly = false;
	}

	else if (WhichView == BesideUFO)
	{
		WhichProjection = PERSP;
		Xrot = Yrot = 0.;
		gluLookAt(3., 28., 4., 0., 0., 0., -1., 1., 0.);
		MouseLock = false;
		ScaleOnly = true;
	}

	else if (WhichView == TopView)
	{
		WhichProjection = PERSP;
		Scale = 1.0;
		Xrot = Yrot = 0.;
		gluLookAt(0., 52., 0., 0., 0., 0., -1., 1., 0.);
		MouseLock = true;
		ScaleOnly = false;
	}


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

	if (Light2On)
		glEnable(GL_LIGHT2);
	else
		glDisable(GL_LIGHT2);


	// since we are using glScalef( ), be sure normals get unitized:

	glEnable(GL_NORMALIZE);


	// Draw Earth

	glEnable(GL_LIGHTING);
	glPushMatrix();
	glRotatef(WorldAngle, 0., 1., 0.);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, Tex0);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glMatrixMode(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	SetMaterial(1., 1., 1., 50.);
	glColor3f(.8, .8, .8);
	OsuSphere(10, 50, 50);
	glDisable(GL_TEXTURE_2D);
	glPopMatrix();
	glDisable(GL_LIGHTING);


	// Draw Moon and Pointlight

	glPushMatrix();
	glRotatef((WorldAngle / 5), 0., 1., 0.);

	glEnable(GL_LIGHTING);
	SetPointLight(GL_LIGHT0, 0., 0., 50., 1., 1., 1., 1., 0.);
	if (Light0On)
		glEnable(GL_LIGHT0);
	else
		glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);

	glTranslatef(0., 0., 50.);
	glColor3f(1., 1., 1.);
	OsuSphere(0.7, 25, 25);
	glPopMatrix();


	// Draw the UFO

	glShadeModel(GL_FLAT);
	glEnable(GL_LIGHTING);

	glPushMatrix();
	glTranslatef(0., 25., 0.);
	glRotatef(90., 1., 0., 0.);
	SetMaterial(0.1, 0.1, 0.1, 50.);
	glColor3f(0.0, 0.0, 0.0);
	OsuSphere(1, 50, 50);
	glScalef(1., 1., 0.25);
	glutSolidTorus(1, 1.5, 100, 100);
	glPopMatrix();

	glDisable(GL_LIGHTING);


	// UFO Spotlight

	if (Light1On)
	{
		SetSpotLight(GL_LIGHT1, 0., 13, 0., 0., -1., 0., 0., 1., 0.);
		glEnable(GL_LIGHT1);
	}
	else
		glDisable(GL_LIGHT1);

	if (Light1Disco)
	{
		SetSpotLight(GL_LIGHT1, 0., 13., 0., 0., -1., 0., redufo, blueufo, greenufo);
		glEnable(GL_LIGHT1);
	}
	else
		glDisable(GL_LIGHT1);

	glPushMatrix();
	glTranslatef(0., 13., 0.);
	glColor3f(0., 1., 0.);
	if (Light1Disco)
	{
		glColor3f(redufo, blueufo, greenufo);
	}
	else
	{
		glColor3f(0., 1., 0.);
	}
	glutSolidSphere(0.25, 25., 25.);
	glPopMatrix();

	// Torus Ring

	glPushMatrix();
	glTranslatef(-15., 0., 0.);
	glRotatef(90., 0., 1., 0.);
	glScalef(1., 1., 0.75);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_LIGHTING);
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
	SetMaterial(0., 0., 1.0, 0.);
	glutSolidTorus(1, 1.5, 200, 200);
	glPopMatrix();


	// Lighting Beside Ring
	glEnable(GL_LIGHTING);
	glPushMatrix();
	SetPointLight(GL_LIGHT2, -12., 10., 0., 1., 0., 0., 1., 0.005);
	glEnable(GL_LIGHT2);
	glDisable(GL_LIGHTING);
	glTranslatef(-12., 10., 0.);
	glColor3f(1., 0., 0.);
	glutSolidSphere(0.25, 25., 25.);
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
	DoRasterString(2., 95., 0., (char*)"Light Show");

	glColor3f(1., 1., 1.);
	if (MouseLock)
		DoRasterString(75., 95., 0., (char*)"Mouse Locked");
	if (ScaleOnly)
		DoRasterString(80., 95., 0., (char*)"Scale Only");

	glColor3f(1., 1., 1.);
	DoRasterString(2., 22., 0., (char*)"(V) Cycle View");
	DoRasterString(2., 17., 0., (char*)"(D) Disco Mode");
	DoRasterString(2., 12., 0., (char*)"(F) Freeze Animation");
	DoRasterString(2., 7., 0., (char*)"(R) Reset");
	DoRasterString(2., 2., 0., (char*)"(Q) Quit");

	DoRasterString(69., 12., 0., (char*)"(1) Light Switch 1");
	DoRasterString(69., 7., 0., (char*)"(2) Light Switch 2");
	DoRasterString(69., 2., 0., (char*)"(3) Light Switch 3");


	// swap the double-buffered framebuffers:

	glutSwapBuffers();


	// be sure the graphics buffer has been sent:
	// note: be sure to use glFlush( ) here, not glFinish( ) !

	glFlush();
}


void
DoViewMenu(int id)
{
	if (id == 4)
		WhichView = Regular;
	else if (id == 3)
		WhichView = TopView;
	else if (id == 2)
		WhichView = BesideUFO;
	else if (id == 1)
		WhichView = BesideRing;
	else if (id == 0)
		WhichView = InsideRing;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoLightsMenu(int id)
{
	if (id == 0)
		Light0On = !Light0On;
	else if (id == 1)
		Light1On = !Light1On;
	else if (id == 2)
		Light2On = !Light2On;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoDiscoMenu(int id)
{
	if (id == 1)
		Light1Disco = true;
	else if (id == 0)
		Light1Disco = false;

	glutSetWindow(MainWindow);
	glutPostRedisplay();
}

void
DoDistortMenu(int id)
{
	Distort = id;
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

	int viewmenu = glutCreateMenu(DoViewMenu);
	glutAddMenuEntry("Inside Ring", 0);
	glutAddMenuEntry("Beside Ring", 1);
	glutAddMenuEntry("Beside UFO", 2);
	glutAddMenuEntry("Top View", 3);
	glutAddMenuEntry("Reset View", 4);

	int lightsmenu = glutCreateMenu(DoLightsMenu);
	glutAddMenuEntry("Light 1 - On/Off", 0);
	glutAddMenuEntry("Light 2 - On/Off", 1);
	glutAddMenuEntry("Light 3 - On/Off", 2);

	int discomenu = glutCreateMenu(DoDiscoMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int distortmenu = glutCreateMenu(DoDistortMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int projmenu = glutCreateMenu(DoProjectMenu);
	glutAddMenuEntry("Orthographic", ORTHO);
	glutAddMenuEntry("Perspective", PERSP);

	int debugmenu = glutCreateMenu(DoDebugMenu);
	glutAddMenuEntry("Off", 0);
	glutAddMenuEntry("On", 1);

	int mainmenu = glutCreateMenu(DoMainMenu);
	glutAddSubMenu("Views", viewmenu);
	glutAddSubMenu("Light Switches", lightsmenu);
	glutAddSubMenu("Disco Mode", discomenu);
	glutAddSubMenu("Distortion", distortmenu);
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

	Texture = BmpToTexture("worldtex.bmp", &width, &height);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &Tex0);
	glBindTexture(GL_TEXTURE_2D, Tex0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, Texture);

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

	// create the object:
	Sphere = glGenLists(1);
	glNewList(Sphere, GL_COMPILE);
	glColor3f(1, 0, 0);
	OsuSphere(10, 50, 50);
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

	case 'v':
	case 'V':
		if (WhichView == 0)
			WhichView = 4;
		else
			WhichView--;
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

	case 'd':
	case 'D':
		Light1Disco = !Light1Disco;
		break;

	case '3':
		Light2On = !Light2On;
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
	WhichView = Regular;
	Xrot = Yrot = 0.;
	Light0On = Light1On = Light2On = true;
	Light1Disco = false;
	Distort = 0;
	WorldAngle = 0;
	MouseLock = false;
	ScaleOnly = false;
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

