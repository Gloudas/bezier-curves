#include <vector>
#include <iostream>
#include <fstream>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

#ifdef OSX
#include <GLUT/glut.h>
#include <OpenGL/glu.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#endif

#include <time.h>
#include <math.h>

using namespace std;


//****************************************************
// Some Classes
//****************************************************
class Viewport {
  public:
    int w, h; // width and height
};

class Vector3 {
public:
	float x,y,z;
};

class BezierPatch {
public:
	BezierPatch();
	// stores the Vector3 values for each of the (16) control points
	vector< vector< Vector3> > points;
};

BezierPatch::BezierPatch() {
	points.resize(4);
	for(unsigned int i=0; i<4; i++) {
		points[i].resize(4);
	}
}

//****************************************************
// Global Variables
//****************************************************
Viewport    viewport;

int numPatches;
vector<BezierPatch> inputPatches;
vector<BezierPatch> outputPatches;
float subdivision;
bool uniform;

bool* keyStates = new bool[256];
bool* specialKeys = new bool[256];

bool toggleWireframe = true;

float x = 0.0, y = 0.0, z = 5.0;
float translate_x = 0.0, translate_y = 0.0;
float angle_x = 0.0, angle_y = 0.0;

//****************************************************
// reshape viewport if the window is resized
//****************************************************
void myReshape(int w, int h) {
  viewport.w = w;
  viewport.h = h;

  glViewport(0,0,viewport.w,viewport.h);// sets the rectangle that will be the window
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity(); 
  gluPerspective(60.0, (float)w/h, 1, 40);               // loading the identity matrix for the screen
  //gluLookAt(x, y, z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

  //----------- setting the projection -------------------------
  // glOrtho sets left, right, bottom, top, zNear, zFar of the chord system

  // glOrtho(-1, 1 + (w-400)/200.0 , -1 -(h-400)/200.0, 1, 1, -1); // resize type = add
  // glOrtho(-w/400.0, w/400.0, -h/400.0, h/400.0, 1, -1); // resize type = center

  //glOrtho(-1, 1, -1, 1, 1, -1);    // resize type = stretch

  //------------------------------------------------------------
}

//****************************************************
// Keyboard Event Handling
//***************************************************
bool shift_pressed;

void normalKeysDown(unsigned char key, int x, int y) {
	keyStates[key] = true;

	shift_pressed = (glutGetModifiers() == GLUT_ACTIVE_SHIFT);
	if(key == 27) {
		exit(0);
	} else if(key==15) {
		cout<<"SHIFT PRESSED"<<endl;
	}

}

void normalKeysUp(unsigned char key, int x, int y) {
	keyStates[key] = false;
}

void specialKeysDown(int key, int x, int y) {
	specialKeys[key] = true;
	shift_pressed = (glutGetModifiers() == GLUT_ACTIVE_SHIFT);
}

void specialKeysUp(int key, int x, int y) {
	specialKeys[key] = false;
}

void toggleWireframeDisplay() {
	toggleWireframe = (!toggleWireframe);
}

void keyOperations() {
	if(keyStates['-']) {
		z += 0.01f;
		if(z >= 40.0f) {
			z = 40.0f;
		}
	} else if (keyStates['=']) {
		//int mod = glutGetModifiers();
		//if(shift_pressed) {
			z += -0.01f;
			if(z <= 1.0f) {
				z = 1.0f;
			}
		//}
	} else if(specialKeys[GLUT_KEY_LEFT]) {
		if(shift_pressed) {
			translate_x -= 0.01f;
		} else {
			angle_y -= 0.1f;
		}
	} else if(specialKeys[GLUT_KEY_RIGHT]) {
		if(shift_pressed) {
			translate_x += 0.01f;
		} else {
			angle_y += 0.1f;
		}
	} else if(specialKeys[GLUT_KEY_UP]) {
		if(shift_pressed) {
			translate_y += 0.01f;
		} else {
			angle_x -= 0.1f;
		}
	} else if(specialKeys[GLUT_KEY_DOWN]) {
		if(shift_pressed) {
			translate_y -= 0.01f;
		} else {
			angle_x += 0.1f;
		}
	} else if (keyStates['w']) {
		toggleWireframeDisplay();
	}
}

//****************************************************
// sets the window up
//****************************************************
void initScene(){
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent
  myReshape(viewport.w,viewport.h);
}

double BezierBlend(int k, double mu, int n) {

	int nn,kn,nkn;
	double blend=1;
	nn = n;
	kn = k;
	nkn = n - k;

	while (nn >= 1) {
	  	blend *= nn;
	  	nn--;
	  	if (kn > 1) {
	     	blend /= (double)kn;
	     	kn--;
	  	}
	  	if (nkn > 1) {
	     	blend /= (double)nkn;
	     	nkn--;
	  	}
	}
	if (k > 0)
		blend *= pow(mu,(double)k);
	if (n-k > 0)
	  	blend *= pow(1-mu,(double)(n-k));

	return blend;
}

void computeUniformSubdivision(const BezierPatch inputBezier, BezierPatch *outputBezier) {

	unsigned int i,j,ki,kj;
	double mui, muj, bi, bj;

	int ni = inputBezier.points.size();
	int nj = inputBezier.points[0].size();
	int resolutionI = ni * (int)(1.001/subdivision);
	int resolutionJ = nj * (int)(1.001/subdivision);

	vector< vector< Vector3> > input = inputBezier.points;
	// instantiate the output bezier with size resolutionI * resolutionJ
	vector< vector< Vector3> > output;
	output.resize(resolutionI);
	for (i=0; i<resolutionI; i++) {
		output[i].resize(resolutionJ);
	}

	for (i=0; i<resolutionI; i++) {
		mui = i / (double)(resolutionI-1);	// note: i dont understand the -1 here. Maybe because there is an extra point added in bezier?
		for (j=0; j<resolutionJ; j++) {
			muj = j / (double)(resolutionJ-1);
			output[i][j].x = 0;
			output[i][j].y = 0;
			output[i][j].z = 0;
			for (ki=0; ki<ni; ki++) {
				bi = BezierBlend(ki, mui, ni);
				for (kj=0; kj<nj; kj++) {
					bj = BezierBlend(kj, muj, nj);
					output[i][j].x += (input[ki][kj].x * bi * bj);
					output[i][j].y += (input[ki][kj].y * bi * bj);
					output[i][j].z += (input[ki][kj].z * bi * bj);
				}
			}
		}		
	}

	outputBezier->points = output;
}

//***************************************************
// function that does the actual drawing
//***************************************************
void myDisplay() {

	keyOperations();

	glClear(GL_COLOR_BUFFER_BIT);                // clear the color buffer (sets everything to black)

	glMatrixMode(GL_MODELVIEW);                  // indicate we are specifying camera transformations
	glLoadIdentity();   

	gluLookAt(x, y, z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);


	glTranslatef(translate_x, translate_y, 0.0f);
	glRotatef(angle_x, 1.0, 0.0, 0.0);
	glRotatef(angle_y, 0.0, 1.0, 0.0);
	                         // make sure transformation is "zero'd"
	glColor3f(1.0f,0.0f,0.0f); 		//default of red dot
	glPointSize(1.0f);

	if(toggleWireframe) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	} else {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
	}
	
	glutSolidTorus(0.5, 3, 15, 30);


	//----------------------- code to draw objects --------------------------

	// compute all bezier patches
	for(unsigned int i=0; i<numPatches; i++) {
		BezierPatch outputPatch = BezierPatch();
		// TODO - calculate outputPatch adaptively if appropriate
		computeUniformSubdivision(inputPatches[i], &outputPatch);
		outputPatches.push_back(outputPatch);
	}

	// draw all bezier patches
	for(unsigned int i=0; i<numPatches; i++) {

		BezierPatch nextPatch = outputPatches[i];

		int ni = nextPatch.points.size();
		int nj = nextPatch.points[0].size();
		for (unsigned int i=0; i<ni; i++) {
			for (unsigned int j=0; j<nj; j++) {
				// for now, just draw dots for each control point
				glBegin(GL_POINTS);
				glVertex3f(nextPatch.points[i][j].x, nextPatch.points[i][j].y, nextPatch.points[i][j].z);
				glEnd();
			}
		}
	}

	/* LEFTOVER STUFF
	glColor3f(1.0f,0.0f,0.0f);                   // setting the color to pure red 90% for the rect
	glBegin(GL_POLYGON);                         // draw rectangle 
	glVertex3f(-0.8f, 0.0f, 0.0f);               // bottom left corner of rectangle
	glVertex3f(-0.8f, 0.5f, 0.0f);               // top left corner of rectangle
	glVertex3f( 0.0f, 0.5f, 0.0f);               // top right corner of rectangle
	glVertex3f( 0.0f, 0.0f, 0.0f);               // bottom right corner of rectangle
	glEnd(); */
	//-----------------------------------------------------------------------

	glFlush();
	glutSwapBuffers();                           // swap buffers (we earlier set double buffer)
}


//****************************************************
// called by glut when there are no messages to handle
//****************************************************
void myFrameMove() {
  //nothing here for now
#ifdef _WIN32
  Sleep(10);                                   //give ~10ms back to OS (so as not to waste the CPU)
#endif
  glutPostRedisplay(); // forces glut to call the display function (myDisplay())
}

void parseInput(string file) {

	ifstream inpfile(file.c_str());
	if(!inpfile.is_open()) {
		cout << "Unable to open file" << endl;
	} else {

		// process first line -> number of patches
		if (inpfile.good()) {
			string line, numPatchesString;
			getline(inpfile,line);
			stringstream ss(line);
			ss >> numPatchesString;
			numPatches = atoi(numPatchesString.c_str());
		}

		// parse all bezier patches 
		BezierPatch newBezier = BezierPatch();
		int row = 0;

		while(inpfile.good()) {

			vector<string> splitline;
			string line, token;

			getline(inpfile,line);
			stringstream ss(line);
			while (ss >> token) {
				splitline.push_back(token);
			}

			// blank line -- move onto next patch
			if(splitline.size() == 0) {
				inputPatches.push_back(newBezier);
				newBezier = BezierPatch();
				row = 0;
				continue;
			} else if (splitline.size() == 12) {
				// assume input patch has 16 * 3 values
				newBezier.points[row][0].x = atof(splitline[0].c_str());
				newBezier.points[row][0].y = atof(splitline[1].c_str());
				newBezier.points[row][0].z = atof(splitline[2].c_str());
				newBezier.points[row][1].x = atof(splitline[3].c_str());
				newBezier.points[row][1].y = atof(splitline[4].c_str());
				newBezier.points[row][1].z = atof(splitline[5].c_str());
				newBezier.points[row][2].x = atof(splitline[6].c_str());
				newBezier.points[row][2].y = atof(splitline[7].c_str());
				newBezier.points[row][2].z = atof(splitline[8].c_str());
				newBezier.points[row][3].x = atof(splitline[9].c_str());
				newBezier.points[row][3].y = atof(splitline[10].c_str());
				newBezier.points[row][3].z = atof(splitline[11].c_str());
				row++;	
			} else {
				cout << "there was an issue with parsing input files \n" << endl;
			}
		}

		inpfile.close();
	}
	int debug;
	debug = 5;
}


int main(int argc, char *argv[]) {

	// patch input file
	string inputFile = argv[1];
	parseInput(inputFile);

	subdivision = atof(argv[2]);

	uniform = true;
	if (argc > 3) {
		string adaptiveFlag = argv[3];
		if (adaptiveFlag == "-a") {
			uniform = false;
		}
	}

	//This initializes glut
	glutInit(&argc, argv);
	//This tells glut to use a double-buffered window with red, green, and blue channels 
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	// Initalize theviewport size
	viewport.w = 400;
	viewport.h = 400;
	//The size and position of the window
	glutInitWindowSize(viewport.w, viewport.h);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("as3 Bezier Curves");
	initScene();                                 // quick function to set up scene
	glutDisplayFunc(myDisplay);                  // function to run when its time to draw something
	glutReshapeFunc(myReshape);                  // function to run when the window gets resized
	glutIdleFunc(myFrameMove);                   // function to run when not handling any other task

	glutKeyboardFunc(normalKeysDown);
	glutKeyboardUpFunc(normalKeysUp);
	glutSpecialFunc(specialKeysDown);
	glutSpecialUpFunc(specialKeysUp);

	glutMainLoop();                              // infinite loop that will keep drawing and resizing and whatever else

	return 0;
}