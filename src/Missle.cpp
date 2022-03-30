#include "resource.h"
#include "d3dtypes.h"
#include "LoadWorld.hpp"
#include "world.hpp"
#include "GlobalSettings.hpp"
#include <string.h>
#include "GameLogic.hpp"
#include "Missle.hpp"
#include "col_local.h"
#include "XAudio2Versions.h"
#include "Dice.hpp"

GUNLIST* your_missle;
int firemissle = 0;
XMFLOAT3 MissleSave;
int lastgun = 0;
int mdmg = 0;
int num_your_missles = 0;
extern int foundcollisiontrue;
extern int numberofsquares;
extern XMFLOAT3 saveoldvelocity;
extern XMFLOAT3 savevelocity;
extern int excludebox;
extern float monsterdist;
float culldist = 1450.0f;
int savefailed = 0;
int SavingThrow(int damage, int player, int level, int missleid, int isplayer, int id);
extern XMFLOAT3 eTest;
extern int damageroll;

void FirePlayerMissle(float x, float y, float z, float angy, int owner, int shoot, XMFLOAT3 velocity, float lookangy, FLOAT fTimeKeysave)
{

	int perspectiveview = 1;

	XMFLOAT3 MissleMove;
	XMFLOAT3 MissleVelocity;
	int misslecount = 0;
	int misslespot = 0;
	int gun_angle;

	if (maingameloop) {
		//only update fire in mainloop
		if (player_list[trueplayernum].firespeed > 0)
		{
			player_list[trueplayernum].firespeed--;

			if (player_list[trueplayernum].firespeed <= 0)
				player_list[trueplayernum].firespeed = 0;
		}

		for (int i = 0; i < num_monsters; i++)
		{
			//only update fire in mainloop
			if (monster_list[i].bIsPlayerValid == TRUE && monster_list[i].firespeed > 0)
			{
				monster_list[i].firespeed--;
				if (monster_list[trueplayernum].firespeed <= 0)
					monster_list[trueplayernum].firespeed = 0;
			}
		}
	}

	MissleSave.x = m_vLookatPt.x - m_vEyePt.x;
	MissleSave.y = m_vLookatPt.y - m_vEyePt.y;
	MissleSave.z = m_vLookatPt.z - m_vEyePt.z;

	if (shoot == 0)
	{
		gun_angle = -(int)angy + (int)90;

		if (gun_angle >= 360)
			gun_angle = gun_angle - 360;
		if (gun_angle < 0)
			gun_angle = gun_angle + 360;
	}
	else
	{
		gun_angle = (int)angy;
	}

	if (your_gun[current_gun].x_offset <= 0)
	{
		firemissle = 0;
	}
	else if (firemissle == 1 ||	shoot == 1)
	{
		firemissle = 0;
		MissleVelocity.x = 32.0f * sinf(angy * k);

		if (perspectiveview == 0)
		{
			MissleVelocity.y = 32.0f * sinf(0 * k);
		}
		else
		{
			float newangle = 0;
			newangle = fixangle(look_up_ang, 90);
			MissleVelocity.y = 32.0f * sinf(newangle * k);
		}

		MissleVelocity.z = 32.0f * cosf(angy * k);

		if (shoot == 1)
			MissleVelocity = velocity;

		for (misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
		{
			if (your_missle[misslecount].active == 0)
			{
				misslespot = misslecount;
				break;
			}
		}

		//Missle speed
		float r = 30.0f;
		XMFLOAT3 savevelocity;
		savevelocity.x = r * sinf(angy * k);
		if (perspectiveview == 1)
		{
			savevelocity.y = r * sinf(lookangy * k);
		}
		else
		{
			savevelocity.y = r * sinf(0.0f * k);
		}
		savevelocity.z = r * cosf(angy * k);

		if (perspectiveview == 1)
			savevelocity = MissleSave;

		if (strstr(your_gun[current_gun].gunname, "SCROLL-MAGICMISSLE") != NULL)
		{
			your_missle[misslespot].model_id = 103;
			your_missle[misslespot].skin_tex_id = 205;
		}
		else if (strstr(your_gun[current_gun].gunname, "SCROLL-FIREBALL") != NULL)
		{
			your_missle[misslespot].model_id = 104;
			your_missle[misslespot].skin_tex_id = 288;
		}
		else if (strstr(your_gun[current_gun].gunname, "SCROLL-LIGHTNING") != NULL)
		{
			your_missle[misslespot].model_id = 105;
			your_missle[misslespot].skin_tex_id = 278;
		}
		else if (strstr(your_gun[current_gun].gunname, "SCROLL-HEALING") != NULL)
		{
			if (player_list[trueplayernum].health < player_list[trueplayernum].hp)
			{
				int roll = 0;
				roll = random_num(8) + 1;

				int hp = (roll * (int)(player_list[trueplayernum].hd));
				player_list[trueplayernum].health = player_list[trueplayernum].health + hp;

				if (player_list[trueplayernum].health > player_list[trueplayernum].hp)
					player_list[trueplayernum].health = player_list[trueplayernum].hp;

				PlayWavSound(SoundID("potion"), 100);

				PlayWavSound(SoundID("chant"), 100);
				//StartFlare(3);
				//update healing dice, no roll
				dice[1].roll = 0;
				sprintf_s(dice[1].name, "die8s%d", roll);

				sprintf_s(gActionMessage, "You heal by %d health", hp);
				UpdateScrollList(0, 245, 255);
				your_gun[current_gun].x_offset--;
				if (your_gun[current_gun].x_offset <= 0)
				{
					your_gun[current_gun].x_offset = 0;
					your_gun[current_gun].active = 0;
				}

				LevelUp(player_list[trueplayernum].xp);
			}
			return;
		}
		//set length to 1
		//TODO: fix this
		D3DVECTOR s1 = calculatemisslelength(savevelocity);

		savevelocity.x = s1.x;
		savevelocity.y = s1.y;
		savevelocity.z = s1.z;

		your_missle[misslespot].current_frame = 0;
		your_missle[misslespot].current_sequence = 0;
		your_missle[misslespot].x = x;
		your_missle[misslespot].y = y - 10.0f;
		your_missle[misslespot].z = z;
		your_missle[misslespot].rot_angle = (float)gun_angle;
		your_missle[misslespot].velocity = savevelocity;
		your_missle[misslespot].active = 1;
		your_missle[misslespot].owner = (int)owner;

		your_missle[misslespot].playernum = (int)owner;
		your_missle[misslespot].playertype = (int)1;
		your_missle[misslespot].guntype = current_gun;

		lastgun = current_gun;

		int attackbonus = your_gun[lastgun].sattack;
		int damagebonus = your_gun[lastgun].sdamage;
		int weapondamage = your_gun[lastgun].damage2;
		int action;

		action = random_num(weapondamage) + 1;
		sprintf_s(dice[1].name, "%ss%d", dice[1].prefix, action);
		mdmg = action;

		your_missle[misslespot].dmg = action;
		your_gun[current_gun].x_offset--;

		if (your_gun[current_gun].x_offset <= 0)
		{
			your_gun[current_gun].x_offset = 0;
			your_gun[current_gun].active = 0;
		}
		float qdist = FastDistance(
			player_list[trueplayernum].x - your_missle[misslespot].x,
			player_list[trueplayernum].y - your_missle[misslespot].y,
			player_list[trueplayernum].z - your_missle[misslespot].z);
		your_missle[misslespot].qdist = qdist;

		if (your_missle[misslespot].sexplode != 0)
		{
			//DSound_Delete_Sound(your_missle[misslespot].sexplode);
			your_missle[misslespot].sexplode = 0;
		}
		if (your_missle[misslespot].smove != 0)
			//DSound_Delete_Sound(your_missle[misslespot].smove);
			your_missle[misslespot].smove = 0;

		your_missle[misslespot].sexplode = DSound_Replicate_Sound(SoundID("explod2"));

		if (current_gun == 18)
			your_missle[misslespot].smove = DSound_Replicate_Sound(SoundID("spell1"));
		else if (current_gun == 19)
			your_missle[misslespot].smove = DSound_Replicate_Sound(SoundID("spell2"));
		if (current_gun == 20)
			your_missle[misslespot].smove = DSound_Replicate_Sound(SoundID("spell3"));

		num_your_missles++;

		dice[0].roll = 0;
		dice[1].roll = 1;

		PlayWavSound(your_missle[misslecount].smove, 100);
	}

	saveoldvelocity = savevelocity;

	int pspeed = (18 - player_list[trueplayernum].hd);
	if (pspeed < 6)
		pspeed = 6;

	if (player_list[trueplayernum].firespeed == pspeed / 2 && current_gun == lastgun)
	{
		int attackbonus = your_gun[lastgun].sattack;
		int damagebonus = your_gun[lastgun].sdamage;
		int weapondamage = your_gun[lastgun].damage2;

		sprintf_s(dice[1].name, "%ss%d", dice[1].prefix, mdmg);
		dice[1].roll = 0;
	}

	for (misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{
		if (your_missle[misslecount].active == 1)
		{
			MissleMove.x = your_missle[misslecount].x;
			MissleMove.y = your_missle[misslecount].y;
			MissleMove.z = your_missle[misslecount].z;

			XMFLOAT3 collidenow;
			XMFLOAT3 saveeye;
			float qdist = FastDistance(
				player_list[trueplayernum].x - your_missle[misslecount].x,
				player_list[trueplayernum].y - your_missle[misslecount].y,
				player_list[trueplayernum].z - your_missle[misslecount].z);
			your_missle[misslecount].qdist = qdist;

			collidenow.x = MissleMove.x;
			collidenow.y = MissleMove.y;
			collidenow.z = MissleMove.z;

			excludebox = 1;
			MakeBoundingBox();
			excludebox = 0;
			saveeye = m_vEyePt;
			m_vEyePt = collidenow;

			//TODO: fix radius
			eRadius = { 10.0f, 10.0f, 10.0f };

			savevelocity = your_missle[misslecount].velocity;

			float realspeed = 600.0f;

			savevelocity.x = (savevelocity).x * realspeed * fTimeKeysave;
			savevelocity.y = (savevelocity).y * realspeed * fTimeKeysave;
			savevelocity.z = (savevelocity).z * realspeed * fTimeKeysave;
			
			foundcollisiontrue = 0;
			XMFLOAT3 result;

			result = collideWithWorld(RadiusDivide(collidenow, eRadius), RadiusDivide(savevelocity, eRadius));
			result = RadiusMultiply(result, eRadius);

			eRadius = { 25.0f, 50.0f, 25.0f };

			if (foundcollisiontrue == 1 || qdist > 5000.0f)
			{
				your_missle[misslecount].active = 2;

				//Bounce like a billard ball.
				/*
				your_missle[misslecount].active = 1;

				XMFLOAT3 a = your_missle[misslecount].velocity;
				XMVECTOR m1 = XMVector3Normalize(XMLoadFloat3(&eTest));
				XMVECTOR m2 = XMVector3Normalize(XMLoadFloat3(&a));
				XMVECTOR fDotVector = XMVector3Dot(m2, m1);
				float fDot = XMVectorGetX(fDotVector);
				XMVECTOR mv = XMLoadFloat3(&your_missle[misslecount].velocity);
				XMVECTOR v = 1.0f * (-2.0f * (fDot) * m1 + mv);
				XMFLOAT3 result;
				XMStoreFloat3(&result, v);

				your_missle[misslecount].velocity = result;
				*/

				int volume;
				volume = 100 - (int)((100 * your_missle[misslecount].qdist) / ((numberofsquares * monsterdist) / 2));

				if (volume < 10)
					volume = 10;

				if (volume > 100)
					volume = 100;

				PlayWavSound(your_missle[misslecount].sexplode, volume);
			}
			else
			{

				int volume;
				volume = 100 - (int)((100 * your_missle[misslecount].qdist) / ((numberofsquares * monsterdist) / 2));

				if (volume < 10)
					volume = 10;

				if (volume > 100)
					volume = 100;

				//DSound_Set_Volume(your_missle[misslecount].smove, volume);

				your_missle[misslecount].x = result.x;
				your_missle[misslecount].y = result.y;
				your_missle[misslecount].z = result.z;
			}

			m_vEyePt = saveeye;
			your_missle[misslecount].qdist = qdist;
		}
	}
}


D3DVECTOR calculatemisslelength(XMFLOAT3 velocity)
{
	//todo: fix this
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


void DrawMissle()
{
	int ob_type = 0;

	for (int misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{

		if (your_missle[misslecount].active == 1)
		{
			float wx = your_missle[misslecount].x;
			float wy = your_missle[misslecount].y;
			float wz = your_missle[misslecount].z;

			ob_type = your_missle[misslecount].model_id;
			float current_frame = your_missle[misslecount].current_frame;
			int angle = (int)your_missle[misslecount].rot_angle;

			PlayerToD3DVertList(ob_type,
				current_frame,
				angle,
				your_missle[misslecount].skin_tex_id,
				USE_DEFAULT_MODEL_TEX, wx, wy, wz);
		}
	}
}


void ApplyMissleDamage(int playernum)
{
	int misslecount = 0, i = 0;
	D3DVECTOR MissleMove;
	float qdist;

	//type 0=monster 1=player
	for (misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{
		if (your_missle[misslecount].active == 1)
		{
			//deal with your missles on monster
			for (i = 0; i < num_monsters; i++)
			{
				if (monster_list[i].bIsPlayerValid == TRUE &&
					monster_list[i].bIsPlayerAlive == TRUE &&
					monster_list[i].health > 0 &&
					strcmp(monster_list[i].rname, "SLAVE") != 0 &&
					strcmp(monster_list[i].rname, "CENTAUR") != 0)
				{

					qdist = FastDistance(your_missle[misslecount].x - monster_list[i].x,
						your_missle[misslecount].y - monster_list[i].y,
						your_missle[misslecount].z - monster_list[i].z);

					if (qdist < 60.0f)
					{
						if (your_missle[misslecount].playertype == 0)
						{
							//missle is a monsters
							//dont explode it
						}
						else if (your_missle[misslecount].owner == trueplayernum)
						{
							//it is your missle
							your_missle[misslecount].active = 2;

							int volume;
							volume = 100 - (int)((100 * your_missle[misslecount].qdist) / ((numberofsquares * monsterdist) / 2));
							if (volume < 10)
								volume = 10;

							if (volume > 100)
								volume = 100;

							PlayWavSound(your_missle[misslecount].sexplode, volume);

							int attackbonus = your_gun[your_missle[misslecount].guntype].sattack;
							int damagebonus = your_gun[your_missle[misslecount].guntype].sdamage;
							int weapondamage = your_gun[your_missle[misslecount].guntype].damage2;
							int action;

							int lvl;
							lvl = player_list[trueplayernum].hd;
							if (lvl < 1)
								lvl = 1;

							action = your_missle[misslecount].dmg * lvl;

							damageroll = your_missle[misslecount].dmg;

							action = action + damagebonus;

							int result = SavingThrow(action, 1, your_missle[misslecount].owner, misslecount, 0, i);

							if (result != action)
							{
								sprintf_s(gActionMessage, "%s made a saving throw!", monster_list[i].rname);
								UpdateScrollList(0, 245, 255);
								sprintf_s(gActionMessage, "Your spell hit a %s for %d hp (%d/2).", monster_list[i].rname, result, action);
								UpdateScrollList(0, 245, 255);
								action = result;
							}
							else
							{
								sprintf_s(gActionMessage, "Your spell hit a %s for %d hp.", monster_list[i].rname, action);
								UpdateScrollList(0, 245, 255);
							}

							DisplayDamage(monster_list[i].x, monster_list[i].y, monster_list[i].z, 1, i, 0);

							monster_list[i].health = monster_list[i].health - action;
							int painaction = random_num(3);
							SetMonsterAnimationSequence(i, 3 + painaction);
							monster_list[i].ability = 0;

							for (int t = 0; t < num_monsters; t++)
							{
								if (monster_list[t].ability == 2 && monster_list[t].dist < 600.0f)
								{
									monster_list[t].ability = 0;
								}
							}

							if (monster_list[i].health <= 0)
							{

								PlayWavSound(monster_list[i].sdie, 100);

								int deathaction = random_num(3);

								SetMonsterAnimationSequence(i, 12 + deathaction);
								monster_list[i].bIsPlayerAlive = FALSE;
								monster_list[i].bIsPlayerValid = FALSE;
								monster_list[i].health = 0;

								int gd = (random_num(monster_list[i].hd * 10) + monster_list[i].hd * 10) + 1;
								int xp = XpPoints(monster_list[i].hd, monster_list[i].hp);
								sprintf_s(gActionMessage, "You killed a %s worth %d xp.", monster_list[i].rname, xp);
								UpdateScrollList(0, 245, 255);
								player_list[trueplayernum].frags++;

								AddTreasure(monster_list[i].x, monster_list[i].y, monster_list[i].z, gd);

								player_list[trueplayernum].xp += xp;
								LevelUp(player_list[trueplayernum].xp);

							}
						}
					}
				}
			}

			//deal with missles hitting you
			i = trueplayernum;
			if (player_list[i].bIsPlayerValid == TRUE &&
				player_list[i].bIsPlayerAlive == TRUE && player_list[i].health > 0)
			{
				qdist = FastDistance(your_missle[misslecount].x - player_list[i].x,
					your_missle[misslecount].y - player_list[i].y,
					your_missle[misslecount].z - player_list[i].z);

				if (qdist < 60.0f)
				{
					if (your_missle[misslecount].owner == i &&
						your_missle[misslecount].playertype == 1)
					{
						//its current players missle missle dont explode it
					}
					else if (your_missle[misslecount].owner != trueplayernum &&
	 						 your_missle[misslecount].playertype == 1)
					{
						//its other players missle
						your_missle[misslecount].active = 2;
						
						int volume;
						volume = 100 - (int)((100 * your_missle[misslecount].qdist) / ((numberofsquares * monsterdist) / 2));

						if (volume < 10)
							volume = 10;

						if (volume > 100)
							volume = 100;

						PlayWavSound(your_missle[misslecount].sexplode, volume);

						int dmg = 0;
						int raction = 0;
						if (your_missle[misslecount].y_offset == 1.0f)
						{
							raction = random_num(4) + 1;
						}
						else if (your_missle[misslecount].y_offset == 2.0f)
						{
							raction = random_num(6) + 1;
						}
						else if (your_missle[misslecount].y_offset == 3.0f)
						{
							raction = random_num(8) + 2;
						}

						int lvl;
						lvl = your_missle[misslecount].dmg;
						lvl = lvl / 2;
						if (lvl <= 1)
							lvl = 1;

						raction = raction * lvl;
						int result = SavingThrow(raction, 1, your_missle[misslecount].owner, misslecount, 1, trueplayernum);

						if (result != raction)
						{
							sprintf_s(gActionMessage, "You made your saving throw!");
							UpdateScrollList(255, 0, 0);

							if (result == 0) {
								sprintf_s(gActionMessage, "A natural 20! No spell damage.");
							}
							else {
								sprintf_s(gActionMessage, "A spell hit you for %d hp (%d/2).", result, raction);
							}

							UpdateScrollList(255, 0, 0);
							raction = result;
						}
						else
						{
							sprintf_s(gActionMessage, "A spell hit you for %d hp", raction);
							UpdateScrollList(255, 0, 0);
						}

						ApplyPlayerDamage(trueplayernum, raction);
					}
					else if (your_missle[misslecount].playertype == 0)
					{
						//its a monsters missle
						your_missle[misslecount].active = 2;
						
						int volume;
						volume = 100 - (int)((100 * your_missle[misslecount].qdist) / ((numberofsquares * monsterdist) / 2));

						if (volume < 10)
							volume = 10;

						if (volume > 100)
							volume = 100;

						PlayWavSound(your_missle[misslecount].sexplode, volume);

						int dmg = 0;
						int raction = 0;
						if (your_missle[misslecount].y_offset == 1.0f)
						{
							raction = random_num(4) + 1;
						}
						else if (your_missle[misslecount].y_offset == 2.0f)
						{
							raction = random_num(6) + 1;
						}
						else if (your_missle[misslecount].y_offset == 3.0f)
						{
							raction = random_num(8) + 2;
						}

						int lvl;
						lvl = monster_list[your_missle[misslecount].owner].hd;
						lvl = lvl / 2;
						if (lvl <= 1)
							lvl = 1;
						raction = raction * lvl;
						int result = SavingThrow(raction, 0, your_missle[misslecount].owner, misslecount, 1, trueplayernum);

						if (result != raction)
						{
							sprintf_s(gActionMessage, "You made your saving throw!");
							UpdateScrollList(255, 0, 0);

							if (result == 0) {
								sprintf_s(gActionMessage, "A natural 20! No spell damage.");
							}
							else {
								sprintf_s(gActionMessage, "A spell hit you for %d hp (%d/2).", result, raction);
							}

							UpdateScrollList(255, 0, 0);
							raction = result;
						}
						else
						{
							sprintf_s(gActionMessage, "A spell hit you for %d hp", raction);
							UpdateScrollList(255, 0, 0);
						}
						ApplyPlayerDamage(trueplayernum, raction);
					}
				}
			}

			//missle near monster
			for (i = 0; i < num_monsters; i++)
			{
				if (monster_list[i].bIsPlayerValid == TRUE &&
					monster_list[i].bIsPlayerAlive == TRUE && monster_list[i].health > 0)
				{
					qdist = FastDistance(your_missle[misslecount].x - monster_list[i].x,
						your_missle[misslecount].y - monster_list[i].y,
						your_missle[misslecount].z - monster_list[i].z);

					if (qdist < 60.0f)
					{
						if (your_missle[misslecount].playertype == 0)
						{
							//yours does nothing
						}
						else {
							your_missle[misslecount].active = 2;
						}
					}
				}
			}
		}
	}
}


int SavingThrow(int damage, int player, int level, int missleid, int isplayer, int id)
{
	//saving throw = 10 + level of speel + ADJUSTMENT (bonus)
	int action = random_num(dice[3].sides) + 1;

	int save = 0;
	int hd = 1;

	int bonus = 0;

	if (isplayer == 1)
	{
		bonus = player_list[id].hd / 2;
	}
	else
	{
		bonus = monster_list[id].hd / 2;
	}

	if (bonus <= 0)
		bonus = 0;

	if (player == 1)
	{
		hd = player_list[your_missle[missleid].owner].hd;
	}
	else
	{
		hd = monster_list[your_missle[missleid].owner].hd;
	}

	hd = hd / 2;
	if (hd <= 0)
		hd = 1;

	save = 10 + hd;

	if (save >= 20)
		save = 20;

	if (action + bonus >= save)
	{
		if (isplayer) {
			PlayWavSound(SoundID("savethrow"), 100);
		}

		savefailed = 0;

		if (action == 20 && isplayer)
		{
			PlayWavSound(SoundID("save20"), 100);
			damage = 0;
		}
		else {
			damage = damage / 2;
			if (damage <= 0)
				damage = 1;
		}
	}
	else
	{
		savefailed = 1;
	}

	if (isplayer == 1)
	{
		showsavingthrow = 1;
		dice[3].roll = 0;
		sprintf_s(dice[3].name, "%ss%d", dice[3].prefix, action);
	}
	return damage;
}

void FireMonsterMissle(int monsterid, int type)
{

	XMFLOAT3 MissleMove;
	XMFLOAT3 MissleVelocity;
	int misslecount = 0;
	int misslespot = 0;
	float gun_angle;
	float angy;

	angy = monster_list[monsterid].rot_angle;
	gun_angle = angy;

	angy = 360 - angy;

	angy = fixangle(angy, 90);

	for (misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{
		if (your_missle[misslecount].active == 0)
		{
			misslespot = misslecount;
			break;
		}
	}

	float r = 30.0f;
	XMFLOAT3 savevelocity;

	XMFLOAT3 slope;
	XMFLOAT3 slope1;

	slope.x = monster_list[monsterid].x;
	slope.y = monster_list[monsterid].y;
	slope.z = monster_list[monsterid].z;

	slope1.x = player_list[monster_list[monsterid].closest].x;
	slope1.y = player_list[monster_list[monsterid].closest].y - 10.0f;
	slope1.z = player_list[monster_list[monsterid].closest].z;

	XMVECTOR scalc = XMLoadFloat3(&slope1) - XMLoadFloat3(&slope);
	XMVECTOR final = XMVector3Normalize(scalc);
	XMFLOAT3 final2;
	XMStoreFloat3(&final2, final);

	savevelocity.x = r * sinf(angy * k);
	savevelocity.y = r * final2.y;
	savevelocity.z = r * cosf(angy * k);

	your_missle[misslespot].model_id = 102;

	float qdist = FastDistance(player_list[trueplayernum].x - your_missle[misslespot].x,
							   player_list[trueplayernum].y - your_missle[misslespot].y,
							   player_list[trueplayernum].z - your_missle[misslespot].z);
	
	your_missle[misslespot].qdist = qdist;

	int volume;
	volume = 100 - (int)((100 * your_missle[misslespot].qdist) / ((numberofsquares * monsterdist) / 2));

	if (volume < 10)
		volume = 10;

	if (volume > 100)
		volume = 100;

	if (your_missle[misslespot].smove != 0)
	{
		DSound_Delete_Sound(your_missle[misslespot].smove);
		your_missle[misslespot].smove = 0;
	}

	if (type == 1)
	{
		your_missle[misslespot].model_id = 103;
		your_missle[misslespot].skin_tex_id = 205;
		your_missle[misslespot].smove = DSound_Replicate_Sound(SoundID("spell1"));
	}
	else if (type == 2)
	{
		your_missle[misslespot].model_id = 104;
		your_missle[misslespot].skin_tex_id = 288;
		your_missle[misslespot].smove = DSound_Replicate_Sound(SoundID("spell2"));
	}
	else if (type == 3)
	{
		your_missle[misslespot].model_id = 105;
		your_missle[misslespot].skin_tex_id = 278;
		your_missle[misslespot].smove = DSound_Replicate_Sound(SoundID("spell3"));
	}
	else if (type == 4)
	{
		your_missle[misslespot].model_id = 105;
		your_missle[misslespot].skin_tex_id = 278;
	}

	//todo: fix this why 
	//savevelocity = calculatemisslelength(savevelocity);

	D3DVECTOR s1 = calculatemisslelength(savevelocity);

	savevelocity.x = s1.x;
	savevelocity.y = s1.y;
	savevelocity.z = s1.z;

	PlayWavSound(your_missle[misslecount].smove, volume);

	your_missle[misslespot].current_frame = 0;
	your_missle[misslespot].current_sequence = 0;
	your_missle[misslespot].x = monster_list[monsterid].x;
	your_missle[misslespot].y = monster_list[monsterid].y + 10.0f;
	your_missle[misslespot].z = monster_list[monsterid].z;
	your_missle[misslespot].rot_angle = (float)gun_angle;
	your_missle[misslespot].velocity = savevelocity;
	your_missle[misslespot].active = 1;
	your_missle[misslespot].owner = monsterid;
	your_missle[misslespot].y_offset = (float)type;
	your_missle[misslespot].playernum = (int)monsterid;
	your_missle[misslespot].playertype = (int)0;

	if (your_missle[misslespot].sexplode != 0)
	{
		//ToODO: fix delete sounds
		DSound_Delete_Sound(your_missle[misslespot].sexplode);
		your_missle[misslespot].sexplode = 0;

	}
	your_missle[misslespot].sexplode = DSound_Replicate_Sound(SoundID("explod2"));
	num_your_missles++;
}
