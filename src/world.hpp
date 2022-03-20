
#ifndef __WORLD_H__
#define __WORLD_H__

#include <windows.h>
#include <windowsx.h>
#include "d3dtypes.h"
#include "GlobalSettings.hpp"
#include <DirectXMath.h>

using namespace DirectX;

#define MAX_NUM_TEXTURES 400
#define SPOT_LIGHT_SOURCE 1
#define DIRECTIONAL_LIGHT_SOURCE 2
#define POINT_LIGHT_SOURCE 3

#define SPOT_LIGHT_SOURCE 1
#define DIRECTIONAL_LIGHT_SOURCE 2
#define POINT_LIGHT_SOURCE 3

#define USE_DEFAULT_MODEL_TEX	0
#define USE_PLAYERS_SKIN		1

// A structure for our custom vertex type. We added texture coordinates
typedef struct CUSTOMVERTEXTEST
{
	XMFLOAT3 position; // The position
	D3DCOLOR color;    // The color
	FLOAT nx, ny, nz;   // Normals
#ifndef SHOW_HOW_TO_USE_TCI
	FLOAT tu, tv;   // The texture coordinates

#endif
};

typedef struct texturemapping_typ
{
	float tu[4];
	float tv[4];
	int texture;
	char tex_alias_name[100];
	BOOL is_alpha_texture;

} TEXTUREMAPPING, * texturemapping_ptr;

typedef struct tagD3DAppMode
{
	int w;				   /* width */
	int h;				   /* height */
	int bpp;			   /* bits per pixel */
	BOOL bThisDriverCanDo; /*can current D3D driver render in this mode?*/
} D3DAppMode;

typedef struct setupinfo_typ
{
	D3DAppMode vmode;
	int screen;
	int control;
	BOOL sound;

} SETUPINFO, * setupinfo_ptr;

typedef struct vert_typ
{
	float x;
	float y;
	float z;

} VERT, * vert_ptr;

typedef struct light_typ
{
	BYTE r;
	BYTE g;
	BYTE b;

} LIGHT, * light_ptr;

typedef struct lightsource_typ
{
	BYTE r;
	BYTE g;
	BYTE b;

	int command;

	float direction_x;
	float direction_y;
	float direction_z;

	float position_x;
	float position_y;
	float position_z;

	float rcolour;
	float gcolour;
	float bcolour;

	float flickerrange;
	int flickerrangedir;
	float flickeratt;

} LIGHTSOURCE, * lightsouce_ptr;

typedef struct objectlist_typ
{
	float x;
	float y;
	float z;
	float rot_angle;
	int monsterid;
	int monstertexture;
	int type;
	int ability;
	char name[50];
	LIGHT* lit;
	LIGHTSOURCE* light_source;

} OBJECTLIST, * objectlist_ptr;

typedef struct dsonline_typ
{
	char ds_name[50];
	char ds_numplayers[50];
	char ds_ipaddress[50];
	char ds_ipproxy[50];
	char ds_webipaddress[50];
	char ds_type[50];
	char ds_action[255];
	char ds_message[255];
	char ds_version[50];
	char ds_game[50];
} DSONLINE, * dsonline_ptr;



typedef struct dsini_typ
{
	int x;
	int y;

} DSINI, * dsini_ptr;

typedef struct gunlist_typ
{
	float x;
	float y;
	float z;
	float x_offset;
	float y_offset;
	float z_offset;
	float rot_angle;
	int type;
	int light;
	int current_frame;
	int current_sequence;
	char name[256];
	int model_id;
	int skin_tex_id;
	int sound_id;
	char file[256];
	XMFLOAT3 velocity;
	int active;
	DWORD owner;
	int playernum;
	int playertype;

	int damage1;
	int damage2;
	int sattack;
	int sdamage;

	DWORD smove;
	DWORD sexplode;
	char gunname[80];
	int guntype;
	float qdist;
	int dmg;
	bool critical;

} GUNLIST, * gunlist_ptr;

typedef struct player_typ
{
	float x;
	float y;
	float z;
	float rot_angle;
	int monsterid;
	int model_id;
	int skin_tex_id;
	int current_weapon;
	int current_weapon_tex;
	int current_car;
	int current_frame;
	int current_sequence;
	int ping;
	int frags;
	int health;
	BOOL draw_external_wep;
	BOOL bIsFiring;
	BOOL bIsRunning;
	BOOL bIsPlayerAlive;
	BOOL bIsPlayerInWalkMode;
	BOOL bStopAnimating;
	BOOL bIsPlayerValid;
	char name[256];

	DWORD RRnetID;

	float gunx;
	float guny;
	float gunz;
	int gunid;
	int guntex;
	int gunangle;
	int animationdir;
	int volume;
	char chatstr[180];

	float dist;
	int sattack;
	int sdie;
	int syell;
	int sweapon;
	int mdmg;
	int hd;
	int ac;
	int hp;
	int damage1;
	int damage2;
	int thaco;
	int xp;
	int gold;
	float speed;
	char rname[80];
	char gunname[80];
	char texturename[80];
	char monsterweapon[80];
	int keys;
	int rings;
	int ability;
	int closest;

	int firespeed;
	int attackspeed;
	int applydamageonce;
	int takedamageonce;

} PLAYER, * player_ptr;

typedef struct pmdata_typ
{
	VERT** w;
	VERT* t;
	int* f;
	int* num_vert;
	D3DPRIMITIVETYPE* poly_cmd;
	int* texture_list;
	int* num_verts_per_object; // new line added by BILL
	int* num_faces_per_object; // new line added by BILL

	int tex_alias;
	int num_frames;
	int num_verts;
	int num_faces;
	int num_polys_per_frame;
	int num_verts_per_frame;
	int sequence_start_frame[50];
	int sequence_stop_frame[50];
	int texture_maps[100];

	BOOL use_indexed_primitive;
	float skx;
	float sky;
	float scale;
	char name[256];

} PLAYERMODELDATA, * pmdata_ptr;
/*
typedef struct objectdata_typ
{
	VERT v[2000]; // 6000
	VERT t[2000]; // 6000
	int f[2000];
	int num_vert[2000];
	D3DPRIMITIVETYPE poly_cmd[2000];
	int tex[2000];
	BOOL use_texmap[2000];
	char name[256];
	VERT connection[4];
	char typedesc[255];

	int hd;
	int ac;
	int hp;
	int damage;



} OBJECTDATA,*objectdata_ptr;
*/

typedef struct objectdata_typ
{

	//watch this carefully .. ds.log
	//v and t are vertices -
	// everything else polygones

	VERT v[2900]; // 6000
	VERT t[2900]; // 6000 //2200
	int f[2900];
	int num_vert[1600];
	D3DPRIMITIVETYPE poly_cmd[1600];
	int tex[1600];
	BOOL use_texmap[1600];
	char name[256];
	VERT connection[4];
	char typedesc[255];

	int hd;
	int ac;
	int hp;
	int damage;

} OBJECTDATA, * objectdata_ptr;
typedef struct modellistdisplay
{

	int model_id;
	int modeltexture;
	int scale;
	char name[255];
	char monsterweapon[80];
	int mtype;

} MODELLIST, * modellist_ptr;

typedef struct LevelMod
{
	int num;
	int objectid;
	int jump;
	char Function[255];
	char Text1[255];
	char Text2[255];
	int active;
	float currentheight;

} LEVELMOD, * levelmod_ptr;

typedef struct SwitchMod
{
	int num;
	int count;
	int objectid;
	int active;
	int direction;
	float savelocation;
	float x;
	float y;
	float z;

} SWITCHMOD, * swithcmod_ptr;

typedef struct DSRegistry
{
	char name[255];
	char key[255];
	char registered[255];

} DSREGISTRY, * dsregistry_ptr;

typedef struct DSServerInfo
{
	char name[255];
	char ipaddress[255];
	char players[255];

} DSSERVERINFO, * dsserverinfo_ptr;

typedef struct Merchant
{
	int object;
	int price;
	float qty;
	char Text1[255];
	char Text2[255];
	int active;
	int show;

} MERCHANT, * merchant_ptr;

typedef struct poly_sort
{
	float dist;
	int vert_index;
	int srcstart;
	int srcfstart;
	int vertsperpoly;
	int facesperpoly;
	int texture;

} POLY_SORT;

typedef struct camera_float
{
	float x;
	float y;
	float z;
	float dir;
} CAMERAFLOAT;



extern PLAYERMODELDATA* pmdata;

extern int* verts_per_poly;
extern int number_of_polys_per_frame;
extern int* faces_per_poly;
extern int* src_f;
extern D3DVERTEX temp_v[120000]; // debug
extern D3DVERTEX* src_v;
extern D3DPRIMITIVETYPE* dp_commands;
extern BOOL* dp_command_index_mode;
extern int* num_vert_per_object;
extern float sin_table[361];
extern float cos_table[361];
extern int* num_vert_per_object;
extern int num_polys_per_object[500];
extern int num_triangles_in_scene;
extern int num_verts_in_scene;
extern int num_dp_commands_in_scene;
extern int cnt;
extern int tempvcounter;
extern int num_light_sources;
extern int oblist_length;
//extern LPDIRECT3DVERTEXBUFFER9 g_pVB;
//extern LPDIRECT3DVERTEXBUFFER9 g_pVBBoundingBox;
//extern LPDIRECT3DVERTEXBUFFER9 g_pVBMonsterCaption;

extern float playerx;
extern float playery;
extern float playerz;
extern float rotatex;
extern float rotatey;
extern OBJECTLIST* oblist;
extern TEXTUREMAPPING TexMap[MAX_NUM_TEXTURES];
extern POLY_SORT ObjectsToDraw[MAX_NUM_QUADS];
extern int countboundingbox;
extern MODELLIST* model_list;
extern GUNLIST* your_gun;
extern PLAYER* monster_list;
extern PLAYER* item_list;
extern PLAYER* player_list2;
extern PLAYER* player_list;
extern int num_monsters;
extern int countmodellist;
extern int num_your_guns;
extern int itemlistcount;
extern int num_players2;
extern int* texture_list_buffer;
extern CUSTOMVERTEXTEST* pVertices;
extern CUSTOMVERTEXTEST* pBoundingBox;
extern CUSTOMVERTEXTEST* pMonsterCaption;

extern int cnt_f;

extern XMFLOAT3 m_vLookatPt;
extern XMFLOAT3 m_vEyePt;
extern float k;
extern D3DVALUE angy;
extern int maingameloop2;
extern D3DVALUE look_up_ang;
extern int trueplayernum;
extern int current_gun;
extern int g_ob_vert_count;
extern int runflag;
extern XMFLOAT3 eRadius;
extern XMFLOAT3 GunTruesave;

extern float wWidth;
extern float wHeight;
extern int objectcollide;




void DrawMonsters();
void DrawModel();
void DrawItems();
void DrawIndexedItems(int fakel, int vert_index);
void UpdateWorld(float fElapsedTime);
int random_num(int num);
void PrintMessage(HWND hwnd, char* message1, char* message2, int message_mode);
char* _itoa(int x);
void InitDS();
//void UpdateVertexBuffer(IDirect3DDevice9* pd3dDevice);
HRESULT AnimateCharacters();
float FastDistance(float fx, float fy, float fz);
int FindModelID(char* p);
int FindGunTexture(char* p);
float fixangle(float angle, float adjust);
void SetMonsterAnimationSequence(int player_number, int sequence_number);
int initDSTimer();
void RenderText(char* p);
void MakeBoundingBox();
int OpenDoor(int doornum, float dist, FLOAT fTimeKey);
void MoveMonsters(float fElapsedTime);
XMFLOAT3 collideWithWorld(XMFLOAT3 position, XMFLOAT3 velocity);
float FastDistance(float fx, float fy, float fz);
void WakeUpMonsters();
void MonsterHit();
void DrawPlayerGun();
XMFLOAT3 RadiusMultiply(XMFLOAT3 vector, XMFLOAT3 eRadius);
XMFLOAT3 RadiusDivide(XMFLOAT3 vector, XMFLOAT3 eRadius);
//void AddWorldLight(int ob_type, int angle, int oblist_index, IDirect3DDevice9* pd3dDevice);
void display_message(float x, float y, char text[2048], int r, int g, int b, float fontx, float fonty, int fonttype);
void SetPlayerAnimationSequence(int player_number, int sequence_number);
//void FlashLight(IDirect3DDevice9* pd3dDevice);
void ActivateSwitch();
//void ScanMod();
void PlayWaveFile(char* filename);
void GetItem();
void PlayWavSound(int id, int volume);
int SoundID(char* name);
int FreeSlave();
void CheckMidiMusic();
int MakeDice();


#endif // __WORLD_H__
