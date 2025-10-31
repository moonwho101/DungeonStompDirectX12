
#ifndef __MISSLE_H
#define __MISSLE_H

#include <stdio.h>
#include <io.h>
#include <wtypes.h>

extern GUNLIST *your_missle;
extern char gActionMessage[2048];

D3DVECTOR calculatemisslelength(XMFLOAT3 velocity);
void FirePlayerMissle(float x, float y, float z, float angy, int owner, int shoot, XMFLOAT3 velocity, float lookangy, FLOAT fTimeKeysave);

#endif