
#include "resource.h"
#include "d3dtypes.h"
#include "LoadWorld.hpp"
#include "world.hpp"
#include "GlobalSettings.hpp"
#include <string.h>
#include "DirectInput.hpp"
#include "GameLogic.hpp"
#include "Missle.hpp"



//TODO: fix
float wWidth = 1;
float wHeight = 1;
int num_players2 = 0;
POLY_SORT ObjectsToDraw[200000];

PLAYER* monster_list;
OBJECTLIST* oblist;

LONGLONG DSTimer();
double time_factor = 0;
LONGLONG gametimer2 = 0;
LONGLONG gametimerlast2 = 0;
int maingameloop2 = 0;

LONGLONG gametimer = 0;
LONGLONG gametimerlast = 0;
int maingameloop = 0;

LONGLONG gametimer3 = 0;
LONGLONG gametimerlast3 = 0;
int maingameloop3 = 0;

FLOAT elapsegametimersave = 0;

extern FLOAT fTimeKeysave;
XMFLOAT3 savevelocity;
XMFLOAT3 saveoldvelocity;

float playerspeed = 300.0f;

D3DVECTOR realEye;
XMFLOAT3 EyeTrue;
D3DMATRIX matView;

//Move
int direction = 0;
int directionlast = 0;
int savelastmove = 0;
int savelaststrifemove = 0;
float playerspeedmax = 280.0f;
float playerspeedlevel = 280.0f;
float movespeed = 0.0f;
float movespeedold = 0.0f;
float movespeedsave = 0.0f;
float movetime = 0.0f;
float moveaccel = 400.0f;
int playermove = 0;
int playermovestrife = 0;
float cameraheight = 50.0f;
float currentspeed = 0;

char gfinaltext[2048];
extern int textcounter;
gametext gtext[200];

XMFLOAT3 LookTrue;

XMFLOAT3 m_vLookatPt;
XMFLOAT3 m_vEyePt;
D3DVALUE look_up_ang;
D3DVALUE angy = 60;

XMFLOAT3 modellocation;
CAMERAFLOAT cameraf;
extern int src_collide[MAX_NUM_VERTICES];

extern LEVELMOD* levelmodify;
extern SWITCHMOD* switchmodify;
float fDot2last = 0;

void PlaySong();
void DrawMissles();
void DrawMissle();
void ApplyMissleDamage(int playernum);

extern int jumpvdir;
extern int jumpstart;
extern float jumpcount;
extern D3DVECTOR jumpv;
extern float lastjumptime;
extern float cleanjumpspeed;
float totaldist = 0;
D3DVECTOR gravityvector;
extern int lastcollide;
float gravitytime = 0;
D3DVECTOR gravityvectorold;
extern int foundcollisiontrue;
extern int nojumpallow;
int gravitydropcount = 0;
extern int jump;


void InitDS()
{
	HRESULT hr;

	verts_per_poly = new int[MAX_NUM_QUADS];
	src_v = new D3DVERTEX[MAX_NUM_QUADS];
	dp_commands = new D3DPRIMITIVETYPE[MAX_NUM_QUADS];
	dp_command_index_mode = new BOOL[MAX_NUM_QUADS];
	num_vert_per_object = new int[MAX_NUM_QUADS];
	faces_per_poly = new int[MAX_NUM_QUADS];
	src_f = new int[MAX_NUM_FACE_INDICES];
	texture_list_buffer = new int[MAX_NUM_QUADS];
	pmdata = new PLAYERMODELDATA[MAX_NUM_PMMODELS];
	your_gun = new GUNLIST[MAX_NUM_GUNS];
	model_list = new MODELLIST[MAX_NUM_PLAYERS];
	monster_list = new PLAYER[MAX_NUM_MONSTERS];
	item_list = new PLAYER[MAX_NUM_ITEMS];
	player_list2 = new PLAYER[MAX_NUM_ITEMS];
	player_list = new PLAYER[1];
	your_missle = new GUNLIST[MAX_MISSLE];
	oblist = new OBJECTLIST[MAX_OBJECTLIST];


	levelmodify = new LEVELMOD[50];
	switchmodify = new SWITCHMOD[50];

	num_monsters = 0;
	countmodellist = 0;
	num_your_guns = 0;
	itemlistcount = 0;
	num_players2 = 0;

	m_vEyePt.x = 1500;
	m_vEyePt.y = 290;
	m_vEyePt.z = 1000;

	CLoadWorld* pCWorld;
	pCWorld = new CLoadWorld();

	for (int i = 0; i < MAX_NUM_VERTICES; i++)
	{
		//src_on[i] = 1;
		src_collide[i] = 1;
	}


	for (int i = 0; i < 1; i++)
	{
		player_list[i].frags = 0;
		player_list[i].x = 500;
		player_list[i].y = 22;
		player_list[i].z = 500;
		player_list[i].dist = 0;
		player_list[i].rot_angle = 0;
		player_list[i].model_id = 0;
		player_list[i].skin_tex_id = 0;
		player_list[i].bIsFiring = FALSE;
		player_list[i].ping = 0;
		player_list[i].health = 20;
		player_list[i].rings = 0;
		player_list[i].keys = 0;
		player_list[i].bIsPlayerAlive = TRUE;
		player_list[i].bStopAnimating = FALSE;
		player_list[i].current_weapon = 0;
		player_list[i].current_car = 0;
		player_list[i].current_frame = 0;
		player_list[i].current_sequence = 0;
		player_list[i].bIsPlayerInWalkMode = TRUE;
		player_list[i].RRnetID = 0;
		player_list[i].bIsPlayerValid = TRUE;
		player_list[i].animationdir = 0;
		player_list[i].gunid = 12;
		player_list[i].guntex = 11;
		player_list[i].volume = 0;
		player_list[i].gold = 0;
		player_list[i].sattack = 0;
		player_list[i].sdie = 0;
		player_list[i].sweapon = 0;
		player_list[i].syell = 0;
		player_list[i].ability = 0;

		strcpy_s(player_list[i].name, "");
		strcpy_s(player_list[i].chatstr, "5");
		strcpy_s(player_list[i].name, "Dungeon Stomp");
		strcpy_s(player_list[i].rname, "Crom");

		player_list[i].ac = 7;
		player_list[i].hd = 1;
		player_list[i].hp = 20;
		player_list[i].damage1 = 1;
		player_list[i].damage2 = 4;
		player_list[i].thaco = 19;
		player_list[i].xp = 0;
		player_list[i].firespeed = 0;
		player_list[i].attackspeed = 0;
		player_list[i].applydamageonce = 0;
		player_list[i].takedamageonce = 0;
		//		player_list[i].gunid=FindModelID("AXE");
		//		player_list[i].guntex=FindGunTexture("AXE");
	}

	for (int i = 0; i < MAX_NUM_3DS; i++)
	{
		player_list2[i].frags = 0;
		player_list2[i].dist = 500;
		player_list2[i].x = 500;
		player_list2[i].y = 22;
		player_list2[i].z = 500;
		player_list2[i].rot_angle = 0;
		player_list2[i].model_id = 0;
		player_list2[i].skin_tex_id = 0;
		player_list2[i].bIsFiring = FALSE;
		player_list2[i].ping = 0;
		player_list2[i].health = 920;
		player_list2[i].rings = 0;
		player_list2[i].keys = 0;
		player_list2[i].bIsPlayerAlive = TRUE;
		player_list2[i].bStopAnimating = FALSE;
		player_list2[i].current_weapon = 0;
		player_list2[i].current_car = 0;
		player_list2[i].current_frame = 0;
		player_list2[i].current_sequence = 0;
		player_list2[i].bIsPlayerInWalkMode = TRUE;
		player_list2[i].RRnetID = 0;
		player_list2[i].bIsPlayerValid = FALSE;
		player_list2[i].animationdir = 0;
		player_list2[i].gunid = 12;
		player_list2[i].guntex = 11;
		player_list2[i].volume = 0;
		player_list2[i].gold = 0;
		player_list2[i].sattack = 0;
		player_list2[i].sdie = 0;
		player_list2[i].sweapon = 0;
		player_list2[i].syell = 0;
		player_list2[i].ability = 0;

		strcpy_s(player_list2[i].name, "");
		strcpy_s(player_list2[i].chatstr, "5");
		strcpy_s(player_list2[i].name, "Dungeon Stomp");

		player_list2[i].ac = 7;
		player_list2[i].hd = 1;
		player_list2[i].hp = 920;
		player_list2[i].damage1 = 1;
		player_list2[i].damage2 = 4;
		player_list2[i].thaco = 19;
		player_list2[i].xp = 0;
		player_list2[i].firespeed = 0;
		player_list2[i].attackspeed = 0;
		player_list2[i].applydamageonce = 0;
		player_list2[i].takedamageonce = 0;
	}

	for (int i = 0; i < MAX_NUM_MONSTERS; i++)
	{
		monster_list[i].frags = 0;
		monster_list[i].x = 500;
		monster_list[i].y = 22;
		monster_list[i].z = 500;
		monster_list[i].dist = 500;
		monster_list[i].rot_angle = 0;
		monster_list[i].model_id = 0;
		monster_list[i].skin_tex_id = 0;
		monster_list[i].bIsFiring = FALSE;
		monster_list[i].ping = 0;
		monster_list[i].health = 5;
		monster_list[i].bIsPlayerAlive = TRUE;
		monster_list[i].bStopAnimating = FALSE;
		monster_list[i].current_weapon = 0;
		monster_list[i].current_car = 0;
		monster_list[i].current_frame = 0;
		monster_list[i].current_sequence = 2;
		monster_list[i].bIsPlayerInWalkMode = TRUE;
		monster_list[i].RRnetID = 0;
		monster_list[i].bIsPlayerValid = FALSE;
		monster_list[i].animationdir = 0;
		monster_list[i].volume = 0;
		monster_list[i].gold = 0;
		monster_list[i].rings = 0;
		monster_list[i].keys = 0;
		monster_list[i].ability = 0;
		monster_list[i].sattack = 0;
		monster_list[i].sdie = 0;
		monster_list[i].sweapon = 0;
		monster_list[i].syell = 0;

		strcpy_s(monster_list[i].name, "");
		strcpy_s(monster_list[i].chatstr, "5");
		strcpy_s(monster_list[i].name, "Dungeon Stomp");

		monster_list[i].ac = 7;
		monster_list[i].hd = 1;
		monster_list[i].hp = 8;
		monster_list[i].damage1 = 1;
		monster_list[i].damage2 = 8;
		monster_list[i].thaco = 19;
		monster_list[i].xp = 0;
		monster_list[i].firespeed = 0;
		monster_list[i].attackspeed = 0;
		monster_list[i].applydamageonce = 0;
		monster_list[i].takedamageonce = 0;
		monster_list[i].closest = trueplayernum;
	}
	for (int i = 0; i < MAX_NUM_ITEMS; i++)
	{
		item_list[i].frags = 0;
		item_list[i].x = 500;
		item_list[i].y = 22;
		item_list[i].z = 500;
		item_list[i].dist = 500;
		item_list[i].rot_angle = 0;
		item_list[i].model_id = 0;
		item_list[i].skin_tex_id = 0;
		item_list[i].bIsFiring = FALSE;
		item_list[i].ping = 0;
		item_list[i].health = 5;
		item_list[i].bIsPlayerAlive = TRUE;
		item_list[i].bStopAnimating = FALSE;
		item_list[i].current_weapon = 0;
		item_list[i].current_car = 0;
		item_list[i].current_frame = 0;
		item_list[i].current_sequence = 2;
		item_list[i].bIsPlayerInWalkMode = TRUE;
		item_list[i].RRnetID = 0;
		item_list[i].bIsPlayerValid = FALSE;
		item_list[i].animationdir = 0;
		item_list[i].gold = 0;
		item_list[i].ability = 0;
		item_list[i].firespeed = 0;
		strcpy_s(item_list[i].name, "");
		strcpy_s(item_list[i].chatstr, "5");
		strcpy_s(item_list[i].name, "Dungeon Stomp");
	}

	for (int i = 0; i < MAX_NUM_GUNS; i++)
	{
		your_gun[i].model_id = 0;
		your_gun[i].current_frame = 0;
		your_gun[i].current_sequence = 0;

		if (i <= 0)
			your_gun[i].active = 1;
		else
			your_gun[i].active = 0;

		//		your_gun[i].active = 1;
		your_gun[i].damage1 = 1;
		your_gun[i].damage2 = 4;
	}

	for (int i = 0; i < MAX_MISSLE; i++)
	{
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


	//for (int i = 0; i < MAX_NUM_GUNS; i++)
	//{

	//	if (i == 0)
	//	{
	//		your_gun[i].active = 1;
	//		your_gun[i].x_offset = 0;
	//	}
	//	else if (i == 18)
	//	{
	//		your_gun[i].active = 1;
	//		your_gun[i].x_offset = 2;
	//	}
	//	else
	//	{
	//		your_gun[i].active = 0;

	//		your_gun[i].x_offset = 0;
	//	}
	//}


	player_list[trueplayernum].gunid = your_gun[current_gun].model_id;
	player_list[trueplayernum].guntex = your_gun[current_gun].skin_tex_id;
	player_list[trueplayernum].damage1 = your_gun[current_gun].damage1;
	player_list[trueplayernum].damage2 = your_gun[current_gun].damage2;




	if (!pCWorld->LoadImportedModelList("modellist.dat"))
	{
		//PrintMessage(m_hWnd, "LoadImportedModelList failed", NULL, LOGFILE_ONLY);
		//return FALSE;
	}

	if (!pCWorld->LoadObjectData("objects.dat"))
	{
		//PrintMessage(m_hWnd, "LoadObjectData failed", NULL, LOGFILE_ONLY);
		//return FALSE;
	}

	if (!pCWorld->LoadWorldMap("level1.map"))
	{
		//PrintMessage(m_hWnd, "LoadObjectData failed", NULL, LOGFILE_ONLY);
		//return FALSE;
	}


	pCWorld->LoadMod("level1.mod");


	SetStartSpot();

	//float k = (float)0.017453292;
	float fangle = (float)90 * k;

	for (int i = 0; i <= 360; i++)
	{
		float fangle = (float)i * k;
		sin_table[i] = (float)sin(fangle);
		cos_table[i] = (float)cos(fangle);
	}


	player_list[trueplayernum].gunid = FindModelID("AXE");
	player_list[trueplayernum].guntex = FindGunTexture("AXE");

	//GiveWeapons();
	initDSTimer();

	MakeDice();

	strcpy_s(gActionMessage, "Dungeon Stomp 1.90. Copyright 2021 Aptisense.");
	UpdateScrollList(0, 245, 255);
	strcpy_s(gActionMessage, "Press SPACE to open things. Press E to jump.");
	UpdateScrollList(0, 245, 255);

	strcpy_s(gActionMessage, "WASD to move. Press Q and Z to cycle weapons.");
	UpdateScrollList(0, 245, 255);

	strcpy_s(gActionMessage, "Give K=Weapons X=Experience P=Music G=Gravity (use + -).");
	UpdateScrollList(0, 245, 255);

	strcpy_s(gActionMessage, "F5=Load F6=Save. Created by Mark Longo. Good luck!");
	UpdateScrollList(0, 245, 255);

}


void UpdateWorld(float fElapsedTime) {

	number_of_polys_per_frame = 0;
	num_triangles_in_scene = 0;
	num_verts_in_scene = 0;
	num_dp_commands_in_scene = 0;
	cnt = 0;
	cnt_f = 0;
	tempvcounter = 0;
	num_light_sources = 0;
	g_ob_vert_count = 0;

	int gun_angle;
	gun_angle = -(int)angy + (int)90;

	if (gun_angle >= 360)
		gun_angle = gun_angle - 360;
	if (gun_angle < 0)
		gun_angle = gun_angle + 360;

	player_list[trueplayernum].gunangle = gun_angle;


	ActivateSwitch();

	if (maingameloop) {
		CheckMidiMusic();
	}

	for (int q = 0; q < oblist_length; q++)
	{
		int angle = (int)oblist[q].rot_angle;
		int ob_type = oblist[q].type;

		//120 text   //6 lamp post
		//35  monster //57 flamesnothit
		//131 startpos /58 torch

		//120 = text

		//if (ob_type != 120 && ob_type != 35 && ob_type != 131 && ob_type != 6)
		if (ob_type != 35 && ob_type != 131 && ob_type != 6 && ob_type != 120)
		{
			float	qdist = FastDistance(m_vEyePt.x - oblist[q].x, m_vEyePt.y - oblist[q].y, m_vEyePt.z - oblist[q].z);

			objectcollide = 1;

			if (strstr(oblist[q].name, "nohit") != NULL)
			{
				objectcollide = 0;
			}

			if (qdist < 1500)
			{
				ObjectToD3DVertList(ob_type, angle, q);
			}
		}
		//ObjectToD3DVertList(ob_type, angle, q, pd3dDevice);
	}

	FreeSlave();
	ApplyMissleDamage(1);

	DrawMissles();
	WakeUpMonsters();
	MoveMonsters(fElapsedTime);
	DrawMonsters();
	OpenChest();
	DrawItems();
	DrawModel();
	DrawPlayerGun();
	MakeBoundingBox();

	//TODO: fix this
	DirectX::XMFLOAT3 em = { 0.0f, 0.0f, 0.0f };

	FirePlayerMissle(player_list[trueplayernum].x, player_list[trueplayernum].y,
		player_list[trueplayernum].z,
		angy, trueplayernum, 0, em, look_up_ang, fElapsedTime);



	DrawMissle();

	int lsort = 0;
	for (lsort = 0; lsort < number_of_polys_per_frame; lsort++)
	{
		int i = ObjectsToDraw[lsort].vert_index;
		int vert_index = ObjectsToDraw[lsort].srcstart;
		int fperpoly = (float)ObjectsToDraw[lsort].srcfstart;
		int face_index = ObjectsToDraw[lsort].srcfstart;

		if (dp_command_index_mode[i] == 1) {  //USE_NON_INDEXED_DP


		}
		else {
			DrawIndexedItems(lsort, vert_index);
		}
	} // end for i

	num_light_sources = 0;
}

/*
void UpdateVertexBuffer(IDirect3DDevice9* pd3dDevice) {

	g_pVB->Lock(0, sizeof(&pVertices), (void**)&pVertices, 0);

	for (int j = 0; j < cnt; j++)
	{
		D3DXVECTOR3 a = D3DXVECTOR3(src_v[j].x, src_v[j].y, src_v[j].z);

		pVertices[j].position = a;
		pVertices[j].color = D3DCOLOR_RGBA(105, 105, 105, 0); //0xffffffff;
		pVertices[j].tu = src_v[j].tu;
		pVertices[j].tv = src_v[j].tv;
		pVertices[j].nx = src_v[j].nx;
		pVertices[j].ny = src_v[j].ny;
		pVertices[j].nz = src_v[j].nz;
	}

	g_pVB->Unlock();
}

*/


void SetMonsterAnimationSequence(int player_number, int sequence_number)
{
	int model_id;
	int start_frame;

	//turned on again for mulitplayer this is a problem getting a sequence out of order
	if (monster_list[player_number].bIsPlayerValid == FALSE)
		return;

	monster_list[player_number].current_sequence = sequence_number;

	model_id = monster_list[player_number].model_id;

	monster_list[player_number].animationdir = 0;
	start_frame = pmdata[model_id].sequence_start_frame[sequence_number];
	monster_list[player_number].current_frame = start_frame;

}

void SetPlayerAnimationSequence(int player_number, int sequence_number)
{
	int model_id;
	int start_frame;

	if (player_list[trueplayernum].bIsPlayerAlive == FALSE && player_number == trueplayernum)
		return;

	if (player_list[player_number].bStopAnimating == TRUE)
		return;

	player_list[player_number].current_sequence = sequence_number;
	model_id = player_list[player_number].model_id;
	player_list[player_number].animationdir = 0;

	start_frame = pmdata[model_id].sequence_start_frame[sequence_number];

	if (start_frame == 66)
	{
		start_frame = 70;
		player_list[player_number].animationdir = 1;
	}
	player_list[player_number].current_frame = start_frame;

}

HRESULT AnimateCharacters()
{
	int i;
	int startframe;
	int curr_frame;
	int stop_frame;
	int curr_seq;
	static LONGLONG fLastTimeRun = 0;
	static LONGLONG fLastTimeRunLast = 0;

	//TODO:
	int jump = 0;

	//Only take damge from one swing
	if (player_list[trueplayernum].current_frame == 52) {
		for (i = 0; i < num_monsters; i++)
		{
			monster_list[i].takedamageonce = 0;
		}
	}

	for (int i = 0; i < 1; i++)
	{

		int mod_id = player_list[i].model_id;
		curr_frame = player_list[i].current_frame;
		stop_frame = pmdata[mod_id].sequence_stop_frame[player_list[i].current_sequence];
		startframe = pmdata[mod_id].sequence_start_frame[player_list[i].current_sequence];
		if (player_list[i].bStopAnimating == FALSE)
		{

			if (player_list[i].animationdir == 0)
			{
				if (curr_frame >= stop_frame)
				{
					curr_seq = player_list[i].current_sequence;
					player_list[i].current_frame = pmdata[mod_id].sequence_stop_frame[curr_seq];
					player_list[i].animationdir = 1;

					if (player_list[i].current_frame == 71)
					{
						if (i == trueplayernum)
						{
							//current player
							jump = 0;

							if (runflag == 1 && jump == 0)
							{
								SetPlayerAnimationSequence(i, 1);
							}
							else if (runflag == 1 && jump == 1)
							{
							}
							else
							{
								SetPlayerAnimationSequence(i, 0);
							}
						}
						else
						{

							SetPlayerAnimationSequence(i, 0);
						}
					}

					if (player_list[i].current_frame == 183 || player_list[i].current_frame == 189 || player_list[i].current_frame == 197)
					{
						//player is dead
						player_list[i].bStopAnimating = TRUE;
					}

					if (i == trueplayernum && curr_seq == 1 && runflag == 1)
					{
					}
					else
					{
						if (curr_seq == 0 || curr_seq == 1 || curr_seq == 6)
						{
							if (i == trueplayernum && curr_seq == 0 && jump == 0)
							{

								//TODO:Remove
								SetPlayerAnimationSequence(i, 0);

								int raction = random_num(8);

								//if (raction == 0)
								//	SetPlayerAnimationSequence(i, 7);// flip
								//else if (raction == 1)
								//	SetPlayerAnimationSequence(i, 8);// Salute
								//else if (raction == 2)
								//	SetPlayerAnimationSequence(i, 9);// Taunt
								//else if (raction == 3)
								//	SetPlayerAnimationSequence(i, 10);// Wave
								//else if (raction == 4)
								//	SetPlayerAnimationSequence(i, 11); // Point
								//else
								//	SetPlayerAnimationSequence(i, 0);

							}
						}
						else
						{
							if (runflag == 1 && i == trueplayernum && jump == 0)
							{
								SetPlayerAnimationSequence(i, 1);
							}
							else
							{
								if (i == trueplayernum && jump == 1)
								{
								}
								else {
									SetPlayerAnimationSequence(i, 0);
								}

							}
						}
					}
				}
				else
				{
					player_list[i].current_frame++;
				}
			}
			else
			{
				if (curr_frame <= startframe)
				{
					curr_seq = player_list[i].current_sequence;
					player_list[i].current_frame = pmdata[mod_id].sequence_start_frame[curr_seq];
					player_list[i].animationdir = 0;
				}
				else
				{
					player_list[i].current_frame--;
				}
			}
		}
	}


	for (i = 0; i < num_monsters; i++)
	{

		int mod_id = monster_list[i].model_id;
		curr_frame = monster_list[i].current_frame;
		stop_frame = pmdata[mod_id].sequence_stop_frame[monster_list[i].current_sequence];
		startframe = pmdata[mod_id].sequence_start_frame[monster_list[i].current_sequence];
		if (monster_list[i].bStopAnimating == FALSE)
		{

			if (monster_list[i].animationdir == 0)
			{

				if (curr_frame >= stop_frame)
				{
					curr_seq = monster_list[i].current_sequence;
					monster_list[i].current_frame = pmdata[mod_id].sequence_stop_frame[curr_seq];
					monster_list[i].animationdir = 1;
					if (curr_seq == 1)
					{
					}
					else
					{
					}
					SetMonsterAnimationSequence(i, 0);

					if (monster_list[i].current_frame == 183 || monster_list[i].current_frame == 189 || monster_list[i].current_frame == 197)
					{
						monster_list[i].bStopAnimating = TRUE;
					}
				}
				else
				{
					if (monster_list[i].current_frame != 183 || monster_list[i].current_frame != 189 || monster_list[i].current_frame != 197)
					{
						monster_list[i].current_frame++;
					}
				}
			}
			else
			{
				if (curr_frame <= startframe)
				{
					curr_seq = monster_list[i].current_sequence;
					monster_list[i].current_frame = pmdata[mod_id].sequence_start_frame[curr_seq];
					monster_list[i].animationdir = 0;
				}
				else
				{
					monster_list[i].current_frame--;
				}
			}
		}
	}

	for (i = 0; i < itemlistcount; i++)
	{
		item_list[i].current_frame++;

		if (item_list[i].current_frame > 91)
			item_list[i].current_frame = 87;
	}


	return 0;
}








float fixangle(float angle, float adjust)
{

	float chunka = 0;

	if (adjust >= 0)
	{
		angle = angle + adjust;
		if (angle >= 360)
		{
			chunka = (float)fabs((float)angle) - 360.0f;
			if (chunka < 0)
				chunka = 0;
			angle = chunka;
		}
	}
	else
	{
		if (angle + adjust < 0)
		{

			chunka = adjust + angle;
			chunka = 360.0f - (float)fabs((float)chunka);
			if (chunka > 360)
				chunka = 360;

			angle = chunka;
		}
		else
			angle = angle + adjust;
	}

	return angle;
}

int initDSTimer()
{

	LONGLONG perf_cnt;
	LONGLONG count;
	int a = 0;

	if (QueryPerformanceFrequency((LARGE_INTEGER*)&perf_cnt))
	{


		// set scaling factor
		time_factor = (double)1.0 / (double)perf_cnt;

		QueryPerformanceCounter((LARGE_INTEGER*)&count);



		//		elapsegametimer = count;
		//		elapsegametimersave = 0;
		//		elapsegametimerlast = count;

		gametimerlast = count;
		//		gametimerlastpost = count;
		gametimerlast2 = count;
		//		gametimerlastdoor = count;
	}

	return 1;
}


void DrawMissles()
{

	XMFLOAT3 vw1, vw2;

	fDot2 = 0.0f;
	weapondrop = 0;

	fDot2 = 0.0f;
	for (int misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{

		if (your_missle[misslecount].active == 2)
		{
			//TODO: REMOVE whne textures are working in DX12
			//your_missle[misslecount].active = 0;
			//	continue;

			float wx = your_missle[misslecount].x;
			float wy = your_missle[misslecount].y;
			float wz = your_missle[misslecount].z;

			int cresult;
			int tex;
			tex = your_missle[misslecount].skin_tex_id;

			XMFLOAT3 collidenow;
			XMFLOAT3 normroadold;
			XMFLOAT3 work1, work2, vDiff;
			normroadold.x = 50;
			normroadold.y = 0;
			normroadold.z = 0;

			work1.x = m_vEyePt.x;
			work1.y = m_vEyePt.y;
			work1.z = m_vEyePt.z;

			work2.x = wx;
			work2.y = wy;
			work2.z = wz;

			work1.y = wy;

			XMVECTOR r1 = XMLoadFloat3(&work1) - XMLoadFloat3(&work2);

			//vDiff = Normalize(vDiff);
			//D3DXVec3Normalize(&vDiff, &vDiff);
			r1 = XMVector3Normalize(r1);

			XMVECTOR final, final2;

			final = XMVector3Normalize(r1);
			final2 = XMVector3Normalize(XMLoadFloat3(&normroadold));
			float fDot = XMVectorGetX(XMVector3Dot(final, final2));

			float convangle;
			convangle = (float)acos(fDot) / k;

			fDot = convangle;

			if (work2.z < work1.z)
			{
			}
			else
			{
				fDot = 180.0f + (180.0f - fDot);
			}

			normroadold.x = 0;
			normroadold.y = 50;
			normroadold.z = 0;

			work1.x = m_vEyePt.x;
			work1.y = m_vEyePt.y;
			work1.z = m_vEyePt.z;

			work1 = EyeTrue;

			work2.x = wx;
			work2.y = wy;
			work2.z = wz;

			XMVECTOR r4 = XMLoadFloat3(&work1) - XMLoadFloat3(&work2);
			final = XMVector3Normalize(r4);
			final2 = XMVector3Normalize(XMLoadFloat3(&normroadold));
			fDot2 = XMVectorGetX(XMVector3Dot(final, final2));

			fDot2last = fDot2;

			convangle = (float)acos(fDot2) / k;

			fDot2 = convangle;
			fDot2 = -1 * fixangle(fDot2, -90);

			fDot2last = fDot2;

			//Show blood splatter

			int bloodmodel = 100;

			if (your_missle[misslecount].critical) {
				bloodmodel = 124;
			}

			PlayerToD3DVertList(bloodmodel,
				0,
				(int)fixangle(fDot, 180),
				tex,
				USE_PLAYERS_SKIN, wx, wy, wz);

			fDot2 = 0.0f;

			if (maingameloop2)
			{
				cresult = CycleBitMap(your_missle[misslecount].skin_tex_id);
				if (cresult != -1)
				{

					if (cresult < tex)
					{
						your_missle[misslecount].active = 0;
					}
					else
					{
						tex = cresult;
						your_missle[misslecount].skin_tex_id = tex;
					}
				}
			}
		}
	}
}

/*
FLOAT D3DXVec3Length
(CONST D3DXVECTOR3* pV)
{
#ifdef D3DX_DEBUG
	if (!pV)
		return 0.0f;
#endif

#ifdef __cplusplus
	return sqrtf(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
#else
	return (FLOAT)sqrt(pV->x * pV->x + pV->y * pV->y + pV->z * pV->z);
#endif
}


FLOAT D3DXVec3Dot
(CONST D3DXVECTOR3* pV1, CONST D3DXVECTOR3* pV2)
{
#ifdef D3DX_DEBUG
	if (!pV1 || !pV2)
		return 0.0f;
#endif

	return pV1->x * pV2->x + pV1->y * pV2->y + pV1->z * pV2->z;
}


D3DXVECTOR3* WINAPI D3DXVec3Normalize(D3DXVECTOR3* pout, const D3DXVECTOR3* pv)
{
	FLOAT norm;

	norm = D3DXVec3Length(pv);
	if (!norm)
	{
		pout->x = 0.0f;
		pout->y = 0.0f;
		pout->z = 0.0f;
	}
	else
	{
		pout->x = pv->x / norm;
		pout->y = pv->y / norm;
		pout->z = pv->z / norm;
	}

	return pout;
}



D3DXVECTOR3* D3DXVec3Cross (D3DXVECTOR3* pOut, CONST D3DXVECTOR3* pV1, CONST D3DXVECTOR3* pV2)
{
	D3DXVECTOR3 v;

#ifdef D3DX_DEBUG
	if (!pOut || !pV1 || !pV2)
		return NULL;
#endif

	v.x = pV1->y * pV2->z - pV1->z * pV2->y;
	v.y = pV1->z * pV2->x - pV1->x * pV2->z;
	v.z = pV1->x * pV2->y - pV1->y * pV2->x;

	*pOut = v;
	return pOut;
}

*/


void display_message(float x, float y, char text[2048], int r, int g, int b, float fontx, float fonty, int fonttype) {


	return;
}


void DrawIndexedItems(int fakel, int vert_index)
{
	D3DPRIMITIVETYPE command;
	int face_index = 0;
	XMFLOAT3 vw1, vw2, vw3;
	float workx, worky, workz;

	if (dp_command_index_mode[fakel] == 0) //USE_INDEXED_DP
	{
		int dwIndexCount = ObjectsToDraw[fakel].facesperpoly * 3;
		int dwVertexCount = ObjectsToDraw[fakel].vertsperpoly;
		command = dp_commands[fakel];
		face_index = ObjectsToDraw[fakel].srcfstart;

		int prim = ObjectsToDraw[fakel].facesperpoly;

		dp_command_index_mode[fakel] = 1;
		verts_per_poly[fakel] = dwIndexCount;
		ObjectsToDraw[fakel].srcstart = cnt;
		ObjectsToDraw[fakel].vertsperpoly = dwIndexCount;

		for (int t = 0; t < (int)dwIndexCount; t++)
		{
			int f_index = src_f[face_index + t];
			memset(&temp_v[t], 0, sizeof(D3DVERTEX));
			memcpy(&temp_v[t], &src_v[vert_index + f_index],
				sizeof(D3DVERTEX));
		}

		int counttri = 0;

		for (int t = 0; t < (int)dwIndexCount; t++)
		{
			if (counttri == 0)
			{

				vw1.x = temp_v[t].x;
				vw1.y = temp_v[t].y;
				vw1.z = temp_v[t].z;

				vw2.x = temp_v[t + 1].x;
				vw2.y = temp_v[t + 1].y;
				vw2.z = temp_v[t + 1].z;

				vw3.x = temp_v[t + 2].x;
				vw3.y = temp_v[t + 2].y;
				vw3.z = temp_v[t + 2].z;

				// calculate the NORMAL for the road using the CrossProduct <-important!

				XMVECTOR vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
				XMVECTOR vDiff2 = XMLoadFloat3(&vw3) - XMLoadFloat3(&vw2);

				//D3DXVECTOR3 vCross, final;

				//D3DXVec3Cross(&vCross, &vDiff, &vDiff2);
				//D3DXVec3Normalize(&final, &vCross);

				XMVECTOR  vCross, final;
				vCross = XMVector3Cross(vDiff, vDiff2);
				final = XMVector3Normalize(vCross);
				XMFLOAT3 final2;
				XMStoreFloat3(&final2, final);

				workx = -final2.x;
				worky = -final2.y;
				workz = -final2.z;

				workx = -final2.x;
				worky = -final2.y;
				workz = -final2.z;
			}

			counttri++;
			if (counttri > 2)
				counttri = 0;

			temp_v[t].nx = workx;
			temp_v[t].ny = worky;
			temp_v[t].nz = workz;

		}

		int start_cnt = cnt;

		for (int j = 0; j < dwIndexCount; j++)
		{
			//D3DXVECTOR3 a = D3DXVECTOR3(temp_v[j].x, temp_v[j].y, temp_v[j].z);
			src_v[cnt].x = temp_v[j].x;
			src_v[cnt].y = temp_v[j].y;
			src_v[cnt].z = temp_v[j].z;
			src_v[cnt].tu = temp_v[j].tu;
			src_v[cnt].tv = temp_v[j].tv;
			src_v[cnt].nx = temp_v[j].nx;
			src_v[cnt].ny = temp_v[j].ny;
			src_v[cnt].nz = temp_v[j].nz;
			cnt++;
		}

		//SmoothNormals(start_cnt);

	}
}
