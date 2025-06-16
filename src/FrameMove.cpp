#include "d3dtypes.h"
#include "LoadWorld.hpp"
#include "world.hpp"
#include "GlobalSettings.hpp"
#include <string.h>
#include "DirectInput.hpp"
#include "GameLogic.hpp"
#include "Missle.hpp"
#include "FrameMove.hpp"
#include "CameraBob.hpp"

extern int gravityon;
int movement = 1;
void PlayerJump(const FLOAT& fTimeKey);
void FindDoors(const FLOAT& fTimeKey);
void GameTimers(const FLOAT& fTimeKey);
bool MovePlayer(const FLOAT& fTimeKey);
void StrifePlayer(FLOAT& fTimeKey, bool addVel);
void PlayerAnimation();
void CheckAngle();

extern CameraBob bobY;
int playercurrentmove = 0;

HRESULT FrameMove(double fTime, FLOAT fTimeKey)
{
	float cameradist = 50.0f;

	GameTimers(fTimeKey);
	CheckAngle();
	GetItem();
	MonsterHit();
	FindDoors(fTimeKey);
	bool addVel = MovePlayer(fTimeKey);
	StrifePlayer(fTimeKey, addVel);
	PlayerAnimation();

	savelastmove = playermove;
	savelaststrifemove = playermovestrife;

	XMFLOAT3 result;

	result = collideWithWorld(RadiusDivide(m_vEyePt, eRadius), RadiusDivide(savevelocity, eRadius));
	result = RadiusMultiply(result, eRadius);

	m_vEyePt.x = result.x;
	m_vEyePt.y = result.y;
	m_vEyePt.z = result.z;

	PlayerJump(fTimeKey);

	if (look_up_ang < -89.3f)
		look_up_ang = -89.3f;

	if (look_up_ang > 89.3f)
		look_up_ang = 89.3f;


	float newangle = 0;
	newangle = fixangle(look_up_ang, 90);

	m_vLookatPt.x = m_vEyePt.x + cameradist * sinf(newangle * k) * sinf(angy * k);
	m_vLookatPt.y = m_vEyePt.y + cameradist * cosf(newangle * k);
	m_vLookatPt.z = m_vEyePt.z + cameradist * sinf(newangle * k) * cosf(angy * k);

	playercurrentmove = playermove;
	playermove = 0;
	playermovestrife = 0;

	EyeTrue = m_vEyePt;

	EyeTrue.y = m_vEyePt.y + cameraheight;

	LookTrue = m_vLookatPt;
	LookTrue.y = m_vLookatPt.y + cameraheight;

	cameraf.x = LookTrue.x;
	cameraf.y = LookTrue.y;
	cameraf.z = LookTrue.z;

	//GunTruesave = EyeTrue;

	return S_OK;
}

void CheckAngle()
{
	if (angy >= 360)
		angy = angy - 360;

	if (angy < 0)
		angy += 360;
}


void PlayerJump(const FLOAT& fTimeKey)
{
	XMFLOAT3 result;

	if (gravityon == 1) {
		// Physics parameters
		static float verticalVelocity = 0.0f;
		const float gravity = -2600.0f; // units/sec^2 (downward)
		const float jumpImpulse = 900.0f; // initial jump velocity
		const float minLandVelocity = -880.0f;
		static bool wasOnGround = false;

		// Start jump
		if (jumpstart == 1) {
			jumpstart = 0;
			verticalVelocity = jumpImpulse;
			lastjumptime = 0.0f;
			gravitytime = 0.0f;
			cleanjumpspeed = jumpImpulse;
			totaldist = 0.0f;
			gravityvector.y = gravity / 2.0f; // for compatibility
			jump = 1;
			wasOnGround = false;
		}

		// Apply gravity
		if (jump == 1 || !wasOnGround) {
			verticalVelocity += gravity * fTimeKey;
		}

		// Integrate position
		savevelocity.x = 0.0f;
		savevelocity.y = verticalVelocity * fTimeKey;
		savevelocity.z = 0.0f;

		foundcollisiontrue = 0;

		result = collideWithWorld(RadiusDivide(m_vEyePt, eRadius), RadiusDivide(savevelocity, eRadius));
		result = RadiusMultiply(result, eRadius);

		m_vEyePt.x = result.x;
		m_vEyePt.y = result.y;
		m_vEyePt.z = result.z;

		// Check for ground collision
		if (foundcollisiontrue != 0) {
			// Landed
			if (!wasOnGround && verticalVelocity < 0.0f) {
				if (gravitytime >= 0.2f) // shorter time for more responsive landing
					PlayWavSound(SoundID("jump_land"), 100);

				wasOnGround = true;
			}
			verticalVelocity = 0.0f;
			jump = 0;
			nojumpallow = 0;
			gravitydropcount = 0;
			
			gravitytime = 0.0f;
		} else {
			// In air
			nojumpallow = 1;
			gravitytime += fTimeKey;
			wasOnGround = false;
		}

		// Clamp downward velocity
		if (verticalVelocity < minLandVelocity)
			verticalVelocity = minLandVelocity;

		modellocation = m_vEyePt;
	}
}
void PlayerAnimation()
{
	// 1 = forward
	// 2 = left 
	// 3 = right
	// 4 = backward  

	if (player_list[trueplayernum].current_sequence != 2)
	{

		if ((playermove == 1 || playermove == 4) && movement == 0)
		{
			if (savelastmove != playermove && jump == 0)
			{
				SetPlayerAnimationSequence(trueplayernum, 1);
			}

			movement = 1;
		}
		else if (playermove == 0 && movement == 1)
		{
			if (savelastmove != playermove && jump == 0)
			{
				SetPlayerAnimationSequence(trueplayernum, 0);
			}

			movement = 0;
		}
	}

}


// Replace MovePlayer and StrifePlayer with more physics-like movement

bool MovePlayer(const FLOAT& fTimeKey)
{
    // Physics parameters
    static XMFLOAT3 velocity = {0.0f, 0.0f, 0.0f};
    static XMFLOAT3 acceleration = {0.0f, 0.0f, 0.0f};
    const float maxSpeed = playerspeedmax;
    const float accelRate = 1200.0f; // units/sec^2
    const float friction = 8.0f;     // friction coefficient

    // Calculate desired direction
    XMFLOAT3 desiredDir = {0.0f, 0.0f, 0.0f};
    if (playermove == 1) { // forward
        desiredDir.x += sinf(angy * k);
        desiredDir.z += cosf(angy * k);
    }
    if (playermove == 4) { // backward
        desiredDir.x -= sinf(angy * k);
        desiredDir.z -= cosf(angy * k);
    }

    // Normalize desired direction
    float len = sqrtf(desiredDir.x * desiredDir.x + desiredDir.z * desiredDir.z);
    if (len > 0.01f) {
        desiredDir.x /= len;
        desiredDir.z /= len;
    }

    // Apply acceleration in desired direction
    acceleration.x = desiredDir.x * accelRate;
    acceleration.y = 0.0f;
    acceleration.z = desiredDir.z * accelRate;

    // Apply friction if no input
    if (len < 0.01f) {
        acceleration.x = -velocity.x * friction;
        acceleration.z = -velocity.z * friction;
    }

    // Integrate velocity
    velocity.x += acceleration.x * fTimeKey;
    velocity.z += acceleration.z * fTimeKey;

    // Clamp speed
    float speed = sqrtf(velocity.x * velocity.x + velocity.z * velocity.z);
    if (speed > maxSpeed) {
        velocity.x = (velocity.x / speed) * maxSpeed;
        velocity.z = (velocity.z / speed) * maxSpeed;
    }

    // Output to savevelocity for world collision
    savevelocity.x = velocity.x * fTimeKey;
    savevelocity.y = 0.0f;
    savevelocity.z = velocity.z * fTimeKey;

    // If velocity is very small, zero it out
    if (fabsf(velocity.x) < 0.01f) velocity.x = 0.0f;
    if (fabsf(velocity.z) < 0.01f) velocity.z = 0.0f;

    // Return true if we are moving (for strafe to add velocity)
    return (speed > 0.01f);
}

void StrifePlayer(FLOAT& fTimeKey, bool addVel)
{
    // Physics parameters
    static XMFLOAT3 strafeVel = {0.0f, 0.0f, 0.0f};
    static XMFLOAT3 strafeAccel = {0.0f, 0.0f, 0.0f};
    const float maxStrafeSpeed = playerspeedmax;
    const float strafeAccelRate = 1200.0f;
    const float strafeFriction = 8.0f;

    XMFLOAT3 desiredStrafe = {0.0f, 0.0f, 0.0f};
    if (playermovestrife == 6) { // left
        float ang = angy - 90.0f;
        if (ang < 0) ang += 360.0f;
        desiredStrafe.x = sinf(ang * k);
        desiredStrafe.z = cosf(ang * k);
    }
    if (playermovestrife == 7) { // right
        float ang = angy + 90.0f;
        if (ang >= 360.0f) ang -= 360.0f;
        desiredStrafe.x = sinf(ang * k);
        desiredStrafe.z = cosf(ang * k);
    }

    // Normalize
    float len = sqrtf(desiredStrafe.x * desiredStrafe.x + desiredStrafe.z * desiredStrafe.z);
    if (len > 0.01f) {
        desiredStrafe.x /= len;
        desiredStrafe.z /= len;
    }

    // Apply acceleration
    strafeAccel.x = desiredStrafe.x * strafeAccelRate;
    strafeAccel.y = 0.0f;
    strafeAccel.z = desiredStrafe.z * strafeAccelRate;

    // Apply friction if no input
    if (len < 0.01f) {
        strafeAccel.x = -strafeVel.x * strafeFriction;
        strafeAccel.z = -strafeVel.z * strafeFriction;
    }

    // Integrate velocity
    strafeVel.x += strafeAccel.x * fTimeKey;
    strafeVel.z += strafeAccel.z * fTimeKey;

    // Clamp speed
    float speed = sqrtf(strafeVel.x * strafeVel.x + strafeVel.z * strafeVel.z);
    if (speed > maxStrafeSpeed) {
        strafeVel.x = (strafeVel.x / speed) * maxStrafeSpeed;
        strafeVel.z = (strafeVel.z / speed) * maxStrafeSpeed;
    }

    // Add to savevelocity
    if (addVel) {
        savevelocity.x += strafeVel.x * fTimeKey;
        savevelocity.z += strafeVel.z * fTimeKey;
    } else {
        savevelocity.x = strafeVel.x * fTimeKey;
        savevelocity.z = strafeVel.z * fTimeKey;
    }
    savevelocity.y = 0.0f;

    // Zero out if very small
    if (fabsf(strafeVel.x) < 0.01f) strafeVel.x = 0.0f;
    if (fabsf(strafeVel.z) < 0.01f) strafeVel.z = 0.0f;
}
void FindDoors(const FLOAT& fTimeKey)
{
	//Find doors
	for (int q = 0; q < oblist_length; q++)
	{
		if (strstr(oblist[q].name, "door") != NULL)
		{
			//door
			float qdist = FastDistance(
				m_vLookatPt.x - oblist[q].x,
				m_vLookatPt.y - oblist[q].y,
				m_vLookatPt.z - oblist[q].z);
			OpenDoor(q, qdist, fTimeKey);
		}
	}
}

void GameTimers(const FLOAT& fTimeKey)
{

	static float elapsedTime = 0.0f;
	static float lastTime = 0.0f;
	float kAnimationSpeed = 7.0f;



	// Get the current time in milliseconds
	float time = (float)GetTickCount64();

	// Find the time that has elapsed since the last time that was stored
	elapsedTime = time - lastTime;

	// To find the current t we divide the elapsed time by the ratio of 1 second / our anim speed.
	// Since we aren't using 1 second as our t = 1, we need to divide the speed by 1000
	// milliseconds to get our new ratio, which is a 5th of a second.
	float t = elapsedTime / (1000.0f / kAnimationSpeed);
	gametimerAnimation = t;
	// If our elapsed time goes over a 5th of a second, we start over and go to the next key frame
	if (elapsedTime >= (1000.0f / kAnimationSpeed))
	{
		//Animation Cycle
		maingameloop3 = 1;
		lastTime = (float)GetTickCount64();

	}
	else {
		maingameloop3 = 0;
	}


	if (maingameloop3) {
		AnimateCharacters();
	}


	//gametimerAnimation

		// To find the current t we divide the elapsed time by the ratio of 1 second / our anim speed.
	// Since we aren't using 1 second as our t = 1, we need to divide the speed by 1000
	// milliseconds to get our new ratio, which is a 5th of a second.
	


	//if ((gametimer3 - gametimerlast3) * time_factor >= 110.0f / 1000.0f)



	gametimer2 = DSTimer();

	if ((gametimer2 - gametimerlast2) * time_factor >= 60.0f / 1000.0f)
	{
		//Torch & Teleport Cycle
		maingameloop2 = 1;
		gametimerlast2 = DSTimer();
	}
	else
	{
		maingameloop2 = 0;
	}

	gametimer = DSTimer();

	if ((gametimer - gametimerlast) * time_factor >= 40.0f / 1000)
	{
		//Rotation coins, keys, diamonds
		maingameloop = 1;
		gametimerlast = DSTimer();
	}
	else
	{
		maingameloop = 0;
	}

	fTimeKeysave = fTimeKey;
	elapsegametimersave = fTimeKey;
}
