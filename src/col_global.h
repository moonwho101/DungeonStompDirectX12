//Collision global header
#pragma warning(disable : 4244)
#include <math.h>

//Collision global header, include this in your application

//Output structure, contains all the information of the collision
struct CollisionTrace
{
	bool foundCollision;
	float nearestDistance;
	float intersectionPoint[3];
	float finalPosition[3];
};
/*
//Exported functions
extern "C" __declspec(dllexport) CollisionTrace* collideAndSlide(float* position,float* vel);
extern "C" __declspec(dllexport) void colSetUnitsPerMeter(float UPM);
extern "C" __declspec(dllexport) void colSetGravity(float* grav );
extern "C" __declspec(dllexport) void colSetRadius(float radius);
extern "C" __declspec(dllexport) void colSetTrianglePool(int numtriangle, float** pool);
*/