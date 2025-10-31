
#ifndef __FRAMEMOVE_H
#define __FRAMEMOVE_H

#include "d3dtypes.h"

extern int jumpstart;
extern int nojumpallow;
extern int jumpvdir;
extern float cleanjumpspeed;
extern float lastjumptime;

extern LONGLONG DSTimer();
extern double time_factor;
extern LONGLONG gametimer2;
extern LONGLONG gametimerlast2;
extern int maingameloop2;

extern LONGLONG gametimer;
extern LONGLONG gametimerlast;
extern int maingameloop;

extern float gametimerAnimation;
extern float gametimer3;
extern float gametimerlast3;
extern int maingameloop3;
extern FLOAT fTimeKeysave;
extern float playerspeed;
// Move
extern int direction;
extern int directionlast;
extern int savelastmove;
extern int savelaststrifemove;
extern float playerspeedmax;
extern float playerspeedlevel;
extern float movespeed;
extern float movespeedold;
extern float movespeedsave;
extern float movetime;
extern float moveaccel;
extern int playermove;
extern int playermovestrife;
extern float cameraheight;
extern float currentspeed;
extern XMFLOAT3 savevelocity;
extern XMFLOAT3 saveoldvelocity;

extern int jump;
extern float jumpcount;
extern XMFLOAT3 jumpv;
extern D3DVECTOR gravityvector;

extern float totaldist;
extern int gravitydropcount;

extern XMFLOAT3 EyeTrue;
extern int lastcollide;
extern int foundcollisiontrue;
extern float gravitytime;
extern D3DVECTOR gravityvectorold;
extern XMFLOAT3 modellocation;
extern XMFLOAT3 LookTrue;
extern CAMERAFLOAT cameraf;

#endif