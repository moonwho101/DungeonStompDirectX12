
#include "d3dtypes.h"
#include "LoadWorld.hpp"
#include "world.hpp"
#include "GlobalSettings.hpp"
#include <string.h>
#include "GameLogic.hpp"
#include "Missle.hpp"
#include "col_local.h"
#include "XAudio2Versions.h"
#include "dice.hpp"

struct diceroll dice[50];
int numdice;
D3DVERTEX2 MakeVertex(float x, float y, float z);
int showsavingthrow = 0;
extern int d20roll;
extern int damageroll;

D3DVERTEX2 MakeVertex(float x, float y, float z) {

	D3DVERTEX2 d;

	d.x = x;
	d.y = y;
	d.z = z;

	return d;
}

int MakeDice()
{
	float x1, x2, y1, y2;
	float scale = 16.0f;
	float adjust = 100.0f;

	int viewportwidth = wWidth;
	int viewportheight = wHeight;

	x1 = 70.0f + scale;
	x2 = 10.0f;

	y1 = 110.0f + scale;
	y2 = 50.0f;

	dice[0].dicebox[0] = MakeVertex(0, 0, 1);
	dice[0].dicebox[1] = MakeVertex(0, 0, 0);
	dice[0].dicebox[2] = MakeVertex(0, 1, 1);
	dice[0].dicebox[3] = MakeVertex(0, 1, 0);

	dice[0].dicebox[1].x = (viewportwidth / 2) - x1;
	dice[0].dicebox[3].x = (viewportwidth / 2) - x2;
	dice[0].dicebox[0].x = (viewportwidth / 2) - x1;
	dice[0].dicebox[2].x = (viewportwidth / 2) - x2;

	dice[0].dicebox[1].y = viewportheight - y1;
	dice[0].dicebox[3].y = viewportheight - y1;
	dice[0].dicebox[0].y = viewportheight - y2;
	dice[0].dicebox[2].y = viewportheight - y2;
	dice[0].sides = 20;
	strcpy_s(dice[0].name, "die20s20");
	strcpy_s(dice[0].prefix, "die20");
	dice[0].rollnum = 8;
	dice[0].roll = 0;
	dice[0].rollmax = 5;

	dice[1].dicebox[0] = MakeVertex(0, 0, 1);
	dice[1].dicebox[1] = MakeVertex(0, 0, 0);
	dice[1].dicebox[2] = MakeVertex(0, 1, 1);
	dice[1].dicebox[3] = MakeVertex(0, 1, 0);

	dice[1].dicebox[1].x = (viewportwidth / 2) + x2;
	dice[1].dicebox[3].x = (viewportwidth / 2) + x1;
	dice[1].dicebox[0].x = (viewportwidth / 2) + x2;
	dice[1].dicebox[2].x = (viewportwidth / 2) + x1;

	dice[1].dicebox[1].y = viewportheight - y1;
	dice[1].dicebox[3].y = viewportheight - y1;
	dice[1].dicebox[0].y = viewportheight - y2;
	dice[1].dicebox[2].y = viewportheight - y2;

	strcpy_s(dice[1].name, "die4s4");
	strcpy_s(dice[1].prefix, "die4");
	dice[1].rollnum = 4;
	dice[1].roll = 0;
	dice[1].sides = 4;
	dice[1].rollmax = 3;

	dice[2].dicebox[0] = MakeVertex(0, 0, 1);
	dice[2].dicebox[1] = MakeVertex(0, 0, 0);
	dice[2].dicebox[2] = MakeVertex(0, 1, 1);
	dice[2].dicebox[3] = MakeVertex(0, 1, 0);

	dice[2].dicebox[1].x = (viewportwidth / 2) + adjust + x2;
	dice[2].dicebox[3].x = (viewportwidth / 2) + adjust + x1;
	dice[2].dicebox[0].x = (viewportwidth / 2) + adjust + x2;
	dice[2].dicebox[2].x = (viewportwidth / 2) + adjust + x1;

	dice[2].dicebox[1].y = viewportheight - y1;
	dice[2].dicebox[3].y = viewportheight - y1;
	dice[2].dicebox[0].y = viewportheight - y2;
	dice[2].dicebox[2].y = viewportheight - y2;

	strcpy_s(dice[2].name, "die6s6");
	strcpy_s(dice[2].prefix, "die6");
	dice[2].rollnum = 6;
	dice[2].roll = 0;
	dice[2].sides = 6;
	dice[2].rollmax = 4;

	dice[3].dicebox[0] = MakeVertex(0, 0, 1);
	dice[3].dicebox[1] = MakeVertex(0, 0, 0);
	dice[3].dicebox[2] = MakeVertex(0, 1, 1);
	dice[3].dicebox[3] = MakeVertex(0, 1, 0);

	dice[3].dicebox[1].x = (viewportwidth / 2) + (adjust * 2.0f) + x2;
	dice[3].dicebox[3].x = (viewportwidth / 2) + (adjust * 2.0f) + x1;
	dice[3].dicebox[0].x = (viewportwidth / 2) + (adjust * 2.0f) + x2;
	dice[3].dicebox[2].x = (viewportwidth / 2) + (adjust * 2.0f) + x1;

	dice[3].dicebox[1].y = viewportheight - y1;
	dice[3].dicebox[3].y = viewportheight - y1;
	dice[3].dicebox[0].y = viewportheight - y2;
	dice[3].dicebox[2].y = viewportheight - y2;
	dice[3].sides = 20;
	strcpy_s(dice[3].name, "die20s20");
	strcpy_s(dice[3].prefix, "die20");
	dice[3].rollnum = 8;
	dice[3].roll = 0;
	dice[3].rollmax = 5;

	x1 = 70.0f + scale;
	x2 = 10.0f;
	y1 = 100.0f + scale;
	y2 = 40.0f;

	//crosshair[0] = D3DTLVERTEX(D3DVECTOR(0, 0, 0.99f), 0.5f, -1, 0, 0, 1);
	//crosshair[1] = D3DTLVERTEX(D3DVECTOR(0, 0, 0.99f), 0.5f, -1, 0, 0, 0);
	//crosshair[2] = D3DTLVERTEX(D3DVECTOR(0, 0, 0.99f), 0.5f, -1, 0, 1, 1);
	//crosshair[3] = D3DTLVERTEX(D3DVECTOR(0, 0, 0.99f), 0.5f, -1, 0, 1, 0);

	//int offset = 5;

	//crosshair[0].sx = (viewportwidth / 2) - offset;
	//crosshair[1].sx = (viewportwidth / 2) - offset;
	//crosshair[2].sx = (viewportwidth / 2) + offset;
	//crosshair[3].sx = (viewportwidth / 2) + offset;

	//crosshair[0].sy = (viewportheight / 2) + offset;
	//crosshair[1].sy = (viewportheight / 2) - offset;
	//crosshair[2].sy = (viewportheight / 2) + offset;
	//crosshair[3].sy = (viewportheight / 2) - offset;

	return 1;
}

void MakeDamageDice()
{

	float x1, x2, y1, y2;
	float scale = 16.0f;

	x1 = 70.0f + scale;
	x2 = 10.0f;
	y1 = 110.0f + scale;
	y2 = 50.0f;

	int a;

	int gunmodel = 0;
	for (a = 0; a < num_your_guns; a++)
	{
		if (your_gun[a].model_id == player_list[trueplayernum].gunid)
		{
			gunmodel = a;
		}
	}

	if (your_gun[gunmodel].damage2 == 4)
	{
		strcpy_s(dice[1].name, "die4s4");
		strcpy_s(dice[1].prefix, "die4");
		dice[1].rollnum = 4;
		dice[1].roll = 0;
		dice[1].sides = 4;
		dice[1].rollmax = 3;
	}
	else if (your_gun[gunmodel].damage2 == 6)
	{
		strcpy_s(dice[1].name, "die6s6");
		strcpy_s(dice[1].prefix, "die6");
		dice[1].rollnum = 6;
		dice[1].roll = 0;
		dice[1].sides = 6;
		dice[1].rollmax = 4;
	}
	else if (your_gun[gunmodel].damage2 == 8)
	{
		strcpy_s(dice[1].name, "die8s8");
		strcpy_s(dice[1].prefix, "die8");
		dice[1].rollnum = 8;
		dice[1].roll = 0;
		dice[1].sides = 8;
		dice[1].rollmax = 5;
	}
	else if (your_gun[gunmodel].damage2 == 10)
	{

		strcpy_s(dice[1].name, "die10s10");
		strcpy_s(dice[1].prefix, "die10");
		dice[1].rollnum = 10;
		dice[1].roll = 0;
		dice[1].sides = 10;
		dice[1].rollmax = 5;
	}
	else if (your_gun[gunmodel].damage2 == 12)
	{

		strcpy_s(dice[1].name, "die12s12");
		strcpy_s(dice[1].prefix, "die12");
		dice[1].rollnum = 12;
		dice[1].roll = 0;
		dice[1].sides = 12;
		dice[1].rollmax = 4;
	}


	dice[1].dicebox[0] = MakeVertex(0, 0, 1);
	dice[1].dicebox[1] = MakeVertex(0, 0, 0);
	dice[1].dicebox[2] = MakeVertex(0, 1, 1);
	dice[1].dicebox[3] = MakeVertex(0, 1, 0);

	dice[1].dicebox[1].x = (wWidth / 2) + x2;
	dice[1].dicebox[3].x = (wWidth / 2) + x1;
	dice[1].dicebox[0].x = (wWidth / 2) + x2;
	dice[1].dicebox[2].x = (wWidth / 2) + x1;

	dice[1].dicebox[1].y = wHeight - y1;
	dice[1].dicebox[3].y = wHeight - y1;
	dice[1].dicebox[0].y = wHeight - y2;
	dice[1].dicebox[2].y = wHeight - y2;

}



extern int usespell;
//extern int spellhiton;
extern int hitmonster;
extern int savefailed;
extern int criticalhiton;
int spellhiton = 0;

void SetDiceTexture(bool showroll)
{
	for (int i = 0; i < numdice; i++)
	{

		if (dice[i].roll == 1)
		{
			//roll the die

			if (maingameloop)
				dice[i].rollnum++;
			if (dice[i].rollnum > dice[i].rollmax)
			{

				dice[i].rollnum = 1;
			}
			sprintf_s(dice[i].name, "%sr%d", dice[i].prefix, dice[i].rollnum);
		}
	}



	
	char junk[255];

	if (usespell == 1)
	{
		spellhiton = player_list[trueplayernum].hd;
	}
	else
	{
		spellhiton = 0;
	}

	if (hitmonster == 1 || usespell == 1)
	{

		if (strstr(your_gun[current_gun].gunname, "SCROLL-HEALING") != NULL)
		{
			strcpy_s(junk, "Attack      Heal");
		}
		else
		{
			strcpy_s(junk, "Attack      Damage");
		}
	}
	else
	{
		strcpy_s(junk, "Attack");
	}

	if (numdice == 3 && hitmonster == 1)
	{
		
	}
	else if (numdice == 3 && hitmonster == 0)
	{
		strcpy_s(junk, "Attack");
	}

	if (showsavingthrow > 0)
	{
		display_message((wWidth / 2) + 222.0f - 90.0f, wHeight - 30.0f, "Save", 255, 255, 0, 12.5, 16, 0);
		if (savefailed == 1)
			display_message((wWidth / 2) + 212.0f - 90.0f, wHeight - 15.0f, "Failed", 255, 255, 0, 12.5, 16, 0);
	}
	//if (showlisten > 0)
	//{

	//	display_message((wWidth / 2) + 111.0f, wHeight - 30.0f, "Listen", vp, 255, 255, 0, 12.5, 16, 0);
	//	if (listenfailed == 1)
	//		display_message((wWidth / 2) + 111.0f, wHeight - 15.0f, "Failed", vp, 255, 255, 0, 12.5, 16, 0);
	//}

	display_message((wWidth / 2) - 32.0f, wHeight - 30.0f, junk, 255, 255, 0, 12.5, 16, 0);

	if (criticalhiton == 1)
	{
		strcpy_s(junk, "Critical Hit  X 2");
		display_message((wWidth / 2) - 30.0f, wHeight - 145.0f, junk, 255, 255, 0, 12.5, 16, 0);
	}
	if (spellhiton > 1)
	{
		sprintf_s(junk, "           X %d", spellhiton);
		display_message((wWidth / 2) - 30.0f, wHeight - 130.0f, junk, 0, 255, 0, 12.5, 16, 0);
	}
	else
	{
		int gunmodel = 0;
		for (int a = 0; a < num_your_guns; a++)
		{

			if (your_gun[a].model_id == player_list[trueplayernum].gunid)
			{

				gunmodel = a;
			}
		}

		int attackbonus = your_gun[gunmodel].sattack;
		int damagebonus = your_gun[gunmodel].sdamage;

		if (attackbonus > 0 || damagebonus > 0)
		{
			sprintf_s(junk, "   +%d      +%d", attackbonus, damagebonus);
			display_message((wWidth / 2) - 30.0f, wHeight - 130.0f, junk, 0, 255, 0, 12.5, 16, 0);
		}
	}


	if (showroll) {
		sprintf_s(junk, "%d", d20roll);
		display_message((wWidth / 2) - 15.0f, wHeight - 60.0f, junk, 0, 255, 0, 12.5, 16, 0);

		sprintf_s(junk, "%d", damageroll);
		display_message((wWidth / 2) + 70.0f, wHeight - 60.0f, junk, 0, 255, 0, 12.5, 16, 0);
	}




}

int thaco(int ac, int thaco)
{

	int result;

	result = thaco - ac;

	return result;
}

