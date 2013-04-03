#include "SupportClasses.h"
#include <algorithm>
#include <cmath>

#define PI 3.14159265

inline float sqr(float x) {return x * x;}



Vector3::Vector3() {
	this->x = 0.0;
	this->y = 0.0;
	this->x = 0.0;
}

Vector3::Vector3(float x1, float y1, float z1) {
	if(x1 == 0.0f) x1=0*1;
	if(y1 == 0.0f) y1=0*1;
	if(z1 == 0.0f) z1=0*1;
	this->x = x1;
	this->y = y1;
	this->z = z1;
}

Vector3 Vector3::operator+(Vector3 v) {
	Vector3 result;
	result.x = x + v.x;
	result.y = y + v.y;
	result.z = z + v.z;
	return result;
}

Vector3 Vector3::operator-(Vector3 v) {
	Vector3 result;
	result.x = x - v.x;
	result.y = y - v.y;
	result.z = z - v.z;
	return result;
}

Vector3 Vector3::operator*(float scalar) {
	Vector3 result;
	result.x = x*scalar;
	result.y = y*scalar;
	result.z = z*scalar;
	return result;
}

Vector3 Vector3::operator/(float scalar) {
	Vector3 result;
	result.x = x/scalar;
	result.y = y/scalar;
	result.z = z/scalar;
	return result;
}

Vector3 Vector3::normalize() {
	Vector3 result;
	float magnitude = sqrt(sqr(this->x)+sqr(this->y)+sqr(this->z));
	result.x = x/magnitude;
	result.y = y/magnitude;
	result.z = z/magnitude;
	if(result.x==0.0f) result.x = 0;
	if(result.y==0.0f) result.y = 0;
	if(result.z==0.0f) result.z = 0;
	return result;
}

Point Vector3::vectorToPoint() {
	Point output;
	output.x = x;
	output.y = y;
	output.z = z;
	return output;
}

float Vector3::magnitude() {
	return sqrt(sqr(this->x)+sqr(this->y)+sqr(this->z));
}

float Vector3::dotProduct(const Vector3 v1, const Vector3 v2) {
	return ((v1.x*v2.x) + (v1.y*v2.y) + (v1.z*v2.z));
}

Vector3 Vector3::crossProduct(Vector3 v1, Vector3 v2) {
	vector <vector <float> > vectorProduct(2, vector<float>(2, 0.0));
	vectorProduct[0][0] = v1.y;
	vectorProduct[1][0] = v1.z;
	vectorProduct[0][1] = v2.y;
	vectorProduct[1][1] = v2.z;
	float x = Matrix::twoDeterminant(vectorProduct);

	vectorProduct[0][0] = v1.x;
	vectorProduct[0][1] = v2.x;
	float y = -1*Matrix::twoDeterminant(vectorProduct);

	vectorProduct[1][0] = v1.y;
	vectorProduct[1][1] = v2.y;
	float z = Matrix::twoDeterminant(vectorProduct);

	return Vector3(x, y, z);
}

Vector3 Vector3::pointSubtraction(Point target_point, Point initial_point) {
	Point temp;
	temp = target_point-initial_point;
	Vector3 result(temp.x, temp.y, temp.z);
	return result;
}

Vector3 Vector3::power(float constant) {
	Vector3 output;
	output.x = pow(this->x, constant);
	output.y = pow(this->y, constant);
	output.z = pow(this->z, constant);
	return output;
}


float Matrix::twoDeterminant(vector < vector <float> > input) {
	float ad = input[0][0] * input[1][1];
	float bc = input[0][1] * input[1][0];
	return (ad-bc);
}

Point::Point() {
	this->x = 0.0;
	this->y = 0.0;
	this->z = 0.0;
}

Point::Point(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

Point Point::operator+(Point p) {
	Point result;
	result.x = (x + p.x);
	result.y = (y + p.y);
	result.z = (z + p.z);
	return result;
}

Point Point::operator-(Point p) {
	Point result;
	result.x = x - p.x;
	result.y = y - p.y;
	result.z = z - p.z;
	return result;
}

Point Point::operator*(float scalar) {
	Point result;
	result.x = x * scalar;
	result.y = y * scalar;
	result.z = z * scalar;
	return result;
}

Point Point::operator/(float scalar) {
	Point result;
	result.x = x / scalar;
	result.y = y / scalar;
	result.z = z / scalar;
	return result;
}

Normal::Normal() {
	this->x=0.0;
	this->y = 0.0;
	this->z = 0.0;
}

//Normalizes the points given to it
Normal::Normal(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
	float magnitude = sqrt(sqr(this->x)+sqr(this->y)+sqr(this->z));
	if (magnitude!=0) {
		this->x = this->x/magnitude;
		this->y = this->y/magnitude;
		this->z = this->z/magnitude;
	}
}

Normal Normal::pointSubtraction(Point target_point, Point initial_point) {
	Point temp;
	temp = target_point-initial_point;
	Normal result(temp.x, temp.y, temp.z);
	return result.normalize();
}

//Always returns a normalized Vector
Normal Normal::operator+(Normal n) {
	Normal result;
	result.x = x + n.x;
	result.y = y + n.y;
	result.z = z + n.z;
	result = result.normalize();
	return result;
}

//Always returns a normalized vector
Normal Normal::operator-(Normal n) {
	Normal result;
	result.x = x - n.x;
	result.y = y - n.y;
	result.z = z - n.z;
	result = result.normalize();
	return result;
}

Vector3 Normal::normalToVector() {
	Vector3 output;
	output.x = x;
	output.y = y;
	output.z = z;
	return output;
}

Normal Normal::normalize() {
	if(x==0 && y==0 && z==0) {
		Normal normal;
		return normal;
	} else {
		Normal result;
		float magnitude = sqrt(sqr(this->x)+sqr(this->y)+sqr(this->z));
		result.x = x/magnitude;
		result.y = y/magnitude;
		result.z = z/magnitude;
		return result;
	}
}