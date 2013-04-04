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

class ParametricPoint {
public:
	ParametricPoint(float u, float v);
	float u,v;
};

ParametricPoint::ParametricPoint(float u, float v) {
	this->u = u;
	this->v = v;
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

class Triangle {
public:
	Triangle(PointAndNormal p1, ParametricPoint uv1, PointAndNormal p2, ParametricPoint uv2, PointAndNormal p3, ParametricPoint uv3);
	vector<PointAndNormal> points;
	vector<ParametricPoint> parametrics;
};

Triangle::Triangle(PointAndNormal p1, ParametricPoint uv1, PointAndNormal p2, ParametricPoint uv2, PointAndNormal p3, ParametricPoint uv3) {
	points.push_back(p1);
	points.push_back(p2);
	points.push_back(p3);
	parametrics.push_back(uv1);
	parametrics.push_back(uv2);
	parametrics.push_back(uv3);
}



//****************************************************
// Global Variables
//****************************************************
Viewport    viewport;
int numPatches;
vector<BezierPatch> inputPatches;
vector<BezierPatch> outputPatches;
Vector3 minDimensions = Vector3();
Vector3 maxDimensions = Vector3();
float subdivision;
float tau;		// used for tesselation testing
bool uniform;

bool* keyStates = new bool[256];
bool* specialKeys = new bool[256];

bool toggleWireframe = true;
bool toggleSmooth = false;

float x = 0.0, y = 0.0, z = 0.0;
float translate_x = 0.0, translate_z = 0.0;
float angle_x = 0.0, angle_z = 0.0;

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
		y += 0.5f;
		if(y >= 40.0f) {
			//y = 40.0f;
		}
	} else if (keyStates['=']) {
		y += -0.5f;
		if(y <= 1.0f) {  // 5.0
			//y = 1.0f;
		}
	} else if(specialKeys[GLUT_KEY_LEFT]) {
		if(shift_pressed) {
			translate_x += .5f;
		} else {
			angle_z -= 5.0f;
		}
	} else if(specialKeys[GLUT_KEY_RIGHT]) {
		if(shift_pressed) {
			translate_x -= .5f;
		} else {
			angle_z += 5.0f;
		}
	} else if(specialKeys[GLUT_KEY_UP]) {
		if(shift_pressed) {
			translate_z += .5f;
		} else {
			angle_x += 5.0f; //.1
		}
	} else if(specialKeys[GLUT_KEY_DOWN]) {
		if(shift_pressed) {
			translate_z -= .5f;
		} else {
			angle_x -= 5.0f;
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
	glEnable(GL_DEPTH_TEST);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Clear to black, fully transparent
	myReshape(viewport.w,viewport.h);
}

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
	if (temp.x==0 && temp.y==0 && temp.z==0) {
		int debug = 5;
	}

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
	if (temp.x != temp.x) {
		// x is nan
		output.normal.x = 0;
		output.normal.y = 0;
		output.normal.z = 0;
	}

	return output;
}

void uniformSubDividePatch(const BezierPatch patch, vector <vector<PointAndNormal> > & newPoints) {
	// computer number of subdivisions for our step size
	int numDiv = (int)(1.001/subdivision);
	float u,v;
	unsigned int iu, iv;
	PointAndNormal point;

	for (iu=0; iu<=numDiv; iu++) {
		u = iu * subdivision;
		vector<PointAndNormal> newVector;
		newPoints.push_back(newVector);
		for (iv=0; iv<=numDiv; iv++) {
			v = iv * subdivision;

			// evaluate the point and normal
			point = bezPatchInterp(patch, u, v);
			newPoints[iu].push_back(point);

		}
	}
}

void adaptiveSubDividePatch(const BezierPatch patch, Triangle subTriangle, vector<Triangle> & newTriangles, int recursionDepth) {
	
	float edgeError;
	bool e1Fail, e2Fail, e3Fail; 
	e1Fail = e2Fail = e3Fail = false;

	if (recursionDepth > 5) {
		newTriangles.push_back(subTriangle);
		return;
	}

	// check which edges fail  
	Point edge1Midpoint = (subTriangle.points[0].point + subTriangle.points[1].point) / 2;
	float edge1u = (subTriangle.parametrics[0].u + subTriangle.parametrics[1].u) / 2;
	float edge1v = (subTriangle.parametrics[0].v + subTriangle.parametrics[1].v) / 2;
	PointAndNormal edge1Surface = bezPatchInterp(patch, edge1u, edge1v);
	edgeError = Vector3::pointSubtraction(edge1Surface.point, edge1Midpoint).magnitude();
	if (edgeError >= tau) {
		e1Fail = true;
	}

	Point edge2Midpoint = (subTriangle.points[0].point + subTriangle.points[2].point) / 2;
	float edge2u = (subTriangle.parametrics[0].u + subTriangle.parametrics[2].u) / 2;
	float edge2v = (subTriangle.parametrics[0].v + subTriangle.parametrics[2].v) / 2;
	PointAndNormal edge2Surface = bezPatchInterp(patch, edge2u, edge2v);
	edgeError = Vector3::pointSubtraction(edge2Surface.point, edge2Midpoint).magnitude();
	if (edgeError >= tau) {
		e2Fail = true;
	}

	Point edge3Midpoint = (subTriangle.points[1].point + subTriangle.points[2].point) / 2;
	float edge3u = (subTriangle.parametrics[1].u + subTriangle.parametrics[2].u) / 2;
	float edge3v = (subTriangle.parametrics[1].v + subTriangle.parametrics[2].v) / 2;
	PointAndNormal edge3Surface = bezPatchInterp(patch, edge3u, edge3v);
	edgeError = Vector3::pointSubtraction(edge3Surface.point, edge3Midpoint).magnitude();
	if (edgeError >= tau) {
		e3Fail = true;
	}

	// if no failures, add subTriangle to newTriangles (base case)
	if (!e1Fail && !e2Fail && !e3Fail) {
		newTriangles.push_back(subTriangle);
		return;
	}

	// if there are failures, create new triangles according to failed edge
	ParametricPoint edge1uv = ParametricPoint(edge1u, edge1v);
	ParametricPoint edge2uv = ParametricPoint(edge2u, edge2v);
	ParametricPoint edge3uv = ParametricPoint(edge3u, edge3v);

	if (e1Fail && !e2Fail && !e3Fail) {
		Triangle tri1 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], edge1Surface, edge1uv, subTriangle.points[2], subTriangle.parametrics[2]);
		Triangle tri2 = Triangle(edge1Surface, edge1uv, subTriangle.points[1], subTriangle.parametrics[1], subTriangle.points[2], subTriangle.parametrics[2]);
		adaptiveSubDividePatch(patch, tri1, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri2, newTriangles, recursionDepth+1);
	} else if (!e1Fail && e2Fail && !e3Fail) {
		Triangle tri1 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], subTriangle.points[1], subTriangle.parametrics[1], edge2Surface, edge2uv);
		Triangle tri2 = Triangle(edge2Surface, edge2uv, subTriangle.points[1], subTriangle.parametrics[1], subTriangle.points[2], subTriangle.parametrics[2]);
		adaptiveSubDividePatch(patch, tri1, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri2, newTriangles, recursionDepth+1);
	} else if (!e1Fail && !e2Fail && e3Fail) {
		Triangle tri1 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], subTriangle.points[1], subTriangle.parametrics[1], edge3Surface, edge3uv);
		Triangle tri2 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], edge3Surface, edge3uv, subTriangle.points[2], subTriangle.parametrics[2]);
		adaptiveSubDividePatch(patch, tri1, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri2, newTriangles, recursionDepth+1);
	} else if (e1Fail && e2Fail && !e3Fail) {
		Triangle tri1 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], edge1Surface, edge1uv, edge2Surface, edge2uv);
		Triangle tri2 = Triangle(edge1Surface, edge1uv, subTriangle.points[1], subTriangle.parametrics[1], edge2Surface, edge2uv);
		Triangle tri3 = Triangle(edge2Surface, edge2uv, subTriangle.points[1], subTriangle.parametrics[1], subTriangle.points[2], subTriangle.parametrics[2]);
		adaptiveSubDividePatch(patch, tri1, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri2, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri3, newTriangles, recursionDepth+1);
	} else if (!e1Fail && e2Fail && e3Fail) {
		Triangle tri1 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], subTriangle.points[1], subTriangle.parametrics[1], edge3Surface, edge3uv);
		Triangle tri2 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], edge3Surface, edge3uv, edge2Surface, edge2uv);
		Triangle tri3 = Triangle(edge2Surface, edge2uv, edge3Surface, edge3uv, subTriangle.points[2], subTriangle.parametrics[2]);
		adaptiveSubDividePatch(patch, tri1, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri2, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri3, newTriangles, recursionDepth+1);
	} else if (e1Fail && !e2Fail && e3Fail) {
		Triangle tri1 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], edge1Surface, edge1uv, subTriangle.points[2], subTriangle.parametrics[2]);
		Triangle tri2 = Triangle(edge1Surface, edge1uv, subTriangle.points[1], subTriangle.parametrics[1], edge3Surface, edge3uv);
		Triangle tri3 = Triangle(edge1Surface, edge1uv, edge3Surface, edge3uv, subTriangle.points[2], subTriangle.parametrics[2]);
		adaptiveSubDividePatch(patch, tri1, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri2, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri3, newTriangles, recursionDepth+1);
	} else if (e1Fail && e2Fail && e3Fail) {
		Triangle tri1 = Triangle(subTriangle.points[0], subTriangle.parametrics[0], edge1Surface, edge1uv, edge2Surface, edge2uv);
		Triangle tri2 = Triangle(edge1Surface, edge1uv, subTriangle.points[1], subTriangle.parametrics[1], edge3Surface, edge3uv);
		Triangle tri3 = Triangle(edge1Surface, edge1uv, edge2Surface, edge2uv, edge3Surface, edge3uv);
		Triangle tri4 = Triangle(edge2Surface, edge2uv, edge3Surface, edge3uv, subTriangle.points[2], subTriangle.parametrics[2]);
		adaptiveSubDividePatch(patch, tri1, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri2, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri3, newTriangles, recursionDepth+1);
		adaptiveSubDividePatch(patch, tri4, newTriangles, recursionDepth+1);
	}

}

void myDisplay() {

	keyOperations();

	glClear(GL_COLOR_BUFFER_BIT);                // clear the color buffer (sets everything to black)
	glClear(GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);                  // indicate we are specifying camera transformations
	glLoadIdentity();   

	float xcenter = (minDimensions.x+maxDimensions.x)/2;
	float ycenter = (minDimensions.y+maxDimensions.y)/2;
	float zcenter = (minDimensions.z+maxDimensions.z)/2;
	float width = max( max(maxDimensions.x-minDimensions.x, maxDimensions.y-minDimensions.y), maxDimensions.z-minDimensions.z) + 3.0;
	gluLookAt(xcenter, (ycenter+y+width), zcenter, xcenter, ycenter, zcenter, 0.0, 0.0, 1.0);
	glTranslatef(translate_x, 0.0f, translate_z);
	glRotatef(angle_z, 0.0, 0.0, 1.0);
	glRotatef(angle_x, 1.0, 0.0, 0.0);
	//glColor3f(.2f,0.0f,0.0f);
	//glPointSize(1.0f);

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

	//----------------------- code to draw objects --------------------------
	//Add ambient light
    GLfloat ambientColor[] = {0.5f, 0.5f, 0.5f, 1.0f}; //Color(0.2, 0.2, 0.2)
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientColor);

    //Add directed light
    GLfloat lightColor1[] = {0.5f, 0.2f, 0.2f, 1.0f}; //Color (0.5, 0.2, 0.2)
    //Coming from the direction (-1, 0.5, 0.5)
    GLfloat lightPos1[] = {-1.0f, 0.5f, 3.5f, 0.0f};
    glLightfv(GL_LIGHT1, GL_DIFFUSE, lightColor1);
    glLightfv(GL_LIGHT1, GL_POSITION, lightPos1);

	//Add positioned light
    GLfloat lightColor0[] = {0.5f, 0.5f, 0.5f, 1.0f}; //Color (0.5, 0.5, 0.5)
    GLfloat lightPos0[] = {4.0f, 0.0f, 8.0f, 1.0f}; //Positioned at (4, 0, 8)
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos0);


    // COMPUTE ALL BEZIER POINTS

    vector <vector< vector<PointAndNormal> > > allOutputPoints;		// used for uniform
    vector <vector <Triangle> > allOutputTriangles;		// used for adaptive

    if (uniform) {

		for(unsigned int i=0; i<numPatches; i++) {

			vector <vector<PointAndNormal> > newPoints;
			uniformSubDividePatch(inputPatches[i], newPoints);
			allOutputPoints.push_back(newPoints);
		}

    } else {

    	// Adaptive
    	for (unsigned int i=0; i<numPatches; i++) {

    		// Find the four corners of the patch
    		PointAndNormal P1 = bezPatchInterp(inputPatches[i], 0, 0);
    		ParametricPoint parametricP1 = ParametricPoint(0, 0);
    		PointAndNormal P2 = bezPatchInterp(inputPatches[i], 1.0, 0);
    		ParametricPoint parametricP2 = ParametricPoint(1.0, 0);
    		PointAndNormal P3 = bezPatchInterp(inputPatches[i], 0, 1.0);
    		ParametricPoint parametricP3 = ParametricPoint(0, 1.0);
    		PointAndNormal P4 = bezPatchInterp(inputPatches[i], 1.0, 1.0);
    		ParametricPoint parametricP4 = ParametricPoint(1.0, 1.0);
    		// Create the two starting triangles from the patch
    		Triangle triangle1 = Triangle(P1, parametricP1, P2, parametricP2, P3, parametricP3);
    		Triangle triangle2 = Triangle(P2, parametricP2, P3, parametricP3, P4, parametricP4);

    		vector<Triangle> newTriangles;
    		// Adaptively subdivide first triangle
    		adaptiveSubDividePatch(inputPatches[i], triangle1, newTriangles, 0);
    		// Adaptively subdivide second triangle
    		adaptiveSubDividePatch(inputPatches[i], triangle2, newTriangles, 0);

    		allOutputTriangles.push_back(newTriangles);
    	}

    }

    if (uniform) {
		for(unsigned int patch=0; patch<allOutputPoints.size(); patch++) {
			for (unsigned int i=0; i<(allOutputPoints[patch].size()-1); i++) {
				//glBegin(GL_LINE_STRIP);
				for (unsigned int j = 0; j<(allOutputPoints[patch][i].size()-1); j++) {
					
					glBegin(GL_TRIANGLES);
					//cout << "here's a normal:   " << allOutputPoints[patch][i][j].normal.x << allOutputPoints[patch][i][j].normal.y << allOutputPoints[patch][i][j].normal.z << endl;
					glNormal3f(allOutputPoints[patch][i][j].normal.x, allOutputPoints[patch][i][j].normal.y, allOutputPoints[patch][i][j].normal.z);
					glVertex3f(allOutputPoints[patch][i][j].point.x, allOutputPoints[patch][i][j].point.y, allOutputPoints[patch][i][j].point.z);

					glNormal3f(allOutputPoints[patch][i+1][j].normal.x, allOutputPoints[patch][i+1][j].normal.y, allOutputPoints[patch][i+1][j].normal.z);
					glVertex3f(allOutputPoints[patch][i+1][j].point.x, allOutputPoints[patch][i+1][j].point.y, allOutputPoints[patch][i+1][j].point.z);

					glNormal3f(allOutputPoints[patch][i][j+1].normal.x, allOutputPoints[patch][i][j+1].normal.y, allOutputPoints[patch][i][j+1].normal.z);
					glVertex3f(allOutputPoints[patch][i][j+1].point.x, allOutputPoints[patch][i][j+1].point.y, allOutputPoints[patch][i][j+1].point.z);
					glEnd();

					glBegin(GL_TRIANGLES);
					//cout << "here's a normal:   " << allOutputPoints[patch][i][j].normal.x << allOutputPoints[patch][i][j].normal.y << allOutputPoints[patch][i][j].normal.z << endl;
					glNormal3f(allOutputPoints[patch][i+1][j].normal.x, allOutputPoints[patch][i+1][j].normal.y, allOutputPoints[patch][i+1][j].normal.z);
					glVertex3f(allOutputPoints[patch][i+1][j].point.x, allOutputPoints[patch][i+1][j].point.y, allOutputPoints[patch][i+1][j].point.z);

					glNormal3f(allOutputPoints[patch][i+1][j+1].normal.x, allOutputPoints[patch][i+1][j+1].normal.y, allOutputPoints[patch][i+1][j+1].normal.z);
					glVertex3f(allOutputPoints[patch][i+1][j+1].point.x, allOutputPoints[patch][i+1][j+1].point.y, allOutputPoints[patch][i+1][j+1].point.z);

					glNormal3f(allOutputPoints[patch][i][j+1].normal.x, allOutputPoints[patch][i][j+1].normal.y, allOutputPoints[patch][i][j+1].normal.z);
					glVertex3f(allOutputPoints[patch][i][j+1].point.x, allOutputPoints[patch][i][j+1].point.y, allOutputPoints[patch][i][j+1].point.z);
					glEnd();

				}
			}
		}
	} else {

		// adaptive subdivision
		for (unsigned int i=0; i<(allOutputTriangles.size()); i++) {
			for (unsigned int j=0; j<(allOutputTriangles[i].size()); j++) {

				glBegin(GL_TRIANGLES);

				glNormal3f(allOutputTriangles[i][j].points[0].normal.x, allOutputTriangles[i][j].points[0].normal.y, allOutputTriangles[i][j].points[0].normal.z);
				glVertex3f(allOutputTriangles[i][j].points[0].point.x, allOutputTriangles[i][j].points[0].point.y, allOutputTriangles[i][j].points[0].point.z);

				glNormal3f(allOutputTriangles[i][j].points[1].normal.x, allOutputTriangles[i][j].points[1].normal.y, allOutputTriangles[i][j].points[1].normal.z);
				glVertex3f(allOutputTriangles[i][j].points[1].point.x, allOutputTriangles[i][j].points[1].point.y, allOutputTriangles[i][j].points[1].point.z);

				glNormal3f(allOutputTriangles[i][j].points[2].normal.x, allOutputTriangles[i][j].points[2].normal.y, allOutputTriangles[i][j].points[2].normal.z);
				glVertex3f(allOutputTriangles[i][j].points[2].point.x, allOutputTriangles[i][j].points[2].point.y, allOutputTriangles[i][j].points[2].point.z);

				glEnd();
			}
		}
	}
	//-----------------------------------------------------------------------

	//glClear(GL_DEPTH_BUFFER_BIT);
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

				if (newBezier.points[row][0].x < minDimensions.x) {
					minDimensions.x = newBezier.points[row][0].x;
				}
				if (newBezier.points[row][0].y < minDimensions.y) {
					minDimensions.y = newBezier.points[row][0].y;
				}
				if (newBezier.points[row][0].z < minDimensions.z) {
					minDimensions.z = newBezier.points[row][0].z;
				}
				if (newBezier.points[row][1].x < minDimensions.x) {
					minDimensions.x = newBezier.points[row][1].x;
				}
				if (newBezier.points[row][1].y < minDimensions.y) {
					minDimensions.y = newBezier.points[row][1].y;
				}
				if (newBezier.points[row][1].z < minDimensions.z) {
					minDimensions.z = newBezier.points[row][1].z;
				}
				if (newBezier.points[row][2].x < minDimensions.x) {
					minDimensions.x = newBezier.points[row][2].x;
				}
				if (newBezier.points[row][2].y < minDimensions.y) {
					minDimensions.y = newBezier.points[row][2].y;
				}
				if (newBezier.points[row][2].z < minDimensions.z) {
					minDimensions.z = newBezier.points[row][2].z;
				}
				if (newBezier.points[row][3].x < minDimensions.x) {
					minDimensions.x = newBezier.points[row][3].x;
				}
				if (newBezier.points[row][3].y < minDimensions.y) {
					minDimensions.y = newBezier.points[row][3].y;
				}
				if (newBezier.points[row][3].z < minDimensions.z) {
					minDimensions.z = newBezier.points[row][3].z;
				}
				// MAX VALUES
				if (newBezier.points[row][0].x > maxDimensions.x) {
					maxDimensions.x = newBezier.points[row][0].x;
				}
				if (newBezier.points[row][0].y > maxDimensions.y) {
					maxDimensions.y = newBezier.points[row][0].y;
				}
				if (newBezier.points[row][0].z > maxDimensions.z) {
					maxDimensions.z = newBezier.points[row][0].z;
				}
				if (newBezier.points[row][1].x > maxDimensions.x) {
					maxDimensions.x = newBezier.points[row][1].x;
				}
				if (newBezier.points[row][1].y > maxDimensions.y) {
					maxDimensions.y = newBezier.points[row][1].y;
				}
				if (newBezier.points[row][1].z > maxDimensions.z) {
					maxDimensions.z = newBezier.points[row][1].z;
				}
				if (newBezier.points[row][2].x > maxDimensions.x) {
					maxDimensions.x = newBezier.points[row][2].x;
				}
				if (newBezier.points[row][2].y > maxDimensions.y) {
					maxDimensions.y = newBezier.points[row][2].y;
				}
				if (newBezier.points[row][2].z > maxDimensions.z) {
					maxDimensions.z = newBezier.points[row][2].z;
				}
				if (newBezier.points[row][3].x > maxDimensions.x) {
					maxDimensions.x = newBezier.points[row][3].x;
				}
				if (newBezier.points[row][3].y > maxDimensions.y) {
					maxDimensions.y = newBezier.points[row][3].y;
				}
				if (newBezier.points[row][3].z > maxDimensions.z) {
					maxDimensions.z = newBezier.points[row][3].z;
				}
				row++;	
			} else {
				cout << "there was an issue with parsing input files \n" << endl;
			}
		}

		inpfile.close();
	}
}


int main(int argc, char *argv[]) {

	// patch input file
	string inputFile = argv[1];
	cout<<"In"<<endl;
	parseInput(inputFile);
	cout<<"Out"<<endl;

	subdivision = atof(argv[2]);
	tau = atof(argv[2]);

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
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	//glEnable(GL_DEPTH_TEST);
	// Initalize theviewport size
	viewport.w = 800;
	viewport.h = 800;
	//The size and position of the window
	glutInitWindowSize(viewport.w, viewport.h);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("as3 Bezier Curves");
	cout<<"Got here"<<endl;
	initScene();        
	//This tells glut to use a double-buffered window with red, green, and blue channels                        // quick function to set up scene
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