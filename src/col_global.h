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
