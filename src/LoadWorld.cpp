//-----------------------------------------------------------------------------
// File: LoadWorld.cpp
//
// Desc: Code for loading the DungeonStomp 3D world
//
// Copyright (c) 2001, Mark Longo, All rights reserved
//-----------------------------------------------------------------------------

#include <math.h>
#include <time.h>
#include <stdio.h>
#include "LoadWorld.hpp"
#include "world.hpp"
#include "ImportMD2.hpp"
#include "Import3DS.hpp"
#include "GlobalSettings.hpp"
#include "GameLogic.hpp"
#include "Missle.hpp"

#define MAX_NUM_OBJECTS_PER_CELL 250
#define MD2_MODEL 0
#define k3DS_MODEL 1

OBJECTDATA* obdata;
int obdata_length = 0;
int oblist_length = 0;
int* num_vert_per_object;
int num_polys_per_object[500];
int num_triangles_in_scene = 0;
int num_verts_in_scene = 0;
int num_dp_commands_in_scene = 0;
int cnt = 0;
int number_of_tex_aliases = 0;
int countmodellist = 0;
int num_your_guns = 0;
int num_monsters = 0;
int startposcounter;
int doorcounter;
int textcounter;
int slistcount = 0;
int player_count = 0;
int op_gun_count = 0;
int your_gun_count = 0;
int car_count = 0;
int type_num;
int num_imported_models_loaded = 0;
int save_dat_scale;
int mtype;
int addanewdoor = 0;
int addanewkey = 0;
int addanewtext = 0;
int addanewstartpos = 0;
int cell_length[2000];
int cell_xlist[2000][20];
int cell_zlist[2000][20];
int cell_list_debug[2000];
int addanewplayer = 0;
char g_model_filename[256];
float monx, mony, monz;
int totalmod;
int outside = 0;

extern int usespell;
extern struct gametext gtext[200];
extern int ResetSound();

TEXTUREMAPPING  TexMap[MAX_NUM_TEXTURES];

MSOUNDLIST slist[500];

CLoadWorld* pCWorld;
int load_level(char* filename);
void MakeDamageDice();
/*
struct doors
{

	int doornum;
	float angle;
	int swing;
	int key;
	int open;
	float saveangle;
	int type;
	int listen;
	int y;
	float up;
};

*/
typedef struct startposition
{
	float x;
	float y;
	float z;
	float angle;
} startpos2;

extern char levelname[80];
int current_level;
extern D3DVALUE angy;
extern D3DVALUE look_up_ang;

extern PLAYER* item_list;
extern PLAYER* player_list2;
extern PLAYER* player_list;
extern int num_monsters;
extern int countswitches;

//typedef enum _D3DPRIMITIVETYPE {
//	D3DPT_POINTLIST = 1,
//	D3DPT_LINELIST = 2,
//	D3DPT_LINESTRIP = 3,
//	D3DPT_TRIANGLELIST = 4,
//	D3DPT_TRIANGLESTRIP = 5,
//	D3DPT_TRIANGLEFAN = 6,
//	D3DPT_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
//} D3DPRIMITIVETYPE;

struct startposition startpos[200];

LEVELMOD* levelmodify;
SWITCHMOD* switchmodify;

int DSound_Replicate_Sound(int id);

CLoadWorld::CLoadWorld()
{
	pCWorld = this;
}

BOOL CLoadWorld::LoadWorldMap(char* filename)
{
	FILE* fp;
	char s[256];
	char p[256];
	int y_count = 30;
	int done = 0;
	int object_count = 0;
	int vert_count = 0;
	int pv_count = 0;
	int poly_count = 0;
	int object_id;
	BOOL lwm_start_flag = TRUE;
	int mem_counter = 0;
	int lit_vert;
	char monsterid[256];
	char monstertexture[256];
	char mnum[256];
	int ability = 0;
	char abil[256];
	char globaltext[2048];
	char bigbuf[2048];
	char bigbuf2[2048];
	int bufc = 0;
	int bufc2 = 0;
	int loop1 = 0;

	//oblist = new OBJECTLIST[1000];


	doorcounter = 0;
	textcounter = 0;
	startposcounter = 0;
	countswitches = 0;

	BYTE red, green, blue;

	char path[255];

	//sprintf(path, "..\\bin\\%s", filename);
	sprintf_s(path, "%s", filename);

	if (fopen_s(&fp, path, "r") != 0)
	{
		//PrintMessage(hwnd, "Error can't load file ", filename, SCN_AND_FILE);
		//MessageBox(hwnd, "Error can't load file", NULL, MB_OK);
		return FALSE;
	}

	//PrintMessage(hwnd, "Loading map ", filename, SCN_AND_FILE);
	int num_light_sources_in_map = 0;
	int num_light_sources = 0;
	outside = 0;
	while (done == 0)
	{
		fscanf_s(fp, "%s", &s, 256);

		if (strcmp(s, "OBJECT") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);

			object_id = CheckObjectId((char*)&p);

			if (strstr(p, "door") != NULL)
			{
				addanewdoor = 1;
			}
			else if (strstr(p, "text") != NULL)
			{
				addanewtext = 1;
			}
			else if (strstr(p, "startpos") != NULL)
			{
				addanewstartpos = 1;
			}
			else if (strstr(p, "!") != NULL)
			{
				if (object_id == 35)
					addanewplayer = 2;
				else
					addanewplayer = 1;
			}

			if (object_id == -1)
			{
				//PrintMessage(hwnd, "Error Bad Object ID in: LoadWorld ", p, SCN_AND_FILE);
				//MessageBox(hwnd, "Error Bad Object ID in: LoadWorld", NULL, MB_OK);
				return FALSE;
			}
			if (lwm_start_flag == FALSE)
				object_count++;

			oblist[object_count].type = object_id;
			strcpy_s(oblist[object_count].name, 10000, p);

			//num_lverts = num_vert_per_object[object_id];
			//mem_counter += sizeof(LIGHT) * num_lverts;

			oblist[object_count].light_source = new LIGHTSOURCE;
			oblist[object_count].light_source->command = 0;

			lwm_start_flag = FALSE;
		}

		if (strcmp(s, "CO_ORDINATES") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			oblist[object_count].x = (float)atof(p);

			fscanf_s(fp, "%s", &p, 256);
			oblist[object_count].y = (float)atof(p) + 28.0f;

			fscanf_s(fp, "%s", &p, 256);
			oblist[object_count].z = (float)atof(p);

			if (addanewplayer > 0)
			{

				monx = oblist[object_count].x;
				mony = oblist[object_count].y + 44.0f;
				monz = oblist[object_count].z;
			}

			if (addanewstartpos == 1)
			{

				startpos[startposcounter].x = oblist[object_count].x;
				startpos[startposcounter].y = oblist[object_count].y + 100.0f;
				startpos[startposcounter].z = oblist[object_count].z;
			}
		}

		if (strcmp(s, "ROT_ANGLE") == 0)
		{
			float mid = 0;
			float mtex = 0;
			float monnum = 0;

			int truemonsterid = 0;

			if (addanewplayer > 0)
			{
				fscanf_s(fp, "%s", &p, 256);
				fscanf_s(fp, "%s", &monsterid, 256);
				fscanf_s(fp, "%s", &monstertexture, 256);
				fscanf_s(fp, "%s", &mnum, 256);
				fscanf_s(fp, "%s", &abil, 256);

				truemonsterid = (int)atof(monsterid);

				if (addanewplayer == 2)
				{
					mid = (float)FindModelID(monsterid);

					if (strcmp(monstertexture, "0") == 0)
						mtex = 0;
					else if (strcmp(monstertexture, "-1") == 0)
						mtex = -1;
					else
						mtex = (float)FindTextureAlias(monstertexture);

					monnum = (float)atof(mnum);
				}
				else
				{
					mid = (float)atof(monsterid);
					mtex = (float)FindTextureAlias(monstertexture);
					monnum = (float)atof(mnum);
				}


				if (mtex == 94) {
					//94-101
					int raction = random_num(7);  // 94 + 7 = 101
					mtex += raction;
				}

				oblist[object_count].rot_angle = (float)atof(p);
				ability = (int)atof(abil);
				oblist[object_count].ability = (int)atof(abil);
			}
			else
			{

				if (addanewtext == 1)
				{

					fscanf_s(fp, "%s", &abil, 256);
					fgets(bigbuf, 2048, fp);

					bufc2 = 0;

					for (loop1 = 1; loop1 < (int)strlen(bigbuf); loop1++)
					{

						if (bigbuf[loop1] != 13 && bigbuf[loop1] != 10)
							bigbuf2[bufc2++] = bigbuf[loop1];
					}

					bigbuf2[bufc2] = '\0';

					strcpy_s(globaltext, bigbuf2);
					if (strstr(bigbuf2, "outside") != NULL)
					{
						outside = 1;
					}

					oblist[object_count].rot_angle = (float)0;
					oblist[object_count].ability = (int)atof(abil);
				}
				else
				{

					if (addanewdoor == 1)
					{
						fscanf_s(fp, "%s", &p, 256);
						fscanf_s(fp, "%s", &abil, 256);

						oblist[object_count].rot_angle = (float)atof(p);
						ability = (int)atof(abil);
						oblist[object_count].ability = (int)atof(abil);
						addanewkey = 0;
					}
					else if (addanewstartpos == 1)
					{

						fscanf_s(fp, "%s", &p, 256);
						oblist[object_count].rot_angle = (float)atof(p);
						startpos[startposcounter].angle = (float)atof(p);
						startposcounter++;
						addanewstartpos = 0;
					}

					else
					{

						fscanf_s(fp, "%s", &p, 256);
						oblist[object_count].rot_angle = (float)atof(p);
					}
				}
			}

			if (addanewplayer > 0)
			{

				if (addanewplayer == 2)
				{
					if (mtex == 0)
					{
						AddModel(monx, mony, monz, (float)atof(p), mid, mtex, monnum, monsterid, monstertexture, ability);
					}
					else if (mtex == -1) {
						AddItem(monx, mony - 10.0f, monz, (float)atof(p), mid, mtex, monnum, monsterid, monstertexture, ability);

						//AddItem(monx, mony - 10.0f, monz, (float)atof(p), 6, -1, 455, "POTION", "-1", ability);
					}
					else if (mtex == -2)
					{
						//AddItem(monx, mony - 10.0f, monz, (float)atof(p), mid, mtex, monnum, monsterid, monstertexture, ability);
					}
					else
					{

						int sc = 0;

						int s1, s2, s3, s4, s5, s6, s7;
						float speed = 0;
						char name[80];
						char dm[80];

						for (sc = 0; sc < slistcount; sc++)
						{
							if (slist[sc].id == mid)
							{
								s1 = slist[sc].attack;
								s2 = slist[sc].die;
								s3 = slist[sc].weapon;
								s4 = slist[sc].yell;
								s5 = slist[sc].ac;
								s6 = slist[sc].hd;
								s7 = slist[sc].thaco;
								strcpy_s(name, slist[sc].name);
								speed = slist[sc].speed;
								strcpy_s(dm, slist[sc].damage);
								break;
							}
						}

						AddMonster(monx, mony, monz, (float)atof(p), mid, mtex, monnum, s1, s2, s3, s4, s5, s6, dm, s7, name, speed, ability);
					}
				}

				addanewplayer = 0;


				oblist[object_count].monstertexture = (int)mtex;


				oblist[object_count].monsterid = (int)monnum;
			}

			if (addanewdoor == 1)
			{

				door[doorcounter].angle = (float)atof(p);
				door[doorcounter].saveangle = (float)atof(p);
				door[doorcounter].doornum = object_count;
				door[doorcounter].key = (int)atof(abil);
				door[doorcounter].open = 0;
				door[doorcounter].swing = 0;
				door[doorcounter].type = 0;
				door[doorcounter].listen = 0;
				door[doorcounter].y = 0;
				door[doorcounter].up = 0;
				addanewdoor = 0;
				doorcounter++;
			}

			if (addanewtext == 1)
			{
				strcpy_s(gtext[textcounter].text, globaltext);
				gtext[textcounter].textnum = object_count;
				gtext[textcounter].type = (int)atof(abil);
				gtext[textcounter].shown = 0;
				addanewtext = 0;
				textcounter++;
			}
		}

		if (strcmp(s, "LIGHT_ON_VERT") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			lit_vert = atoi(p);

			fscanf_s(fp, "%s", &p, 256);
			red = atoi(p);
			oblist[object_count].lit[lit_vert].r = red;

			fscanf_s(fp, "%s", &p, 256);
			green = atoi(p);
			oblist[object_count].lit[lit_vert].g = green;

			fscanf_s(fp, "%s", &p, 256);
			blue = atoi(p);
			oblist[object_count].lit[lit_vert].b = blue;
		}

		if (strcmp(s, "LIGHT_SOURCE") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);

			if (strcmp(p, "Spotlight") == 0)
				oblist[object_count].light_source->command = SPOT_LIGHT_SOURCE;

			if (strcmp(p, "directional") == 0)
				oblist[object_count].light_source->command = DIRECTIONAL_LIGHT_SOURCE;

			if (strcmp(p, "point") == 0)
				oblist[object_count].light_source->command = POINT_LIGHT_SOURCE;

			if (strcmp(p, "flicker") == 0)
				oblist[object_count].light_source->command = 900;

			fscanf_s(fp, "%s", &p, 256);

			if (strcmp(p, "POS") == 0)
			{

				//oblist[object_count].light_source->flickerrangedir = 0;
				//int raction = random_num(10);
				//oblist[object_count].light_source->flickeratt = (float)raction * 0.1f;
				//raction = random_num(15);
				//oblist[object_count].light_source->flickerrange = (float)raction;
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->position_x = (float)atof(p);
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->position_y = (float)atof(p);
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->position_z = (float)atof(p);
			}

			fscanf_s(fp, "%s", &p, 256);

			if (strcmp(p, "DIR") == 0)
			{
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->direction_x = (float)atof(p);
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->direction_y = (float)atof(p);
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->direction_z = (float)atof(p);
			}

			fscanf_s(fp, "%s", &p, 256);

			if (strcmp(p, "COLOUR") == 0)
			{
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->rcolour = (float)atof(p);
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->gcolour = (float)atof(p);
				fscanf_s(fp, "%s", &p, 256);
				oblist[object_count].light_source->bcolour = (float)atof(p);
			}
			num_light_sources_in_map++;
		}

		if (strcmp(s, "END_FILE") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			oblist_length = object_count + 1;
			done = 1;
		}
	}
	fclose(fp);

	//_itoa_s(oblist_length, buffer, 100, 10);
	//PrintMessage(hwnd, buffer, " map objects loaded (oblist_length)", SCN_AND_FILE);
	//PrintMessage(hwnd, "\n", NULL, LOGFILE_ONLY);

	/*
	for (int i = 0; i < 100;i++) {

		char buf[100];
		sprintf_s(buf, 100, "%s",oblist[i].name);
	}
	*/

	return TRUE;
}

int CLoadWorld::CheckObjectId(char* p)
{
	int i;
	char* buffer2;

	for (i = 0; i < obdata_length; i++)
	{
		buffer2 = obdata[i].name;

		if (strcmp(buffer2, p) == 0)
		{
			return i;
		}
	}
	//PrintMessage(hwnd, buffer2, "ERROR bad ID in : CheckObjectId ", SCN_AND_FILE);
	//MessageBox(hwnd, buffer2, "Bad ID in :CheckObjectId", MB_OK);

	return -1; //error
}

BOOL CLoadWorld::LoadObjectData(char* filename)
{
	FILE* fp;
	int i;
	int done = 0;
	int object_id = 0;
	int object_count = 0;
	int old_object_id = 0;
	int vert_count = 0;
	int pv_count = 0;
	int poly_count = 0;
	int texture = 0;
	int conn_cnt = 0;
	int num_v;
	BOOL command_error;
	float dat_scale;
	char buffer[256];
	char s[256];
	char p[256];

	obdata = new OBJECTDATA[1000];

	int maxvertcount = 0;

	int vertcountfinal = 0;
	int polycountfinal = 0;

	if (fopen_s(&fp, filename, "r") != 0)
	{
		//PrintMessage(hwnd, "ERROR can't load ", filename, SCN_AND_FILE);
		//MessageBox(hwnd, filename, "Error can't load file", MB_OK);
		return FALSE;
	}

	//PrintMessage(hwnd, "Loading ", filename, SCN_AND_FILE);

	while (done == 0)
	{
		command_error = TRUE;

		fscanf_s(fp, "%s", &s, 256);

		if ((strcmp(s, "OBJECT") == 0) || (strcmp(s, "Q2M_OBJECT") == 0))

		{
			dat_scale = 1.0;

			old_object_id = object_id;

			fscanf_s(fp, "%s", &p, 256); // read object ID
			object_id = atoi(p);

			// find the highest object_id

			if (object_id > object_count)
				object_count = object_id;

			if ((object_id < 0) || (object_id > 399))
			{
				//MessageBox(hwnd, "Error Bad Object ID in: LoadObjectData", NULL, MB_OK);
				return FALSE;
			}

			num_vert_per_object[old_object_id] = vert_count;
			num_polys_per_object[old_object_id] = poly_count;

			vertcountfinal += vert_count;
			polycountfinal += poly_count;

			_itoa_s(vert_count, buffer, _countof(buffer), 10);

			if (vert_count > maxvertcount)
				maxvertcount = vert_count;

			//PrintMessage(hwnd, buffer, " vert_count objects", LOGFILE_ONLY);
			_itoa_s(poly_count, buffer, _countof(buffer), 10);
			//PrintMessage(hwnd, buffer, " polycount objects", LOGFILE_ONLY);

			//PrintMessage(hwnd, "\n", NULL, LOGFILE_ONLY);

			vert_count = 0;
			poly_count = 0;
			conn_cnt = 0;

			fscanf_s(fp, "%s", &p, 256); // read object name
			//PrintMessage(hwnd, p, " vert_count objects", LOGFILE_ONLY);

			strcpy_s((char*)obdata[object_id].name, 256, (char*)&p);
			command_error = FALSE;
		}

		if (strcmp(s, "SCALE") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			dat_scale = (float)atof(p);
			command_error = FALSE;
		}

		if (strcmp(s, "SHADOW") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			int shadow = (float)atof(p);
			
			obdata[object_id].shadow = shadow;

			command_error = FALSE;
		}

		if (strcmp(s, "TEXTURE") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);

			texture = CheckValidTextureAlias(p);


			//TODO: fix texture

			//texture = 1;

			if (texture == -1)
			{
				//PrintMessage(hwnd, "Error in objects.dat - Bad Texture ID", p, LOGFILE_ONLY);
				//MessageBox(hwnd, "Error in objects.dat", "Bad Texture ID", MB_OK);
				fclose(fp);
				return FALSE;
			}
			command_error = FALSE;
		}

		if (strcmp(s, "TYPE") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			command_error = FALSE;
		}

		if (strcmp(s, "TRI") == 0)
		{
			for (i = 0; i < 3; i++)
			{
				ReadObDataVert(fp, object_id, vert_count, dat_scale);
				vert_count++;
			}

			obdata[object_id].use_texmap[poly_count] = TRUE;
			obdata[object_id].tex[poly_count] = texture;
			obdata[object_id].num_vert[poly_count] = 3;
			obdata[object_id].poly_cmd[poly_count] = D3DPT_TRIANGLELIST; //POLY_CMD_TRI;
			poly_count++;
			command_error = FALSE;
		}

		if (strcmp(s, "QUAD") == 0)
		{
			ReadObDataVert(fp, object_id, vert_count, dat_scale);
			ReadObDataVert(fp, object_id, vert_count + 1, dat_scale);
			ReadObDataVert(fp, object_id, vert_count + 3, dat_scale);
			ReadObDataVert(fp, object_id, vert_count + 2, dat_scale);

			vert_count += 4;

			obdata[object_id].use_texmap[poly_count] = TRUE;
			obdata[object_id].tex[poly_count] = texture;
			obdata[object_id].num_vert[poly_count] = 4;
			obdata[object_id].poly_cmd[poly_count] = D3DPT_TRIANGLESTRIP; //POLY_CMD_QUAD;
			poly_count++;
			command_error = FALSE;
		}

		if (strcmp(s, "TRITEX") == 0)
		{
			for (i = 0; i < 3; i++)
			{
				ReadObDataVertEx(fp, object_id, vert_count, dat_scale);
				vert_count++;
			}

			obdata[object_id].use_texmap[poly_count] = FALSE;
			obdata[object_id].tex[poly_count] = texture;
			obdata[object_id].num_vert[poly_count] = 3;
			obdata[object_id].poly_cmd[poly_count] = D3DPT_TRIANGLELIST; // POLY_CMD_TRI_TEX;
			poly_count++;
			command_error = FALSE;
		}

		if (strcmp(s, "QUADTEX") == 0)
		{
			ReadObDataVertEx(fp, object_id, vert_count, dat_scale);
			ReadObDataVertEx(fp, object_id, vert_count + 1, dat_scale);
			ReadObDataVertEx(fp, object_id, vert_count + 3, dat_scale);
			ReadObDataVertEx(fp, object_id, vert_count + 2, dat_scale);

			vert_count += 4;

			obdata[object_id].use_texmap[poly_count] = FALSE;
			obdata[object_id].tex[poly_count] = texture;
			obdata[object_id].num_vert[poly_count] = 4;
			obdata[object_id].poly_cmd[poly_count] = D3DPT_TRIANGLESTRIP; //POLY_CMD_QUAD_TEX;
			poly_count++;
			command_error = FALSE;
		}

		if (strcmp(s, "TRI_STRIP") == 0)
		{
			// Get numbers of verts in triangle strip
			fscanf_s(fp, "%s", &p, 256);
			num_v = atoi(p);

			for (i = 0; i < num_v; i++)
			{
				ReadObDataVertEx(fp, object_id, vert_count, dat_scale);
				vert_count++;
			}

			obdata[object_id].use_texmap[poly_count] = TRUE;
			obdata[object_id].tex[poly_count] = texture;
			obdata[object_id].num_vert[poly_count] = num_v;
			obdata[object_id].poly_cmd[poly_count] = D3DPT_TRIANGLESTRIP; //POLY_CMD_TRI_STRIP;
			poly_count++;
			command_error = FALSE;
		}

		if (strcmp(s, "TRI_FAN") == 0)
		{
			// Get numbers of verts in triangle fan
			fscanf_s(fp, "%s", &p, 256);
			num_v = atoi(p);

			for (i = 0; i < num_v; i++)
			{
				ReadObDataVertEx(fp, object_id, vert_count, dat_scale);
				vert_count++;
			}

			obdata[object_id].use_texmap[poly_count] = TRUE;
			obdata[object_id].tex[poly_count] = texture;
			obdata[object_id].num_vert[poly_count] = num_v;
			obdata[object_id].poly_cmd[poly_count] = D3DPT_TRIANGLEFAN; //POLY_CMD_TRI_FAN;
			poly_count++;
			command_error = FALSE;
		}

		if (strcmp(s, "CONNECTION") == 0)
		{
			if (conn_cnt < 4)
			{
				fscanf_s(fp, "%s", &p, 256);
				obdata[object_id].connection[conn_cnt].x = (float)atof(p);
				fscanf_s(fp, "%s", &p, 256);
				obdata[object_id].connection[conn_cnt].y = (float)atof(p);
				fscanf_s(fp, "%s", &p, 256);
				obdata[object_id].connection[conn_cnt].z = (float)atof(p);
				conn_cnt++;
			}
			else
			{
				fscanf_s(fp, "%s", &p, 256);
				fscanf_s(fp, "%s", &p, 256);
				fscanf_s(fp, "%s", &p, 256);
			}
			command_error = FALSE;
		}

		if (strcmp(s, "END_FILE") == 0)
		{
			num_vert_per_object[object_id] = vert_count;
			num_polys_per_object[object_id] = poly_count;
			obdata_length = object_count + 1;
			command_error = FALSE;
			done = 1;
		}

		if (command_error == TRUE)
		{
			_itoa_s(object_id, buffer, _countof(buffer), 10);
			//PrintMessage(NULL, "CLoadWorld::LoadObjectData - ERROR in objects.dat, object : ", buffer, LOGFILE_ONLY);
			//MessageBox(hwnd, s, "Unrecognised command", MB_OK);
			fclose(fp);
			return FALSE;
		}
	}
	fclose(fp);

	//_itoa_s(obdata_length, buffer, _countof(buffer), 10);

	//PrintMessage(hwnd, buffer, " DAT objects loaded", SCN_AND_FILE);

	//PrintMessage(hwnd, "\n", NULL, LOGFILE_ONLY);

	//_itoa_s(vertcountfinal, buffer, _countof(buffer), 10);
	//PrintMessage(hwnd, buffer, " vert_count objects", SCN_AND_FILE);
	//PrintMessage(hwnd, "\n", NULL, LOGFILE_ONLY);
	//_itoa_s(polycountfinal, buffer, _countof(buffer), 10);
	//PrintMessage(hwnd, buffer, " polycount objects", SCN_AND_FILE);

	//PrintMessage(hwnd, "\n", NULL, LOGFILE_ONLY);

	//	char buf2[100];

	//sprintf_s(buffer, "MAXVERTCOUNT %d", maxvertcount);
	//PrintMessage(hwnd, buffer, "", LOGFILE_ONLY);

	//for (i = 0; i < obdata_length; i++)
	//{

//		sprintf_s(buffer, "%d %s", i, obdata[i].name);
	//	PrintMessage(hwnd, buffer, "", LOGFILE_ONLY);
	//}

	return TRUE;
}

BOOL CLoadWorld::ReadObDataVertEx(FILE* fp, int object_id, int vert_count, float dat_scale)
{
	float x, y, z;
	char p[256];

	fscanf_s(fp, "%s", &p, 256);
	x = dat_scale * (float)atof(p);
	obdata[object_id].v[vert_count].x = x;

	fscanf_s(fp, "%s", &p, 256);
	y = dat_scale * (float)atof(p);
	obdata[object_id].v[vert_count].y = y;

	fscanf_s(fp, "%s", &p, 256);
	z = dat_scale * (float)atof(p);
	obdata[object_id].v[vert_count].z = z;

	fscanf_s(fp, "%s", &p, 256);
	obdata[object_id].t[vert_count].x = (float)atof(p);

	fscanf_s(fp, "%s", &p, 256);
	obdata[object_id].t[vert_count].y = (float)atof(p);

	return TRUE;
}

BOOL CLoadWorld::ReadObDataVert(FILE* fp, int object_id, int vert_count, float dat_scale)
{
	float x, y, z;
	char p[256];

	fscanf_s(fp, "%s", &p, 256);
	x = dat_scale * (float)atof(p);
	obdata[object_id].v[vert_count].x = x;

	fscanf_s(fp, "%s", &p, 256);
	y = dat_scale * (float)atof(p);
	obdata[object_id].v[vert_count].y = y;

	fscanf_s(fp, "%s", &p, 256);
	z = dat_scale * (float)atof(p);
	obdata[object_id].v[vert_count].z = z;

	return TRUE;
}

/*
BOOL CLoadWorld::LoadRRTextures(char* filename, IDirect3DDevice9* pd3dDevice)
{
	FILE* fp;
	char s[256];
	char p[256];

	int y_count = 30;
	int done = 0;
	int object_count = 0;
	int vert_count = 0;
	int pv_count = 0;
	int poly_count = 0;
	int tex_alias_counter = 0;
	int tex_counter = 0;
	int i;
	BOOL start_flag = TRUE;
	BOOL found;
	//LPDIRECTDRAWSURFACE7 lpDDsurface;

	if (fopen_s(&fp, filename, "r") != 0)
	{
		//PrintMessage(hwnd, "ERROR can't open ", filename, SCN_AND_FILE);
		//MessageBox(hwnd, filename, "Error can't open", MB_OK);
		//return FALSE;
	}

	//D3DTextr_InvalidateAllTextures();

	//for (i = 0; i < MAX_NUM_TEXTURES; i++)
		//TexMap[i].is_alpha_texture = FALSE;

	//for (i = 0; i < MAX_NUM_TEXTURES; i++)
		//lpddsImagePtr[i] = NULL;

	//NumTextures = 0;

	while (done == 0)
	{
		found = FALSE;
		fscanf_s(fp, "%s", &s, 256);

		if (strcmp(s, "AddTexture") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			//PrintMessage(hwnd, "Loading ", p, LOGFILE_ONLY);
			//strcpy_s(ImageFile[tex_counter], p);

			if (strstr(p, "die8s2.bmp") != NULL)
			{

				int aaa = 1;
			}

			//if (D3DTextr_CreateTextureFromFile(p, 0, 0) != S_OK)
				//PrintMessage(NULL, "CLoadWorld::LoadRRTextures() - Can't load texture ", p, LOGFILE_ONLY);


				// Use D3DX to create a texture from a file based image
			//if (FAILED(D3DXCreateTextureFromFile(pd3dDevice, charToWChar(p), &g_pTextureList[tex_counter])))


			//TODO: Fix this

			//if (D3DXCreateTextureFromFile(pd3dDevice, charToWChar(p), &g_pTextureList[tex_counter]))
			//{
			//	int a = 1;
			//}
			//else {
			//	tex_counter++;
			//	found = TRUE;
			//}
		}

		if (strcmp(s, "Alias") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);

			fscanf_s(fp, "%s", &p, 256);

			strcpy_s((char*)TexMap[tex_alias_counter].tex_alias_name, 100, (char*)&p);

			TexMap[tex_alias_counter].texture = tex_counter - 1;

			fscanf_s(fp, "%s", &p, 256);
			if (strcmp(p, "AlphaTransparent") == 0)
				TexMap[tex_alias_counter].is_alpha_texture = TRUE;

			i = tex_alias_counter;
			fscanf_s(fp, "%s", &p, 256);

			if (strcmp(p, "WHOLE") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.0;
			}

			if (strcmp(p, "TL_QUAD") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)0.5;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)0.5;
				TexMap[i].tv[2] = (float)0.5;
				TexMap[i].tu[3] = (float)0.5;
				TexMap[i].tv[3] = (float)0.0;
			}

			if (strcmp(p, "TR_QUAD") == 0)
			{
				TexMap[i].tu[0] = (float)0.5;
				TexMap[i].tv[0] = (float)0.5;
				TexMap[i].tu[1] = (float)0.5;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)0.5;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.0;
			}
			if (strcmp(p, "LL_QUAD") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.5;
				TexMap[i].tu[2] = (float)0.5;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)0.5;
				TexMap[i].tv[3] = (float)0.5;
			}
			if (strcmp(p, "LR_QUAD") == 0)
			{
				TexMap[i].tu[0] = (float)0.5;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.5;
				TexMap[i].tv[1] = (float)0.5;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.5;
			}
			if (strcmp(p, "TOP_HALF") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)0.5;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)0.5;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.0;
			}
			if (strcmp(p, "BOT_HALF") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.5;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.5;
			}
			if (strcmp(p, "LEFT_HALF") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)0.5;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)0.5;
				TexMap[i].tv[3] = (float)0.0;
			}
			if (strcmp(p, "RIGHT_HALF") == 0)
			{
				TexMap[i].tu[0] = (float)0.5;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.5;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.0;
			}
			if (strcmp(p, "TL_TRI") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)0.0;
				TexMap[i].tu[1] = (float)1.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)0.0;
				TexMap[i].tv[2] = (float)1.0;
			}
			if (strcmp(p, "BR_TRI") == 0)
			{
			}

			tex_alias_counter++;
			found = TRUE;
		}

		if (strcmp(s, "END_FILE") == 0)
		{
			//PrintMessage(hwnd, "\n", NULL, LOGFILE_ONLY);
			number_of_tex_aliases = tex_alias_counter;
			//NumTextures = tex_counter;
			found = TRUE;
			done = 1;
		}

		if (found == FALSE)
		{
			//PrintMessage(hwnd, "File Error: Syntax problem :", p, SCN_AND_FILE);
			//MessageBox(hwnd, "p", "File Error: Syntax problem ", MB_OK);
			//return FALSE;
		}
	}
	fclose(fp);

	//D3DTextr_RestoreAllTextures(GetDevice());
	//DDCOLORKEY ckey;
	// set color key to black, for crosshair texture.
	// so any pixels in crosshair texture with color RGB 0,0,0 will be transparent
	//ckey.dwColorSpaceLowValue = RGB_MAKE(0, 0, 0);
	//ckey.dwColorSpaceHighValue = 0L;

	//for (i = 0; i < NumTextures; i++)
	//{
	//	lpDDsurface = D3DTextr_GetSurface(ImageFile[i]);
	//	lpddsImagePtr[i] = lpDDsurface;

	//	if (strstr(ImageFile[i], "@") != NULL || strstr(ImageFile[i], "fontA") != NULL || strstr(ImageFile[i], "die") != NULL || strstr(ImageFile[i], "dungeont") != NULL || strstr(ImageFile[i], "button") != NULL || strstr(ImageFile[i], "lightmap") != NULL || strstr(ImageFile[i], "flare") != NULL || strstr(ImageFile[i], "pb8") != NULL || strstr(ImageFile[i], "pb0") != NULL || strstr(ImageFile[i], "crosshair") != NULL || strstr(ImageFile[i], "pbm") != NULL || strstr(ImageFile[i], "box1") != NULL)
	//	{

	//		if (lpddsImagePtr[i])
	//			lpddsImagePtr[i]->SetColorKey(DDCKEY_SRCBLT, &ckey);
	//	}
	//	else
	//	{

	//		DDCOLORKEY ckeyfix;
	//		ckeyfix.dwColorSpaceLowValue = RGB_MAKE(9, 99, 99);
	//		ckeyfix.dwColorSpaceHighValue = 0L;

	//		if (lpddsImagePtr[i])
	//			lpddsImagePtr[i]->SetColorKey(DDCKEY_SRCBLT, &ckeyfix);
	//	}
	//}
	//PrintMessage(hwnd, "CMyD3DApplication::LoadRRTextures - suceeded", NULL, LOGFILE_ONLY);

	return TRUE;
}

*/

int CheckValidTextureAlias(char* alias)
{
	int i;
	char* buffer2;

	for (i = 0; i < number_of_tex_aliases; i++)
	{
		buffer2 = TexMap[i].tex_alias_name;

		if (_strcmpi(buffer2, alias) == 0)
		{
			return i;
		}
	}

	return -1; //error
}

int FindTextureAlias(char* alias)
{
	int i;
	char* buffer2;

	for (i = 0; i < number_of_tex_aliases; i++)
	{
		buffer2 = TexMap[i].tex_alias_name;

		if (_strcmpi(buffer2, alias) == 0)
		{
			return i;
		}
	}

	return -1; //error
}

int random_num(int num)
{

	UINT rndNum;

	rndNum = rand() % num;

	return rndNum;
}

BOOL CLoadWorld::LoadImportedModelList(char* filename)
{
	FILE* fp;
	FILE* fp2;
	char p[256];
	int done = 0;
	//	int i;
	char command[256];
	float f;
	int model_id;
	//char model_name[256];
	char model_filename[256];
	int model_tex_alias;

	float scale;
	int Q2M_Anim_Counter = 0;
	BOOL command_recognised;
	BOOL Model_loaded_flags[1000] = { FALSE };
	slistcount = 0;
	//	num_players   = 0;

	int num_op_guns = 0;
	int model_format;


	if (fopen_s(&fp, filename, "r") != 0)
	{
		//PrintMessage(hwnd, "ERROR can't load ", filename, SCN_AND_FILE);
		//MessageBox(hwnd, filename, "Error can't load file", MB_OK);
		return FALSE;
	}

	//PrintMessage(hwnd, "Loading ", filename, SCN_AND_FILE);

	fscanf_s(fp, "%s", &command, 256);

	if (strcmp(command, "FILENAME") == 0)
		fscanf_s(fp, "%s", &p, 256); // ignore comment
	else
		return FALSE;

	while (done == 0)
	{
		command_recognised = FALSE;
		scale = (float)0.4;

		fscanf_s(fp, "%s", &command, 256); // get next command

		fscanf_s(fp, "%s", &p, 256); // read player number
		type_num = atoi(p);

		fscanf_s(fp, "%s", &p, 256); // find model file foramt, MD2 or 3DS ?

		if (strcmp(p, "MD2") == 0)
		{
			mtype = 0;
			model_format = MD2_MODEL;
		}

		if (strcmp(p, "3DS") == 0)
		{
			model_format = k3DS_MODEL;
			mtype = 1;
		}

		fscanf_s(fp, "%s", &p, 256); //  ignore comment "model_id"

		fscanf_s(fp, "%s", &p, 256); // read model id
		model_id = atoi(p);

		fscanf_s(fp, "%s", &model_filename, 256); // read filename for object

		fscanf_s(fp, "%s", &p, 256); //  ignore comment "tex"

		fscanf_s(fp, "%s", &p, 256); // read texture alias id

		model_tex_alias = FindTextureAlias(p) + 1;

		if (strcmp(command, "PLAYER") == 0)
		{
			fscanf_s(fp, "%s", &p, 256); // ignore comment pos
			fscanf_s(fp, "%s", &p, 256); // x pos
			fscanf_s(fp, "%s", &p, 256); // y pos
			fscanf_s(fp, "%s", &p, 256); // z pos
			fscanf_s(fp, "%s", &p, 256); // ignore comment angle
			fscanf_s(fp, "%s", &p, 256); // angle
			fscanf_s(fp, "%s", &p, 256); // ignore comment scale
			fscanf_s(fp, "%f", &f);
			scale = f;

			fscanf_s(fp, "%s", &p, 256); // Don't draw players external weapon
										 //			if(strcmp(p, "NO_EXTERNAL_WEP") == 0)
										 //				player_list[type_num].draw_external_wep = FALSE;


			fscanf_s(fp, "%s", &p, 256);

			//char junk[255];
			//sprintf(junk, "..\\bin\\%s", p);

			if (fopen_s(&fp2, p, "r") != 0)
				//			if (fopen_s(&fp2, junk, "r") != 0)
			{
				//PrintMessage(hwnd, "ERROR can't load ", filename, SCN_AND_FILE);
				//MessageBox(hwnd, filename, "Error can't load file", MB_OK);
				return FALSE;
			}

			//load .snd file

			fscanf_s(fp2, "%s", &p, 256);
			slist[slistcount].attack = SoundID(p);
			fscanf_s(fp2, "%s", &p, 256);
			slist[slistcount].die = SoundID(p);
			fscanf_s(fp2, "%s", &p, 256);
			slist[slistcount].weapon = SoundID(p);
			fscanf_s(fp2, "%s", &p, 256);
			slist[slistcount].yell = SoundID(p);
			player_count++;
			slist[slistcount].id = model_id;

			fclose(fp2);

			//ac
			fscanf_s(fp, "%s", &p, 256);
			fscanf_s(fp, "%s", &p, 256);
			slist[slistcount].ac = atoi(p);
			//hd
			fscanf_s(fp, "%s", &p, 256);
			fscanf_s(fp, "%s", &p, 256);
			slist[slistcount].hd = atoi(p);
			//damage
			fscanf_s(fp, "%s", &p, 256);
			fscanf_s(fp, "%s", &p, 256);
			strcpy_s(slist[slistcount].damage, p);
			//thaco
			fscanf_s(fp, "%s", &p, 256);
			fscanf_s(fp, "%s", &p, 256);
			slist[slistcount].thaco = atoi(p);
			//name
			fscanf_s(fp, "%s", &p, 256); //
			strcpy_s(slist[slistcount].name, p);
			strcpy_s(model_list[countmodellist].name, p);
			//speed
			fscanf_s(fp, "%s", &p, 256); //
			slist[slistcount].speed = (float)atof(p);

			fscanf_s(fp, "%s", &p, 256); //
			strcpy_s(model_list[countmodellist].monsterweapon, p);

			slistcount++;
			model_list[countmodellist].model_id = model_id;
			model_list[countmodellist].mtype = mtype;

			if (model_tex_alias == -1)
				model_list[countmodellist].modeltexture = model_tex_alias;
			else if (model_tex_alias == -2)
				model_list[countmodellist].modeltexture = model_tex_alias;
			else
				model_list[countmodellist].modeltexture = model_tex_alias - 1;
			countmodellist++;

			LoadPlayerAnimationSequenceList(model_id);

			command_recognised = TRUE;
		}

		if (strcmp(command, "YOURGUN") == 0)
		{
			fscanf_s(fp, "%s", &p, 256); // ignore comment pos

			fscanf_s(fp, "%s", &p, 256); // x pos
			your_gun[type_num].x_offset = (float)atoi(p);

			fscanf_s(fp, "%s", &p, 256); // y pos
			your_gun[type_num].z_offset = (float)atoi(p);

			fscanf_s(fp, "%s", &p, 256); // z pos
			your_gun[type_num].y_offset = (float)atoi(p);

			fscanf_s(fp, "%s", &p, 256); // ignore comment angle
			fscanf_s(fp, "%s", &p, 256); // angle
			your_gun[type_num].rot_angle = (float)atoi(p);

			your_gun[type_num].model_id = model_id;
			your_gun[type_num].skin_tex_id = model_tex_alias - 1;
			num_your_guns++;

			fscanf_s(fp, "%s", &p, 256); // ignore comment scale
			fscanf_s(fp, "%f", &f);
			scale = f;

			fscanf_s(fp, "%s", &p, 256); // ignore comment scale
			fscanf_s(fp, "%s", &p, 256); // ignore comment scale

			char d[80];
			char build[80];
			char build2[80];
			strcpy_s(d, p);
			int i = 0;
			int flag = 0, count = 0, count2 = 0;
			for (i = 0; i < (int)strlen(d); i++)
			{

				if (d[i] == 'd')
				{
					flag = 1;
				}
				else
				{

					if (flag == 0)
					{
						build[count++] = d[i];
					}
					else
					{
						build2[count2++] = d[i];
					}
				}
			}

			build[count] = '\0';
			build2[count2] = '\0';

			your_gun[type_num].damage1 = atoi(build);
			your_gun[type_num].damage2 = atoi(build2);

			fscanf_s(fp, "%s", &p, 256); // ignore comment scale
			fscanf_s(fp, "%s", &p, 256); // ignore comment scale

			your_gun[type_num].sattack = atoi(p);
			;

			fscanf_s(fp, "%s", &p, 256); // ignore comment scale
			fscanf_s(fp, "%s", &p, 256); // ignore comment scale

			your_gun[type_num].sdamage = atoi(p);
			;

			fscanf_s(fp, "%s", &p, 256); // ignore comment scale
			strcpy_s(your_gun[type_num].gunname, p);

			fscanf_s(fp, "%s", &p, 256); // ignore comment scale
			your_gun[type_num].guntype = atoi(p);

			your_gun_count++;
			//LoadYourGunAnimationSequenceList(model_id);
			command_recognised = TRUE;
		}

		if (strcmp(command, "END_FILE") == 0)
		{
			done = 1;
			command_recognised = TRUE;
		}

		if (command_recognised == TRUE)
		{
			if (Model_loaded_flags[model_id] == FALSE)
			{
				//PrintMessage(hwnd, "loading  ", model_filename, LOGFILE_ONLY);

				if (model_format == MD2_MODEL)
					ImportMD2_GLCMD(model_filename, model_tex_alias, model_id, scale);

				if (model_format == k3DS_MODEL)
					Import3DS(model_filename, model_id, scale);

				Model_loaded_flags[model_id] = TRUE;
				Q2M_Anim_Counter++;
			}
		}
		else
		{
			//PrintMessage(hwnd, "command unrecognised ", command, SCN_AND_FILE);
			//MessageBox(hwnd, command, "command unrecognised", MB_OK);
			return FALSE;
		}

	} // end while loop

	num_imported_models_loaded = player_count + your_gun_count + op_gun_count;
	//PrintMessage(hwnd, "\n", NULL, LOGFILE_ONLY);
	fclose(fp);
	return TRUE;
}

int CLoadWorld::FindModelID(char* p)
{

	int i = 0;

	for (i = 0; i < countmodellist; i++)
	{

		if (strcmp(model_list[i].name, p) == 0)
		{
			return model_list[i].model_id;
		}
	}

	for (i = 0; i < num_your_guns; i++)
	{

		if (strcmp(your_gun[i].gunname, p) == 0)
		{
			return your_gun[i].model_id;
		}
	}

	return 0;
}

void CLoadWorld::AddMonster(float x, float y, float z, float rot_angle, float monsterid, float monstertexture, float monnum, int s1, int s2, int s3, int s4, int s5, int s6, char damage[80], int thaco, char name[80], float speed, int ability)
{

	//if (monsterenable == 0)
		//return;

	char build[80];
	int count = 0;
	char build2[80];
	int count2 = 0;
	int flag = 0;

	monster_list[num_monsters].bIsPlayerValid = TRUE;
	monster_list[num_monsters].bIsPlayerAlive = TRUE;
	monster_list[num_monsters].x = x;
	monster_list[num_monsters].y = y;
	monster_list[num_monsters].z = z;
	monster_list[num_monsters].rot_angle = rot_angle;
	monster_list[num_monsters].model_id = (int)monsterid;
	monster_list[num_monsters].skin_tex_id = (int)monstertexture;

	monster_list[num_monsters].current_sequence = (int)1;

	int gd = random_num(5) + 1;


	monster_list[num_monsters].current_frame = (int)40 + gd;
	monster_list[num_monsters].animationdir = 0;

	if (gd == 1 || gd == 2)
		monster_list[num_monsters].animationdir = 1;

	strcpy_s(monster_list[num_monsters].rname, name);
	monster_list[num_monsters].speed = speed;
	monster_list[num_monsters].sattack = DSound_Replicate_Sound(s1);
	monster_list[num_monsters].sdie = DSound_Replicate_Sound(s2);
	monster_list[num_monsters].sweapon = DSound_Replicate_Sound(s3);
	monster_list[num_monsters].syell = DSound_Replicate_Sound(s4);

	monster_list[num_monsters].ac = s5;
	monster_list[num_monsters].hd = s6;

	monster_list[num_monsters].attackspeed = 0;
	monster_list[num_monsters].firespeed = 0;

	monster_list[num_monsters].thaco = thaco;

	int raction = random_num(s6 * 8) + s6;

	monster_list[num_monsters].hp = raction;
	monster_list[num_monsters].health = raction;

	int i = 0;

	for (i = 0; i < (int)strlen(damage); i++)
	{

		if (damage[i] == 'd')
		{
			flag = 1;
		}
		else
		{

			if (flag == 0)
			{
				build[count++] = damage[i];
			}
			else
			{
				build2[count2++] = damage[i];
			}
		}
	}

	build[count] = '\0';
	build2[count2] = '\0';

	monster_list[num_monsters].damage1 = atoi(build);
	monster_list[num_monsters].damage2 = atoi(build2);

	strcpy_s(monster_list[num_monsters].chatstr, "5");

	if (monsterid == 0 || monsterid == 2)
	{
		monster_list[num_monsters].draw_external_wep = TRUE;
	}

	else
	{
		monster_list[num_monsters].draw_external_wep = FALSE;
	}

	monster_list[num_monsters].monsterid = (int)monnum;

	monster_list[num_monsters].ability = (int)ability;

	monster_list[num_monsters].bStopAnimating = FALSE;

	if (ability == 3)
	{
		SetMonsterAnimationSequence(num_monsters, 12);
		monster_list[num_monsters].bIsPlayerAlive = FALSE;
		monster_list[num_monsters].bIsPlayerValid = FALSE;
		monster_list[num_monsters].current_frame = 183;
		monster_list[num_monsters].bStopAnimating = TRUE;
	}

	if (ability == 1)
		monster_list[num_monsters].bStopAnimating = TRUE;

	//	monster_list[num_monsters].player_type=ability;

	strcpy_s(monster_list[num_monsters].rname, name);

	num_monsters++;
	//countplayers++;
}

void CLoadWorld::LoadPlayerAnimationSequenceList(int model_id)
{
	int i;

	i = model_id;

	pmdata[i].sequence_start_frame[0] = 0; // Stand
	pmdata[i].sequence_stop_frame[0] = 39;

	pmdata[i].sequence_start_frame[1] = 40; // run
	pmdata[i].sequence_stop_frame[1] = 45;

	pmdata[i].sequence_start_frame[2] = 46; // attack
	pmdata[i].sequence_stop_frame[2] = 53;

	pmdata[i].sequence_start_frame[3] = 54; // pain1
	pmdata[i].sequence_stop_frame[3] = 57;

	pmdata[i].sequence_start_frame[4] = 58; // pain2
	pmdata[i].sequence_stop_frame[4] = 61;

	pmdata[i].sequence_start_frame[5] = 62; // pain3
	pmdata[i].sequence_stop_frame[5] = 65;

	pmdata[i].sequence_start_frame[6] = 66; // jump
	pmdata[i].sequence_stop_frame[6] = 71;

	pmdata[i].sequence_start_frame[7] = 72; // flip
	pmdata[i].sequence_stop_frame[7] = 83;

	pmdata[i].sequence_start_frame[8] = 84; // Salute
	pmdata[i].sequence_stop_frame[8] = 94;

	pmdata[i].sequence_start_frame[9] = 95; // Taunt
	pmdata[i].sequence_stop_frame[9] = 111;

	pmdata[i].sequence_start_frame[10] = 112; // Wave
	pmdata[i].sequence_stop_frame[10] = 122;

	pmdata[i].sequence_start_frame[11] = 123; // Point
	pmdata[i].sequence_stop_frame[11] = 134;

	pmdata[i].sequence_start_frame[12] = 178; // death
	pmdata[i].sequence_stop_frame[12] = 183;

	pmdata[i].sequence_start_frame[13] = 184; // death
	pmdata[i].sequence_stop_frame[13] = 189;

	pmdata[i].sequence_start_frame[14] = 190; // death
	pmdata[i].sequence_stop_frame[14] = 197;

	//178-183
	//184-189
	//190-197
}

void SetStartSpot()
{

	int result;

	//m_pd3dDevice->SetRenderState(D3DRENDERSTATE_AMBIENT, NULL);

	/*
	if (strcmp(ds_reg->registered,"0") ==0 && current_level>=3 && multiplay_flag==FALSE) {
		DSPostQuit();
		Pause(TRUE);
		UINT resultclick = MessageBox(main_window_handle,"Please register at http://www.murk.on.ca","Register Game",MB_OK);
		Pause(FALSE);
		PostQuitMessage(0);
	}
	*/

	if (startposcounter == 0)
		return;

	result = random_num(startposcounter);

	m_vEyePt.x = startpos[result].x;
	m_vEyePt.y = startpos[result].y;
	m_vEyePt.z = startpos[result].z;


	//m_vLookatPt = m_vEyePt;
	//cameraf.x = m_vLookatPt.x;
	//cameraf.y = m_vLookatPt.y;
	//cameraf.z = m_vLookatPt.z;
	angy = startpos[result].angle;

	//modellocation = m_vEyePt;
	//UpdateMainPlayer();

}

BOOL CLoadWorld::LoadMod(char* filename)
{
	FILE* fp;
	char s[256];
	//	char p[256];
	int counter = 0;
	int done = 0;
	char bigbuf[2048];

	char path[255];
	//sprintf(path, "..\\bin\\%s", filename);
	sprintf_s(path, "%s", filename);

	if (fopen_s(&fp, path, "r") != 0)
	{
		//PrintMessage(hwnd, "Error can't load file ", filename, SCN_AND_FILE);
		//MessageBox(hwnd, "Error can't load file", NULL, MB_OK);
		return FALSE;
	}

	//PrintMessage(hwnd, "Loading map ", filename, SCN_AND_FILE);

	while (done == 0)
	{

		fscanf_s(fp, "%s", &s, 256);
		//objectid

		if (strcmp(s, "END_FILE") == 0)
		{
			done = 1;
		}
		else
		{
			//num
			levelmodify[counter].num = atoi(s);
			//objectid
			fscanf_s(fp, "%s", &s, 256);
			levelmodify[counter].objectid = atoi(s);
			//active
			fscanf_s(fp, "%s", &s, 256);
			levelmodify[counter].active = atoi(s);
			//Function
			fscanf_s(fp, "%s", &s, 256);
			strcpy_s(levelmodify[counter].Function, s);
			//jump
			fscanf_s(fp, "%s", &s, 256);
			levelmodify[counter].jump = atoi(s);
			//text1
			fgets(bigbuf, 2048, fp);
			strcpy_s(levelmodify[counter].Text1, bigbuf);
			//text2
			strcpy_s(levelmodify[counter].Text2, "");
			levelmodify[counter].currentheight = -9999;
		}

		counter++;
	}
	fclose(fp);
	totalmod = counter - 1;
	return TRUE;
}

int save_game(char* filename)
{
	sprintf_s(gActionMessage, "Saving Game...");
	UpdateScrollList(0, 255, 0);

	int perspectiveview = 1;

	FILE* fp;
	int montry;
	//Pause(TRUE);
	//UINT resultclick = MessageBox(main_window_handle, "Save Game?", "Save Game", MB_ICONQUESTION | MB_YESNO);
	//Pause(FALSE);
	//if (resultclick == IDYES)
	//{

	if (strlen(filename) > 0)
	{
		if (fopen_s(&fp, filename, "wb+") != 0)
		{
			return 0;
		}
	}
	else
	{

		if (fopen_s(&fp, "ds.sav", "wb+") != 0)
		{
			return 0;
		}
	}

	fwrite(levelname, sizeof(char), 50, fp);
	fwrite(&current_level, sizeof(int), 1, fp);
	fwrite(&angy, sizeof(int), 1, fp);
	fwrite(&look_up_ang, sizeof(int), 1, fp);
	fwrite(&perspectiveview, sizeof(int), 1, fp);
	fwrite(&current_gun, sizeof(int), 1, fp);

	fwrite(&player_list[trueplayernum], sizeof(struct player_typ), 1, fp);

	fwrite(&num_players2, sizeof(int), 1, fp);
	for (montry = 0; montry < num_players2; montry++)
	{
		fwrite(&player_list2[montry], sizeof(struct player_typ), 1, fp);
	}

	fwrite(&num_monsters, sizeof(int), 1, fp);
	for (montry = 0; montry < num_monsters; montry++)
	{
		fwrite(&monster_list[montry], sizeof(struct player_typ), 1, fp);
	}
	fwrite(&itemlistcount, sizeof(int), 1, fp);
	for (montry = 0; montry < itemlistcount; montry++)
	{
		fwrite(&item_list[montry], sizeof(struct player_typ), 1, fp);
	}
	fwrite(&doorcounter, sizeof(int), 1, fp);
	for (montry = 0; montry < doorcounter; montry++)
	{
		fwrite(&door[montry], sizeof(struct doors), 1, fp);
	}

	fwrite(&num_your_guns, sizeof(int), 1, fp);
	for (montry = 0; montry < num_your_guns; montry++)
	{
		fwrite(&your_gun[montry], sizeof(struct gunlist_typ), 1, fp);
	}

	fwrite(&countswitches, sizeof(int), 1, fp);
	for (montry = 0; montry < countswitches; montry++)
	{
		fwrite(&switchmodify[montry], sizeof(struct SwitchMod), 1, fp);
	}
	fwrite(&totalmod, sizeof(int), 1, fp);
	for (montry = 0; montry < totalmod; montry++)
	{
		fwrite(&levelmodify[montry], sizeof(struct LevelMod), 1, fp);
	}
	fwrite(&textcounter, sizeof(int), 1, fp);
	for (montry = 0; montry < textcounter; montry++)
	{
		fwrite(&gtext[montry], sizeof(struct gametext), 1, fp);
	}

	fclose(fp);
	return 1;
	//}

	//return 0;
}

int load_game(char* filename)
{

	sprintf_s(gActionMessage, "Loading Game...");
	UpdateScrollList(0, 255, 0);



	int perspectiveview = 1;
	FILE* fp;
	int montry;
	//Pause(TRUE);
	//UINT resultclick = MessageBox(main_window_handle, "Load Game?", "Load Game", MB_ICONQUESTION | MB_YESNO);
	//Pause(FALSE);
	//if (resultclick == IDYES)
	//{

	if (strlen(filename) > 0)
	{

		if (fopen_s(&fp, filename, "rb") != 0)
		{
			return 0;
		}
	}
	else
	{

		if (fopen_s(&fp, "ds.sav", "rb") != 0)
		{
			return 0;
		}
	}

	fread(levelname, sizeof(char), 50, fp);
	fread(&current_level, sizeof(int), 1, fp);
	fread(&angy, sizeof(int), 1, fp);
	fread(&look_up_ang, sizeof(int), 1, fp);
	fread(&perspectiveview, sizeof(int), 1, fp);
	fread(&current_gun, sizeof(int), 1, fp);
	load_level(levelname);
	fread(&player_list[trueplayernum], sizeof(struct player_typ), 1, fp);

	//player_list[trueplayernum].y += 100.0f;
	m_vEyePt.x = player_list[trueplayernum].x;
	m_vEyePt.y = player_list[trueplayernum].y;
	m_vEyePt.z = player_list[trueplayernum].z;

	m_vLookatPt.x = player_list[trueplayernum].x;
	m_vLookatPt.y = player_list[trueplayernum].y + 10.0f;
	m_vLookatPt.z = player_list[trueplayernum].z;

	fread(&num_players2, sizeof(int), 1, fp);

	for (montry = 0; montry < num_players2; montry++)
	{
		fread(&player_list2[montry], sizeof(struct player_typ), 1, fp);
	}

	fread(&num_monsters, sizeof(int), 1, fp);

	for (montry = 0; montry < num_monsters; montry++)
	{
		fread(&monster_list[montry], sizeof(struct player_typ), 1, fp);
	}

	fread(&itemlistcount, sizeof(int), 1, fp);

	for (montry = 0; montry < itemlistcount; montry++)
	{
		fread(&item_list[montry], sizeof(struct player_typ), 1, fp);
	}

	fread(&doorcounter, sizeof(int), 1, fp);

	for (montry = 0; montry < doorcounter; montry++)
	{
		fread(&door[montry], sizeof(struct doors), 1, fp);
		oblist[door[montry].doornum].rot_angle = (float)door[montry].angle;

		if (door[montry].y != 0)
			oblist[door[montry].doornum].y = (float)door[montry].y;
	}

	fread(&num_your_guns, sizeof(int), 1, fp);
	for (montry = 0; montry < num_your_guns; montry++)
	{
		fread(&your_gun[montry], sizeof(struct gunlist_typ), 1, fp);
	}

	//SwitchGun(current_gun);
	player_list[trueplayernum].gunid = your_gun[current_gun].model_id;
	player_list[trueplayernum].guntex = your_gun[current_gun].skin_tex_id;
	player_list[trueplayernum].damage1 = your_gun[current_gun].damage1;
	player_list[trueplayernum].damage2 = your_gun[current_gun].damage2;

	if (strstr(your_gun[current_gun].gunname, "SCROLL") != NULL)
	{
		usespell = 1;
	}
	else
	{
		usespell = 0;
	}

	

	fread(&countswitches, sizeof(int), 1, fp);
	for (montry = 0; montry < countswitches; montry++)
	{
		fread(&switchmodify[montry], sizeof(struct SwitchMod), 1, fp);
	}
	fread(&totalmod, sizeof(int), 1, fp);
	for (montry = 0; montry < totalmod; montry++)
	{
		fread(&levelmodify[montry], sizeof(struct LevelMod), 1, fp);
	}
	fread(&textcounter, sizeof(int), 1, fp);
	for (montry = 0; montry < textcounter; montry++)
	{
		fread(&gtext[montry], sizeof(struct gametext), 1, fp);
	}

	fclose(fp);

	MakeDamageDice();

	SetPlayerAnimationSequence(trueplayernum, 0);

	return 1;
	//}
	//return 0;
}

void ClearObjectList();

int load_level(char* filename)
{

	char level[255];

	/*
	if (strcmp(pCMyApp->ds_reg->registered,"0") ==0 && pCMyApp->current_level>=3) {

		Pause(TRUE);
		UINT resultclick = MessageBox(main_window_handle,"Please register at http://www.murk.on.ca","Register Game",MB_OK);
		Pause(FALSE);
		PostQuitMessage(0);
	}
	*/

	if (strlen(filename) > 0)
	{
		strcpy_s(level, filename);
		strcat_s(level, ".map");
		strcpy_s(levelname, filename);
	}
	else
	{
		sprintf_s(levelname, "level%d", current_level);
		strcpy_s(level, levelname);
		strcat_s(level, ".map");
	}

	if (!pCWorld->LoadWorldMap(level))
	{
	}

	num_players2 = 0;
	itemlistcount = 0;
	num_monsters = 0;
	

	ClearObjectList();
	ResetSound();
	//pCWorld->LoadSoundFiles(m_hWnd, "sounds.dat");

	if (!pCWorld->LoadWorldMap(level))
	{
		//PrintMessage(m_hWnd, "LoadWorldMap failed", NULL, LOGFILE_ONLY);
		//return FALSE;
	}

	strcpy_s(level, levelname);
	strcat_s(level, ".cmp");

	//if (!pCWorld->InitPreCompiledWorldMap(m_hWnd, level))
	//{
	//	PrintMessage(m_hWnd, "InitPreCompiledWorldMap failed", NULL, LOGFILE_ONLY);
	//	return FALSE;
	//}

	for (int i = 0; i < MAX_NUM_GUNS; i++)
	{

		if (i == 0)
		{
			your_gun[i].active = 1;
			your_gun[i].x_offset = 0;
		}
		else if (i == 18)
		{
			your_gun[i].active = 1;
			your_gun[i].x_offset = 2;
		}
		else
		{
			your_gun[i].active = 0;

			your_gun[i].x_offset = 0;
		}
	}


	player_list[trueplayernum].gunid = your_gun[current_gun].model_id;
	player_list[trueplayernum].guntex = your_gun[current_gun].skin_tex_id;
	player_list[trueplayernum].damage1 = your_gun[current_gun].damage1;
	player_list[trueplayernum].damage2 = your_gun[current_gun].damage2;

	//MakeDamageDice();

	for (int i = 0; i < MAX_MISSLE; i++)
	{

		//if (your_missle[i].sexplode != 0)
			//DSound_Delete_Sound(your_missle[i].sexplode);

		//if (your_missle[i].smove != 0)
			//DSound_Delete_Sound(your_missle[i].smove);

		your_missle[i].model_id = 10;
		your_missle[i].skin_tex_id = 137;
		your_missle[i].current_frame = 0;
		your_missle[i].current_sequence = 0;
		your_missle[i].active = 0;
		your_missle[i].sexplode = 0;
		your_missle[i].smove = 0;
		your_missle[i].qdist = 0.0f;
		your_missle[i].x_offset = 0;
		your_missle[i].y_offset = 0;
		your_missle[i].z_offset = 0;
	}

	strcpy_s(level, levelname);
	strcat_s(level, ".mod");

	pCWorld->LoadMod(level);



	return 1;
}

