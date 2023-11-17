
#include "d3dtypes.h"
#include "LoadWorld.hpp"
#include "world.hpp"
#include "GlobalSettings.hpp"
#include <string.h>
#include "GameLogic.hpp"
#include "Missle.hpp"
#include "Dice.hpp"
#include <timeapi.h>
#include <DirectXMath.h>
#include "CameraBob.hpp"
using namespace DirectX;

#pragma comment( lib, "Winmm.lib" )

//Monster
int monsterenable = 1;
int monstercount = 0;
int monstermoveon = 1;
int showmonstermenu = 1;
int monstercull[1000];
int monstertype[1000];
int monsterangle[1000];
int monsterobject[1000];
int trueplayernum = 0;
int numberofsquares = 11;
float monsterdist = 150.0f;
D3DVERTEX2 showview[10];
DOORS door[200];
int pressopendoor;
//Door
int listendoor = 0;
int listenfailed = 0;

char statusbar[255];
//BoundingBox
int excludebox = 0;
D3DVERTEX2 boundingbox[2000];
int countboundingbox = 0;
extern int currentmonstercollisionid;
extern int criticalhiton;
extern int hitmonster;
int current_gun = 0;
int current_frame = 0;
int weapondrop = 0;
XMFLOAT3 GunTruesave;
int currentmodellist = 0;
float fDot2 = 0;

int damageinprogress = 0;

int d20roll = 20;
int damageroll = 4;

XMFLOAT3 eRadius = XMFLOAT3(25.0f, 50.0f, 25.0f);

SCROLLLISTING scrolllist1[50];
int slistcounter = 0;
int sliststart = 0;
int scrolllistnum = 6;
char gActionMessage[2048];
int scount = 0;

int dialogtextcount = 0;
FLOAT LevelModTime = 0;
FLOAT LevelModLastTime = 0;
int countmodtime = 0;
gametext dialogtext[200];
float cullAngle;

extern int totalmod;
extern LEVELMOD* levelmodify;
extern int countswitches;
extern SWITCHMOD* switchmodify;
extern CameraBob bobY;

void ScanModJump(int jump);
void AddTreasureDrop(float x, float y, float z, int raction);
void FireMonsterMissle(int monsterid, int type);
extern int ResetSound();
char levelname[80];
extern CLoadWorld* pCWorld;
int ResetPlayer();
void ClearObjectList();
void SwitchGun(int gun);
void statusbardisplay(float x, float length, int type);

void MoveMonsters(float fElapsedTime)
{
	//0 = Normal
	//1 = slave
	//2 = line of site
	//3 = dead
	//4 = statue
	//5 = 250 distance
	//6 = fire only

	//if (monstermoveon == 0)
		//return;

	//Protect against a long game PAUSE and mosnters running off the screen.
	//if (fTimeKeysave > 1.0f) {
		//fTimeKeysave = 0;
	//}

	XMFLOAT3 work1, work2, vw1, vw2;
	int i;

	XMFLOAT3 saveeyept;
	int cullloop2 = 0;
	int raction = 0;
	int skipmonster = 0;

	XMFLOAT3 result;

	int cullloop = 0;
	int cullflag = 0;

	XMFLOAT3 collidenow;
	XMFLOAT3 normroadold;

	saveeyept = m_vEyePt;
	normroadold.x = 50;
	normroadold.y = 0;
	normroadold.z = 0;

	int perspectiveview = 1;


	MakeBoundingBox();

	if (player_list[trueplayernum].bIsPlayerAlive == FALSE)
		return;

	for (i = 0; i < num_monsters; i++)
	{
		cullflag = 0;

		for (cullloop = 0; cullloop < monstercount; cullloop++)
		{


			if (monstercull[cullloop] == monster_list[i].monsterid)
			{
				if (monstertype[cullloop] == 0)
				{

					// is it cloest to me
					if (monster_list[i].closest == trueplayernum)
					{
						if (monster_list[i].current_frame == 183 || monster_list[i].current_frame == 189 || monster_list[i].current_frame == 197)
						{
							monster_list[i].bStopAnimating = TRUE;
						}

						if (monster_list[i].bStopAnimating == FALSE)
							cullflag = 1;

						if (strcmp(monster_list[i].rname, "SLAVE") == 0 || strcmp(monster_list[i].rname, "CENTAUR") == 0 ||
							monster_list[i].ability == 2 || monster_list[i].ability == 5)
						{

							XMFLOAT3 work2, work1;

							work1.x = player_list[trueplayernum].x;
							work1.y = player_list[trueplayernum].y;
							work1.z = player_list[trueplayernum].z;

							work2.x = monster_list[i].x;
							work2.y = monster_list[i].y;
							work2.z = monster_list[i].z;
							int monsteron = 0;

							monsteron = CalculateViewMonster(work2, work1, cullAngle, monster_list[i].rot_angle);
							// monster see
							if (monsteron)
							{

								if (monster_list[i].ability == 2)
									monster_list[i].ability = 0;

								int t = 0;
								int cullloop2 = 0;
								for (t = 0; t < num_monsters; t++)
								{
									for (cullloop2 = 0; cullloop2 < monstercount; cullloop2++)
									{
										if (monstercull[cullloop2] == monster_list[t].monsterid)
										{

											if (monster_list[t].ability == 2 &&
												monster_list[t].dist < 600.0f)
											{
												monster_list[t].ability = 0;
											}

											if (monster_list[t].ability == 5 &&
												monster_list[t].dist < 250.0f)
											{
												monster_list[t].ability = 0;
											}
										}
									}
								}
							}

							cullflag = 0;
						}
					}
				}
				break;
			}
		}



		if (cullflag)
		{

			if (perspectiveview == 1)
			{
				work1.x = player_list[trueplayernum].x;
				work1.y = 0.0f;
				work1.z = player_list[trueplayernum].z;
			}
			else
			{

				work1.x = player_list[trueplayernum].x;
				work1.y = 0.0f;
				work1.z = player_list[trueplayernum].z;
			}

			work2.x = monster_list[i].x;
			work2.y = 0.0f;
			work2.z = monster_list[i].z;

			XMVECTOR P1 = XMLoadFloat3(&work1);
			XMVECTOR P2 = XMLoadFloat3(&work2);
			XMVECTOR vDiff = P1 - P2;
			vDiff = XMVector3Normalize(vDiff);

			currentmonstercollisionid = monster_list[i].monsterid;

			vw1.x = work1.x;
			vw1.y = (float)0;
			vw1.z = work1.z;

			vw2.x = work2.x;
			vw2.y = (float)0;
			vw2.z = work2.z;

			P1 = XMLoadFloat3(&vw1);
			P2 = XMLoadFloat3(&vw2);
			XMVECTOR vDiff2 = P1 - P2;
			//vDiff2 = vw1 - vw2;

			skipmonster = 0;

			if (monster_list[i].current_frame == 50)
			{
				PlayWavSound(monster_list[i].sweapon, monster_list[i].volume);
			}
			else
			{
				monster_list[i].applydamageonce = 0;
			}

			float length = XMVectorGetX(XMVector3Length(vDiff2));

			if (length < 80.0f)
			{
				skipmonster = 1;

				//if there are close attack right away and set attack speed
				if (monster_list[i].attackspeed <= 0)
				{

					SetMonsterAnimationSequence(i, 2);
					monster_list[i].attackspeed = 30 - (monster_list[i].hd);
					if (monster_list[i].attackspeed <= 0)
						monster_list[i].attackspeed = 0;
				}
				else
				{


					if (maingameloop)
					{
						monster_list[i].attackspeed--;
						if (monster_list[i].attackspeed <= 0)
							monster_list[i].attackspeed = 0;
					}


					if (monster_list[i].current_frame == 53 ||
						monster_list[i].current_frame == 83 ||
						monster_list[i].current_frame == 94 ||
						monster_list[i].current_frame == 111 ||
						monster_list[i].current_frame == 122 ||
						monster_list[i].current_frame == 134)
					{

						raction = random_num(10);

						if (raction == 0)
						{
							PlayWavSound(monster_list[i].syell, monster_list[i].volume);
						}

						raction = random_num(5);

						switch (raction)
						{
						case 0:

							SetMonsterAnimationSequence(i, 7);
							break;
						case 1:

							if (raction != 0)
							{
								int raction2 = random_num(4);
								if (raction2 == 0)
									PlayWavSound(monster_list[i].syell, monster_list[i].volume);
							}

							SetMonsterAnimationSequence(i, 8);
							break;
						case 2:
							SetMonsterAnimationSequence(i, 9);
							break;
						case 3:

							SetMonsterAnimationSequence(i, 10);
							break;
						case 4:

							if (raction != 0)
							{
								int raction2 = random_num(3);
								if (raction2 == 0)
									PlayWavSound(monster_list[i].syell, monster_list[i].volume);
							}

							SetMonsterAnimationSequence(i, 11);
							break;
						case 5:
							break;
						}
					}
				}

				if (monster_list[i].current_sequence == 1 || monster_list[i].current_sequence == 0)
				{

					if (monster_list[i].attackspeed == 0)
						SetMonsterAnimationSequence(i, 2);
				}
				if (monster_list[i].current_sequence == 2 && monster_list[i].current_frame == 53)
				{

					monster_list[i].attackspeed = 0;

				}


				if (monster_list[i].current_frame >= 50 && monster_list[i].current_frame <= 50)
				{

					XMFLOAT3 work2, work1;

					work1.x = player_list[trueplayernum].x;
					work1.y = player_list[trueplayernum].y;
					work1.z = player_list[trueplayernum].z;

					work2.x = monster_list[i].x;
					work2.y = monster_list[i].y;
					work2.z = monster_list[i].z;
					int monsteron = 0;

					monsteron = CalculateViewMonster(work2, work1, cullAngle, monster_list[i].rot_angle);
					if (monster_list[i].current_sequence != 5 && monsteron && monster_list[i].dist <= 200.0f)
					{

						int action = random_num(20) + 1;
						if (action >= monster_list[i].thaco - player_list[trueplayernum].ac)
						{
							if (monster_list[i].applydamageonce == 0)
							{

								action = random_num(monster_list[i].damage2) + 1;
								sprintf_s(gActionMessage, "A %s hit you for %d damage", monster_list[i].rname, action);
								UpdateScrollList(255, 0, 0);
								//StartFlare(1);
								ApplyPlayerDamage(trueplayernum, action);
								monster_list[i].applydamageonce = 1;
							}
						}

					}
				}
			}
			else
			{

				if (monster_list[i].current_sequence == 0)
				{

					raction = random_num(20);
					if (monster_list[i].ability != 6 && monster_list[i].ability != 7)
					{

						switch (raction)
						{
						case 0:
							SetMonsterAnimationSequence(i, 7);// flip
							PlayWavSound(monster_list[i].syell, monster_list[i].volume);

							break;
						case 1:
							SetMonsterAnimationSequence(i, 8);
							break;
						case 2:
							SetMonsterAnimationSequence(i, 9);// Taunt
							PlayWavSound(monster_list[i].syell, monster_list[i].volume);

							break;
						case 3:
							SetMonsterAnimationSequence(i, 10);// Wave
							break;
						case 4:
							SetMonsterAnimationSequence(i, 11);// Point
							PlayWavSound(monster_list[i].syell, monster_list[i].volume);
							break;
						case 5:
							SetMonsterAnimationSequence(i, 0);
							break;
						default:
							SetMonsterAnimationSequence(i, 1);
							break;
						}
					}

					int firetype = 0;
					int raction = 0;
					int mspeed = 0;

					if (
						strstr(monster_list[i].rname, "OGRE") != NULL ||
						strstr(monster_list[i].rname, "OGRO") != NULL ||
						strstr(monster_list[i].rname, "PHANTOM") != NULL)
					{

						firetype = 1;
					}
					else if (strstr(monster_list[i].rname, "WRAITH") != NULL ||
						strstr(monster_list[i].rname, "BAUUL") != NULL ||
						strstr(monster_list[i].rname, "FAERIE") != NULL ||
						strstr(monster_list[i].rname, "FULIMO") != NULL ||
						strstr(monster_list[i].rname, "MUMMY") != NULL ||
						strstr(monster_list[i].rname, "HYDRA") != NULL ||
						strstr(monster_list[i].rname, "IMP") != NULL ||
						strstr(monster_list[i].rname, "SORCERER") != NULL ||
						strstr(monster_list[i].rname, "NECROMANCER") != NULL ||
						strstr(monster_list[i].rname, "OKTA") != NULL ||
						strstr(monster_list[i].rname, "SORB") != NULL)
					{

						raction = random_num(2) + 1;

						if (raction == 1)
							firetype = 1;
						else if (raction == 2)
							firetype = 2;
					}
					else if (strstr(monster_list[i].rname, "DEMON") != NULL ||
						strstr(monster_list[i].rname, "DEMONESS") != NULL ||
						strstr(monster_list[i].rname, "FAERIE") != NULL)
					{

						raction = random_num(3) + 1;

						if (raction == 1)
							firetype = 1;
						else if (raction == 2)
							firetype = 2;
						else if (raction == 3)
							firetype = 3;
					}

					if (monster_list[i].dist > 200.0f)
					{
						if (monster_list[i].firespeed == 0 && firetype > 0 && monster_list[i].ability != 7)
						{
							mspeed = (int)25 - (int)monster_list[i].hd;

							if (mspeed < 6)
								mspeed = 6;

							monster_list[i].firespeed = mspeed;
							FireMonsterMissle(i, firetype);
						}
					}
				}
			}

			if (monster_list[i].current_sequence != 1)
			{
				skipmonster = 1;
			}

			XMVECTOR final, final2;
			final = XMVector3Normalize(vDiff2);
			final2 = XMVector3Normalize(XMLoadFloat3(&normroadold));
			XMVECTOR fresult = XMVector3Dot(final, final2);
			float fDot = XMVectorGetX(fresult);

			float convangle;
			convangle = (float)acos(fDot) / k;
			raction = random_num(1);
			if (raction == 0)
				raction = random_num(5);
			else
				raction = -1 * random_num(5);

			fDot = convangle;

			if (vw2.z < vw1.z)
			{
			}
			else
			{
				fDot = 180.0f + (180.0f - fDot);
			}

			if (monster_list[i].ability == 4)
			{
				monster_list[i].current_frame = 0;
				monster_list[i].current_sequence = 0;
				skipmonster = 1;
			}
			else
			{

				monster_list[i].rot_angle = fDot;
			}


			if (monster_list[i].ability == 6 || monster_list[i].ability == 7)
			{
				//shot only
				skipmonster = 1;
			}

			if (skipmonster == 0)
			{

				collidenow.x = monster_list[i].x;
				collidenow.y = monster_list[i].y;
				collidenow.z = monster_list[i].z;

				//inarea = collidenow + final * 2.0f;
				//inarea2.x = inarea.z;
				//inarea2.z = inarea.x;
				//inarea2.y = inarea.y;


				MakeBoundingBox();
				/*
				eRadius = D3DVECTOR((float)monsterx[cullloop], monsterheight[cullloop], (float)monsterx[cullloop]);

				savepos = collidenow;

				float realspeed;

				realspeed = (monster_list[i].speed * 10.0f) * fTimeKeysave;

				if (monster_list[i].health <= 2 && monster_list[i].health != monster_list[i].hp && (monster_list[i].hd < player_list[trueplayernum].hd))
				{
					//RUN  AWAY!!!!!!!!!!!!!!!!!!!
					final = final * -1;
				}

				D3DVECTOR a = final * realspeed;

				result = collideWithWorld(collidenow / eRadius, (final * realspeed) / eRadius);
				result = result * eRadius;
				*/

				//float realspeed = 200.0f * fElapsedTime;

				float realspeed = (165.0f + (monster_list[i].speed * 2.0f)) * fElapsedTime;

				XMVECTOR result = final * realspeed;

				XMFLOAT3 vfinal;
				XMStoreFloat3(&vfinal, result);

				collidenow.x = monster_list[i].x;
				collidenow.y = monster_list[i].y;
				collidenow.z = monster_list[i].z;

				//TODO: FIX THIS
				//result.y = result.y + 10.0f * fElapsedTime;
				vfinal = collideWithWorld(RadiusDivide(collidenow, eRadius), RadiusDivide(vfinal, eRadius));
				vfinal = RadiusMultiply(vfinal, eRadius);

				monster_list[i].x = vfinal.x;
				monster_list[i].y = vfinal.y;
				monster_list[i].z = vfinal.z;

				MakeBoundingBox();
			}


			if (monster_list[i].current_sequence == 1)
			{
				XMFLOAT3 vfinal;

				vfinal.x = 0.0f;
				//fixed april 2005
				vfinal.y = (float)(-300.0f) * fElapsedTime;
				vfinal.z = 0.0f;

				collidenow.x = monster_list[i].x;
				collidenow.y = monster_list[i].y;
				collidenow.z = monster_list[i].z;

				result = collideWithWorld(RadiusDivide(collidenow, eRadius), RadiusDivide(vfinal, eRadius));
				result = RadiusMultiply(result, eRadius);

				if (isnan(result.x)) {
					int a = 1;
				}

				monster_list[i].x = result.x;
				monster_list[i].y = result.y;
				monster_list[i].z = result.z;

				MakeBoundingBox();
			}
		}
	}

	currentmonstercollisionid = -1;

}

XMFLOAT3 RadiusDivide(XMFLOAT3 vector, XMFLOAT3 eRadius) {

	XMFLOAT3  result;

	result.x = vector.x / eRadius.x;
	result.y = vector.y / eRadius.y;
	result.z = vector.z / eRadius.z;

	return result;
}

XMFLOAT3 RadiusMultiply(XMFLOAT3  eRadius, XMFLOAT3 vector) {

	XMFLOAT3  result;

	result.x = vector.x * eRadius.x;
	result.y = vector.y * eRadius.y;
	result.z = vector.z * eRadius.z;

	return result;
}

int CalculateView(XMFLOAT3 EyeBall, XMFLOAT3 LookPoint, float angle, bool distancecheck)
{

	XMFLOAT3 vw1, vw2, work2, work1;// vDiff, normroadold;
	float fDot;
	float convangle;
	float anglework = 0;
	int monsteron = 0;

	if (distancecheck) {
		float qdist = FastDistance(EyeBall.x - LookPoint.x, EyeBall.y - LookPoint.y,
			EyeBall.z - LookPoint.z);

		if (qdist < 170.0f) {
			monsteron = 1;
			return monsteron;
		}
	}

	work2.x = EyeBall.x;
	work2.y = EyeBall.y;
	work2.z = EyeBall.z;

	work1.x = LookPoint.x;
	work1.y = LookPoint.y;
	work1.z = LookPoint.z;
	XMVECTOR vDiff = XMLoadFloat3(&work1) - XMLoadFloat3(&work2);

	//D3DXVec3Normalize(&vDiff, &vDiff);
	vDiff = XMVector3Normalize(vDiff);


	vw1.x = work1.x;
	vw1.y = (float)0;
	vw1.z = work1.z;

	vw2.x = work2.x;
	vw2.y = (float)0;
	vw2.z = work2.z;

	vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);

	XMVECTOR normroadold = XMVectorSet(50, 0, 0, 0);

	//normroadold.x = 50;
	//normroadold.y = 0;
	//normroadold.z = 0;

	//D3DXVec3Normalize(&final, &vDiff);
	XMVECTOR r1 = XMVector3Normalize(vDiff);
	XMVECTOR r2 = XMVector3Normalize(normroadold);

	//D3DXVec3Normalize(&final2, &normroadold);
	//fDot = D3DXVec3Dot(&final, &final2);

	fDot = XMVectorGetX(XMVector3Dot(r1, r2));

	convangle = (float)acos(fDot) / k;

	fDot = convangle;

	if (vw2.z < vw1.z)
	{
	}
	else
	{
		fDot = (180.0f + (180.0f - fDot));
	}

	fDot = (360.00f) - fDot;

	anglework = fixangle(fDot, +90.0f);

	if (angy + angle >= 360.0f)
	{
		if (anglework <= fixangle(angy, +angle) && anglework >= 0.0f)
		{
			monsteron = 1;
		}
		if (anglework >= fixangle(angy, -angle) && anglework <= 360.0f)
		{
			monsteron = 1;
		}
	}
	else if (angy - angle <= 0.0f)
	{
		if (anglework <= fixangle(angy, +angle) && anglework >= 0.0f)
		{
			monsteron = 1;
		}
		if (anglework >= fixangle(angy, -angle) && anglework <= 360.0f)
		{
			monsteron = 1;
		}
	}
	else
	{

		if (anglework <= fixangle(angy, +angle) && anglework >= fixangle(angy, -angle))
			monsteron = 1;
	}

	return monsteron;
}

int CalculateViewMonster(XMFLOAT3 EyeBall, XMFLOAT3 LookPoint, float angle, float angy)
{

	XMFLOAT3 vw1, vw2, work2, work1;//, vDiff, normroadold;
	float fDot;
	float convangle;
	float anglework = 0;
	int monsteron = 0;

	work2.x = EyeBall.x;
	work2.y = EyeBall.y;
	work2.z = EyeBall.z;

	work1.x = LookPoint.x;
	work1.y = LookPoint.y;
	work1.z = LookPoint.z;
	//vDiff = work1 - work2;

	XMVECTOR vDiff = XMLoadFloat3(&work1) - XMLoadFloat3(&work2);

	//D3DXVec3Normalize(&vDiff, &vDiff);
	vDiff = XMVector3Normalize(vDiff);
	//D3DXVec3Normalize(&vDiff, &vDiff);

	//vDiff = Normalize(vDiff);

	vw1.x = work1.x;
	vw1.y = (float)0;
	vw1.z = work1.z;

	vw2.x = work2.x;
	vw2.y = (float)0;
	vw2.z = work2.z;

	//vDiff = vw1 - vw2;

	vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
	XMVECTOR normroadold = XMVectorSet(50, 0, 0, 0);
	//D3DXVec3Normalize(&final, &vDiff);

	//final = Normalize(vDiff);
	//D3DXVec3Normalize(&final2, &normroadold);
	//final2 = Normalize(normroadold);
	//fDot = D3DXVec3Dot(&final, &final2);


	XMVECTOR r1 = XMVector3Normalize(vDiff);
	XMVECTOR r2 = XMVector3Normalize(normroadold);

	//D3DXVec3Normalize(&final2, &normroadold);
	//fDot = D3DXVec3Dot(&final, &final2);

	fDot = XMVectorGetX(XMVector3Dot(r1, r2));


	//fDot = dot(final, final2);

	convangle = (float)acos(fDot) / k;

	fDot = convangle;

	if (vw2.z < vw1.z)
	{
	}
	else
	{
		fDot = (180.0f + (180.0f - fDot));
	}

	anglework = fDot;

	if (angy + angle >= 360.0f)
	{
		if (anglework <= fixangle(angy, +angle) && anglework >= 0.0f)
		{
			monsteron = 1;
		}
		if (anglework >= fixangle(angy, -angle) && anglework <= 360.0f)
		{
			monsteron = 1;
		}
	}
	else if (angy - angle <= 0.0f)
	{
		if (anglework <= fixangle(angy, +angle) && anglework >= 0.0f)
		{
			monsteron = 1;
		}
		if (anglework >= fixangle(angy, -angle) && anglework <= 360.0f)
		{
			monsteron = 1;
		}
	}
	else
	{

		if (anglework <= fixangle(angy, +angle) && anglework >= fixangle(angy, -angle))
			monsteron = 1;
	}

	return monsteron;
}

void WakeUpMonsters()
{
	int ob_type = 0;
	int monsteron = 0;
	float qdist = 0.0f;
	int angle = 0;
	monstercount = 0;

	for (int q = 0; q < oblist_length; q++)
	{

		float wx = oblist[q].x;
		float wy = oblist[q].y;
		float wz = oblist[q].z;

		angle = (int)oblist[q].rot_angle;
		ob_type = oblist[q].type;

		//if (ob_type == 6)
		//{
		//	qdist = FastDistance(
		//		player_list[trueplayernum].x - wx,
		//		player_list[trueplayernum].y - wy,
		//		player_list[trueplayernum].z - wz);
		//	if (qdist <= 1600.0f)
		//		ObjectToD3DVertList(ob_type, angle, q);
		//}

		int montry = 0;

		if (strcmp(oblist[q].name, "!monster1") == 0 && oblist[q].monstertexture > 0 && monsterenable == 1)
		{



			for (montry = 0; montry < num_monsters; montry++)
			{
				if (oblist[q].monsterid == monster_list[montry].monsterid)
				{
					qdist = FastDistance(
						player_list[trueplayernum].x - monster_list[montry].x,
						player_list[trueplayernum].y - monster_list[montry].y,
						player_list[trueplayernum].z - monster_list[montry].z);

					monster_list[montry].dist = qdist;

					if (qdist < (numberofsquares * monsterdist) / 2)
					{
						wx = monster_list[montry].x;
						wy = monster_list[montry].y;
						wz = monster_list[montry].z;

						D3DVECTOR work1;

						work1.x = monster_list[montry].x;
						work1.y = monster_list[montry].y;
						work1.z = monster_list[montry].z;

						monsteron = SceneInBox(work1);

						if (monsteron)
						{
							monstertype[monstercount] = 0;
							monsterobject[monstercount] = (int)monster_list[montry].model_id;
							monsterangle[monstercount] = (int)monster_list[montry].rot_angle;
							monstercull[monstercount] = oblist[q].monsterid;
							monstercount++;
							monster_list[montry].volume = 100 - (int)((100 * qdist) / ((numberofsquares * monsterdist) / 2));
							if (monster_list[montry].volume > 100)
								monster_list[montry].volume = 100;

							if (monster_list[montry].volume < 10)
								monster_list[montry].volume = 10;
						}

						break;
					}
				}
			}
		}

		if (strcmp(oblist[q].name, "!monster1") == 0 && oblist[q].monstertexture == -1 && monsterenable == 1)
		{


			// 3ds monster
			for (montry = 0; montry < itemlistcount; montry++)
			{
				if (oblist[q].monsterid == item_list[montry].monsterid && item_list[montry].bIsPlayerAlive == TRUE)
				{
					qdist = FastDistance(
						player_list[trueplayernum].x - item_list[montry].x,
						player_list[trueplayernum].y - item_list[montry].y,
						player_list[trueplayernum].z - item_list[montry].z);

					item_list[montry].dist = qdist;

					if (qdist < (numberofsquares * monsterdist) / 2)
					{
						wx = item_list[montry].x;
						wy = item_list[montry].y;
						wz = item_list[montry].z;

						D3DVECTOR work1;

						work1.x = item_list[montry].x;
						work1.y = item_list[montry].y;
						work1.z = item_list[montry].z;

						monsteron = SceneInBox(work1);
						if (monsteron)
						{
							monstertype[monstercount] = 2;
							monsterobject[monstercount] = item_list[montry].model_id;
							monsterangle[monstercount] = (int)item_list[montry].rot_angle;
							monstercull[monstercount] = oblist[q].monsterid;
							monstercount++;
						}

						break;
					}
				}
			}
		}

		if (strcmp(oblist[q].name, "!monster1") == 0 && oblist[q].monstertexture == 0 && monsterenable == 1)
		{



			for (montry = 0; montry < num_players2; montry++)
			{
				if (oblist[q].monsterid == player_list2[montry].monsterid)
				{
					qdist = FastDistance(
						player_list[trueplayernum].x - player_list2[montry].x,
						player_list[trueplayernum].y - player_list2[montry].y,
						player_list[trueplayernum].z - player_list2[montry].z);
					player_list2[montry].dist = qdist;

					if (qdist < (numberofsquares * monsterdist) / 2)
					{
						wx = player_list2[montry].x;
						wy = player_list2[montry].y;
						wz = player_list2[montry].z;

						D3DVECTOR work1;

						work1.x = player_list2[montry].x;
						work1.y = player_list2[montry].y;
						work1.z = player_list2[montry].z;
						int monsteron;

						monsteron = SceneInBox(work1);

						if (monsteron)
						{
							monstertype[monstercount] = 1;
							monsterobject[monstercount] = player_list2[montry].model_id;
							monsterangle[monstercount] = (int)player_list2[montry].rot_angle;
							monstercull[monstercount] = oblist[q].monsterid;
							monstercount++;
						}
						break;
					}
				}
			}
		}
	}
}

int SceneInBox(D3DVECTOR point)
{

	float xp[5];
	float yp[5];

	float mx[5];

	float mz[5];

	float eyex;
	float eyez;
	float boxsize = 810.0f;
	float pntx;
	float pnty;
	int result;
	float xx, zz;
	float cosine, sine;

	eyex = m_vLookatPt.x;
	eyez = m_vLookatPt.z;

	eyex = 0;
	eyez = 0;

	int angle = 0;

	angle = (int)360 - (int)angy;

	cosine = cos_table[angle];
	sine = sin_table[angle];

	//left side of me
	xp[0] = eyex - 1100.0f;
	xp[1] = eyex - 800.0f;
	//right side of me
	xp[2] = eyex + 800.0f;
	xp[3] = eyex + 1100.0f;

	//behind me
	yp[1] = eyez - 900.0f;
	yp[2] = eyez - 900.0f;

	//front of me
	yp[3] = eyez + 1900.0f;
	yp[0] = eyez + 1900.0f;

	pntx = point.x;
	pnty = point.z;

	xx = m_vLookatPt.x;
	zz = m_vLookatPt.z;

	mx[0] = xx + (xp[0] * cosine - yp[0] * sine);
	mz[0] = zz + (xp[0] * sine + yp[0] * cosine);

	mx[1] = xx + (xp[1] * cosine - yp[1] * sine);
	mz[1] = zz + (xp[1] * sine + yp[1] * cosine);

	mx[2] = xx + (xp[2] * cosine - yp[2] * sine);
	mz[2] = zz + (xp[2] * sine + yp[2] * cosine);

	mx[3] = xx + (xp[3] * cosine - yp[3] * sine);
	mz[3] = zz + (xp[3] * sine + yp[3] * cosine);

	showview[0].x = mx[2];
	showview[0].y = m_vLookatPt.y;
	showview[0].z = mz[2];
	showview[0].nx = 0;
	showview[0].ny = 1.0f;
	showview[0].nz = 0;
	showview[0].tu = 0;
	showview[0].tv = 0;

	showview[1].x = mx[1];
	showview[1].y = m_vLookatPt.y;
	showview[1].z = mz[1];
	showview[1].nx = 0;
	showview[1].ny = 1.0f;
	showview[1].nz = 0;
	showview[1].tu = 1.0f;
	showview[1].tv = 0;

	showview[2].x = mx[3];
	showview[2].y = m_vLookatPt.y;
	showview[2].z = mz[3];
	showview[2].nx = 0;
	showview[2].ny = 1.0f;
	showview[2].nz = 0;
	showview[2].tu = 0;
	showview[2].tv = 1.0f;

	showview[3].x = mx[0];
	showview[3].y = m_vLookatPt.y;
	showview[3].z = mz[0];
	showview[3].nx = 0;
	showview[3].ny = 1.0f;
	showview[3].nz = 0;
	showview[3].tu = 1.0f;
	showview[3].tv = 1.0f;

	result = pnpoly(4, mx, mz, pntx, pnty);

	if (result)
		return 1;

	return 0;
}

int pnpoly(int npol, float* xp, float* yp, float x, float y)
{
	int i, j, c = 0;

	for (i = 0, j = npol - 1; i < npol; j = i++)
	{
		if ((((yp[i] <= y) && (y < yp[j])) ||
			((yp[j] <= y) && (y < yp[i]))) &&
			(x < (xp[j] - xp[i]) * (y - yp[i]) / (yp[j] - yp[i]) + xp[i]))

			c = !c;
	}
	return c;
}

int OpenDoor(int doornum, float dist, FLOAT fTimeKey)
{

	int i = 0;
	int j = 0;
	float angle;
	int direction = +5;
	int flag = 1;
	int senddoorinfo = 0;
	int savedoorspot = 0;

	savedoorspot = 0;

	for (int i = 0; i < doorcounter; i++)
	{
		if (door[i].doornum == doornum)
		{

			savedoorspot = i;
			if (door[i].type == 1)
			{

				return 2;
			}

			if (door[i].open == 0 && listendoor == 1 && dist <= 100.0f ||
				door[i].open == 2 && listendoor == 1 && dist <= 100.0f

				)
			{

				if (door[i].listen == 0)
				{
					//dice[2].roll = 1;
					numdice = 3;
					door[i].listen = 1;
					listendoor = 0;
					//int action = random_num(dice[2].sides) + 1;
					//dice[2].roll = 0;
					//sprintf_s(dice[2].name, "%ss%d", dice[2].prefix, action);
					//PlayWavSound(SoundID("diceroll"), 100);
					//showlisten = 1;
					//if (action > 2)
					//{
						//listenfailed = 1;
						//strcpy_s(gActionMessage, "Listen at door failed.");
						//UpdateScrollList(0, 255, 255);
					//}
					//else
					//{

						//listenfailed = 0;
						//strcpy_s(gActionMessage, "Listening at the door reveals...");
						//UpdateScrollList(0, 255, 255);
						//ListenAtDoor();
					//}
				}
			}

			if (door[i].open == 0 && pressopendoor == 1 && dist <= 170.0f ||
				door[i].open == 2 && pressopendoor == 1 && dist <= 170.0f)
			{

				flag = 1;
				if (door[i].key != 0 && door[i].key != -1 || door[i].key == -2)
				{
					strcpy_s(gActionMessage, "This door is locked");
					UpdateScrollList(255, 255, 0);
					flag = 0;

					if (player_list[trueplayernum].keys > 0)
					{
						flag = 1;
						strcpy_s(gActionMessage, "You unlocked the door with a key.");

						if (door[i].key > 0)
							door[i].key = 0;

						UpdateScrollList(255, 255, 0);
						player_list[trueplayernum].keys--;

						if (player_list[trueplayernum].keys < 0)
							player_list[trueplayernum].keys = 0;
					}
				}
				if (flag)
				{

					if (door[i].open == 0)
						door[i].open = 1;
					if (door[i].open == 2)
						door[i].open = 3;
					PlayWavSound(SoundID("dooropen"), 100);
					if (door[i].key == -1 || door[i].key == -2)
					{
						//lift the door instead
						PlayWavSound(SoundID("stone"), 100);
						door[i].open = -99;
					}

					senddoorinfo = 1;
					for (j = 0; j < num_monsters; j++)
					{
						float qdist = FastDistance(
							oblist[doornum].x - monster_list[j].x,
							oblist[doornum].y - monster_list[j].y,
							oblist[doornum].z - monster_list[j].z);
						if (qdist < (10 * monsterdist) / 2)
						{
							if (monster_list[j].bIsPlayerValid == TRUE && monster_list[j].ability == 1)
							{
								monster_list[j].bStopAnimating = FALSE;
								monster_list[j].ability = 0;
								if (strcmp(monster_list[j].rname, "SLAVE") == 0)
								{
									sprintf_s(gActionMessage, "You found a slave. 50 xp.");
									UpdateScrollList(255, 0, 255);
									player_list[trueplayernum].xp += 50;
								}
							}
						}
					}
				}
			}

			float openspeed = 5.0f;

			if (door[i].open == -99)
			{
				float up;
				up = 10.0f;
				//Moveup - door goes upward
				up = (125.0f * fTimeKey);

				oblist[doornum].y = oblist[doornum].y + up;
				door[i].up += up;

				if (door[i].up > 160.0f)
				{
					senddoorinfo = 0;
					door[i].open = -100;
					door[i].y = (int)oblist[doornum].y;
				}
			}
			else
			{

				if (door[i].saveangle == 0 || door[i].saveangle == 270)
				{

					angle = door[i].angle;

					if (door[i].open == 3)
					{

						if (angle > door[i].saveangle)
						{
							angle = angle - 105.0f * fTimeKey;

							if (angle < 0)
								angle = 0;

							oblist[doornum].rot_angle = (float)angle;
							door[i].angle = angle;

						}
						else
						{
							door[i].open = 0;
						}
					}
					else if (door[i].open == 1)
					{
						if (angle < door[i].saveangle + 89.0f)
						{
							angle = angle + 105.0f * fTimeKey;

							if (angle > 359.0f)
								angle = 359;

							oblist[doornum].rot_angle = (float)angle;
							door[i].angle = angle;
							door[i].open = 1;
						}
						else
						{
							door[i].open = 2;
						}
					}
				}
				else
				{

					angle = door[i].angle;

					if (door[i].open == 3)
					{

						if (angle < door[i].saveangle)
						{
							angle = angle + 105.0f * fTimeKey;

							if (angle > 359.0f)
								angle = 359;

							oblist[doornum].rot_angle = (float)angle;
							door[i].angle = angle;
						}
						else
						{
							door[i].open = 0;
						}
					}
					else if (door[i].open == 1)
					{
						if (angle > door[i].saveangle - 89.0f)
						{
							angle = angle - 105.0f * fTimeKey;

							if (angle < 0)
								angle = 0;

							oblist[doornum].rot_angle = (float)angle;
							door[i].angle = angle;
							door[i].open = 1;
						}
						else
						{
							door[i].open = 2;
						}
					}
				}
			}

		}
	}

	return 1;
}

void MakeBoundingBox()
{

	int montry = 0;
	int cullloop = 0;
	int cullflag = 0;
	int i;
	countboundingbox = 0;

	//if (excludebox == 0)
	/*for (int i = 0; i < num_players; i++)
	{
		cullflag = 0;
		for (cullloop = 0; cullloop < monstercount; cullloop++)
		{
			if (monstercull[cullloop] == (int)i + (int)999)
			{
				if (monstertype[cullloop] == 5 && i != trueplayernum && player_list[i].bIsPlayerAlive == TRUE)
				{

					wx = player_list[i].x;
					wy = player_list[i].y;
					wz = player_list[i].z;

					PlayerNonIndexedBox(0, 0, (int)player_list[i].rot_angle);
					monsterheight[cullloop] = objectheight;
					monsterx[cullloop] = objectx;
					monsterz[cullloop] = objectz;
				}
				break;
			}
		}
	}*/

	for (i = 0; i < num_players2; i++)
	{
		cullflag = 0;
		for (cullloop = 0; cullloop < monstercount; cullloop++)
		{
			if (monstercull[cullloop] == player_list2[i].monsterid)
			{
				if (monstertype[cullloop] == 1)
				{

					float wx = player_list2[i].x;
					float wy = player_list2[i].y;
					float wz = player_list2[i].z;

					PlayerIndexedBox(monsterobject[cullloop], 0, (int)player_list2[i].rot_angle, wx, wy, wz);

					//monsterheight[cullloop] = objectheight;
					//monsterx[cullloop] = objectx;
					//monsterz[cullloop] = objectz;
				}
				break;
			}
		}
	}

	//heee
	//if (excludebox == 0) {
		for (i = 0; i < num_monsters; i++)
		{
			cullflag = 0;
			for (cullloop = 0; cullloop < monstercount; cullloop++)
			{

				if (monstercull[cullloop] == monster_list[i].monsterid)
				{

					if (monster_list[i].bIsPlayerAlive == TRUE &&
						monster_list[i].bIsPlayerValid == TRUE)
					{

						if (monstertype[cullloop] == 0 && currentmonstercollisionid != monster_list[i].monsterid)
							//if (monstertype[cullloop] == 0 )
						{

							float wx = monster_list[i].x;
							float wy = monster_list[i].y;
							float wz = monster_list[i].z;

							PlayerNonIndexedBox(monsterobject[cullloop], 0, (int)monster_list[i].rot_angle, wx, wy, wz);
							//monstertrueheight[cullloop] = objecttrueheight;
							//monsterheight[cullloop] = objectheight;
							//monsterx[cullloop] = objectx;
							//monsterz[cullloop] = objectz;
						}
					}
					break;
				}
			}
		//}
	}
}

void PlayerNonIndexedBox(int pmodel_id, int curr_frame, int angle, float wx, float wy, float wz)
{
	int i, j;
	int num_verts_per_poly;
	int num_poly;
	int i_count;
	short v_index;
	float x, y, z;
	float rx, ry, rz;
	float tx, ty;
	int count_v;

	float min_x, min_y, min_z;
	float max_x, max_y, max_z;

	float x_off = 0;
	float y_off = 0;
	float z_off = 0;

	min_x = wx;
	min_y = wy;
	min_z = wz;
	max_x = wx;
	max_y = wy;
	max_z = wz;

	vert_ptr tp;

	D3DPRIMITIVETYPE p_command;
	BOOL error = TRUE;

	if (angle >= 360)
		angle = angle - 360;
	if (angle < 0)
		angle += 360;

	//float cosine = cos_table[angle];
	//float sine = sin_table[angle];

	float cosine = (float)cos(angle * k);
	float sine = (float)sin(angle * k);

	if (curr_frame >= pmdata[pmodel_id].num_frames)
		curr_frame = 0;

	i_count = 0;

	num_poly = pmdata[pmodel_id].num_polys_per_frame;

	for (i = 0; i < num_poly; i++)
	{
		p_command = pmdata[pmodel_id].poly_cmd[i];
		num_verts_per_poly = pmdata[pmodel_id].num_vert[i];

		count_v = 0;

		for (j = 0; j < num_verts_per_poly; j++)
		{
			v_index = pmdata[pmodel_id].f[i_count];

			tp = &pmdata[pmodel_id].w[curr_frame][v_index];
			x = tp->x + x_off;
			z = tp->y + y_off;
			y = tp->z + z_off;

			rx = wx + (x * cosine - z * sine);
			ry = wy + y;
			rz = wz + (x * sine + z * cosine);

			tx = pmdata[pmodel_id].t[i_count].x * pmdata[pmodel_id].skx;
			ty = pmdata[pmodel_id].t[i_count].y * pmdata[pmodel_id].sky;
			if (D3DVAL(rx) > max_x)
				max_x = D3DVAL(rx);
			if (D3DVAL(rx) < min_x)
				min_x = D3DVAL(rx);

			if (D3DVAL(ry) > max_y)
				max_y = D3DVAL(ry);
			if (D3DVAL(ry) < min_y)
				min_y = D3DVAL(ry);

			if (D3DVAL(rz) > max_z)
				max_z = D3DVAL(rz);
			if (D3DVAL(rz) < min_z)
				min_z = D3DVAL(rz);

			i_count++;
		}
	}

	float objectheight = max_y - min_y;
	float objectx = max_x - min_x;
	float objectz = max_z - min_z;
	float objecty = max_y - min_y;

	if (objectx > 80.0f || objectz > 80.0f)
	{
		objectx = 35.0f;
	}
	else
	{
		objectx = 20.0f;
	}

	float objecttrueheight = objectheight;
	if (objectheight < 150.0f)
	{
		objectheight = 50.0f;
	}
	else
	{
		objectheight = 70.0f;
	}

	min_x = wx - objectx;
	max_x = wx + objectx;

	min_y = wy - objecty / 2;
	max_y = wy + objecty / 2;

	min_z = wz - objectx;
	max_z = wz + objectx;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	//2nd

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	//3rd

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	//4th
	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;
	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;
	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	//5th

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 1;

	countboundingbox++;

	return;
}

void PlayerIndexedBox(int pmodel_id, int curr_frame, int angle, float wx, float wy, float wz)
{
	int i, j;
	int num_verts_per_poly;
	int num_faces_per_poly;
	int num_poly;
	int poly_command;
	int i_count, face_i_count;
	float x, y, z;
	float rx, ry, rz;
	float tx, ty;
	int count_v = 0;

	float min_x, min_y, min_z;
	float max_x, max_y, max_z;

	float x_off = 0;
	float y_off = 0;
	float z_off = 0;

	min_x = wx;
	min_y = wy;
	min_z = wz;
	max_x = wx;
	max_y = wy;
	max_z = wz;

	if (curr_frame >= pmdata[pmodel_id].num_frames)
		curr_frame = 0;

	curr_frame = 0;
	//float cosine = cos_table[angle];
	//float sine = sin_table[angle];

	float cosine = (float)cos(angle * k);
	float sine = (float)sin(angle * k);

	i_count = 0;
	face_i_count = 0;

	//	if(rendering_first_frame == TRUE)
	//		fp = fopen("ds.txt","a");

	// process and transfer the model data from the pmdata structure
	// to the array of D3DVERTEX2 structures, src_v

	num_poly = pmdata[pmodel_id].num_polys_per_frame;

	for (i = 0; i < num_poly; i++)
	{
		poly_command = pmdata[pmodel_id].poly_cmd[i];
		num_verts_per_poly = pmdata[pmodel_id].num_verts_per_object[i];
		num_faces_per_poly = pmdata[pmodel_id].num_faces_per_object[i];
		count_v = 0;
		for (j = 0; j < num_verts_per_poly; j++)
		{

			x = pmdata[pmodel_id].w[curr_frame][i_count].x + x_off;
			z = pmdata[pmodel_id].w[curr_frame][i_count].y + y_off;
			y = pmdata[pmodel_id].w[curr_frame][i_count].z + z_off;

			rx = wx + (x * cosine - z * sine);
			ry = wy + y;
			rz = wz + (x * sine + z * cosine);

			tx = pmdata[pmodel_id].t[i_count].x * pmdata[pmodel_id].skx;
			ty = pmdata[pmodel_id].t[i_count].y * pmdata[pmodel_id].sky;

			if (D3DVAL(rx) > max_x)
				max_x = D3DVAL(rx);
			if (D3DVAL(rx) < min_x)
				min_x = D3DVAL(rx);

			if (D3DVAL(ry) > max_y)
				max_y = D3DVAL(ry);
			if (D3DVAL(ry) < min_y)
				min_y = D3DVAL(ry);

			if (D3DVAL(rz) > max_z)
				max_z = D3DVAL(rz);
			if (D3DVAL(rz) < min_z)
				min_z = D3DVAL(rz);

			i_count++;

		} // end for j

	} // end for vert_cnt

	// got min & max make box

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;


	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	//2nd

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	//3rd

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;
	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	//4th
	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;
	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;
	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;
	boundingbox[countboundingbox].x = max_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	//5th

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = max_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = max_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	boundingbox[countboundingbox].x = min_x;
	boundingbox[countboundingbox].y = min_y;
	boundingbox[countboundingbox].z = min_z;
	boundingbox[countboundingbox].monster = 0;

	countboundingbox++;

	//objectheight = max_y - min_y;
	//objectx = max_x - min_x;
	//objectz = max_z - min_z;


	return;
}

void MonsterHit()
{

	int j = 0;

	int i = 0;
	int cullflag = 0;
	int cullloop = 0;
	float qdist;
	int action;
	int actionnum = 0;
	int criticaldamage = 0;
	char savehp[200];

	int hitplayer = 0;

	if (player_list[trueplayernum].current_sequence != 2) {
		damageinprogress = 0;
	}

	if (player_list[trueplayernum].current_frame >= 50 && player_list[trueplayernum].current_frame <= 50 && !damageinprogress)
	{
		damageinprogress = 1;
		action = random_num(dice[0].sides) + 1;
		actionnum = action;
		dice[0].roll = 0;
		sprintf_s(dice[0].name, "%ss%d", dice[0].prefix, action);



		if (!hitmonster) {
			dice[1].roll = 0;
			sprintf_s(dice[1].name, "dieblank");
			d20roll = action;
		}
	}

	cullflag = 0;
	float lastdist = 99999;
	int savepos = -1;
	for (i = 0; i < num_monsters; i++)
	{

		for (cullloop = 0; cullloop < monstercount; cullloop++)
		{

			if (monstercull[cullloop] == monster_list[i].monsterid)
			{

				statusbardisplay((float)monster_list[i].hp, (float)monster_list[i].hp, 1);
				strcpy_s(savehp, statusbar);
				statusbardisplay((float)monster_list[i].health, (float)monster_list[i].hp, 0);

				for (j = 0; j < (int)strlen(statusbar); j++)
				{

					savehp[j] = statusbar[j];
				}

				//monsterhitdisplay

				sprintf_s(monster_list[i].chatstr, "%s AC:%d!HP:%d %s",
					monster_list[i].rname,
					player_list[trueplayernum].thaco - monster_list[i].ac,
					monster_list[i].health, savehp);

				if (monster_list[i].current_sequence != 5 &&
					monster_list[i].current_frame != 183 &&
					monster_list[i].current_frame != 189 &&
					monster_list[i].current_frame != 197 && monster_list[i].health > 0)
				{
					if (monster_list[i].dist < lastdist && monster_list[i].bIsPlayerAlive == TRUE &&
						strcmp(monster_list[i].rname, "SLAVE") != 0 &&
						strcmp(monster_list[i].rname, "CENTAUR") != 0)
					{
						cullflag = 1;
						lastdist = monster_list[i].dist;
						savepos = i;
						//monster_list[i].takedamageonce = 0;
					}
				}
			}
		}
	}

	if (savepos == -1)
	{

		//if (actionnum != 0)
//			PlayerHit(actionnum);
		return;
	}

	i = savepos;

	if (cullflag == 1)
	{

		float wx = monster_list[i].x;
		float wy = monster_list[i].y;
		float wz = monster_list[i].z;

		//if (perspectiveview == 0)
		//{
		//	qdist = FastDistance(
		//		m_vLookatPt.x - wx,
		//		m_vLookatPt.y - wy,
		//		m_vLookatPt.z - wz);
		//}
		/*else
		{*/
		qdist = FastDistance(
			m_vEyePt.x - wx,
			m_vEyePt.y - wy,
			m_vEyePt.z - wz);
		//}

		if (qdist < 100.0f && player_list[trueplayernum].current_frame >= 50 && player_list[trueplayernum].current_frame <= 50 && (monster_list[i].takedamageonce == 0))
		{

			XMFLOAT3 work1;

			hitplayer = 1;
			work1.x = wx;
			work1.y = wy;
			work1.z = wz;
			int monsteron = 0;

			monsteron = CalculateView(m_vEyePt, work1, 45.0f, false);
			if (player_list[trueplayernum].current_sequence != 5 && monsteron)
			{

				action = random_num(dice[0].sides) + 1;
				//action = random_num(20) + 1;
				actionnum = action;
				if (action == 20)
					criticaldamage = 1;

				dice[0].roll = 0;
				sprintf_s(dice[0].name, "%ss%d", dice[0].prefix, action);

				d20roll = action;

				int a;

				int gunmodel = 0;
				for (a = 0; a < num_your_guns; a++)
				{

					if (your_gun[a].model_id == player_list[trueplayernum].gunid)
					{

						gunmodel = a;
					}
				}

				int attackbonus = your_gun[gunmodel].sattack;
				int damagebonus = your_gun[gunmodel].sdamage;
				int weapondamage = your_gun[gunmodel].damage2;
				action = action + attackbonus;
				//No damage if the mosnter is already in pain.
				if (action >= player_list[trueplayernum].thaco - monster_list[i].ac)
				{
					monster_list[i].takedamageonce = 1;

					action = random_num(weapondamage) + 1;

					damageroll = action;

					int painaction = random_num(3);
					SetMonsterAnimationSequence(i, 3 + painaction);

					dice[1].roll = 0;
					sprintf_s(dice[1].name, "%ss%d", dice[1].prefix, action);

					int bloodspot = -1;

					if (criticaldamage == 1)
					{

						action = (action + (damagebonus)) * 2;
						criticalhiton = 1;
						PlayWavSound(SoundID("natural20"), 100);
						sprintf_s(gActionMessage, "Wow. A natural 20, double damage.");
						UpdateScrollList(0, 255, 255);
						sprintf_s(gActionMessage, "You hit a  %s for %d damage", monster_list[i].rname, action);
						UpdateScrollList(0, 255, 255);
						bloodspot = DisplayDamage(monster_list[i].x, monster_list[i].y - 10.0f, monster_list[i].z, trueplayernum, i, true);
						hitmonster = 1;
					}
					else
					{

						//PlayWaveFile(".\\Sounds\\impact.wav");
						action = action + damagebonus;
						sprintf_s(gActionMessage, "You hit a %s for %d damage", monster_list[i].rname, action);
						UpdateScrollList(0, 255, 255);
						bloodspot = DisplayDamage(monster_list[i].x, monster_list[i].y - 10.0f, monster_list[i].z, trueplayernum, i, false);
						hitmonster = 1;
					}

					monster_list[i].health = monster_list[i].health - (int)action;
					monster_list[i].ability = 0;
					int t = 0;
					int cullloop2 = 0;
					// monstersee
					for (t = 0; t < num_monsters; t++)
					{
						for (cullloop2 = 0; cullloop2 < monstercount; cullloop2++)
						{
							if (monstercull[cullloop2] == monster_list[t].monsterid)
							{

								if (monster_list[t].ability == 2 &&
									monster_list[t].dist < 600.0f

									)
								{
									monster_list[t].ability = 0;
								}
							}
						}
					}

					PlayWavSound(monster_list[i].sattack, 100);

					if (monster_list[i].health <= 0)
					{
						if (bloodspot != -1)
						{
							//your_missle[bloodspot].y = your_missle[bloodspot].y - 30.0f;
							//your_missle[bloodspot].active = 0;
						}
						PlayWavSound(monster_list[i].sdie, 100);

						int deathaction = random_num(3);

						SetMonsterAnimationSequence(i, 12 + deathaction);
						monster_list[i].bIsPlayerAlive = FALSE;
						monster_list[i].bIsPlayerValid = FALSE;
						monster_list[i].health = 0;
						int gd = (random_num(monster_list[i].hd * 10) + monster_list[i].hd * 10) + 1;
						int xp = XpPoints(monster_list[i].hd, monster_list[i].hp);

						sprintf_s(gActionMessage, "You killed a %s worth %d xp.", monster_list[i].rname, xp);
						UpdateScrollList(255, 0, 255);
						player_list[trueplayernum].frags++;

						AddTreasure(monster_list[i].x, monster_list[i].y, monster_list[i].z, gd);

						player_list[trueplayernum].xp += xp;
						LevelUp(player_list[trueplayernum].xp);
					}

					statusbardisplay((float)monster_list[i].hp, (float)monster_list[i].hp, 1);
					strcpy_s(savehp, statusbar);
					statusbardisplay((float)monster_list[i].health, (float)monster_list[i].hp, 0);

					for (j = 0; j < (int)strlen(statusbar); j++)
					{

						savehp[j] = statusbar[j];
					}

					sprintf_s(monster_list[i].chatstr, "%s AC:%d!HP:%d %s",
						monster_list[i].rname,
						player_list[trueplayernum].thaco - monster_list[i].ac,
						monster_list[i].health, savehp);

					//return;
				}
			}
		}
	}

	//if (hitplayer == 0)
		//PlayerHit(actionnum);

}



void DrawPlayerGun(int shadow)
{
	// DRAW YOUR GUN ///////////////////////////////////////////

	BOOL use_player_skins_flag = false;
	int i = 0;
	float skx, sky;
	int ob_type;
	float angle;

	float wx = 0;
	float wy = 0;
	float wz = 0;
	int perspectiveview = 1;


	if (player_list[trueplayernum].model_id == 0 &&
		player_list[trueplayernum].bIsPlayerAlive &&
		strstr(your_gun[current_gun].gunname, "SCROLL") == NULL)
	{
		skx = (float)0.40000000 / (float)256;
		sky = (float)1.28000000 / (float)256;
		use_player_skins_flag = 1;

		i = current_gun;
		i = 0;
		if (current_gun < num_your_guns)
		{

			ob_type = player_list[trueplayernum].gunid;
			current_frame = player_list[trueplayernum].current_frame;
			angle = player_list[trueplayernum].gunangle;

			if (perspectiveview == 1)
			{
				weapondrop = 1;
				fDot2 = look_up_ang;
			}
			else
			{
				weapondrop = 0;
				fDot2 = 0.0f;
			}
			if (weapondrop == 0)
			{
				wx = player_list[trueplayernum].x;
				wy = player_list[trueplayernum].y;
				wz = player_list[trueplayernum].z;
			}
			else
			{
				float adjust = 10.0f;

				if (shadow == 1) {
					adjust = -15.0f;
					fDot2 = 0.0f;
				}
				
				wx = GunTruesave.x;
				wy = GunTruesave.y + adjust;// +bobY.getY();
				wz = GunTruesave.z;
				//wx = m_vEyePt.x;
				//wy = m_vEyePt.y + 50.0f;
				//wz = m_vEyePt.z;
			}

			int nextFrame = GetNextFramePlayer();

			PlayerToD3DVertList(ob_type,
				current_frame,
				angle,
				player_list[trueplayernum].guntex,
				USE_DEFAULT_MODEL_TEX, wx, wy, wz, nextFrame);
			fDot2 = 0.0f;
			weapondrop = 0;

			//				}
		}
	}
	else if (player_list[trueplayernum].model_id != 0 &&
		player_list[trueplayernum].bIsPlayerAlive &&
		strstr(your_gun[current_gun].gunname, "SCROLL") == NULL)
	{

		ob_type = player_list[trueplayernum].gunid;
		current_frame = player_list[trueplayernum].current_frame;
		angle = player_list[trueplayernum].gunangle;


		if (perspectiveview == 1)
		{
			weapondrop = 1;
			fDot2 = look_up_ang;
		}
		else
		{
			weapondrop = 0;
			fDot2 = 0.0f;
		}
		if (weapondrop == 0)
		{
			wx = player_list[trueplayernum].x;
			wy = player_list[trueplayernum].y;
			wz = player_list[trueplayernum].z;
		}
		else
		{

			wx = GunTruesave.x;
			wy = GunTruesave.y;
			wz = GunTruesave.z;

			//wx = m_vEyePt.x;
			//wy = m_vEyePt.y;
			//wz = m_vEyePt.z;

		}

		int getgunid = currentmodellist;

		if (strcmp(model_list[getgunid].monsterweapon, "NONE") != 0)
		{

			PlayerToD3DVertList(FindModelID(model_list[getgunid].monsterweapon),
				current_frame,
				angle,
				FindGunTexture(model_list[getgunid].monsterweapon),
				USE_DEFAULT_MODEL_TEX, wx, wy, wz);
		}
	}
}

void AddTreasure(float x, float y, float z, int gold)
{

	int i = 0;

	//fix this

	int raction;

	for (i = 0; i < MAX_NUM_ITEMS; i++)
	{

		if (item_list[i].bIsPlayerValid == FALSE)
		{

			if (gold == 0)
			{
				raction = 0;
			}
			else
			{
				raction = random_num(10);
			}

			if (raction == 0)
			{
				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y - 10.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("POTION");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 85;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = 0;

				strcpy_s(item_list[i].rname, "POTION");
			}
			else if (raction == 1)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y - 10.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("diamond");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 85;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = gold;

				strcpy_s(item_list[i].rname, "diamond");
			}
			else if (raction == 2)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y - 10.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("SCROLL-MAGICMISSLE-");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 0;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = gold;

				strcpy_s(item_list[i].rname, "SCROLL-MAGICMISSLE-");
			}
			else if (raction == 3)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y - 10.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("SCROLL-FIREBALL-");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 0;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = gold;

				strcpy_s(item_list[i].rname, "SCROLL-FIREBALL-");
			}
			else if (raction == 4)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y - 10.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("SCROLL-LIGHTNING-");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 0;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = gold;

				strcpy_s(item_list[i].rname, "SCROLL-LIGHTNING-");
			}
			else if (raction == 5)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y - 10.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("SCROLL-HEALING-");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 0;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = gold;

				strcpy_s(item_list[i].rname, "SCROLL-HEALING-");
			}
			else if (raction == 6)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y - 10.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("spellbook");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 0;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = gold;

				strcpy_s(item_list[i].rname, "spellbook");
			}
			else
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y - 10.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("COIN");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 85;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = gold;

				strcpy_s(item_list[i].rname, "COIN");
			}

			if (i >= itemlistcount)
				itemlistcount++;

			break;
		}
	}
}

int UpdateScrollList(int r, int g, int b)
{
	strcpy_s(scrolllist1[slistcounter].text, gActionMessage);
	scrolllist1[slistcounter].num = slistcounter;
	scrolllist1[slistcounter].r = r;
	scrolllist1[slistcounter].g = g;
	scrolllist1[slistcounter].b = b;
	sliststart = slistcounter;

	slistcounter++;

	if (slistcounter >= scrolllistnum)
	{
		slistcounter = 0;
	}

	return 1;
}

void DrawModel()
{
	int cullflag = 0;
	BOOL use_player_skins_flag = false;
	float qdist = 0.0f;
	int perspectiveview = 1;

	for (int i = 0; i < num_players2; i++)
	{

		if (player_list2[i].bIsPlayerValid == TRUE)
		{

			cullflag = 0;
			for (int cullloop = 0; cullloop < monstercount; cullloop++)
			{
				if (monstercull[cullloop] == player_list2[i].monsterid)
				{
					cullflag = 1;
					break;
				}
			}

			if (cullflag == 1)
			{

				float wx = player_list2[i].x;
				float wy = player_list2[i].y;
				float wz = player_list2[i].z;

				XMFLOAT3 work1;
				work1.x = wx;
				work1.y = wy;
				work1.z = wz;
				int monsteron = 0;

				monsteron = CalculateView(m_vEyePt, work1, cullAngle, true);

				if (monsteron)
				{

					use_player_skins_flag = 1;
					current_frame = player_list2[i].current_frame;

					if (perspectiveview == 0)
					{
						qdist = FastDistance(m_vLookatPt.x - player_list2[i].x,
							m_vLookatPt.y - player_list2[i].y,
							m_vLookatPt.z - player_list2[i].z);
					}
					else
					{
						qdist = FastDistance(m_vEyePt.x - player_list2[i].x,
							m_vEyePt.y - player_list2[i].y,
							m_vEyePt.z - player_list2[i].z);
					}



					if (player_list2[i].model_id == 36 && pressopendoor && qdist <= 100.0f ||
						strstr(player_list2[i].rname, "cdoorclosed") != NULL && pressopendoor && qdist <= 100.0f)
					{

						player_list2[i].model_id--;
						strcpy_s(player_list2[i].rname, "-");

						PlayWavSound(SoundID("creak1"), 100);

						int gd = 0;

						if (player_list2[i].ability == 1)
						{
							gd = random_num(100) + 50;
						}
						else if (player_list2[i].ability == 2)
						{
							gd = random_num(500) + 500;
						}
						else if (player_list2[i].ability == 0)
						{
							gd = 0;
						}

						AddTreasure(player_list2[i].x, player_list2[i].y + 20.0f, player_list2[i].z, gd);
					}

					PlayerToD3DVertList(player_list2[i].model_id,
						player_list2[i].current_frame, player_list2[i].rot_angle,
						player_list2[i].skin_tex_id,
						USE_DEFAULT_MODEL_TEX, wx, wy, wz);
				}
			}
		}
	}
}

void ApplyPlayerDamage(int playerid, int damage)
{

	int raction;
	int perspectiveview = 1;

	if (player_list[trueplayernum].bIsPlayerAlive == FALSE || player_list[trueplayernum].health <= 0)
		return;
	PlayWavSound(SoundID("pimpact"), 100);
	player_list[trueplayernum].health = player_list[trueplayernum].health - damage;
	PlayWavSound(SoundID("pain1"), 100);

	raction = (int)random_num(3);

	switch (raction)
	{

	case 0:
		SetPlayerAnimationSequence(trueplayernum, 3);// pain1
		break;
	case 1:
		SetPlayerAnimationSequence(trueplayernum, 4);// pain2
		break;
	case 2:
		SetPlayerAnimationSequence(trueplayernum, 5);// pain3
		break;
	}

	if (player_list[trueplayernum].health <= 0)
	{
		player_list[trueplayernum].health = 0;

		//raction = (int)random_num(3);

		//switch (raction)
		//{
		//case 0:
		//	SetPlayerAnimationSequence(trueplayernum, 12); // death1
		//	break;
		//case 1:
		//	SetPlayerAnimationSequence(trueplayernum, 13);// death2
		//	break;
		//case 2:
		//	SetPlayerAnimationSequence(trueplayernum, 14);// death3
		//	break;
		//}

		player_list[trueplayernum].bIsPlayerAlive = FALSE;

		if (perspectiveview == 1)
			m_vLookatPt = m_vEyePt;
		//m_vLookatPt=m_vEyePt;

		look_up_ang = -20;
		perspectiveview = 0;
		sprintf_s(gActionMessage, "Great Crom. You are dead!");
		UpdateScrollList(0, 255, 255);
		sprintf_s(gActionMessage, "Press SPACE to rise again...");
		UpdateScrollList(0, 255, 255);
	}

}

void ScanModJump(int jump)
{

	int flag = 1;
	int newjump = 0;

	while (flag == 1)
	{
		if (levelmodify[jump - 1].active == 0)
		{

			levelmodify[jump - 1].active = 1;
			jump = levelmodify[jump - 1].jump;
			if (jump == 0)
				flag = 0;
		}
		else
			flag = 0;
	}
}

void ActivateSwitch()
{

	int i = 0;
	int j = 0;
	float qdist;

	for (i = 0; i < countswitches; i++)
	{

		if (switchmodify[i].active != 2)
		{
			qdist = FastDistance(
				player_list[trueplayernum].x - switchmodify[i].x,
				player_list[trueplayernum].y - switchmodify[i].y,
				player_list[trueplayernum].z - switchmodify[i].z);

			if (qdist < 200.0f && switchmodify[i].active == 0 && pressopendoor == 1)
			{
				switchmodify[i].active = 1;
				PlayWavSound(SoundID("stone"), 100);

			}

			if (switchmodify[i].active == 1)
			{

				j = switchmodify[i].num;

				if (switchmodify[i].direction == 1)
					player_list2[j].x = player_list2[j].x + 5.0f;
				else if (switchmodify[i].direction == 2)
					player_list2[j].z = player_list2[j].z + 5.0f;
				else if (switchmodify[i].direction == 3)
					player_list2[j].x = player_list2[j].x - 5.0f;
				else if (switchmodify[i].direction == 4)
					player_list2[j].z = player_list2[j].z - 5.0f;

				if (switchmodify[i].count > 2)
				{
					switchmodify[i].active = 2;
				}
				switchmodify[i].count++;
			}
		}
	}
}

void AddTreasureDrop(float x, float y, float z, int raction)
{
	int i = 0;

	//fix this
	for (i = 0; i < MAX_NUM_ITEMS; i++)
	{

		if (item_list[i].bIsPlayerValid == FALSE)
		{

			if (raction == 0)
			{
				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y + 40.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("POTION");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 85;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = 0;

				strcpy_s(item_list[i].rname, "POTION");
			}
			else if (raction == 1)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y + 40.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("diamond");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 85;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = 10;

				strcpy_s(item_list[i].rname, "diamond");
			}
			else if (raction == 2)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y + 40.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("COIN");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 85;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = 5;

				strcpy_s(item_list[i].rname, "COIN");
			}
			else if (raction == 3)
			{

				item_list[i].bIsPlayerValid = TRUE;
				item_list[i].x = x;
				item_list[i].y = y + 40.0f;
				item_list[i].z = z;
				item_list[i].rot_angle = 0;
				item_list[i].model_id = FindModelID("KEY2");
				item_list[i].skin_tex_id = (int)-1;
				item_list[i].current_sequence = 0;
				item_list[i].current_frame = 85;
				item_list[i].draw_external_wep = FALSE;
				item_list[i].monsterid = (int)9999;
				item_list[i].bIsPlayerAlive = TRUE;
				item_list[i].gold = 5;

				strcpy_s(item_list[i].rname, "KEY2");
			}

			if (i >= itemlistcount)
				itemlistcount++;

			break;
		}
	}
}

int DisplayDialogText(char* text, float yloc)
{
	int i;
	char junk[4048];
	char buf[4048];
	int bufcount = 0;
	float adjust = 0.0f;
	float y = 0.0f;
	dialogtextcount = 0;
	float counter = 0;
	float ystart = 0;
	float xstart = 0;

	ystart = (FLOAT)(wHeight / 2 - 90.0f) + yloc;

	for (i = 0; i <= (int)strlen(text); i++)
	{

		counter++;
		if (text[i] == '<' || i >= (int)strlen(text))
		{
			buf[bufcount] = '\0';
			bufcount = 0;

			if (counter > xstart)
			{
				xstart = counter;
			}
			counter = 0;

			strcpy_s(dialogtext[dialogtextcount].text, buf);
			dialogtextcount++;
		}
		else
		{
			buf[bufcount] = text[i];

			bufcount++;
		}
	}

	ystart = (FLOAT)(wHeight / 2.0f) - (dialogtextcount * 13.0f) / 2.0f;

	xstart = (FLOAT)(wWidth / 2.0f) - (xstart * 11.2f) / 2.0f;

	ystart += yloc;
	ystart += 13.0f;

	for (i = 0; i < dialogtextcount; i++)
	{
		sprintf_s(junk, "%s", dialogtext[i].text);
		y = ystart + adjust;
		display_message(xstart + 10.0f, y, junk, 0, 255, 255, 12.5, 16, 0);

		adjust += 13.0f;
	}

	return 1;
}

int FreeSlave()
{

	int j = 0;
	float qdist = 0.0f;

	for (j = 0; j < num_monsters; j++)
	{
		float qdist = FastDistance(
			player_list[trueplayernum].x - monster_list[j].x,
			player_list[trueplayernum].y - monster_list[j].y,
			player_list[trueplayernum].z - monster_list[j].z);
		if (qdist < 100.0f)
		{
			if (monster_list[j].bIsPlayerValid == TRUE)
			{

				if (strcmp(monster_list[j].rname, "SLAVE") == 0)
				{
					monster_list[j].bIsPlayerValid = FALSE;
					monster_list[j].bStopAnimating = TRUE;
					monster_list[j].ability = 0;
					PlayWavSound(SoundID("goal1"), 100);
					PlayWavSound(SoundID("thankyou"), 100);
					sprintf_s(gActionMessage, "You set a slave free! 50 xp.");
					UpdateScrollList(255, 0, 255);
					player_list[trueplayernum].xp += 50;
				}
			}
		}
	}
	return 0;
}

int DisplayDamage(float x, float y, float z, int owner, int id, bool criticalhit)
{

	D3DVECTOR MissleVelocity;
	int misslecount = 0;
	int misslespot = 0;
	int gun_angle = 0;

	MissleVelocity.x = 0.0f;
	MissleVelocity.y = 0.0f;
	MissleVelocity.z = 0.0f;

	float monstersize = 25.0f;

	for (int i = 0; i < num_monsters; i++)
	{
		for (int cullloop = 0; cullloop < monstercount; cullloop++)
		{
			if (monstercull[cullloop] == monster_list[id].monsterid)
			{

				//TODO: fix
				//monstersize = monstertrueheight[cullloop];
				monstersize = 60.0f;

				monstersize = monstersize / 4.0f;
			}
		}
	}

	for (misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{
		if (your_missle[misslecount].active == 0)
		{
			misslespot = misslecount;
			break;
		}
	}
	your_missle[misslespot].model_id = 104;
	your_missle[misslespot].skin_tex_id = 370;

	your_missle[misslespot].current_frame = 0;
	your_missle[misslespot].current_sequence = 0;
	your_missle[misslespot].x = x;
	your_missle[misslespot].y = y + monstersize;
	your_missle[misslespot].z = z;
	your_missle[misslespot].rot_angle = (float)gun_angle;
	//your_missle[misslespot].velocity = savevelocity;
	your_missle[misslespot].active = 2;
	your_missle[misslespot].owner = (int)owner;

	your_missle[misslespot].playernum = (int)owner;
	your_missle[misslespot].playertype = (int)1;
	your_missle[misslespot].guntype = current_gun;

	your_missle[misslespot].critical = criticalhit;

	return misslespot;
}

void GetItem()
{

	//  normal cyan 0, 255, 255
	//	action gold : 255, 255, 0
	//	money green : 0, 255, 0
	//	xp purple : 255, 0, 255
	//	health white 255, 255, 255
	//	damage red 255, 0, 0

	int i = 0;
	int cullflag = 0;
	int cullloop = 0;
	float qdist;
	char level[80];
	int foundsomething = 0;

	for (i = 0; i < itemlistcount; i++)
	{
		cullflag = 0;
		for (cullloop = 0; cullloop < monstercount; cullloop++)
		{
			if (monstercull[cullloop] == item_list[i].monsterid)
			{
				cullflag = 1;
				break;
			}
		}

		if (item_list[i].monsterid == 9999 && item_list[i].bIsPlayerAlive == TRUE)
			cullflag = 1;

		if (cullflag == 1)
		{

			float wx = item_list[i].x;
			float wy = item_list[i].y;
			float wz = item_list[i].z;
			int perspectiveview = 1;
			if (perspectiveview == 0)
			{
				qdist = FastDistance(
					m_vLookatPt.x - wx,
					m_vLookatPt.y - wy,
					m_vLookatPt.z - wz);
			}
			else
			{
				qdist = FastDistance(m_vEyePt.x - wx,
					m_vEyePt.y - wy,
					m_vEyePt.z - wz);
			}

			if (qdist < 70.0f)
			{

				if (strcmp(item_list[i].rname, "spiral") == 0)
				{

					foundsomething = 1;
					num_players2 = 0;
					itemlistcount = 0;
					num_monsters = 0;

					ClearObjectList();
					ResetSound();
					//pCWorld->LoadSoundFiles(m_hWnd, "sounds.dat");

					sprintf_s(levelname, "level%d", item_list[i].ability);
					sprintf_s(gActionMessage, "Loading %s...", levelname);
					UpdateScrollList(0, 255, 255);
					int current_level = item_list[i].ability;
					strcpy_s(level, levelname);
					strcat_s(level, ".map");

					if (!pCWorld->LoadWorldMap(level))
					{
						//PrintMessage(m_hWnd, "LoadWorldMap failed", NULL, LOGFILE_ONLY);
						//return FALSE;
					}

					strcpy_s(level, levelname);
					strcat_s(level, ".cmp");

					//if (!pCWorld->InitPreCompiledWorldMap(m_hWnd, level))
					//{
					//    //PrintMessage(m_hWnd, "InitPreCompiledWorldMap failed", NULL, LOGFILE_ONLY);
					//    //return FALSE;
					//}

					strcpy_s(level, levelname);
					strcat_s(level, ".mod");

					pCWorld->LoadMod(level);

					ResetPlayer();
					SetStartSpot();
					PlayWavSound(SoundID("goal2"), 100);
					//StartFlare(2);
				}
				else if (strcmp(item_list[i].rname, "AXE") == 0 ||
					strcmp(item_list[i].rname, "FLAMESWORD") == 0 ||
					strcmp(item_list[i].rname, "BASTARDSWORD") == 0 ||
					strcmp(item_list[i].rname, "BATTLEAXE") == 0 ||
					strcmp(item_list[i].rname, "ICESWORD") == 0 ||
					strcmp(item_list[i].rname, "MORNINGSTAR") == 0 ||
					strcmp(item_list[i].rname, "SPIKEDFLAIL") == 0 ||
					strcmp(item_list[i].rname, "SPLITSWORD") == 0 ||
					strcmp(item_list[i].rname, "SUPERFLAMESWORD") == 0 ||
					strcmp(item_list[i].rname, "LIGHTNINGSWORD") == 0 ||
					strstr(item_list[i].rname, "SCROLL") != NULL)
				{

					foundsomething = 1;

					if (strstr(item_list[i].rname, "SCROLL") != NULL)
					{

						if (strstr(item_list[i].rname, "SCROLL-MAGICMISSLE") != NULL)
						{
							sprintf_s(gActionMessage, "You found a scroll of magic missle");
						}
						else if (strstr(item_list[i].rname, "SCROLL-FIREBALL") != NULL)
						{
							sprintf_s(gActionMessage, "You found a scroll of fireball");
						}
						else if (strstr(item_list[i].rname, "SCROLL-LIGHTNING") != NULL)
						{
							sprintf_s(gActionMessage, "You found a scroll of lightning");
						}
						else if (strstr(item_list[i].rname, "SCROLL-HEALING") != NULL)
						{
							sprintf_s(gActionMessage, "You found a scroll of healing");
						}
					}
					else
					{
						sprintf_s(gActionMessage, "You found a %s", item_list[i].rname);
					}
					UpdateScrollList(0, 255, 255);

					//you got something good! (weapon)
					item_list[i].bIsPlayerAlive = FALSE;
					PlayWavSound(SoundID("potion"), 100);
					//PlayWaveFile(".\\Sounds\\item.wav");
					for (int q = 0; q <= num_your_guns; q++)
					{
						char junk[255];

						strcpy_s(junk, item_list[i].rname);
						junk[strlen(junk) - 1] = '\0';

						if (your_gun[q].model_id == item_list[i].model_id ||
							strcmp(your_gun[q].gunname, junk) == 0)
						{
							if (strstr(your_gun[q].gunname, "SCROLL") != NULL) {
								your_gun[q].active = 1;
								your_gun[q].x_offset = your_gun[q].x_offset + 1;
							}
							else {
								//Switch to this wepaon if it is better
								your_gun[q].active = 1;
								SwitchGun(q);
							}
						}
					}
				}
				else if (strcmp(item_list[i].rname, "POTION") == 0)
				{

					if (player_list[trueplayernum].health < player_list[trueplayernum].hp)
					{
						item_list[i].bIsPlayerAlive = FALSE;
						item_list[i].bIsPlayerValid = FALSE;
						int hp = random_num(8) + 1;
						player_list[trueplayernum].health = player_list[trueplayernum].health + hp;

						if (player_list[trueplayernum].health > player_list[trueplayernum].hp)
							player_list[trueplayernum].health = player_list[trueplayernum].hp;

						PlayWavSound(SoundID("potion"), 100);
						//StartFlare(3);
						sprintf_s(gActionMessage, "You found a potion worth %d health", hp);
						UpdateScrollList(255, 255, 255);
						foundsomething = 1;
						//LevelUp(player_list[trueplayernum].xp);
					}
				}
				else if (strcmp(item_list[i].rname, "cheese1") == 0)
				{

					if (player_list[trueplayernum].health < player_list[trueplayernum].hp)
					{
						item_list[i].bIsPlayerAlive = FALSE;
						item_list[i].bIsPlayerValid = FALSE;
						int hp = random_num(3) + 1;
						player_list[trueplayernum].health = player_list[trueplayernum].health + hp;

						if (player_list[trueplayernum].health > player_list[trueplayernum].hp)
							player_list[trueplayernum].health = player_list[trueplayernum].hp;

						PlayWavSound(SoundID("potion"), 100);
						//StartFlare(3);
						sprintf_s(gActionMessage, "You found some cheese %d health", hp);
						UpdateScrollList(255, 255, 255);
						foundsomething = 1;
						//LevelUp(player_list[trueplayernum].xp);
					}
				}
				else if (strcmp(item_list[i].rname, "bread1") == 0)
				{

					if (player_list[trueplayernum].health < player_list[trueplayernum].hp)
					{
						item_list[i].bIsPlayerAlive = FALSE;
						item_list[i].bIsPlayerValid = FALSE;
						int hp = random_num(5) + 1;
						player_list[trueplayernum].health = player_list[trueplayernum].health + hp;

						if (player_list[trueplayernum].health > player_list[trueplayernum].hp)
							player_list[trueplayernum].health = player_list[trueplayernum].hp;

						PlayWavSound(SoundID("potion"), 100);
						//StartFlare(3);
						sprintf_s(gActionMessage, "You found some bread %d health", hp);
						UpdateScrollList(255, 255, 255);
						foundsomething = 1;
						//LevelUp(player_list[trueplayernum].xp);
					}
				}
				else if (strcmp(item_list[i].rname, "spellbook") == 0)
				{
						item_list[i].bIsPlayerAlive = FALSE;
						item_list[i].bIsPlayerValid = FALSE;

						PlayWavSound(SoundID("potion"), 100);
						//StartFlare(3);
						sprintf_s(gActionMessage, "You found a spellbook");
						UpdateScrollList(255, 255, 255);
						foundsomething = 1;

						char junk[255];

						for (int m = 0; m < 4; m++) {

							if (m == 0)
								strcpy_s(junk, "SCROLL-MAGICMISSLE");
							else if (m == 1)
								strcpy_s(junk, "SCROLL-FIREBALL");
							else if (m == 2)
								strcpy_s(junk, "SCROLL-LIGHTNING");
							else if (m == 3)
								strcpy_s(junk, "SCROLL-HEALING");

							for (int q = 0; q <= num_your_guns; q++)
							{
								if (strcmp(your_gun[q].gunname, junk) == 0)
								{
									your_gun[q].active = 1;
									your_gun[q].x_offset = your_gun[q].x_offset + 1;
								}
							}
						}
				}
				else if (strcmp(item_list[i].rname, "COIN") == 0)
				{
					if (item_list[i].gold == 0)
						item_list[i].gold = 5;

					player_list[trueplayernum].gold += item_list[i].gold;
					player_list[trueplayernum].xp += item_list[i].gold;
					//LevelUp(player_list[trueplayernum].xp);
					PlayWavSound(SoundID("coin"), 100);
					item_list[i].bIsPlayerAlive = FALSE;
					item_list[i].bIsPlayerValid = FALSE;

					foundsomething = 1;
					sprintf_s(gActionMessage, "You found %d coin", item_list[i].gold);
					UpdateScrollList(0, 255, 0);
					//LevelUp(player_list[trueplayernum].xp);
				}
				else if (strcmp(item_list[i].rname, "GOBLET") == 0)
				{

					if (item_list[i].gold == 0)
						item_list[i].gold = 100;

					player_list[trueplayernum].gold += item_list[i].gold;
					player_list[trueplayernum].xp += item_list[i].gold;
					PlayWavSound(SoundID("coin"), 100);
					item_list[i].bIsPlayerAlive = FALSE;
					item_list[i].bIsPlayerValid = FALSE;

					foundsomething = 1;
					sprintf_s(gActionMessage, "You found a goblet worth %d coin", item_list[i].gold);
					UpdateScrollList(0, 255, 0);
					//LevelUp(player_list[trueplayernum].xp);
				}

				else if (strcmp(item_list[i].rname, "KEY2") == 0)
				{

					player_list[trueplayernum].keys++;
					player_list[trueplayernum].xp += 10;
					PlayWavSound(SoundID("potion"), 100);
					item_list[i].bIsPlayerAlive = FALSE;
					item_list[i].bIsPlayerValid = FALSE;

					foundsomething = 1;
					sprintf_s(gActionMessage, "You found a key");
					UpdateScrollList(255, 255, 0);
					//LevelUp(player_list[trueplayernum].xp);
				}
				else if (strcmp(item_list[i].rname, "diamond") == 0)
				{

					player_list[trueplayernum].gold += item_list[i].gold;
					player_list[trueplayernum].xp += item_list[i].gold;
					PlayWavSound(SoundID("coin"), 100);
					item_list[i].bIsPlayerAlive = FALSE;
					item_list[i].bIsPlayerValid = FALSE;
					foundsomething = 1;
					sprintf_s(gActionMessage, "You found a diamond worth %d coin", item_list[i].gold);
					UpdateScrollList(0, 255, 0);
					//LevelUp(player_list[trueplayernum].xp);
				}
				else if (strcmp(item_list[i].rname, "armour") == 0)
				{

					player_list[trueplayernum].xp += 10;
					player_list[trueplayernum].ac = player_list[trueplayernum].ac - 1;
					foundsomething = 1;
					sprintf_s(gActionMessage, "You found some armour");
					UpdateScrollList(0, 100, 255);
					//StartFlare(2);
					PlayWavSound(SoundID("potion"), 100);
					PlayWavSound(SoundID("goal4"), 100);
					item_list[i].bIsPlayerAlive = FALSE;
					item_list[i].bIsPlayerValid = FALSE;
				}
			}
		}
	}
}

int ResetPlayer()
{

	int perspectiveview = 1;
	//DSLevel();

	//merchantcurrent = 0;
	//merchantfound = 0;

	m_vEyePt.x = 780;
	m_vEyePt.y = 160;
	m_vEyePt.z = 780;

	m_vLookatPt.x = 780;
	m_vLookatPt.y = 160;
	m_vLookatPt.z = 780;

	angy = 0;

	//gravitytime = 0.0f;

	//cameraf.x = m_vEyePt.x;
	//cameraf.y = m_vEyePt.y;
	//cameraf.z = m_vEyePt.z;
	//perspectiveview = 1;
	//gravityvector.y = 0.0f;

	if (perspectiveview == 0)
		look_up_ang = 35.0f;
	else
		look_up_ang = 0.0f;

	return 0;

}

void ClearObjectList()
{


	int q = 0;

	//for (q = 0; q < oblist_length; q++)
	//{

	//    delete oblist[q].light_source;
	//}
	//delete oblist;

	////TODO: FIx this
	//oblist = new OBJECTLIST[3000];


	for (int i = 0; i < MAX_NUM_ITEMS; i++)
	{
		item_list[i].frags = 0;
		item_list[i].x = 500;
		item_list[i].y = 22;
		item_list[i].z = 500;
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
		strcpy_s(item_list[i].name, "");
		strcpy_s(item_list[i].chatstr, "5");
		strcpy_s(item_list[i].name, "Dungeon Stomp");
	}
}

int XpPoints(int hd, int hp)
{

	int bxp = 0;
	int hpxp = 0;
	int xp = 0;

	switch (hd)
	{
	case 1:
		bxp = 10;
		hpxp = 1 * hp;
		xp = bxp + hpxp;
		break;
	case 2:
		bxp = 20;
		hpxp = 2 * hp;
		xp = bxp + hpxp;
		break;
	case 3:
		bxp = 35;
		hpxp = 3 * hp;
		xp = bxp + hpxp;
		break;
	case 4:
		bxp = 60;
		hpxp = 4 * hp;
		xp = bxp + hpxp;
		break;
	case 5:
		bxp = 90;
		hpxp = 5 * hp;
		xp = bxp + hpxp;
		break;
	case 6:
		bxp = 150;
		hpxp = 6 * hp;
		xp = bxp + hpxp;
		break;
	case 7:
		bxp = 225;
		hpxp = 8 * hp;
		xp = bxp + hpxp;
		break;
	case 8:
		bxp = 375;
		hpxp = 10 * hp;
		xp = bxp + hpxp;
		break;
	case 9:
		bxp = 600;
		hpxp = 12 * hp;
		xp = bxp + hpxp;
		break;
	case 10:
		bxp = 900;
		hpxp = 14 * hp;
		xp = bxp + hpxp;
	case 11:
		bxp = 900;
		hpxp = 15 * hp;
		xp = bxp + hpxp;
	case 12:
		bxp = 1200;
		hpxp = 16 * hp;
		xp = bxp + hpxp;
	case 13:
		bxp = 1500;
		hpxp = 17 * hp;
		xp = bxp + hpxp;
	case 14:
		bxp = 1800;
		hpxp = 18 * hp;
		xp = bxp + hpxp;
	case 15:
		bxp = 2100;
		hpxp = 19 * hp;
		xp = bxp + hpxp;
	case 16:
		bxp = 2400;
		hpxp = 20 * hp;
		xp = bxp + hpxp;
	case 17:
		bxp = 2700;
		hpxp = 21 * hp;
		xp = bxp + hpxp;
	case 18:
		bxp = 3000;
		hpxp = 22 * hp;
		xp = bxp + hpxp;
	case 19:
		bxp = 3300;
		hpxp = 23 * hp;
		xp = bxp + hpxp;
	case 20:
		bxp = 4000;
		hpxp = 24 * hp;
		xp = bxp + hpxp;
	case 99:
		bxp = 200;
		hpxp = 0;
		xp = hd * bxp;
		break;
	default:
		bxp = 0;
		hpxp = 0;
		xp = 10;
		break;
	}

	return xp;
}

int LevelUp(int xp)
{

	int countlevels = 0;
	int adjust = 800;

	if (xp > 0 && xp <= 2000)
	{

		countlevels = 1;
	}
	else if (xp >= 2001 && xp <= 4000)
	{
		countlevels = 2;
	}
	else if (xp >= 4001 && xp <= 8000)
	{
		countlevels = 3;
	}
	else if (xp >= 8001 && xp <= 18000)
	{
		countlevels = 4;
	}
	else if (xp >= 18001 && xp <= 35000)
	{
		countlevels = 5;
	}
	else if (xp >= 35001 && xp <= 70000)
	{
		countlevels = 6;
	}
	else if (xp >= 70001 && xp <= 125000)
	{
		countlevels = 7;
	}
	else if (xp >= 125001 && xp <= 250000)
	{
		countlevels = 8;
	}
	else if (xp >= 250001 && xp <= 500000)
	{
		countlevels = 9;
	}
	else if (xp >= 500001 && xp <= 750000)
	{
		countlevels = 10;
	}
	else if (xp >= 750001 && xp <= 1000000)
	{
		countlevels = 11;
	}

	else if (xp >= 1000001 && xp <= 1250000)
	{
		countlevels = 12;
	}
	else if (xp >= 1250001 && xp <= 1500000)
	{
		countlevels = 13;
	}
	else if (xp >= 1500001 && xp <= 1750000)
	{
		countlevels = 14;
	}
	else if (xp >= 1750001 && xp <= 2000000)
	{
		countlevels = 15;
	}
	else if (xp >= 2000001 && xp <= 2250000)
	{
		countlevels = 16;
	}
	else if (xp >= 2250001 && xp <= 2500000)
	{
		countlevels = 17;
	}
	else if (xp >= 2500001 && xp <= 2750000)
	{
		countlevels = 18;
	}
	else if (xp >= 2750001 && xp <= 3000000)
	{
		countlevels = 19;
	}
	else if (xp >= 3000001 && xp <= 3250000)
	{
		countlevels = 20;
	}
	else if (xp >= 3250001 && xp <= 3500000)
	{
		countlevels = 21;
	}
	else if (xp >= 3500001 && xp <= 3750000)
	{
		countlevels = 22;
	}
	else if (xp >= 3750001 && xp <= 4000000)
	{
		countlevels = 23;
	}
	else if (xp >= 4000001 && xp <= 4250000)
	{
		countlevels = 24;
	}
	else if (xp >= 4250001 && xp <= 4500000)
	{
		countlevels = 25;
	}
	else if (xp >= 4500001 && xp <= 4750000)
	{
		countlevels = 26;
	}
	else if (xp >= 4750001 && xp <= 5000000)
	{
		countlevels = 27;
	}
	else if (xp >= 5000001 && xp <= 5250000)
	{
		countlevels = 28;
	}
	else if (xp >= 5250001 && xp <= 5500000)
	{
		countlevels = 29;
	}
	else if (xp >= 5500001 && xp <= 5750000)
	{
		countlevels = 30;
	}
	else if (xp >= 5750001 && xp <= 6000000)
	{
		countlevels = 31;
	}
	else if (xp >= 6000001 && xp <= 6250000)
	{
		countlevels = 32;
	}
	else if (xp >= 6250001 && xp <= 6500000)
	{
		countlevels = 33;
	}
	else if (xp >= 6500001 && xp <= 6750000)
	{
		countlevels = 34;
	}
	else if (xp >= 6750001 && xp <= 7000000)
	{
		countlevels = 35;
	}
	else if (xp >= 7000001 && xp <= 7250000)
	{
		countlevels = 36;
	}
	else if (xp >= 7250001 && xp <= 7500000)
	{
		countlevels = 37;
	}
	else if (xp >= 7500001 && xp <= 7750000)
	{
		countlevels = 38;
	}
	else if (xp >= 7750001 && xp <= 8000000)
	{
		countlevels = 39;
	}
	else if (xp >= 8000001 && xp <= 8250000)
	{
		countlevels = 40;
	}
	else if (xp >= 8250001 && xp <= 8500000)
	{
		countlevels = 41;
	}
	else if (xp >= 8500001 && xp <= 8750000)
	{
		countlevels = 42;
	}
	else if (xp >= 8750001 && xp <= 9000000)
	{
		countlevels = 43;
	}
	else if (xp >= 9000001 && xp <= 9250000)
	{
		countlevels = 44;
	}
	else if (xp >= 9250001 && xp <= 9500000)
	{
		countlevels = 45;
	}
	else if (xp >= 9500001 && xp <= 9750000)
	{
		countlevels = 46;
	}
	else if (xp >= 9750001 && xp <= 10000000)
	{
		countlevels = 47;
	}
	else if (xp >= 10000001 && xp <= 10250000)
	{
		countlevels = 48;
	}
	else if (xp >= 10250001 && xp <= 10500000)
	{
		countlevels = 49;
	}
	else if (xp >= 10500001 && xp <= 10750000)
	{
		countlevels = 50;
	}
	else if (xp >= 10750001 && xp <= 11000000)
	{
		countlevels = 51;
	}
	else if (xp >= 11000001 && xp <= 11250000)
	{
		countlevels = 52;
	}
	else if (xp >= 11250001 && xp <= 11500000)
	{
		countlevels = 53;
	}
	else if (xp >= 11500001 && xp <= 11750000)
	{
		countlevels = 54;
	}
	else if (xp >= 11750001 && xp <= 12000000)
	{
		countlevels = 55;
	}
	else if (xp >= 12000001 && xp <= 12250000)
	{
		countlevels = 56;
	}
	else if (xp >= 12250001 && xp <= 12500000)
	{
		countlevels = 57;
	}
	else if (xp >= 12500001 && xp <= 12750000)
	{
		countlevels = 58;
	}
	else if (xp >= 12750001 && xp <= 13000000)
	{
		countlevels = 59;
	}
	else if (xp >= 13000001 && xp <= 13250000)
	{
		countlevels = 60;
	}
	else if (xp >= 13250001 && xp <= 13500000)
	{
		countlevels = 61;
	}

	if (player_list[trueplayernum].hd < countlevels)
	{
		player_list[trueplayernum].hd++;

		int raction = random_num(20) + 1;

		if (raction <= player_list[trueplayernum].hd)
			raction = player_list[trueplayernum].hd;

		player_list[trueplayernum].hp = player_list[trueplayernum].hp + raction;
		player_list[trueplayernum].health = player_list[trueplayernum].hp;

		player_list[trueplayernum].thaco--;

		if (player_list[trueplayernum].thaco <= 0)
			player_list[trueplayernum].thaco = 5;

		sprintf_s(gActionMessage, "You went up a level.  Hit Dice: %d", player_list[trueplayernum].hd);
		UpdateScrollList(255, 0, 255);
		//StartFlare(2);

		PlayWavSound(SoundID("win"), 100);
	}

	return xp;
}

int LevelUpXPNeeded(int xp)
{

	int countlevels = 0;

	if (xp >= 0 && xp <= 2000)
	{
		countlevels = 2000;
	}
	else if (xp >= 2001 && xp <= 4000)
	{
		countlevels = 4000;
	}
	else if (xp >= 4001 && xp <= 8000)
	{
		countlevels = 8000;
	}
	else if (xp >= 8001 && xp <= 18000)
	{
		countlevels = 18000;
	}
	else if (xp >= 18001 && xp <= 35000)
	{
		countlevels = 35000;
	}
	else if (xp >= 35001 && xp <= 70000)
	{
		countlevels = 70000;
	}
	else if (xp >= 70001 && xp <= 125000)
	{
		countlevels = 125000;
	}
	else if (xp >= 125001 && xp <= 250000)
	{
		countlevels = 250000;
	}
	else if (xp >= 250001 && xp <= 500000)
	{
		countlevels = 500000;
	}
	else if (xp >= 500001 && xp <= 750000)
	{
		countlevels = 750000;
	}
	else if (xp >= 750001 && xp <= 1000000)
	{
		countlevels = 1000000;
	}

	else if (xp >= 1000001 && xp <= 1250000)
	{
		countlevels = 1250000;
	}
	else if (xp >= 1250001 && xp <= 1500000)
	{
		countlevels = 1500000;
	}
	else if (xp >= 1500001 && xp <= 1750000)
	{
		countlevels = 1750000;
	}
	else if (xp >= 1750001 && xp <= 2000000)
	{
		countlevels = 2000000;
	}
	else if (xp >= 2000001 && xp <= 2250000)
	{
		countlevels = 2250000;
	}
	else if (xp >= 2250001 && xp <= 2500000)
	{
		countlevels = 2500000;
	}
	else if (xp >= 2500001 && xp <= 2750000)
	{
		countlevels = 2750000;
	}
	else if (xp >= 2750001 && xp <= 3000000)
	{
		countlevels = 3000000;
	}
	else if (xp >= 3000001 && xp <= 3250000)
	{
		countlevels = 3250000;
	}
	else if (xp >= 3250001 && xp <= 3500000)
	{
		countlevels = 3500000;
	}
	else if (xp >= 3500001 && xp <= 3750000)
	{
		countlevels = 3750000;
	}
	else if (xp >= 3750001 && xp <= 4000000)
	{
		countlevels = 4000000;
	}
	else if (xp >= 4000001 && xp <= 4250000)
	{
		countlevels = 4250000;
	}
	else if (xp >= 4250001 && xp <= 4500000)
	{
		countlevels = 4500000;
	}
	else if (xp >= 4500001 && xp <= 4750000)
	{
		countlevels = 4750000;
	}
	else if (xp >= 4750001 && xp <= 5000000)
	{
		countlevels = 5000000;
	}
	else if (xp >= 5000001 && xp <= 5250000)
	{
		countlevels = 5250000;
	}
	else if (xp >= 5250001 && xp <= 5500000)
	{
		countlevels = 5500000;
	}
	else if (xp >= 5500001 && xp <= 5750000)
	{
		countlevels = 5750000;
	}
	else if (xp >= 5750001 && xp <= 6000000)
	{
		countlevels = 6000000;
	}
	else if (xp >= 6000001 && xp <= 6250000)
	{
		countlevels = 6250000;
	}
	else if (xp >= 6250001 && xp <= 6500000)
	{
		countlevels = 6500000;
	}
	else if (xp >= 6500001 && xp <= 6750000)
	{
		countlevels = 6750000;
	}
	else if (xp >= 6750001 && xp <= 7000000)
	{
		countlevels = 7000000;
	}
	else if (xp >= 7000001 && xp <= 7250000)
	{
		countlevels = 7250000;
	}
	else if (xp >= 7250001 && xp <= 7500000)
	{
		countlevels = 7500000;
	}
	else if (xp >= 7500001 && xp <= 7750000)
	{
		countlevels = 7750000;
	}
	else if (xp >= 7750001 && xp <= 8000000)
	{
		countlevels = 8000000;
	}
	else if (xp >= 8000001 && xp <= 8250000)
	{
		countlevels = 8250000;
	}
	else if (xp >= 8250001 && xp <= 8500000)
	{
		countlevels = 8500000;
	}
	else if (xp >= 8500001 && xp <= 8750000)
	{
		countlevels = 8750000;
	}
	else if (xp >= 8750001 && xp <= 9000000)
	{
		countlevels = 9000000;
	}
	else if (xp >= 9000001 && xp <= 9250000)
	{
		countlevels = 9250000;
	}
	else if (xp >= 9250001 && xp <= 9500000)
	{
		countlevels = 9500000;
	}
	else if (xp >= 9500001 && xp <= 9750000)
	{
		countlevels = 9750000;
	}
	else if (xp >= 9750001 && xp <= 10000000)
	{
		countlevels = 10000000;
	}
	else if (xp >= 10000001 && xp <= 10250000)
	{
		countlevels = 10250000;
	}
	else if (xp >= 10250001 && xp <= 10500000)
	{
		countlevels = 10500000;
	}
	else if (xp >= 10500001 && xp <= 10750000)
	{
		countlevels = 10750000;
	}
	else if (xp >= 10750001 && xp <= 11000000)
	{
		countlevels = 11000000;
	}
	else if (xp >= 11000001 && xp <= 11250000)
	{
		countlevels = 11250000;
	}
	else if (xp >= 11250001 && xp <= 11500000)
	{
		countlevels = 11500000;
	}
	else if (xp >= 11500001 && xp <= 11750000)
	{
		countlevels = 11750000;
	}
	else if (xp >= 11750001 && xp <= 12000000)
	{
		countlevels = 12000000;
	}
	else if (xp >= 12000001 && xp <= 12250000)
	{
		countlevels = 12250000;
	}
	else if (xp >= 12250001 && xp <= 12500000)
	{
		countlevels = 12500000;
	}
	else if (xp >= 12500001 && xp <= 12750000)
	{
		countlevels = 12750000;
	}
	else if (xp >= 12750001 && xp <= 13000000)
	{
		countlevels = 13000000;
	}
	else if (xp >= 13000001 && xp <= 13250000)
	{
		countlevels = 13250000;
	}
	else if (xp >= 13250001 && xp <= 13500000)
	{
		countlevels = 13500000;
	}

	return countlevels;

}

void statusbardisplay(float x, float length, int type)
{

	int bar = 8;
	int truelength = 0;
	int i = 0;
	strcpy_s(statusbar, "");

	truelength = (int)((x / length) * bar);

	for (i = 0; i < truelength; i++)
	{

		if (type == 0)
			strcat_s(statusbar, "|");
		else
			strcat_s(statusbar, "`");
	}

	if (strlen(statusbar) == 0 && type == 0)
	{
		if (player_list[trueplayernum].health > 0)
			strcat_s(statusbar, "|");
	}
}

