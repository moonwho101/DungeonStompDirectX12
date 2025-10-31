
#ifndef __IMPORTMD2_H
#define __IMPORTMD2_H

#include <windows.h>
#include "world.hpp"

#define MAX_SKINNAME 64
#define MAX_NUM_SKINS 64

typedef struct
{
	float s;
	float t;
	int index;
} GLVERT;

typedef struct
{
	short s;
	short t;

} MD2TEXVERT;

typedef struct
{
	short face_index[3];
	short tex_index[3];

} MD2FACE;

typedef struct
{
	BYTE v[3];
	BYTE lightnormalindex;

} MD2VERTEX;

typedef struct
{
	float s;
	float t;
	short index;

} MD2VERTINDEX;

typedef struct
{
	int ident;
	int version;

	int skinwidth;
	int skinheight;
	int framesize;

	int num_skins;
	int num_verts;
	int num_tex_verts;
	int num_faces;
	int num_glcmds;
	int num_frames;

	int offset_skins;
	int offset_tex_verts;
	int offset_faces;
	int offset_frames;
	int offset_glcmds;
	int offset_end;

} MD2HEADER;

typedef struct
{
	MD2VERTINDEX vert_index[3];

} FACE;

typedef struct
{
	char framename[16];
	VERT v[600];
} FRAME;

BOOL ImportMD2_GLCMD(char *filename, int texture_alias, int pmodel_id, float scale);

#endif //__IMPORTMD2_H