//--------------------------------------------------------------------------------------
// File: XAudio2Versions.h
//
// Defines preprocessor symbols to distinguish between different versions of XAudio2,
// and includes the appropriate header files for the XAudio2 version used.
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//--------------------------------------------------------------------------------------
#pragma once

// In case _WIN32_WINNT is not already defined, this header will define it to the highest
// version supported by the Windows SDK.
#include <sdkddkver.h>

// Set preprocessor symbols for the different versions of XAudio2.
#ifdef USING_XAUDIO2_REDIST
// The NuGet redistributable always uses XAudio 2.9 (xaudio2_9redist.dll) on all OSs (Windows 7 SP1 or later).
#define USING_XAUDIO2_9
#elif (_WIN32_WINNT >= 0x0A00 /*_WIN32_WINNT_WIN10*/)
// When targeting Windows 10, XAudio 2.9 is used
#define USING_XAUDIO2_9
#elif (_WIN32_WINNT >= 0x0602 /*_WIN32_WINNT_WIN8*/)
// When targeting Windows 8, XAudio 2.8 is used when running on Windows 8 and 8.1,
// and XAudio 2.9 is used on Windows 10. XAudio 2.8 is the lowest common denominator.
#define USING_XAUDIO2_8
#else
// When targeting Windows XP, xaudio2_7.dll from the DirectX SDK is
// used on all OSs.
#define USING_XAUDIO2_7_DIRECTX
#endif


int  DSound_Replicate_Sound(int id);
int DSound_Delete_Sound(int id);
int FindSoundSlot();
extern int musicon;
void StopWav(int id);

#ifdef USING_XAUDIO2_REDIST
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <xapofx.h>
#else
#ifndef USING_XAUDIO2_7_DIRECTX
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#include <xapofx.h>
#pragma comment(lib,"xaudio2.lib")
#else
#include <C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\comdecl.h>
#include <C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\xaudio2.h>
#include <C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\xaudio2fx.h>
#include <C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\xapofx.h>
#pragma warning(push)
#pragma warning( disable : 4005 )
#include <C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include\x3daudio.h>
#pragma warning(pop)
#pragma comment(lib,"x3daudio.lib")
#pragma comment(lib,"xapofx.lib")
#endif // USING_XAUDIO2_7_DIRECTX
#endif // USING_XAUDIO2_REDIST
