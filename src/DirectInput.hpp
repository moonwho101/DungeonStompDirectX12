#ifndef __DIRECTINPUT_H
#define __DIRECTINPUT_H

typedef struct Controls_typ
{
	BOOL Escape;
	int bLeft, bRight;
	BOOL bForward, bBackward;
	BOOL bForwardButton, bJump;
	BOOL bUp, bDown;
	int bHeadUp, bHeadDown;
	BOOL bStepLeft, bStepRight;
	BOOL bFire, bScores;
	BOOL bPrevWeap, bNextWeap;
	BOOL bTravelMode;
	BOOL bInTalkMode;
	BOOL missle;
	BOOL opendoor;
	BOOL bCameraleft;
	BOOL bCameraright;
	BOOL bFire2;
	BOOL spell;
	BOOL dialogpausejoystick;
	BOOL changeviews;
	BOOL xp;
	BOOL bDisableGravity;
	BOOL bSaveGame;
	BOOL bLoadGame;
	BOOL bNextSong;
	BOOL bGiveAllWeapons;
	BOOL bMusicOn;
	BOOL bNextLevel;
	BOOL bPreviousLevel;
} CONTROLS, * Controls_ptr;


extern FLOAT elapsegametimersave;
extern int playermove;
extern int playermovestrife;
extern D3DVALUE look_up_ang;

void CycleNextWeapon();
void CyclePreviousWeapon();
void GiveWeapons();



VOID MovePlayer(CONTROLS* Controls);
void DestroyInputDevice();
float fixangle(float angle, float adjust);
HRESULT SelectInputDevice();
void MakeDamageDice();

#endif