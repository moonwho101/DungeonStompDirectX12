//#include "DXUT.h"
//#include "DXUTgui.h"
//#include "DXUTmisc.h"
//#include "DXUTCamera.h"
//#include "DXUTSettingsDlg.h"
//#include "SDKmisc.h"
//#include "SDKmesh.h"

#include "resource.h"
#include "d3dtypes.h"
#include "LoadWorld.hpp"
#include "world.hpp"
#include "GlobalSettings.hpp"
#include <string.h>

#include "col_local.h"


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

//float** triangle_pool; //Stores the pointers to the traingles used for collision detection
//int numtriangle;	   //Number of traingles in pool
//float unitsPerMeter;   // Set this to match application scale..
//VECTOR gravity;		   //Gravity

//void checkTriangle(CollisionPacket* colPackage, VECTOR p1, VECTOR p2, VECTOR p3);

//D3DVECTOR calculatemisslelength(D3DVECTOR velocity);

//CollisionTrace trace; //Output structure telling the application everything about the collision

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

/*

D3DVECTOR calculatemisslelength(D3DVECTOR velocity)
{

	D3DVECTOR result;
	VECTOR v;

	v.x = velocity.x;
	v.y = velocity.y;
	v.z = velocity.z;

	v.SetLength(1);

	result.x = v.x;
	result.y = v.y;
	result.z = v.z;
	return result;
}

*/



