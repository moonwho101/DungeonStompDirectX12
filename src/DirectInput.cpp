
#include "resource.h"
#include "d3dtypes.h"
#include "LoadWorld.hpp"
#include "world.hpp"
#include "GlobalSettings.hpp"
#include <string.h>
#include <dinput.h>
#include "DirectInput.hpp"
#include "GameLogic.hpp"
#include "Dice.hpp"


//#define DIRECTINPUT_VERSION 0x0800

//Only one input device should be true.
BOOL g_bUseMouse = true;
BOOL g_bUseJoystick = false;
BOOL g_bUseKeyboard = false;


void PlaySong();
extern int damageinprogress;
extern char gActionMessage[2048];
extern int musicon;
int stopmusic = 0;
int nextlevel = 0;

extern char levelname[80];
int ResetPlayer();
void ClearObjectList();
extern int ResetSound();
extern int foundsomething = 0;

int previouslevel = 0;
int currentlevel = 1;
void StopMusic();
void level();
/////////////
// LINKING //
/////////////
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")


//////////////
// INCLUDES //
//////////////

BYTE diks[256];
#define GWL_HINSTANCE       (-6)

IDirectInput8* g_pDI;
IDirectInput8* g_Keyboard_pDI;
IDirectInputDevice8* g_pdidDevice2;			 // The DIDevice2 interface
IDirectInputDevice8* g_Keyboard_pdidDevice2; // The DIDevice2 interface
IDirectInputDevice8* m_mouse;


GUID g_guidJoystick;						 // The GUID for the joystick
BOOL CALLBACK EnumJoysticksCallback(LPCDIDEVICEINSTANCE pInst, VOID* pvContext);
BOOL DelayKey2[256];
float filterx = 0;
float filtery = 0;
BOOL have_i_moved_flag;
//int playermove;
float rotate_camera;

float mousediv = 5.0f;
D3DVALUE angx = 0;

D3DVALUE angz = 0;
int runflag = 0;
int nextsong = 0;
int giveallweapons = 0;
extern int pressopendoor;

int MyHealth = 100;
int jump = 0;
float jumpcount = 0;
XMFLOAT3 jumpv;
int usespell = 0;
int hitmonster = 0;
BOOL firing_gun_flag = 0;
FLOAT fLastGunFireTime = 0;
int criticalhiton = 0;
bool cycleweaponbuttonpressed = FALSE;
bool cycleweaponbuttonpressedjoystick = FALSE;

float use_x, use_y;      // position to use for displaying
float springiness = 9; // tweak to taste.

void smooth_mouse(float time_d, float realx, float realy);

extern int firemissle;

//int playermovestrife;

CONTROLS Controls;

//Jump
int jumpstart = 0;
int nojumpallow = 0;
int jumpvdir = 0;
float cleanjumpspeed = 0;
float lastjumptime = 0;

int gravitybutton = 0;
int gravityon = 1;

bool loadprogress = 0;


HWND hWndGlobal;

HRESULT CreateInputDevice(IDirectInput8* pDI, IDirectInputDevice8* pDIdDevice, GUID guidDevice, const DIDATAFORMAT* pdidDataFormat, DWORD dwFlags);
int save_game(char* filename);
int load_game(char* filename);

//-----------------------------------------------------------------------------
// Name: CreateDInput()
// Desc: Initialize the DirectInput objects.
//-----------------------------------------------------------------------------
HRESULT CreateDInput(HWND hWnd)
{
#define DIDEVTYPE_JOYSTICK 4
	//extern HRESULT WINAPI DirectInputCreateEx(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID * ppvOut, LPUNKNOWN punkOuter);
	// Create the main DirectInput object

	//PrintMessage(NULL, "CD3DApplication::CreateDInput()", NULL, LOGFILE_ONLY);
	HRESULT result;
	// Initialize the main direct input interface.
	result = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_pDI, NULL);
	if (FAILED(result))
	{
		//DisplayError("DirectInputCreate() failed.");
		return E_FAIL;
	}

	// Check to see if a joystick is present. If so, the enumeration callback
	// will save the joystick's GUID, so we can create it later.
	ZeroMemory(&g_guidJoystick, sizeof(GUID));

	g_pDI->EnumDevices(DIDEVTYPE_JOYSTICK, EnumJoysticksCallback,
		&g_guidJoystick, DIEDFL_ATTACHEDONLY);

	// Initialize the direct input interface for the keyboard.
	DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_Keyboard_pDI, NULL);

	if (FAILED(result))
	{
		return 0;
	}


	hWndGlobal = hWnd;

	// Create a keyboard device
	result = CreateInputDevice(g_Keyboard_pDI, g_Keyboard_pdidDevice2, GUID_SysKeyboard, &c_dfDIKeyboard, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

	if (FAILED(result))
	{
		return 0;
	}

	SelectInputDevice();

	return S_OK;
}


//-----------------------------------------------------------------------------
// Name: DestroyDInput()
// Desc: Terminate our usage of DirectInput
//-----------------------------------------------------------------------------
VOID DestroyDInput()
{
	// Destroy the DInput object
	if (g_pDI)
		g_pDI->Release();
	g_pDI = NULL;
}



//-----------------------------------------------------------------------------
// Name: EnumJoysticksCallback()
// Desc: Called once for each enumerated joystick. If we find one,
//       create a device interface on it so we can play with it.
//-----------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticksCallback(LPCDIDEVICEINSTANCE pInst, VOID* pvContext)
{
	memcpy(pvContext, &pInst->guidInstance, sizeof(GUID));

	return DIENUM_STOP;
}



VOID UpdateControls()
{
	int i;
	int look_up = 0;
	int look_down = 0;



	// Note we want to always keyboard, either alone, or with another input
	// device such as a mouse or joystick etc

	// Read from input devices
	if (g_Keyboard_pDI)
	{
		HRESULT hr;
		//  BYTE         diks[256]; // DInput keyboard state buffer
		DIMOUSESTATE dims; // DInput mouse state structure
		DIJOYSTATE2 dijs;   // DInput joystick state structure

		// read the current keyboard state
		if (g_Keyboard_pdidDevice2)
		{
			g_Keyboard_pdidDevice2->Acquire();
			hr = g_Keyboard_pdidDevice2->GetDeviceState(sizeof(diks), &diks);
		}


		// read the current mouse and keyboard state
		if (g_bUseMouse)
		{
			hr = g_Keyboard_pdidDevice2->Acquire();
			hr = g_Keyboard_pdidDevice2->GetDeviceState(sizeof(diks), &diks);

			g_pdidDevice2->Acquire();
			hr = g_pdidDevice2->GetDeviceState(sizeof(DIMOUSESTATE), &dims);
		}

		// read the current joystick state
		if (g_bUseJoystick)
		{
			// Poll the device before reading the current state. This is required
			// for some devices (joysticks) but has no effect for others (keyboard
			// and mice). Note: this uses a DIDevice2 interface for the device.

			g_Keyboard_pdidDevice2->Acquire();
			hr = g_Keyboard_pdidDevice2->GetDeviceState(sizeof(diks), &diks);

			hr = g_pdidDevice2->Poll();

			hr = g_pdidDevice2->Acquire();
			hr = g_pdidDevice2->GetDeviceState(sizeof(DIJOYSTATE2), &dijs);
		}
		//DIJOYSTATE2

		// Check whether the input stream has been interrupted. If so,
		// re-acquire and try again.
		if (hr == DIERR_INPUTLOST)
		{
			//PrintMessage(NULL, "DIERR_INPUTLOST", NULL, LOGFILE_ONLY);

			hr = g_Keyboard_pdidDevice2->Acquire();
			if (FAILED(hr))
			{
				//PrintMessage(NULL, "Acquire Input device FAILED", NULL, LOGFILE_ONLY);
				return; // S_OK;
			}
		}

		// Read Keyboard input only
		if (g_Keyboard_pdidDevice2)
		{

			//DIK_ESCAPE

			Controls.Escape = diks[DIK_ESCAPE] && 0x80;
			Controls.bLeft = diks[DIK_A] && 0x80;
			Controls.bRight = diks[DIK_D] && 0x80;
			Controls.bForward = diks[DIK_W] && 0x80;
			Controls.bBackward = diks[DIK_S] && 0x80;
			Controls.bUp = diks[DIK_NUMPADPLUS] && 0x80;
			Controls.bDown = diks[DIK_NUMPADMINUS] && 0x80;
			Controls.bHeadUp = diks[DIK_PGUP] && 0x80;
			Controls.bHeadDown = diks[DIK_PGDN] && 0x80;
			Controls.bStepLeft = diks[DIK_COMMA] && 0x80;
			Controls.bStepRight = diks[DIK_PERIOD] && 0x80;
			//Controls.bPrevWeap = diks[DIK_DELETE] && 0x80;
			//Controls.bNextWeap = diks[DIK_INSERT] && 0x80;
			Controls.bPrevWeap = diks[DIK_Z] && 0x80;
			Controls.bNextWeap = diks[DIK_Q] && 0x80;


			Controls.bInTalkMode = diks[DIK_SLASH] && 0x80;
			Controls.opendoor = diks[DIK_SPACE] && 0x80;
			// Check both left and right control keys

			Controls.bFire2 = diks[DIK_END] && 0x80;

			Controls.bCameraleft = diks[DIK_LSHIFT] && 0x80;
			Controls.bCameraright = diks[DIK_RSHIFT] && 0x80;
			Controls.missle = diks[DIK_E] && 0x80;
			Controls.bFire = diks[DIK_RCONTROL] && 0x80;
			Controls.spell = diks[DIK_RCONTROL] && 0x80;

			Controls.xp = diks[DIK_X] && 0x80;
			Controls.bDisableGravity = diks[DIK_G] && 0x80;

			Controls.bLoadGame = diks[DIK_F5] && 0x80;
			Controls.bSaveGame = diks[DIK_F6] && 0x80;
			Controls.bNextSong = diks[DIK_P] && 0x80;
			Controls.bGiveAllWeapons = diks[DIK_K] && 0x80;
			Controls.bMusicOn = diks[DIK_I] && 0x80;
			Controls.bNextLevel = diks[DIK_RBRACKET] && 0x80;
			Controls.bPreviousLevel = diks[DIK_LBRACKET] && 0x80;

		}

		// Read mouse input
		if (g_bUseMouse)
		{
			Controls.bLeft = 0;
			Controls.bRight = 0;
			Controls.bHeadUp = 0;
			Controls.bHeadDown = 0;

			int threshhold = 5;

			if (dims.lX < 0)
				Controls.bLeft = dims.lX;
			if (dims.lX > 0)
				Controls.bRight = dims.lX;

			if (dims.lY < 0)
				Controls.bHeadUp = dims.lY;
			if (dims.lY > 0)
				Controls.bHeadDown = dims.lY;

			Controls.bFire = dims.rgbButtons[0] && 0x80;
			Controls.opendoor = diks[DIK_SPACE] && 0x80;
			Controls.bForward = dims.rgbButtons[1] && 0x80;

			if (Controls.bForward == 0)
				Controls.bForward = diks[DIK_W] && 0x80;

			Controls.bStepLeft = diks[DIK_A] && 0x80;
			Controls.bStepRight = diks[DIK_D] && 0x80;

			Controls.bBackward = diks[DIK_S] && 0x80;
			Controls.spell = dims.rgbButtons[0] && 0x80;

			Controls.missle = diks[DIK_E] && 0x80;

		}

		// Read joystick input
		if (g_bUseJoystick)
		{


			/*
			//Control PAD
			if (dijs.rgdwPOV[0] == -1)
			{
			}
			else
			{

				if (dijs.rgdwPOV[0] >= 0 && dijs.rgdwPOV[0] <= 4500 ||
					dijs.rgdwPOV[0] >= 31500)
				{
					//up
					Controls.bForward = 1;
				}
				if (dijs.rgdwPOV[0] >= 4500 && dijs.rgdwPOV[0] <= 13500)
				{
					//right
					Controls.bRight = 1;
				}
				if (dijs.rgdwPOV[0] >= 13500 && dijs.rgdwPOV[0] <= 22500)
				{
					//down
					Controls.bBackward = 1;
				}

				if (dijs.rgdwPOV[0] >= 27000 && dijs.rgdwPOV[0] <= 36000)
				{
					//left
					Controls.bLeft = -1;
				}
			}
			*/



			//Control PAD
			if (dijs.rgdwPOV[0] == -1) {
				cycleweaponbuttonpressedjoystick = FALSE;
			}
			else if (dijs.rgdwPOV[0] >= 4500 && dijs.rgdwPOV[0] <= 13500 && cycleweaponbuttonpressedjoystick == FALSE)
			{
				//right
				CycleNextWeapon();
				cycleweaponbuttonpressedjoystick = TRUE;
			}
			else if (dijs.rgdwPOV[0] >= 27000 && dijs.rgdwPOV[0] <= 36000 && cycleweaponbuttonpressedjoystick == FALSE)
			{
				//left
				CyclePreviousWeapon();
				cycleweaponbuttonpressedjoystick = TRUE;
			}


			//Right controller - look around

			if (dijs.lRx > 32767.0)
			{
				//right
				Controls.bRight = 1;
			}

			if (dijs.lRx < 32767.0)
			{
				//left
				Controls.bLeft = -1;
			}

			if (dijs.lRy < 32767)
			{
				//up
				Controls.bHeadUp = -1.0f;
			}

			if (dijs.lRy > 32767)
			{
				//down
				Controls.bHeadDown = 1.0f;
			}

			//Left Controller - Move
			if (dijs.lX >= 6.0)
			{
				//right
				Controls.bStepRight = 1;
			}

			if (dijs.lX <= -6.0)
			{
				//left
				Controls.bStepLeft = -1;
			}

			if (dijs.lY < 0.0)
			{
				Controls.bForward = 1;
			}
			if (dijs.lY > 0.0)
			{
				Controls.bBackward = 1;
			}




			//sprintf_s(gActionMessage, "%ld %ld %ld", dijs.lRx,dijs.lRy,dijs.lRz); //right controller
			//sprintf_s(gActionMessage, "%ld %ld %ld", dijs.lX,dijs.lY,dijs.lZ); //left controller
			//sprintf_s(gActionMessage, "%ld", dijs.rgdwPOV[0]);  //Control PAD
			//UpdateScrollList(0, 245, 255);

			//Button xbox  0=A 1=B 2=X 3=Y
			Controls.bFire = dijs.rgbButtons[2] && 0x80; //Fire
			Controls.missle = dijs.rgbButtons[0] && 0x80; //Jump
			Controls.opendoor = dijs.rgbButtons[3] && 0x80; //Door
			Controls.spell = dijs.rgbButtons[2] && 0x80;  //Spell

			Controls.bPrevWeap = diks[DIK_INSERT] && 0x80;
			Controls.bNextWeap = diks[DIK_DELETE] && 0x80;

			Controls.dialogpausejoystick = dijs.rgbButtons[3] && 0x80;
			Controls.changeviews = dijs.rgbButtons[1] && 0x80;
		}

		for (i = 0; i < 256; i++)
		{
			if ((diks[i] && 0x80) == FALSE)
				DelayKey2[i] = FALSE;
		}

		MovePlayer(&Controls);

		/*	if (RRAppActive == TRUE && dialogpause == 0)
			{
				MovePlayer(&Controls);
			}

			if (Controls.dialogpausejoystick && dialogpause) {
				gtext[dialognum].shown = 1;
				dialogpause = 0;
			}

			if (!Controls.changeviews) {
				changeviewbuttonpressed = FALSE;
			}

			if (Controls.changeviews && changeviewbuttonpressed == FALSE) {
				SwitchView();
				changeviewbuttonpressed = TRUE;
			}*/

	}
}

VOID WalkMode(CONTROLS* Controls)
{
	int i = 0;
	float speed = 8.0f;
	int perspectiveview = 1;
	//	float step_left_angy;
	//	float step_right_angy;

	filterx = 0;
	filtery = 0;

	have_i_moved_flag = FALSE;

	playermove = 0;

	FLOAT fTime = timeGetTime() * 0.001f; // Get current time in seconds

	if (Controls->Escape) {
		//DXUTShutdown();
	}

	// Update the frame rate once per second
	//if (fTime - fLastGunFireTime > .3f)
	//{
	//	firepause = 0;
	//}

	if (Controls->spell && player_list[trueplayernum].firespeed == 0 && usespell == 1)
	{
		int pspeed = (18 - player_list[trueplayernum].hd);
		if (pspeed < 6)
			pspeed = 6;

		player_list[trueplayernum].firespeed = pspeed;
		firemissle = 1;
	}

	if (Controls->bCameraleft)
	{

		//rotate_camera+=5.0f;
		rotate_camera += 105 * elapsegametimersave;
	}

	if (Controls->bCameraright)
	{

		//rotate_camera-=5.0f;
		rotate_camera -= 105 * elapsegametimersave;
	}


	if (Controls->missle && jump == 0 && nojumpallow == 0)
	{

		jump = 1;
		if (perspectiveview == 1)
			jumpv = { 0.0f, 30.0f, 0.0f };
		else
			jumpv = { 0.0f, 30.0f, 0.0f };
		jumpvdir = 0;
		jumpcount = 0;
		SetPlayerAnimationSequence(trueplayernum, 6);
		PlayWavSound(SoundID("jump1"), 80);
		jumpstart = 1;
	}


	// turn left
	if (Controls->bLeft)
	{
		playermove = 2;
		if (g_bUseMouse)
		{
			filterx = (float)Controls->bLeft / (float)mousediv;
		}
		else
		{
			angy -= 105 * elapsegametimersave;
			have_i_moved_flag = TRUE;
		}
	}

	// turn right
	if (Controls->bRight)
	{
		playermove = 3;
		if (g_bUseMouse)
		{
			filterx = (float)Controls->bRight / (float)mousediv;
		}
		else
		{
			angy += 105 * elapsegametimersave;
			have_i_moved_flag = TRUE;
		}
	}
	// move forward
	runflag = 0;

	if ((Controls->bForward || Controls->bForwardButton))
	{
		playermove = 1;
		have_i_moved_flag = TRUE;
		runflag = 1;
	}


	if (Controls->bDisableGravity && !gravitybutton)
	{
		if (gravityon == 1) {
			strcpy_s(gActionMessage, "Gravity off.");
			UpdateScrollList(0, 245, 255);

			gravityon = 0;
		}
		else {
			gravityon = 1;
			strcpy_s(gActionMessage, "Gravity on.");
			UpdateScrollList(0, 245, 255);

		}

		gravitybutton = 1;
	}
	if (!Controls->bDisableGravity && gravitybutton)
	{
		gravitybutton = 0;
	}

	if (Controls->bSaveGame && !loadprogress)
	{
		save_game("");
	}else  if (Controls->bLoadGame && !loadprogress)
	{
		load_game("");
	}


	if (Controls->bSaveGame || Controls->bLoadGame) {
		loadprogress = TRUE;
	}
	else {
		loadprogress = FALSE;
	}


	// move backward
	if (Controls->bBackward)
	{
		runflag = 1;
		playermove = 4;
		have_i_moved_flag = TRUE;
	}

	// Move vertically up towards sky
	if (Controls->bUp)
		m_vEyePt.y += 2;

	// Move vertically down towards ground
	if (Controls->bDown)
		m_vEyePt.y -= 2;

	if (Controls->bNextWeap && !cycleweaponbuttonpressed) {
		CycleNextWeapon();
	}
	else if (Controls->bPrevWeap && !cycleweaponbuttonpressed) {
		CyclePreviousWeapon();
	}

	if (Controls->bNextWeap || Controls->bPrevWeap) {
		cycleweaponbuttonpressed = TRUE;
	}
	else {
		cycleweaponbuttonpressed = FALSE;
	}

	// fire gun
	if ((Controls->bFire == TRUE) && (MyHealth > 0 && jump == 0) && (player_list[trueplayernum].current_sequence != 2) && player_list[trueplayernum].bIsPlayerAlive == TRUE && usespell == 0 ||
		(Controls->bFire2 == TRUE) && (MyHealth > 0) && jump == 0 && (player_list[trueplayernum].current_sequence != 2) && player_list[trueplayernum].bIsPlayerAlive == TRUE &&
		usespell == 0)
	{


		hitmonster = 0;
		numdice = 2;
		firing_gun_flag = TRUE;
		SetPlayerAnimationSequence(trueplayernum, 2);
		PlayWavSound(SoundID("knightattack"), 100);


		//set_firing_timer_flag = TRUE;


		fLastGunFireTime = timeGetTime() * 0.001f;

		playermove = 5;
		//firepause = 1;

		dice[0].roll = 1;
		dice[1].roll = 1;

		criticalhiton = 0;

	}
	else
	{
		firing_gun_flag = FALSE;
	}




	if (Controls->opendoor)
	{

		pressopendoor = 1;
	}
	else
	{
		pressopendoor = 0;
	}

	if (Controls->xp) {
		player_list[trueplayernum].xp += 10;
		LevelUp(player_list[trueplayernum].xp);
	}

	// tilt head upwards
	if (Controls->bHeadDown)
	{

		if (g_bUseMouse)
		{
			filtery = (float)Controls->bHeadDown / (float)mousediv;
		}
		else
		{
			//	look_up_ang += 4;
			look_up_ang += 105 * elapsegametimersave;
			if (look_up_ang > 90)
				look_up_ang = 89.9f;
		}
	}

	// tilt head downward
	if (Controls->bHeadUp)
	{

		if (g_bUseMouse)
		{
			//look_up_ang += (float)Controls->bHeadUp / (float)5;
			filtery = (float)Controls->bHeadUp / (float)mousediv;
		}
		else
		{

			look_up_ang -= 105 * elapsegametimersave;
			//	look_up_ang -= 4;
			if (look_up_ang < -90)
				look_up_ang = -89.9f;
		}

		//look_up_ang -= 4;
	}

	// side step left


	if (Controls->bStepLeft)
	{
		playermovestrife = 6;
	}

	// side step right

	if (Controls->bStepRight)
	{
		playermovestrife = 7;
	}

	if (Controls->bNextSong && !nextsong)
	{
		nextsong = 1;
		strcpy_s(gActionMessage, "Random Song.");
		UpdateScrollList(0, 245, 255);
		PlaySong();
	}

	if (!Controls->bNextSong && nextsong)
	{
		nextsong = 0;
	}


	if (Controls->bGiveAllWeapons && !giveallweapons)
	{
		GiveWeapons();
		giveallweapons = 1;

	}
	if (!Controls->bGiveAllWeapons && giveallweapons)
	{
		giveallweapons = 0;
	}

	if (Controls->bMusicOn && !stopmusic)
	{
		stopmusic = 1;
		if (musicon) {
			strcpy_s(gActionMessage, "Music Off.");
			UpdateScrollList(0, 245, 255);
			StopMusic();
			musicon = 0;
		}
		else {
			strcpy_s(gActionMessage, "Music On.");
			UpdateScrollList(0, 245, 255);

			musicon = 1;
		}

	}
	if (!Controls->bMusicOn && stopmusic)
	{
		stopmusic = 0;
	}



	if (Controls->bNextLevel && !nextlevel)
	{
		currentlevel++;
		level();
		nextlevel = 1;
	}
	if (!Controls->bNextLevel && nextlevel)
	{
		nextlevel = 0;
	}

	if (Controls->bPreviousLevel && !previouslevel)
	{
		currentlevel--;
		level();
		previouslevel = 1;
	}
	if (!Controls->bPreviousLevel && previouslevel)
	{
		previouslevel = 0;
	}




	if (g_bUseMouse)
	{
		angy += filterx;

		look_up_ang += filtery;

	}

	bool mousefilter = true;
	if (g_bUseMouse)
	{
		if (mousefilter)
		{

			smooth_mouse(elapsegametimersave, filterx, filtery);
			//MouseFilter(filterx, filtery);
			angy += filterx;
			look_up_ang += filtery;
		}
		else
		{
			angy += filterx;
			look_up_ang += filtery;
		}
		if (look_up_ang < -90.0f)
			look_up_ang = -89.9f;

		if (look_up_ang > 90.0f)
			look_up_ang = 89.9f;
	}
}


VOID MovePlayer(CONTROLS* Controls)
{
	// Update the variables for the player


	WalkMode(Controls);


}




//-----------------------------------------------------------------------------
// Name: CreateInputDevice()
// Desc: Create a DirectInput device.
//-----------------------------------------------------------------------------
HRESULT CreateInputDevice(IDirectInput8* pDI, IDirectInputDevice8* pDIdDevice, GUID guidDevice, const DIDATAFORMAT* pdidDataFormat, DWORD dwFlags)
{
	HRESULT result;

	//PrintMessage(NULL, "CD3DApplication::CreateInputDevice()", NULL, LOGFILE_ONLY);

	result = g_pDI->CreateDevice(guidDevice, &pDIdDevice, NULL);
	if (FAILED(result))
	{
		return false;
	}

	//else
		//PrintMessage(NULL, "CD3DApplication::CreateInputDevice() - CreateDeviceEx ok", NULL, LOGFILE_ONLY);

	// Set the device data format. Note: a data format specifies which
	// controls on a device we are interested in, and how they should be
	// reported.


	result = pDIdDevice->SetDataFormat(pdidDataFormat);
	if (FAILED(result))
	{
		return false;
	}

	//HWND hWnd = DXUTGetHWND();

	//TODO: Fix this
	//HWND hWnd = NULL;

	// Set the cooperativity level to let DirectInput know how this device
	// should interact with the system and with other DirectInput applications.
	//if (FAILED(pDIdDevice->SetCooperativeLevel(hWnd, dwFlags)))
	//if (FAILED(pDIdDevice->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE)))

	if (FAILED(pDIdDevice->SetCooperativeLevel(hWndGlobal, dwFlags)))
	{
		//DisplayError("SetCooperativeLevel() failed");
		return E_FAIL;
	}

	if (guidDevice == GUID_SysKeyboard)
		g_Keyboard_pdidDevice2 = pDIdDevice;
	else
		g_pdidDevice2 = pDIdDevice;

	return S_OK;
}


void DestroyInputDevice()
{
	// Unacquire and release the device's interfaces
	if (g_pdidDevice2)
	{
		g_pdidDevice2->Unacquire();
		g_pdidDevice2->Release();
		g_pdidDevice2 = NULL;
	}

	// keyboard

	if (g_Keyboard_pdidDevice2)
	{
		g_Keyboard_pdidDevice2->Unacquire();
		g_Keyboard_pdidDevice2->Release();
		g_Keyboard_pdidDevice2 = NULL;
	}

}



HRESULT SelectInputDevice()
{

	// Destroy the old device
	DestroyInputDevice();

	// Check the dialog controls to see which device type to create


	if (g_bUseKeyboard)
	{
		SetCursor(NULL);
		CreateInputDevice(g_Keyboard_pDI,
			g_Keyboard_pdidDevice2,
			GUID_SysKeyboard, &c_dfDIKeyboard,
			DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
		g_Keyboard_pdidDevice2->Acquire();
	}

	if (g_bUseMouse)
	{
		//c_dfDIMouse2


		CreateInputDevice(g_pDI,
			g_pdidDevice2,
			GUID_SysMouse, &c_dfDIMouse,
			DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
		g_pdidDevice2->Acquire();

		CreateInputDevice(g_Keyboard_pDI,
			g_Keyboard_pdidDevice2,
			GUID_SysKeyboard, &c_dfDIKeyboard,
			DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
		g_Keyboard_pdidDevice2->Acquire();
	}

	if (g_bUseJoystick)
	{

		HRESULT hr = CreateInputDevice(g_Keyboard_pDI, g_Keyboard_pdidDevice2, GUID_SysKeyboard, &c_dfDIKeyboard, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);

		hr = g_Keyboard_pdidDevice2->Acquire();

		hr = CreateInputDevice(g_pDI,
			g_pdidDevice2,
			g_guidJoystick, &c_dfDIJoystick2,
			DISCL_EXCLUSIVE | DISCL_FOREGROUND);

		if (g_pdidDevice2 == NULL) {
			//UINT resultclick = MessageBox(main_window_handle, "DirectInput Error", "Game Controller not found.", MB_OK);
		}

		//pdidInstance->guidInstance = { B34EE010 - 12EE - 11EC - 8001 - 444553540000 }

		hr = g_pdidDevice2->Acquire();

		// Set the range of the joystick axes tp [-1000,+1000]
		DIPROPRANGE diprg;
		diprg.diph.dwSize = sizeof(DIPROPRANGE);
		diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		diprg.diph.dwHow = DIPH_BYOFFSET;
		diprg.lMin = -10;
		diprg.lMax = +10;

		diprg.diph.dwObj = DIJOFS_X; // Set the x-axis range
		hr = g_pdidDevice2->SetProperty(DIPROP_RANGE, &diprg.diph);

		diprg.diph.dwObj = DIJOFS_Y; // Set the y-axis range
		hr = g_pdidDevice2->SetProperty(DIPROP_RANGE, &diprg.diph);

		// Set the dead zone for the joystick axes (because many joysticks
		// aren't perfectly calibrated to be zero when centered).
		DIPROPDWORD dipdw;
		dipdw.diph.dwSize = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dipdw.diph.dwHow = DIPH_DEVICE;
		dipdw.dwData = 1000; // Here, 1000 = 10%

		dipdw.diph.dwObj = DIJOFS_X; // Set the x-axis deadzone
		hr = g_pdidDevice2->SetProperty(DIPROP_DEADZONE, &dipdw.diph);

		dipdw.diph.dwObj = DIJOFS_Y; // Set the y-axis deadzone
		hr = g_pdidDevice2->SetProperty(DIPROP_RANGE, &dipdw.diph);
	}
	return 1;
}



LONGLONG DSTimer()
{
	LONGLONG cur_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&cur_time);

	return cur_time;
}






void CyclePreviousWeapon()
{

	int loopguns = current_gun - 1;

	if (loopguns < 0)
		loopguns = num_your_guns;

	while (your_gun[loopguns].active == 0)
	{
		loopguns--;
		if (loopguns < 0)
			loopguns = num_your_guns;
	}

	if (current_gun != loopguns)
		PlayWavSound(SoundID("switch"), 100);

	current_gun = loopguns;

	if (strstr(your_gun[current_gun].gunname, "SCROLL") != NULL)
	{
		usespell = 1;
	}
	else
	{

		usespell = 0;
	}
	player_list[trueplayernum].gunid = your_gun[current_gun].model_id;
	player_list[trueplayernum].guntex = your_gun[current_gun].skin_tex_id;
	player_list[trueplayernum].damage1 = your_gun[current_gun].damage1;
	player_list[trueplayernum].damage2 = your_gun[current_gun].damage2;

	//if (strstr(your_gun[loopguns].gunname, "FLAME") != NULL ||
	//	strstr(your_gun[loopguns].gunname, "ICE") != NULL ||
	//	strstr(your_gun[loopguns].gunname, "LIGHTNINGSWORD") != NULL)
	//{
	//	bIsFlashlightOn = TRUE;
	//	lighttype = 2;
	//}
	//else
	//{
	//	lighttype = 0;
	//	bIsFlashlightOn = FALSE;
	//}
	MakeDamageDice();
}

void CycleNextWeapon()
{
	int loopguns = current_gun + 1;
	if (loopguns >= num_your_guns)
		loopguns = 0;

	while (your_gun[loopguns].active == 0)
	{
		loopguns++;
		if (loopguns >= num_your_guns)
			loopguns = 0;
	}

	if (current_gun != loopguns)
		PlayWavSound(SoundID("switch"), 100);

	current_gun = loopguns;

	if (strstr(your_gun[current_gun].gunname, "SCROLL") != NULL)
	{
		usespell = 1;
	}
	else
	{

		usespell = 0;
	}

	player_list[trueplayernum].gunid = your_gun[current_gun].model_id;
	player_list[trueplayernum].guntex = your_gun[current_gun].skin_tex_id;
	player_list[trueplayernum].damage1 = your_gun[current_gun].damage1;
	player_list[trueplayernum].damage2 = your_gun[current_gun].damage2;

	MakeDamageDice();

	//if (strstr(your_gun[loopguns].gunname, "FLAME") != NULL ||
	//	strstr(your_gun[loopguns].gunname, "ICE") != NULL ||
	//	strstr(your_gun[loopguns].gunname, "LIGHTNINGSWORD") != NULL)
	//{
	//	bIsFlashlightOn = TRUE;
	//	lighttype = 2;
	//}
	//else
	//{
	//	lighttype = 0;
	//	bIsFlashlightOn = FALSE;
	//}
}


void GiveWeapons()
{

	strcpy_s(gActionMessage, "Give all weapons.");
	UpdateScrollList(0, 245, 255);
	player_list[trueplayernum].keys = 10;

	int i = 0;

	for (i = 0; i < MAX_NUM_GUNS; i++)
	{

		if (i < 10)
			your_gun[i].active = 1;

		your_gun[i].x_offset = 999.0f;
	}

	player_list[trueplayernum].gold = 99999;

	your_gun[18].active = 1;

	your_gun[19].active = 1;
	your_gun[20].active = 1;
	your_gun[21].active = 1;
}




void smooth_mouse(float time_d, float realx, float realy) {
	double d = 1 - exp(log(0.5) * springiness * time_d);

	use_x += (realx - use_x) * d;
	use_y += (realy - use_y) * d;

	//sprintf(gActionMessage, "%9.6f %9.6f",use_x,use_y);
	//UpdateScrollList(0, 245, 255);

	filterx = use_x;
	filtery = use_y;
}




void level() {


	if (currentlevel < 1)
		currentlevel = 19;

	if (currentlevel > 19)
		currentlevel = 1;

	char level[255];
	CLoadWorld* pCWorld;
	pCWorld = new CLoadWorld();

	foundsomething = 1;
	num_players2 = 0;
	itemlistcount = 0;
	num_monsters = 0;

	ClearObjectList();
	ResetSound();
	
	sprintf_s(levelname, "level%d", currentlevel);
	sprintf_s(gActionMessage, "Loading %s...", levelname);
	UpdateScrollList(0, 245, 255);
	int current_level = 1;
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
}

