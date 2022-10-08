//--------------------------------------------------------------------------------------
// File: XAudio2BasicSound.cpp
//
// Simple playback of a .WAV file using XAudio2
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License (MIT).
//--------------------------------------------------------------------------------------
#define WIN32_LEAN_AND_MEAN

#include <xaudio2.h>
#include <Windows.h>
#include <cstdio>
#include <wrl\client.h>
#include "XAudio2Versions.h"
#include "WAVFileReader.h"
#include "world.hpp"

#include <timeapi.h>
#pragma comment( lib, "Winmm.lib" )

// Uncomment to enable the volume limiter on the master voice.
//#define MASTERING_LIMITER

using Microsoft::WRL::ComPtr;

#define MAX_SOUND_BUFFER 40

typedef struct SoundBuf
{
    char name[255];
    IXAudio2SourceVoice* pSourceVoice;
    std::unique_ptr<uint8_t[]> waveFile;
    DirectX::WAVData waveData;
} SOUNDBUFFER;

typedef struct soundlist_typ
{
    int id;
    char name[100];
    char file[100];
    int type;
    int playing;
    int dist;
    IXAudio2SourceVoice* pSourceVoice;
    int soundbufferid;
} SOUNDLIST, * soundlist_ptr;

int soundbuffercount = 0;
int numsounds = 0;
SOUNDBUFFER* sound_buffer;

SOUNDLIST* sound_list;
BOOL LoadSoundFiles(char* filename);

//--------------------------------------------------------------------------------------
// Forward declaration
//--------------------------------------------------------------------------------------
HRESULT PlayWave(_In_z_ LPCWSTR szFilename);
HRESULT FindMediaFileCch(_Out_writes_(cchDest) WCHAR* strDestPath, _In_ int cchDest, _In_z_ LPCWSTR strFilename);


ComPtr<IXAudio2> pXAudio2;
IXAudio2SourceVoice* pSourceVoice;
std::unique_ptr<uint8_t[]> waveFile;
IXAudio2MasteringVoice* pMasteringVoice = nullptr;


int musicon = 1;
int playingsong = 0;
int skipmusic = 0;
int nummidi = 0;
int numcoresounds = 0;
int WaveSongPlaying(int id);
void MakeWave(char* file, std::unique_ptr<uint8_t[]> waveFile);

//--------------------------------------------------------------------------------------
// Entry point to the program
//--------------------------------------------------------------------------------------
int SoundInit()
{
    //
    // Initialize XAudio2
    //
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr))
    {
        wprintf(L"Failed to init COM: %#X\n", static_cast<unsigned long>(hr));
        return 0;
    }

#ifdef USING_XAUDIO2_7_DIRECTX
    // Workaround for XAudio 2.7 known issue
#ifdef _DEBUG
    HMODULE mXAudioDLL = LoadLibraryExW(L"XAudioD2_7.DLL", nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */);
#else
    HMODULE mXAudioDLL = LoadLibraryExW(L"XAudio2_7.DLL", nullptr, 0x00000800 /* LOAD_LIBRARY_SEARCH_SYSTEM32 */);
#endif
    if (!mXAudioDLL)
    {
        wprintf(L"Failed to find XAudio 2.7 DLL");
        CoUninitialize();
        return 0;
    }
#endif // USING_XAUDIO2_7_DIRECTX

    UINT32 flags = 0;
#if defined(USING_XAUDIO2_7_DIRECTX) && defined(_DEBUG)
    flags |= XAUDIO2_DEBUG_ENGINE;
#endif

    hr = XAudio2Create(pXAudio2.GetAddressOf(), flags);
    if (FAILED(hr))
    {
        wprintf(L"Failed to init XAudio2 engine: %#X\n", hr);
        CoUninitialize();
        return 0;
    }

#if !defined(USING_XAUDIO2_7_DIRECTX) && defined(_DEBUG)
    // To see the trace output, you need to view ETW logs for this application:
    //    Go to Control Panel, Administrative Tools, Event Viewer.
    //    View->Show Analytic and Debug Logs.
    //    Applications and Services Logs / Microsoft / Windows / XAudio2. 
    //    Right click on Microsoft Windows XAudio2 debug logging, Properties, then Enable Logging, and hit OK 
    XAUDIO2_DEBUG_CONFIGURATION debug = {};
    debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
    debug.BreakMask = XAUDIO2_LOG_ERRORS;
    pXAudio2->SetDebugConfiguration(&debug, 0);
#endif

    //
    // Create a mastering voice
    //


    if (FAILED(hr = pXAudio2->CreateMasteringVoice(&pMasteringVoice)))
    {
        wprintf(L"Failed creating mastering voice: %#X\n", hr);
        pXAudio2.Reset();
        CoUninitialize();
        return 0;
    }

#ifdef MASTERING_LIMITER
    FXMASTERINGLIMITER_PARAMETERS params = {};
    params.Release = FXMASTERINGLIMITER_DEFAULT_RELEASE;
    params.Loudness = FXMASTERINGLIMITER_DEFAULT_LOUDNESS;

    ComPtr<IUnknown> pVolumeLimiter;
    hr = CreateFX(__uuidof(FXMasteringLimiter), &pVolumeLimiter, &params, sizeof(params));
    if (FAILED(hr))
    {
        wprintf(L"Failed creating mastering limiter: %#X\n", static_cast<unsigned long>(hr));
        pXAudio2.Reset();
        CoUninitialize();
        return hr;
    }

    UINT32 nChannels;
#ifndef USING_XAUDIO2_7_DIRECTX
    XAUDIO2_VOICE_DETAILS details;
    pMasteringVoice->GetVoiceDetails(&details);
    nChannels = details.InputChannels;
#else
    XAUDIO2_DEVICE_DETAILS details;
    hr = pXAudio2->GetDeviceDetails(0, &details);
    if (FAILED(hr))
    {
        wprintf(L"Failed getting voice details: %#X\n", hr);
        pXAudio2.Reset();
        CoUninitialize();
    }
    nChannels = details.OutputFormat.Format.nChannels;
#endif

    XAUDIO2_EFFECT_DESCRIPTOR desc = {};
    desc.InitialState = TRUE;
    desc.OutputChannels = nChannels;
    desc.pEffect = pVolumeLimiter.Get();

    XAUDIO2_EFFECT_CHAIN chain = { 1, &desc };
    hr = pMasteringVoice->SetEffectChain(&chain);
    if (FAILED(hr))
    {
        pXAudio2.Reset();
        pVolumeLimiter.Reset();
        CoUninitialize();
        return hr;
    }
#endif // MASTERING_LIMITER

    sound_buffer = new SOUNDBUFFER[250];
    sound_list = new SOUNDLIST[1250];
    LoadSoundFiles("sounds.dat");

    return 1;
}

void ShutDownSound() {

    //
    // Cleanup XAudio2
    //
    wprintf(L"\nFinished playing\n");

    // All XAudio2 interfaces are released when the engine is destroyed, but being tidy
    pMasteringVoice->DestroyVoice();

    pXAudio2.Reset();

#ifdef USING_XAUDIO2_7_DIRECTX
    if (mXAudioDLL)
        FreeLibrary(mXAudioDLL);
#endif

    CoUninitialize();

}

void PlayWaveFile(char* filename) {

    HRESULT hr;

    wchar_t wtext[120];

    //todo: fix this
    //mbstowcs(wtext, filename, strlen(filename) + 1);//Plus null

    hr = PlayWave(wtext);

    if (FAILED(hr))
    {
        wprintf(L"Failed creating source voice: %#X\n", hr);
        pXAudio2.Reset();
        CoUninitialize();
        return;
    }
}

//--------------------------------------------------------------------------------------
// Name: PlayWave
// Desc: Plays a wave and blocks until the wave finishes playing
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT PlayWave(LPCWSTR szFilename)
{
    //
    // Locate the wave file
    //
    WCHAR strFilePath[MAX_PATH];
    HRESULT hr = FindMediaFileCch(strFilePath, MAX_PATH, szFilename);
    if (FAILED(hr))
    {
        wprintf(L"Failed to find media file: %s\n", szFilename);
        return hr;
    }

    //
    // Read in the wave file
    //

    DirectX::WAVData waveData;

    if (FAILED(hr = DirectX::LoadWAVAudioFromFileEx(strFilePath, waveFile, waveData)))
    {
        wprintf(L"Failed reading WAV file: %#X (%s)\n", hr, strFilePath);
        return hr;
    }

    //
    // Play the wave using a XAudio2SourceVoice
    //

    // Create the source voice

    if (FAILED(hr = pXAudio2->CreateSourceVoice(&pSourceVoice, waveData.wfx)))
    {
        wprintf(L"Error %#X creating source voice\n", hr);
        return hr;
    }

    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = waveData.startAudio;
    buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
    buffer.AudioBytes = waveData.audioBytes;

    if (waveData.loopLength > 0)
    {
        buffer.LoopBegin = waveData.loopStart;
        buffer.LoopLength = waveData.loopLength;
        buffer.LoopCount = 1; // We'll just assume we play the loop twice
    }

#if defined(USING_XAUDIO2_7_DIRECTX) || defined(USING_XAUDIO2_9)
    if (waveData.seek)
    {
        XAUDIO2_BUFFER_WMA xwmaBuffer = {};
        xwmaBuffer.pDecodedPacketCumulativeBytes = waveData.seek;
        xwmaBuffer.PacketCount = waveData.seekCount;
        if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer, &xwmaBuffer)))
        {
            wprintf(L"Error %#X submitting source buffer (xWMA)\n", hr);
            pSourceVoice->DestroyVoice();
            return hr;
        }
    }
#else
    if (waveData.seek)
    {
        wprintf(L"This platform does not support xWMA or XMA2\n");
        pSourceVoice->DestroyVoice();
        return hr;
    }
#endif
    else if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer)))
    {
        wprintf(L"Error %#X submitting source buffer\n", hr);
        pSourceVoice->DestroyVoice();
        return hr;
    }

    hr = pSourceVoice->Start(0);

    return hr;
}

//--------------------------------------------------------------------------------------
// Helper function to try to find the location of a media file
//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT FindMediaFileCch(WCHAR* strDestPath, int cchDest, LPCWSTR strFilename)
{
    bool bFound = false;

    if (!strFilename || strFilename[0] == 0 || !strDestPath || cchDest < 10)
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] = {};
    WCHAR strExeName[MAX_PATH] = {};
    WCHAR* strLastSlash = nullptr;
    GetModuleFileName(nullptr, strExePath, MAX_PATH);
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr(strExePath, TEXT('\\'));
    if (strLastSlash)
    {
        wcscpy_s(strExeName, MAX_PATH, &strLastSlash[1]);

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr(strExeName, TEXT('.'));
        if (strLastSlash)
            *strLastSlash = 0;
    }

    wcscpy_s(strDestPath, cchDest, strFilename);
    if (GetFileAttributes(strDestPath) != 0xFFFFFFFF)
        return S_OK;

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    WCHAR strLeafName[MAX_PATH] = {};
    wcscpy_s(strLeafName, MAX_PATH, strFilename);

    WCHAR strFullPath[MAX_PATH] = {};
    WCHAR strFullFileName[MAX_PATH] = {};
    WCHAR strSearch[MAX_PATH] = {};
    WCHAR* strFilePart = nullptr;

    GetFullPathName(L".", MAX_PATH, strFullPath, &strFilePart);
    if (!strFilePart)
        return E_FAIL;

    while (strFilePart && *strFilePart != '\0')
    {
        swprintf_s(strFullFileName, MAX_PATH, L"%s\\%s", strFullPath, strLeafName);
        if (GetFileAttributes(strFullFileName) != 0xFFFFFFFF)
        {
            wcscpy_s(strDestPath, cchDest, strFullFileName);
            bFound = true;
            break;
        }

        swprintf_s(strFullFileName, MAX_PATH, L"%s\\%s\\%s", strFullPath, strExeName, strLeafName);
        if (GetFileAttributes(strFullFileName) != 0xFFFFFFFF)
        {
            wcscpy_s(strDestPath, cchDest, strFullFileName);
            bFound = true;
            break;
        }

        swprintf_s(strSearch, MAX_PATH, L"%s\\..", strFullPath);
        GetFullPathName(strSearch, MAX_PATH, strFullPath, &strFilePart);
    }
    if (bFound)
        return S_OK;

    // On failure, return the file as the path but also return an error code
    wcscpy_s(strDestPath, cchDest, strFilename);

    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

void MakeWave(char* file, std::unique_ptr<uint8_t[]> waveFile) {


    HRESULT hr;

    DirectX::WAVData waveData;
    hr = DirectX::LoadWAVAudioFromFileEx(charToWChar(file), waveFile, waveData);
    //{
        //wprintf(L"Failed reading WAV file: %#X (%s)\n", hr, strFilePath);
        //return;
   // }
}

BOOL LoadSoundFiles(char* filename)
{
    FILE* fp;

    int soundid = 0;

    char name[256];
    char type[256];
    char file[256];

    int y_count = 30;
    int done = 0;
    int object_count = 0;
    int vert_count = 0;
    int pv_count = 0;
    int poly_count = 0;

    BOOL lwm_start_flag = TRUE;

    if (fopen_s(&fp, filename, "r") != 0)
    {
        //PrintMessage(hwnd, "Error can't load file ", filename, SCN_AND_FILE);
        //MessageBox(hwnd, "Error can't load file", NULL, MB_OK);
        return FALSE;
    }

    //PrintMessage(hwnd, "Loading ", filename, SCN_AND_FILE);
    numsounds = 0;
    nummidi = 0;

    while (done == 0)
    {
        fscanf_s(fp, "%s", &type, 256);
        fscanf_s(fp, "%s", &name, 256);
        fscanf_s(fp, "%s", &file, 256);

        if (strcmp(type, "WAV") == 0 || strcmp(type, "LOP") == 0 || strcmp(type, "MID") == 0)
        {
            strcpy_s(sound_list[numsounds].file, file);
            strcpy_s(sound_list[numsounds].name, name);

            //soundid = DSound_Load_WAV(file);
            //int soundid = 1;

            //if (soundid == -1)
            //{
                //MessageBox(hwnd, "sounds.dat wave file not found", "sounds.dat wave file not found", MB_OK);
            //}
            sound_list[numsounds].id = numsounds;


            if (strcmp(type, "WAV") == 0) {
                sound_list[numsounds].type = 0;
            }
            else if (strcmp(type, "LOP") == 0) {
                sound_list[numsounds].type = 2;
            }
            else if (strcmp(type, "MID") == 0) {
                sound_list[numsounds].type = 1;
                nummidi++;
            }

            sound_list[numsounds].playing = 0;

            //MakeWave(sound_list[numsounds].file, sound_buffer[numsounds].waveFile)

            HRESULT hr;

            TCHAR NPath[100];
            GetCurrentDirectory(100, NPath);

            //DirectX::WAVData waveData;
            hr = DirectX::LoadWAVAudioFromFileEx(charToWChar(file), sound_buffer[numsounds].waveFile, sound_buffer[numsounds].waveData);
            hr = pXAudio2->CreateSourceVoice(&sound_list[numsounds].pSourceVoice, sound_buffer[numsounds].waveData.wfx);
            strcpy_s(sound_buffer[numsounds].name, name);
            sound_list[numsounds].soundbufferid = numsounds;


            numsounds++;
        }

        if (strcmp(type, "END_FILE") == 0)
        {

            done = 1;
        }
    }
    fclose(fp);

    numcoresounds = numsounds;

    return TRUE;
}

int SoundID(char* name)
{
    int i = 0;
    int id = 0;

    for (i = 0; i < numsounds; i++)
    {
        if (strcmp(name, sound_list[i].name) == 0)
        {
            //id = sound_list[i].id;

            id = i;
            break;
        }
    }
    return id;
}

void PlayWavSound(int id, int volume)
{
    HRESULT hr;

    int soundbufferid = sound_list[id].soundbufferid;


    sound_list[id].playing = 1;

    sound_list[id].pSourceVoice->Stop(0);
    sound_list[id].pSourceVoice->FlushSourceBuffers();

    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = {};
    buffer.pAudioData = sound_buffer[soundbufferid].waveData.startAudio;

    buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
    buffer.AudioBytes = sound_buffer[soundbufferid].waveData.audioBytes;

    if (sound_buffer[soundbufferid].waveData.loopLength > 0)
    {
        buffer.LoopBegin = sound_buffer[soundbufferid].waveData.loopStart;
        buffer.LoopLength = sound_buffer[soundbufferid].waveData.loopLength;
        buffer.LoopCount = 1; // We'll just assume we play the loop twice
    }

#if defined(USING_XAUDIO2_7_DIRECTX) || defined(USING_XAUDIO2_9)
    if (sound_buffer[soundbufferid].waveData.seek)
    {
        XAUDIO2_BUFFER_WMA xwmaBuffer = {};
        xwmaBuffer.pDecodedPacketCumulativeBytes = sound_buffer[soundbufferid].waveData.seek;
        xwmaBuffer.PacketCount = sound_buffer[soundbufferid].waveData.seekCount;
        if (FAILED(hr = sound_list[id].pSourceVoice->SubmitSourceBuffer(&buffer, &xwmaBuffer)))
        {
            wprintf(L"Error %#X submitting source buffer (xWMA)\n", hr);
            sound_list[id].pSourceVoice->DestroyVoice();
            return;
        }
    }
#else
    if (waveData.seek)
    {
        wprintf(L"This platform does not support xWMA or XMA2\n");
        sound_list[id].pSourceVoice->DestroyVoice();
        return;
    }
#endif
    else if (FAILED(hr = sound_list[id].pSourceVoice->SubmitSourceBuffer(&buffer)))
    {
        wprintf(L"Error %#X submitting source buffer\n", hr);
        sound_list[id].pSourceVoice->DestroyVoice();
        return;
    }

    hr = sound_list[id].pSourceVoice->SetVolume((float)volume / 100.0f);
    hr = sound_list[id].pSourceVoice->Start(0);
}

int DSound_Delete_Sound(int id)
{
    //sound_list[id].pSourceVoice->Stop(0);
    //sound_list[id].pSourceVoice->FlushSourceBuffers();
    sound_list[id].pSourceVoice->DestroyVoice();
    sound_list[id].type = -1;
    sound_list[id].playing = 0;
    strcpy_s(sound_list[id].name, "");
    sound_list[id].dist = 0;

    return (1);

} // end DSound_Delete_Sound

int  DSound_Replicate_Sound(int id) {

    int currentsound = FindSoundSlot();

    sound_list[currentsound].id = currentsound;
    sound_list[currentsound].type = 0;
    sound_list[currentsound].playing = 0;

    HRESULT hr;

    hr = pXAudio2->CreateSourceVoice(&sound_list[currentsound].pSourceVoice, sound_buffer[id].waveData.wfx);

    sound_list[currentsound].soundbufferid = id;
    sprintf_s(sound_list[currentsound].name, "%d %s", currentsound, sound_buffer[id].name);

    return currentsound;
}

int FindSoundSlot() {

    int currentsound = numsounds;

    //Reuse an old sound
    for (int i = 0; i < numsounds; i++)
    {
        if (sound_list[i].type == -1)
        {
            return i;
        }
    }

    numsounds++;
    return currentsound;
}

void PlaySong() {

    int raction = random_num(nummidi);
    int count = 0;

    if (WaveSongPlaying(playingsong)) {
        sound_list[playingsong].pSourceVoice->Stop(0);
        sound_list[playingsong].pSourceVoice->FlushSourceBuffers();
    }

    for (int i = 0; i < numsounds; i++)
    {
        if (sound_list[i].type == 1)
        {
            if (count == raction) {
                PlayWavSound(SoundID(sound_list[i].name), 90);
                playingsong = i;
                break;
            }
            count++;
        }
    }
}

void StopMusic() {

    if (WaveSongPlaying(playingsong)) {
        sound_list[playingsong].pSourceVoice->Stop(0);
        sound_list[playingsong].pSourceVoice->FlushSourceBuffers();
    }
}

int WaveSongPlaying(int id) {

    bool isRunning;

    XAUDIO2_VOICE_STATE state;
    sound_list[id].pSourceVoice->GetState(&state);
    isRunning = (state.BuffersQueued > 0) != 0;

    sound_list[id].playing = isRunning;

    if (!isRunning) {
        sound_list[id].playing = 0;
        playingsong = 0;
    }

    return isRunning;
}

int WavePlaying(int id) {

    bool isRunning;

    XAUDIO2_VOICE_STATE state;
    sound_list[id].pSourceVoice->GetState(&state);
    isRunning = (state.BuffersQueued > 0) != 0;

    sound_list[id].playing = isRunning;

    if (!isRunning) {
        sound_list[id].playing = 0;
    }

    return isRunning;
}

void CheckMidiMusic()
{
    int raction;
    static FLOAT fLastTimeMusic = 0;
    int i = 0;

    FLOAT fTime = timeGetTime() * 0.01f; // Get current time in seconds

    if (fTime - fLastTimeMusic > 280.0f)//&& skipmusic == 1)
    {
        skipmusic = 0;
        fLastTimeMusic = fTime;

        raction = random_num(4);

        if (raction == 1)
            PlayWavSound(SoundID("effect1"), 100);
        else if (raction == 2)
            PlayWavSound(SoundID("effect2"), 100);
        else if (raction == 3)
            PlayWavSound(SoundID("effect3"), 100);
        else
            PlayWavSound(SoundID("effect4"), 100);
    }

    //if (DMusic_Status_MIDI(currentmidi) != MIDI_STOPPED && musicon == 0)
    //{

    //    DMusic_Stop(currentmidi);
    //}


    if (playingsong != 0)
        WaveSongPlaying(playingsong);


    if (playingsong == 0 && skipmusic == 0 && musicon == 1)
    {
        raction = random_num(2);
        skipmusic = 0;

        PlaySong();
    }
}

int ResetSound()
{

    bool loop = true;

    while (loop) {
        loop = false;

        for (int i = numcoresounds; i < numsounds; i++) {

            WavePlaying(i);
            if (sound_list[i].playing) {
                loop = true;
            }
        }
        Sleep(100);
    }

    for (int i = numcoresounds; i < numsounds; i++)
    {
        //DSound_Delete_Sound(i);
        sound_list[i].type = 0;
        //sound_list[i].pSourceVoice->DestroyVoice();
    }
    numsounds = numcoresounds;

    return 1;
}
