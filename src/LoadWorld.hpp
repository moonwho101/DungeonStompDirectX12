
#ifndef __LOADWORLD_H
#define __LOADWORLD_H

#include <stdio.h>
#include <io.h>
#include <wtypes.h>
#include "ProcessModel.hpp"

#define SPOT_LIGHT_SOURCE 1
#define DIRECTIONAL_LIGHT_SOURCE 2
#define POINT_LIGHT_SOURCE 3


class CLoadWorld //: public CMyD3DApplication
{

public:
	CLoadWorld();
	BOOL LoadMerchantFiles(HWND hwnd, char* filename);
	BOOL LoadWorldMap(char* filename);
	int CheckObjectId(char* p);
	BOOL InitPreCompiledWorldMap(HWND hwnd, char* filename);
	BOOL LoadObjectData(char* filename);
	BOOL ReadObDataVert(FILE* fp, int object_id, int vert_count, float dat_scale);
	BOOL ReadObDataVertEx(FILE* fp, int object_id, int vert_count, float dat_scale);
	//BOOL LoadRRTextures(char* filename, IDirect3DDevice9* pd3dDevice);
	BOOL LoadImportedModelList(HWND hwnd, char* filename);
	void LoadYourGunAnimationSequenceList(int model_id);
	void LoadPlayerAnimationSequenceList(int model_id);
	void LoadDebugAnimationSequenceList(HWND hwnd, char* filename, int model_id);
	int LoadMod(char* filename);
	BOOL LoadSoundFiles(HWND hwnd, char* filename);
	BOOL LoadImportedModelList(char* filename);
	void AddMonster(float x, float y, float z, float rot_angle, float monsterid, float monstertexture, float monnum, int s1, int s2, int s3, int s4, int s5, int s6, char damage[80], int thaco, char name[80], float speed, int ability);
	int FindModelID(char* p);
};

void AddItem(float x, float y, float z, float rot_angle, float monsterid, float monstertexture, float monnum, char modelid[80], char modeltexture[80], int ability);;
void InitWorldMap();
int FindTextureAlias(char* alias);
int CheckValidTextureAlias(char* alias);
extern void AddModel(float x, float y, float z, float rot_angle, float monsterid, float monstertexture, float monnum, char modelid[80], char modeltexture[80], int ability);
static wchar_t* charToWChar(const char* text);
int FindTextureAlias(char* alias);
int CheckValidTextureAlias(char* alias);
extern int monsterenable;

void SetStartSpot();

struct gametext
{

	int textnum;
	int type;
	char text[2048];
	int shown;
};

struct dialogtext
{

	int textnum;
	int type;
	char text[2048];
};

typedef struct msoundlist
{
	int id;
	int yell;
	int attack;
	int die;
	int weapon;

	int hd;
	int ac;
	int hp;
	int thaco;
	float speed;
	char name[80];
	char damage[80];
	int playing;

} MSOUNDLIST;




#endif //__LOADWORLD_H