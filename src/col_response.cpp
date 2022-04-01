#include "col_local.h"
#include "World.hpp"
#include "GlobalSettings.hpp"
#include <DirectXMath.h>

using namespace DirectX;

//Collision
extern CollisionPacket colPack2;
CollisionPacket collisionPackage;
int collisionRecursionDepth = 0;
int lastcollide = 0;
int collisioncode = 1;
int objectcollide = 0;
int foundcollisiontrue = 0;
int currentmonstercollisionid = -1;
extern float collisiondist;
extern BOOL foundcollision;


extern CollisionPacket collisionPackage; //Stores all the parameters and returnvalues
extern int collisionRecursionDepth;		 //Internal variable tracking the recursion depth

extern CollisionPacket collisionPackage; //Stores all the parameters and returnvalues
extern int collisionRecursionDepth;		 //Internal variable tracking the recursion depth

float** triangle_pool; //Stores the pointers to the traingles used for collision detection
int numtriangle;	   //Number of traingles in pool
float unitsPerMeter;   // Set this to match application scale..
VECTOR gravity;		   //Gravity

void checkTriangle(CollisionPacket* colPackage, VECTOR p1, VECTOR p2, VECTOR p3);

float calcy = 0;
float lastmodely = 0;
BOOL foundcollision = 0;
double nearestDistance = -99;
CollisionPacket colPack2;
extern int g_ob_vert_count;
float collisiondist = 4190.0f;

float mxc[100], myc[100], mzc[100], mwc[100];

CollisionTrace trace; //Output structure telling the application everything about the collision

extern XMFLOAT3 eRadius;

XMFLOAT3 eTest;

void ObjectCollision();
float FastDistance(float fx, float fy, float fz);
void calculate_block_location();
extern int countboundingbox;
extern D3DVERTEX2 boundingbox[2000];
extern int src_collide[MAX_NUM_VERTICES];

struct TCollisionPacket
{
	// data about player movement
	XMFLOAT3 velocity;
	XMFLOAT3 sourcePoint;
	// radius of ellipsoid.
	XMFLOAT3 eRadius;
	// data for collision response
	BOOL foundCollision;
	double nearestDistance;					   // nearest distance to hit
	XMFLOAT3 nearestIntersectionPoint;		   // on sphere
	XMFLOAT3 nearestPolygonIntersectionPoint; // on polygon
};

void checkCollision()
{
	VECTOR P1, P2, P3;	  //Temporary variables holding the triangle in R3
	VECTOR eP1, eP2, eP3; //Temporary variables holding the triangle in eSpace

	for (int i = 0; i < numtriangle; i++) //Iterate trough the entire triangle pool
	{
		//I'm sorry for my hard coding, but fill the traingle with the data from the pool
		P1.set(*triangle_pool[i * 9], *triangle_pool[i * 9 + 1], *triangle_pool[i * 9 + 2]);	 //First vertex
		P2.set(*triangle_pool[i * 9 + 3], *triangle_pool[i * 9 + 4], *triangle_pool[i * 9 + 5]); //Second vertex
		P3.set(*triangle_pool[i * 9 + 6], *triangle_pool[i * 9 + 7], *triangle_pool[i * 9 + 8]); //Third vertex

		//Transform to eSpace
		eP1 = P1 / collisionPackage.eRadius;
		eP2 = P2 / collisionPackage.eRadius;
		eP3 = P3 / collisionPackage.eRadius;

		checkTriangle(&collisionPackage, eP1, eP2, eP3);
	}
}


XMFLOAT3 collideWithWorld(XMFLOAT3 position, XMFLOAT3 velocity)
{

	//SILENCERS
	XMFLOAT3 final;

	// All hard-coded distances in this function is
	// scaled to fit the setting above..
	float unitsPerMeter = 100.0f;
	float unitScale = unitsPerMeter / 100.0f;
	float veryCloseDistance = 0.005f * unitScale;

	// do we need to worry?
	if (collisionRecursionDepth > 15)
	{
		collisionRecursionDepth = 0;
		return position;
	}

	// Ok, we need to worry:
	VECTOR vel;
	vel.x = (float)velocity.x;
	vel.y = (float)velocity.y;
	vel.z = (float)velocity.z;

	VECTOR pos;

	pos.x = position.x;
	pos.y = position.y;
	pos.z = position.z;

	collisionPackage.velocity = vel;
	collisionPackage.normalizedVelocity = vel;
	collisionPackage.normalizedVelocity.normalize();
	collisionPackage.velocityLength = vel.length();
	collisionPackage.basePoint = pos;
	collisionPackage.foundCollision = false;
	collisionPackage.nearestDistance = 10000000;
	collisionPackage.realpos.x = pos.x * eRadius.x;
	collisionPackage.realpos.y = pos.y * eRadius.y;
	collisionPackage.realpos.z = pos.z * eRadius.z;

	// Check for collision (calls the collision routines)
	// Application specific!!
	ObjectCollision();
	// If no collision we just move along the velocity
	if (collisionPackage.foundCollision == false)
	{
		final.x = pos.x + vel.x;
		final.y = pos.y + vel.y;
		final.z = pos.z + vel.z;
		collisionRecursionDepth = 0;
		return final;
	}

	/*
	if (collisionPackage.nearestDistance == 0.0f)
	{
		//quick fix for an embedded object - this needs a lot of work this area here we should un embed ourselves simply by looping through objects and push out if we are embedded, i will fix this one day mark my words , i will fix it and i will be quite pleased about that such thing

		final.x = pos.x;
		final.y = pos.y;
		final.z = pos.z;
		collisionRecursionDepth = 0;
		return final;
	}
	*/

	// *** Collision occured ***
	// The original destination point
	VECTOR destinationPoint = pos + vel;
	VECTOR newSourcePoint = pos;

	VECTOR V = vel;
	
	//added by silencer ver 2.0 new untested unconfirmed
	if (collisionPackage.nearestDistance == 0.0f) {
		final.x = pos.x;
		final.y = pos.y;
		final.z = pos.z;
		return final;
	}

	V.SetLength(collisionPackage.nearestDistance);

	newSourcePoint = collisionPackage.basePoint + V;

	VECTOR slidePlaneNormal = newSourcePoint - collisionPackage.intersectionPoint;

	eTest.x = slidePlaneNormal.x;
	eTest.y = slidePlaneNormal.y;
	eTest.z = slidePlaneNormal.z;

	//fixed by tele forgot to normalize slideplane
	slidePlaneNormal.normalize();

	//silencer ver 1.0
	//VECTOR displacementVector=slidePlaneNormal * veryCloseDistance;

	//silencer ver 2.0 - i think it fixed it can u believe it 2 years and these 5 lines did it.
	float factor;
	V.SetLength(veryCloseDistance);
	factor = veryCloseDistance / (V.x * slidePlaneNormal.x + V.y * slidePlaneNormal.y + V.z * slidePlaneNormal.z);

	V.SetLength(1);

	VECTOR displacementVector = V * veryCloseDistance * factor;

	if ((V.x * slidePlaneNormal.x + V.y * slidePlaneNormal.y + V.z * slidePlaneNormal.z) != 0.0f)
	{
		newSourcePoint = newSourcePoint + displacementVector;
		collisionPackage.intersectionPoint = collisionPackage.intersectionPoint + displacementVector;
	}

	// Determine the sliding plane
	VECTOR slidePlaneOrigin = collisionPackage.intersectionPoint;

	PLANE slidingPlane(slidePlaneOrigin, slidePlaneNormal);

	// Again, sorry about formatting.. but look carefully ;)

	VECTOR newDestinationPoint = destinationPoint - slidePlaneNormal * slidingPlane.signedDistanceTo(destinationPoint);

	// Generate the slide vector, which will become our new
	// velocity vector for the next iteration
	VECTOR newVelocityVector = newDestinationPoint - collisionPackage.intersectionPoint;

	// Recurse:
	// dont recurse if the new velocity is very small
	if (newVelocityVector.length() < veryCloseDistance)
	{
		final.x = newSourcePoint.x;
		final.y = newSourcePoint.y;
		final.z = newSourcePoint.z;
		collisionRecursionDepth = 0;

		return final;
	}

	collisionRecursionDepth++;

	XMFLOAT3 newP;
	XMFLOAT3 newV;

	newP.x = newSourcePoint.x;
	newP.y = newSourcePoint.y;
	newP.z = newSourcePoint.z;

	newV.x = newVelocityVector.x;
	newV.y = newVelocityVector.y;
	newV.z = newVelocityVector.z;

	return collideWithWorld(newP, newV);
}

void ObjectCollision()
{

	float centroidx;
	float centroidy;
	float centroidz;

	float qdist = 0;
	int i = 0;
	int count = 0;

	calcy = 0;
	lastmodely = -9999;
	foundcollision = FALSE;
	nearestDistance = -99;
	int vertcount = 0;
	int vertnum = 0;

	colPack2.foundCollision = false;
	colPack2.nearestDistance = 10000000;
	D3DVECTOR realpos;


	vertnum = verts_per_poly[vertcount];

	for (i = 0; i < g_ob_vert_count; i++)
	{
		if (count == 0 && src_collide[i] == 1)
		{

			mxc[0] = src_v[i].x;
			myc[0] = src_v[i].y;
			mzc[0] = src_v[i].z;

			mxc[1] = src_v[i + 1].x;
			myc[1] = src_v[i + 1].y;
			mzc[1] = src_v[i + 1].z;

			mxc[2] = src_v[i + 2].x;
			myc[2] = src_v[i + 2].y;
			mzc[2] = src_v[i + 2].z;

			//  3 2
			//  1 0

			centroidx = (mxc[0] + mxc[1] + mxc[2]) * 0.3333333333333f;
			centroidy = (myc[0] + myc[1] + myc[2]) * 0.3333333333333f;
			centroidz = (mzc[0] + mzc[1] + mzc[2]) * 0.3333333333333f;
			qdist = FastDistance(collisionPackage.realpos.x - centroidx,
				collisionPackage.realpos.y - centroidy,
				collisionPackage.realpos.z - centroidz);

			if (qdist < collisiondist)
			{
				calculate_block_location();
			}
			if (vertnum == 4)
			{
				mxc[0] = src_v[i + 1].x;
				myc[0] = src_v[i + 1].y;
				mzc[0] = src_v[i + 1].z;

				mxc[1] = src_v[i + 3].x;
				myc[1] = src_v[i + 3].y;
				mzc[1] = src_v[i + 3].z;

				mxc[2] = src_v[i + 2].x;
				myc[2] = src_v[i + 2].y;
				mzc[2] = src_v[i + 2].z;


				centroidx = (mxc[0] + mxc[1] + mxc[2]) * 0.3333333333333f;
				centroidy = (myc[0] + myc[1] + myc[2]) * 0.3333333333333f;
				;
				centroidz = (mzc[0] + mzc[1] + mzc[2]) * 0.3333333333333f;
				qdist = FastDistance(collisionPackage.realpos.x - centroidx,
					collisionPackage.realpos.y - centroidy,
					collisionPackage.realpos.z - centroidz);

				if (qdist < collisiondist)
					calculate_block_location();
			}
		}
		count++;
		if (count > vertnum - 1)
		{
			count = 0;
			vertcount++;
			vertnum = verts_per_poly[vertcount];
		}
	}

	vertnum = 3;
	count = 0;

	count = 0;
	vertnum = 4;

	for (i = 0; i < countboundingbox; i++)
	{

		if (count == 0)
		{
			mxc[0] = boundingbox[i].x;
			myc[0] = boundingbox[i].y;
			mzc[0] = boundingbox[i].z;

			mxc[1] = boundingbox[i + 1].x;
			myc[1] = boundingbox[i + 1].y;
			mzc[1] = boundingbox[i + 1].z;

			mxc[2] = boundingbox[i + 2].x;
			myc[2] = boundingbox[i + 2].y;
			mzc[2] = boundingbox[i + 2].z;

			//  3 2
			//  1 0


			centroidx = (mxc[0] + mxc[1] + mxc[2]) * 0.3333333333333f;
			centroidy = (myc[0] + myc[1] + myc[2]) * 0.3333333333333f;
			centroidz = (mzc[0] + mzc[1] + mzc[2]) * 0.3333333333333f;

			qdist = FastDistance(collisionPackage.realpos.x - centroidx,
				collisionPackage.realpos.y - centroidy,
				collisionPackage.realpos.z - centroidz);

			if (qdist < collisiondist + 200.0f)
				calculate_block_location();

			if (vertnum == 4)
			{
				mxc[0] = boundingbox[i + 1].x;
				myc[0] = boundingbox[i + 1].y;
				mzc[0] = boundingbox[i + 1].z;

				mxc[1] = boundingbox[i + 3].x;
				myc[1] = boundingbox[i + 3].y;
				mzc[1] = boundingbox[i + 3].z;

				mxc[2] = boundingbox[i + 2].x;
				myc[2] = boundingbox[i + 2].y;
				mzc[2] = boundingbox[i + 2].z;


				centroidx = (mxc[0] + mxc[1] + mxc[2]) * 0.3333333333333f;
				centroidy = (myc[0] + myc[1] + myc[2]) * 0.3333333333333f;

				centroidz = (mzc[0] + mzc[1] + mzc[2]) * 0.3333333333333f;
				qdist = FastDistance(collisionPackage.realpos.x - centroidx,
					collisionPackage.realpos.y - centroidy,
					collisionPackage.realpos.z - centroidz);

				if (qdist < collisiondist + 200.0f)
					calculate_block_location();
			}
		}
		count++;
		if (count > vertnum - 1)
		{
			count = 0;
		}


	}

}

void calculate_block_location()
{

	TCollisionPacket colPackage;
	// plane data

	D3DVECTOR p1, p2, p3;
	D3DVECTOR pNormal;
	D3DVECTOR pOrigin;
	D3DVECTOR v1, v2;
	D3DVECTOR source;

	D3DVECTOR velocity;

	int nohit = 0;

	p1.x = mxc[0] / eRadius.x;
	p1.y = myc[0] / eRadius.y;
	p1.z = mzc[0] / eRadius.z;

	p2.x = mxc[1] / eRadius.x;
	p2.y = myc[1] / eRadius.y;
	p2.z = mzc[1] / eRadius.z;

	p3.x = mxc[2] / eRadius.x;
	p3.y = myc[2] / eRadius.y;
	p3.z = mzc[2] / eRadius.z;

	//check embedded

	VECTOR pp1;
	VECTOR pp2;
	VECTOR pp3;

	pp1.x = p1.x;
	pp1.y = p1.y;
	pp1.z = p1.z;

	pp2.x = p2.x;
	pp2.y = p2.y;
	pp2.z = p2.z;

	pp3.x = p3.x;
	pp3.y = p3.y;
	pp3.z = p3.z;

	VECTOR vel;
	VECTOR pos;

	checkTriangle(&collisionPackage, pp1, pp2, pp3);

	return;
}



/*
extern "C" __declspec(dllexport) void colSetUnitsPerMeter(float UPM)
{
	unitsPerMeter=UPM;
}

extern "C" __declspec(dllexport) void colSetGravity(float* grav )
{
	gravity.set(grav[0], grav[1], grav[2]);
}

extern "C" __declspec(dllexport) void colSetRadius(float radius)
{
	collisionPackage.eRadius=radius;
}

extern "C" __declspec(dllexport) void colSetTrianglePool(int numtri, float** pool)
{
	numtriangle=numtri;
	triangle_pool=pool;
}
*/




float FastDistance(float fx, float fy, float fz)
{

	int temp;
	int x, y, z;

	x = (int)fabs(fx) * 1024;
	y = (int)fabs(fy) * 1024;
	z = (int)fabs(fz) * 1024;
	if (y < x)
	{
		temp = x;
		x = y;
		y = temp;
	}
	if (z < y)
	{
		temp = y;
		y = z;
		z = temp;
	}
	if (y < x)
	{
		temp = x;
		x = y;
		y = temp;
	}
	int dist = (z + 11 * (y >> 5) + (x >> 2));
	return ((float)(dist >> 10));
}
