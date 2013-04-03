#include "SupportClasses.h"
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

class PointAndNormal {
public:
	PointAndNormal();
	PointAndNormal(Point p, Normal n);
	Point point;
	Normal normal;
};

PointAndNormal::PointAndNormal() {
	this->point = Point();
	this->normal = Normal();
}

PointAndNormal::PointAndNormal(Point p, Normal n) {
	this->point = p;
	this->normal = n;
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
bool toggleSmooth = false;

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
	if(key == 'w') {
		toggleWireframe = !toggleWireframe;
	} else if(key == 's') {
		toggleSmooth = !toggleSmooth;
	}
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
		z += 0.5f;
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
			angle_x -= 10.f; //.1
		}
	} else if(specialKeys[GLUT_KEY_DOWN]) {
		if(shift_pressed) {
			translate_y -= 0.01f;
		} else {
			angle_x += 0.1f;
		}
	}
}

//****************************************************
// sets the window up
//****************************************************
void initScene(){

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent
  myReshape(viewport.w,viewport.h);
}

/*double BezierBlend(int k, double mu, int n) {

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
}*/

//***************************************************
// function that does the actual drawing
//***************************************************

// takes in a curve and a parametric value, returns a point and (optionally) assigns a derivative
Point bezCurveInterp(vector<Point> curve, const float u, Vector3* deriv) {
	Point A, B, C, D, E, p;

	A = curve[0]*(1.0-u) + curve[1]*u;
	B = curve[1]*(1.0-u) + curve[2]*u;
	C = curve[2]*(1.0-u) + curve[3]*u;

	D = A*(1.0-u) + B*u;
	E = B*(1.0-u) + C*u;

	// pick the point on the curve
	p = D*(1.0-u) + E*u;

	Vector3 temp = Vector3::pointSubtraction(E, D) * 3;
	//deriv = new Vector3(temp.x, temp.y, temp.z);
	(*deriv).x = temp.x;
	(*deriv).y = temp.y;
	(*deriv).z = temp.z;

	return p;
}

PointAndNormal bezPatchInterp(BezierPatch patch, float u, float v) {
	
	PointAndNormal output;
	vector<Point> vcurve(4);
	vector<Point> ucurve(4);
	Vector3 dPdv, dPdu, temp;

	// evaluate control points for v curve
	vector<Point> tempCurve(4);
	for (unsigned int i=0; i<4; i++) {
		tempCurve[i] = patch.points[0][i].vectorToPoint();
	}
	vcurve[0] = bezCurveInterp(tempCurve, u, &temp);
	for (unsigned int i=0; i<4; i++) {
		tempCurve[i] = patch.points[1][i].vectorToPoint();
	}
	vcurve[1] = bezCurveInterp(tempCurve, u, &temp);
	for (unsigned int i=0; i<4; i++) {
		tempCurve[i] = patch.points[2][i].vectorToPoint();
	}
	vcurve[2] = bezCurveInterp(tempCurve, u, &temp);
	for (unsigned int i=0; i<4; i++) {
		tempCurve[i] = patch.points[3][i].vectorToPoint();
	}
	vcurve[3] = bezCurveInterp(tempCurve, u, &temp);

	// evaluate control points for u curve
	for (unsigned int i=0; i<4; i++) {
		tempCurve[i] = patch.points[i][0].vectorToPoint();
	}
	ucurve[0] = bezCurveInterp(tempCurve, v, &temp);
	for (unsigned int i=0; i<4; i++) {
		tempCurve[i] = patch.points[i][1].vectorToPoint();
	}
	ucurve[1] = bezCurveInterp(tempCurve, v, &temp);
	for (unsigned int i=0; i<4; i++) {
		tempCurve[i] = patch.points[i][2].vectorToPoint();
	}
	ucurve[2] = bezCurveInterp(tempCurve, v, &temp);
	for (unsigned int i=0; i<4; i++) {
		tempCurve[i] = patch.points[i][3].vectorToPoint();
	}
	ucurve[3] = bezCurveInterp(tempCurve, v, &temp);

	// evaluate surface and derivative for u and v
	output.point = bezCurveInterp(vcurve, v, &dPdv);
	output.point = bezCurveInterp(ucurve, u, &dPdu);

	// take cross product of partials to find normal
	temp = Vector3();
	temp = Vector3::crossProduct(dPdv, dPdu);
	temp = temp.normalize();
	output.normal.x = temp.x;
	output.normal.y = temp.y;
	output.normal.z = temp.z;

	return output;
}

void subDividePatch(const BezierPatch patch, vector <vector<PointAndNormal> > & newPoints) {
	// computer number of subdivisions for our step size
	int numDiv = (int)(1.001/subdivision);
	float u,v;
	unsigned int iu, iv;
	PointAndNormal point;

	for (iu=0; iu<numDiv; iu++) {
		u = iu * subdivision;
		vector<PointAndNormal> newVector;
		newPoints.push_back(newVector);
		for (iv=0; iv<numDiv; iv++) {
			v = iv * subdivision;

			// evaluate the point and normal
			point = bezPatchInterp(patch, u, v);
			newPoints[iu].push_back(point);

		}
	}
}

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
	glColor3f(.2f,0.0f,0.0f); 		//default of red dot
	glPointSize(1.0f);


	if(toggleSmooth) {
		//cout<<"SMooth toggled"<<endl;
		//glEnable(GL_SMOOTH);
		glShadeModel(GL_SMOOTH);
	} else {
		//cout<<"Flat"<<endl;
		//glEnable(GL_FLAT);
		glShadeModel(GL_FLAT);
	}

	if(toggleWireframe) {
		glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	} else {
		glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
	}

	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);
	
	//glutSolidTorus(0.5, 3, 5, 10);

	//----------------------- code to draw objects --------------------------

	// a vector of a collection of points which correspond to a bezier patch's output
	vector <vector< vector<PointAndNormal> > > allOutputPoints;
	//allOutputPoints.resize(numPatches);

	// compute all bezier patches
	for(unsigned int i=0; i<numPatches; i++) {

		vector <vector<PointAndNormal> > newPoints;
		subDividePatch(inputPatches[i], newPoints);
		allOutputPoints.push_back(newPoints);

		//computeUniformSubdivision(inputPatches[i], &outputPatch);
		//outputPatches.push_back(outputPatch);
	}

	//Add ambient light
    GLfloat ambientColor[] = {0.2f, 0.2f, 0.2f, 1.0f}; //Color(0.2, 0.2, 0.2)
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

    //Add directed light
    GLfloat lightColor1[] = {0.5f, 0.2f, 0.2f, 1.0f}; //Color (0.5, 0.2, 0.2)
    //Coming from the direction (-1, 0.5, 0.5)
    GLfloat lightPos1[] = {-1.0f, 0.5f, 3.5f, 0.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);

	for(unsigned int patch=0; patch<allOutputPoints.size(); patch++) {
		for (unsigned int i=0; i<(allOutputPoints[patch].size()-1); i++) {
			for (unsigned int j = 0; j<(allOutputPoints[patch][i].size()-1); j++) {
				glBegin(GL_QUADS);

				// used later for shading
				glNormal3f(allOutputPoints[patch][i][j].normal.x, allOutputPoints[patch][i][j].normal.y, allOutputPoints[patch][i][j].normal.z);

				cout << "here's a normal:   " << allOutputPoints[patch][i][j].normal.x << allOutputPoints[patch][i][j].normal.y << allOutputPoints[patch][i][j].normal.z << endl;
				glColor3f(allOutputPoints[patch][i][j].normal.x, allOutputPoints[patch][i][j].normal.y, allOutputPoints[patch][i][j].normal.z);
				glVertex3f(allOutputPoints[patch][i][j].point.x, allOutputPoints[patch][i][j].point.y, allOutputPoints[patch][i][j].point.z);
				glVertex3f(allOutputPoints[patch][i+1][j].point.x, allOutputPoints[patch][i+1][j].point.y, allOutputPoints[patch][i+1][j].point.z);
				glVertex3f(allOutputPoints[patch][i+1][j+1].point.x, allOutputPoints[patch][i+1][j+1].point.y, allOutputPoints[patch][i+1][j+1].point.z);
				glVertex3f(allOutputPoints[patch][i][j+1].point.x, allOutputPoints[patch][i][j+1].point.y, allOutputPoints[patch][i][j+1].point.z);
				glEnd();


				/*glBegin(GL_POINTS);e
				glVertex3f(allOutputPoints[i][j].point.x, allOutputPoints[i][j].point.y, allOutputPoints[i][j].point.z);
				glEnd();*/
			}
		}
	}

	/* old version
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
	}*/

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
	viewport.w = 800;
	viewport.h = 800;
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