#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

class Viewport {
  public:
    int w, h; // width and height
};

class Matrix {
public:
    static float twoDeterminant(vector < vector <float> > input);
};

class Point {
public:
    float x, y, z;
    Point();
    Point(float x, float y, float z);
    Point operator+(Point p);
    Point operator-(Point p);
    Point operator*(float scalar);
    Point operator/(float scalar);
};

class Vector3 {
public:
	float x, y, z;
	Vector3();
    Vector3(float x, float y, float z);
    Vector3 operator+(Vector3 v);
    Vector3 operator-(Vector3 v);
    Vector3 operator*(float scalar);
    Vector3 operator/(float scalar);
    Vector3 power(float constant);
    Vector3 normalize();
    Point vectorToPoint();
    float magnitude();
    static Vector3 pointSubtraction(Point target_point, Point initial_point);
    static float dotProduct(const Vector3 v1, const Vector3 v2);
    static Vector3 crossProduct(Vector3 v1, Vector3 v2);
};


class Normal {
public:
    float x, y, z;
    Normal();
    Normal(float x, float y, float z);
    Normal operator+(Normal n);
    Normal operator-(Normal n);
    Vector3 normalToVector();
    static Normal pointSubtraction(Point target_point, Point initial_point);
private:
    Normal normalize();
};