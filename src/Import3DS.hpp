#ifndef __IMPORT3DS_H
#define __IMPORT3DS_H

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include "world.hpp"

#define MAX_NUM_3DS_TRIANGLES 10000
#define MAX_NUM_3DS_VERTICES 10000
#define MAX_NUM_3DS_FACES 10000
#define MAX_NAME_LENGTH 256
#define MAX_NUM_3DS_TEXTURES 200
//#define  MAX_NUM_3DS_FRAMES		50
#define MAX_NUM_3DS_FRAMES 1
#define MAX_NUM_3DS_OBJECTS 10000

#define MAIN3DS 0x4D4D // level 1
#define M3DS_VERSION 0x0002
#define MESH_VERSION 0x3D3E

#define EDIT3DS 0x3D3D					// level 1
#define NAMED_OBJECT 0x4000				// level 2
#define TRIANGLE_MESH 0x4100			// level 3
#define TRIANGLE_VERTEXLIST 0x4110		// level 4
#define TRIANGLE_VERTEXOPTIONS 0x4111	// level 4
#define TRIANGLE_MAPPINGCOORS 0x4140	// level 4
#define TRIANGLE_MAPPINGSTANDARD 0x4170 // level 4
#define TRIANGLE_FACELIST 0x4120		// level 4
#define TRIANGLE_SMOOTH 0x4150			// level 5
#define TRIANGLE_MATERIAL 0x4130		// level 5
#define TRI_LOCAL 0x4160				// level 4
#define TRI_VISIBLE 0x4165				// level 4

#define INT_PERCENTAGE 0x0030
#define MASTER_SCALE 0x0100

#define EDIT_MATERIAL 0xAFFF // level 2
#define MAT_NAME01 0xA000	 // level 3

#define TEXTURE_MAP 0xA200	// level 4?
#define MAPPING_NAME 0xA300 // level 5?

#define MATERIAL_AMBIENT 0xA010
#define MATERIAL_DIFFUSE 0xA020
#define MATERIAL_SPECULAR 0xA030
#define MATERIAL_SHININESS 0xA040
#define MATERIAL_SHIN_STRENGTH 0xA041

#define MAPPING_PARAMETERS 0xA351
#define BLUR_PERCENTAGE 0xA353

#define TRANS_PERCENT 0xA050
#define TRANS_FALLOFF_PERCENT 0xA052
#define REFLECTION_BLUR_PER 0xA053
#define RENDER_TYPE 0xA100

#define SELF_ILLUM 0xA084
#define WIRE_THICKNESS 0xA087
#define IN_TRANC 0xA08A
#define SOFTEN 0xA08C

#define KEYFRAME 0xB000 // level 1
#define KEYFRAME_MESH_INFO 0xB002
#define KEYFRAME_START_AND_END 0xB008
#define KEYFRAME_HEADER 0xb00a

#define POS_TRACK_TAG 0xb020
#define ROT_TRACK_TAG 0xb021
#define SCL_TRACK_TAG 0xb022
#define FOV_TRACK_TAG 0xb023
#define ROLL_TRACK_TAG 0xb024
#define COL_TRACK_TAG 0xb025
#define MORPH_TRACK_TAG 0xb026
#define HOT_TRACK_TAG 0xb027
#define FALL_TRACK_TAG 0xb028
#define HIDE_TRACK_TAG 0xb029

#define PIVOT 0xb013

#define NODE_HDR 0xb010
#define NODE_ID 0xb030
#define KFCURTIME 0xb009

typedef struct
{
	float x;
	float y;
	float z;

} VERT3DS;

typedef struct
{
	short v[4];
	int tex;

} FACE3DS;

typedef struct
{
	float x;
	float y;

} MAPPING_COORDINATES;

typedef struct
{
	VERT3DS verts[3];
	MAPPING_COORDINATES texmapcoords[3];
	short material;
	short tex_alias;
	char mat_name[MAX_NAME_LENGTH];

} TRIANGLE3DS;

typedef struct
{
	short framenum;
	long lunknown;
	float rotation_rad;
	float axis_x;
	float axis_y;
	float axis_z;

} ROT_TRACK3DS;

typedef struct
{
	short framenum;
	long lunknown;
	float pos_x;
	float pos_y;
	float pos_z;

} POS_TRACK3DS;

typedef struct
{
	short framenum;
	long lunknown;
	float scale_x;
	float scale_y;
	float scale_z;

} SCL_TRACK3DS;

typedef struct
{
	int verts_start;
	int verts_end;
	short rotkeys;
	short poskeys;
	short sclkeys;
	VERT3DS pivot;
	ROT_TRACK3DS rot_track[MAX_NUM_3DS_FRAMES];
	POS_TRACK3DS pos_track[MAX_NUM_3DS_FRAMES];
	SCL_TRACK3DS scl_track[MAX_NUM_3DS_FRAMES];

	float local_centre_x;
	float local_centre_y;
	float local_centre_z;

} OBJECT3DS;

BOOL Import3DS(char* filename, int pmodel_id, float scale);

BOOL ProcessVertexData(FILE* fp);
BOOL ProcessFaceData(FILE* fp);
void ProcessTriLocalData(FILE* fp);
void ProcessTriSmoothData(FILE* fp);
void ProcessMappingData(FILE* fp);
void ProcessMaterialData(FILE* fp, int pmodel_id);
void AddMaterialName(FILE* fp);
void AddMapName(FILE* fp, int pmodel_id);

void ProcessPositionTrack(FILE* fp);
void ProcessRotationTrack(FILE* fp);
void ProcessPivots(FILE* fp);
void ProcessScaleTrack(FILE* fp);

void ProcessNodeHeader(FILE* fp);
void ProcessNodeId(FILE* fp);

void ProcessMasterScale(FILE* fp);
void Process3DSVersion(FILE* fp);

void PrintLogFile(FILE* logfile, char* commmand);
void Write_pmdata_debugfile(int pmodel_id);
void ReleaseTempMemory();


#endif //__IMPORT3DS_H