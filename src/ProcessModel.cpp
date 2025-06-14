#include "d3dtypes.h"
#include "LoadWorld.hpp"
#include "world.hpp"
#include "GlobalSettings.hpp"
#include <string.h>
#include "GameLogic.hpp"
#include "Missle.hpp"
#include "../Common/MathHelper.h"
#include <cstddef>
#include <functional>
#include <unordered_map>

int itemlistcount = 0;

void ConvertTraingleStrip(int fan_cnt);

void ConvertTraingleFan(int fan_cnt);
extern OBJECTLIST* oblist;
extern OBJECTDATA* obdata;
extern D3DVERTEX2 boundingbox[2000];
int cnt_f = 0;
float px[100], py[100], pz[100], pw[100];
float mx[100], my[100], mz[100], mw[100];
float cx[100], cy[100], cz[100], cw[100];
float tx[10000], ty[10000];

int sharedv[1000];
int track[60000];


void SmoothNormals(int start_cnt);
extern SWITCHMOD* switchmodify;
int countswitches = 0;

int* verts_per_poly;
int number_of_polys_per_frame;
int* faces_per_poly;
int* src_f;
D3DVERTEX2 temp_v[120000]; // debug
int tempvcounter = 0;
D3DVERTEX2* src_v;
int drawthistri = 1;
extern float culldist;

D3DPRIMITIVETYPE* dp_commands;

BOOL* dp_command_index_mode;

#define USE_INDEXED_DP			0
#define USE_NON_INDEXED_DP		1

float k = (float)0.017453292;

struct CUSTOMVERTEX { FLOAT X, Y, Z; DWORD COLOR; };

//LPDIRECT3DVERTEXBUFFER9 v_buffer = NULL;    // the pointer to the vertex buffer

float sin_table[361];
float cos_table[361];

int src_collide[MAX_NUM_QUADS];

//LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL; // Buffer to hold vertices
//LPDIRECT3DVERTEXBUFFER9 g_pVBBoundingBox = NULL; // Buffer to hold vertices
//LPDIRECT3DVERTEXBUFFER9 g_pVBMonsterCaption = NULL; // Buffer to hold vertices

float playerx = 0;
float playery = 0;
float playerz = 0;
float rotatex = 0.0f;
float rotatey = 0.0f;

FLOAT fTimeKeysave = 0;

PLAYER* item_list;
PLAYER* player_list2;
PLAYER* player_list;

int* texture_list_buffer;
int g_ob_vert_count = 0;
extern TEXTUREMAPPING  TexMap[MAX_NUM_TEXTURES];
void DrawModel();
int num_light_sources = 0;

extern float gametimerAnimation;

void ConvertQuad(int fan_cnt);

void CalculateTangentBinormal(D3DVERTEX2& vertex1, D3DVERTEX2& vertex2, D3DVERTEX2& vertex3)
{
    float vector1[3], vector2[3];
    float tuVector[2], tvVector[2];
    float den, length;

    // Calculate the two vectors for this face.
    vector1[0] = vertex2.x - vertex1.x;
    vector1[1] = vertex2.y - vertex1.y;
    vector1[2] = vertex2.z - vertex1.z;

    vector2[0] = vertex3.x - vertex1.x;
    vector2[1] = vertex3.y - vertex1.y;
    vector2[2] = vertex3.z - vertex1.z;

    // Calculate the tu and tv texture space vectors.
    tuVector[0] = vertex2.tu - vertex1.tu;
    tvVector[0] = vertex2.tv - vertex1.tv;

    tuVector[1] = vertex3.tu - vertex1.tu;
    tvVector[1] = vertex3.tv - vertex1.tv;

    // Calculate the denominator of the tangent/binormal equation.
    float result = (tuVector[0] * tvVector[1] - tuVector[1] * tvVector[0]);

    if (result == 0) {
        vertex1.nmx = vertex1.nmy = vertex1.nmz = 0;
        vertex2.nmx = vertex2.nmy = vertex2.nmz = 0;
        vertex3.nmx = vertex3.nmy = vertex3.nmz = 0;
        return;
    }

    den = 1.0f / result;

    // Calculate the cross products and multiply by the coefficient to get the tangent and binormal.
    D3DVERTEX2 tangent, binormal;
    tangent.x = (tvVector[1] * vector1[0] - tvVector[0] * vector2[0]) * den;
    tangent.y = (tvVector[1] * vector1[1] - tvVector[0] * vector2[1]) * den;
    tangent.z = (tvVector[1] * vector1[2] - tvVector[0] * vector2[2]) * den;

    binormal.x = (tuVector[0] * vector2[0] - tuVector[1] * vector1[0]) * den;
    binormal.y = (tuVector[0] * vector2[1] - tuVector[1] * vector1[1]) * den;
    binormal.z = (tuVector[0] * vector2[2] - tuVector[1] * vector1[2]) * den;

    // Normalize the tangent
    length = sqrtf(tangent.x * tangent.x + tangent.y * tangent.y + tangent.z * tangent.z);
    tangent.x /= length;
    tangent.y /= length;
    tangent.z /= length;

    // Normalize the binormal
    length = sqrtf(binormal.x * binormal.x + binormal.y * binormal.y + binormal.z * binormal.z);
    binormal.x /= length;
    binormal.y /= length;
    binormal.z /= length;

    vertex1.nmx = vertex2.nmx = vertex3.nmx = tangent.x;
    vertex1.nmy = vertex2.nmy = vertex3.nmy = tangent.y;
    vertex1.nmz = vertex2.nmz = vertex3.nmz = tangent.z;
}


bool ObjectHasShadow(int object_id) {

	if (object_id == -99 || object_id == -111 || object_id == -1) {
		return false;
	}

	if (obdata[object_id].shadow) {
		return true;
	}

	return false;
}

void ObjectToD3DVertList(int ob_type, float angle, int oblist_index)
{

	int ob_vert_count = 0;
	int poly;
	float qdist = 0;
	int num_vert;
	int vert_cnt;
	int w, i;
	float x, y, z;

	D3DPRIMITIVETYPE poly_command;
	BOOL poly_command_error = TRUE;

	float workx, worky, workz;
	int countnorm = 0;

	float zaveragedist;
	int zaveragedistcount = 0;

	float  wx = oblist[oblist_index].x;
	float  wy = oblist[oblist_index].y;
	float  wz = oblist[oblist_index].z;

	//float cosine = cos_table[angle];
	//float sine = sin_table[angle];

	float cosine = (float)cos(angle * k);
	float sine = (float)sin(angle * k);

	ob_vert_count = 0;
	poly = num_polys_per_object[ob_type];

	int start_cnt = cnt;

	for (w = 0; w < poly; w++)
	{
		num_vert = obdata[ob_type].num_vert[w];

		mx[0] = 0.0f;
		mx[1] = 0.0f;
		mx[2] = 0.0f;
		mx[3] = 0.0f;
		my[0] = 0.0f;
		my[1] = 0.0f;
		my[2] = 0.0f;
		my[3] = 0.0f;
		mz[0] = 0.0f;
		mz[1] = 0.0f;
		mz[2] = 0.0f;
		mz[3] = 0.0f;

		zaveragedist = 0.0f;
		int ctext;

		int fan_cnt = -1;

		//if (p_command == D3DPT_TRIANGLEFAN) {
		fan_cnt = cnt;


		if (strstr(oblist[oblist_index].name, "!") != NULL)
		{
			ObjectsToDraw[number_of_polys_per_frame].texture = oblist[oblist_index].monstertexture;
			ctext = oblist[oblist_index].monstertexture;

			ObjectsToDraw[number_of_polys_per_frame].objectId = ob_type;
		}
		else
		{
			ObjectsToDraw[number_of_polys_per_frame].texture = obdata[ob_type].tex[w];
			ctext = obdata[ob_type].tex[w];

			ObjectsToDraw[number_of_polys_per_frame].objectId = ob_type;// -99;
		}


		//The object casts a shadow
		if (oblist[oblist_index].castshadow == 0) {
			ObjectsToDraw[number_of_polys_per_frame].castshaddow = 0;
		}
		else {
			ObjectsToDraw[number_of_polys_per_frame].castshaddow = 1;
		}

		texture_list_buffer[number_of_polys_per_frame] = ctext;

		int cresult;

		cresult = CycleBitMap(ctext);

		if (cresult != -1)
		{
			oblist[oblist_index].monstertexture = cresult;

			XMFLOAT3 normroadold;
			XMFLOAT3 work1, work2;

			normroadold.x = 50;
			normroadold.y = 0;
			normroadold.z = 0;

			work1.x = m_vEyePt.x;
			work1.y = m_vEyePt.y;
			work1.z = m_vEyePt.z;

			work2.x = wx;
			work2.y = wy;
			work2.z = wz;

			XMVECTOR P1 = XMLoadFloat3(&work1);
			XMVECTOR P2 = XMLoadFloat3(&work2);
			XMVECTOR vDiff = P1 - P2;

			XMVECTOR final = XMVector3Normalize(vDiff);
			XMVECTOR final2 = XMVector3Normalize(XMLoadFloat3(&normroadold));

			XMVECTOR fDotVector = XMVector3Dot(final, final2);
			float fDot = XMVectorGetX(fDotVector);

			float convangle;
			convangle = (float)acos(fDot) / k;

			fDot = convangle;

			if (work2.z < work1.z)
			{
			}
			else
			{
				fDot = 180.0f + (180.0f - fDot);
			}

			//cosine = cos_table[(int)fDot];
			//sine = sin_table[(int)fDot];

			cosine = (float)cos(fDot * k);
			sine = (float)sin(fDot * k);

			
		}

		for (vert_cnt = 0; vert_cnt < num_vert; vert_cnt++)
		{
			x = obdata[ob_type].v[ob_vert_count].x;
			y = obdata[ob_type].v[ob_vert_count].y;
			z = obdata[ob_type].v[ob_vert_count].z;

			tx[vert_cnt] = obdata[ob_type].t[ob_vert_count].x;
			ty[vert_cnt] = obdata[ob_type].t[ob_vert_count].y;

			mx[vert_cnt] = wx + (x * cosine - z * sine);
			my[vert_cnt] = wy + y;
			mz[vert_cnt] = wz + (x * sine + z * cosine);

			zaveragedist = zaveragedist + mz[vert_cnt];
			zaveragedistcount++;

			ob_vert_count++;
			g_ob_vert_count++;

		}
		float centroidx = (mx[0] + mx[1] + mx[2]) * QVALUE;
		float centroidy = (my[0] + my[1] + my[2]) * QVALUE;
		float centroidz = (mz[0] + mz[1] + mz[2]) * QVALUE;

		//zaveragedist = zaveragedist / zaveragedistcount;
		verts_per_poly[number_of_polys_per_frame] = num_vert;

		ObjectsToDraw[number_of_polys_per_frame].vertsperpoly = num_vert;
		ObjectsToDraw[number_of_polys_per_frame].srcstart = cnt;

		poly_command = obdata[ob_type].poly_cmd[w];

		if (obdata[ob_type].use_texmap[w] == FALSE)
		{
			for (i = 0; i < verts_per_poly[number_of_polys_per_frame]; i++)
			{
				src_v[cnt].x = D3DVAL(mx[i]);
				src_v[cnt].y = D3DVAL(my[i]);
				src_v[cnt].z = D3DVAL(mz[i]);
				src_v[cnt].tu = D3DVAL(tx[i]);
				src_v[cnt].tv = D3DVAL(ty[i]);

				if (objectcollide == 1)
					src_collide[cnt] = 1;
				else
					src_collide[cnt] = 0;

				if (i == 0)
				{
					XMFLOAT3  vw1, vw2, vw3;

					vw1.x = D3DVAL(mx[i]);
					vw1.y = D3DVAL(my[i]);
					vw1.z = D3DVAL(mz[i]);

					vw2.x = D3DVAL(mx[i + 1]);
					vw2.y = D3DVAL(my[i + 1]);
					vw2.z = D3DVAL(mz[i + 1]);

					vw3.x = D3DVAL(mx[i + 2]);
					vw3.y = D3DVAL(my[i + 2]);
					vw3.z = D3DVAL(mz[i + 2]);

					// calculate the NORMAL for the road using the CrossProduct <-important!

					XMVECTOR   vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
					XMVECTOR   vDiff2 = XMLoadFloat3(&vw3) - XMLoadFloat3(&vw2);

					XMVECTOR  vCross, final;
					vCross = XMVector3Cross(vDiff, vDiff2);
					final = XMVector3Normalize(vCross);
					XMFLOAT3 final2;
					XMStoreFloat3(&final2, final);

					workx = -final2.x;
					worky = -final2.y;
					workz = -final2.z;
				}

				src_v[cnt].nx = workx;
				src_v[cnt].ny = worky;
				src_v[cnt].nz = workz;

				cnt++;

				if (i == 2)
				{
					CalculateTangentBinormal(src_v[cnt - 3], src_v[cnt - 2], src_v[cnt - 1]);
				}

			}
		}
		else
		{
			for (i = 0; i < verts_per_poly[number_of_polys_per_frame]; i++)
			{
				src_v[cnt].x = D3DVAL(mx[i]);
				src_v[cnt].y = D3DVAL(my[i]);
				src_v[cnt].z = D3DVAL(mz[i]);

				src_v[cnt].tu = D3DVAL(TexMap[ctext].tu[i]);
				src_v[cnt].tv = D3DVAL(TexMap[ctext].tv[i]);

				if (objectcollide == 1)
					src_collide[cnt] = 1;
				else
					src_collide[cnt] = 0;

				if (i == 0)
				{
					XMFLOAT3 vw1, vw2, vw3;

					vw1.x = D3DVAL(mx[i]);
					vw1.y = D3DVAL(my[i]);
					vw1.z = D3DVAL(mz[i]);

					vw2.x = D3DVAL(mx[i + 1]);
					vw2.y = D3DVAL(my[i + 1]);
					vw2.z = D3DVAL(mz[i + 1]);

					vw3.x = D3DVAL(mx[i + 2]);
					vw3.y = D3DVAL(my[i + 2]);
					vw3.z = D3DVAL(mz[i + 2]);

					// calculate the NORMAL for the road using the CrossProduct <-important!
					XMVECTOR vCross, final;
					XMVECTOR   vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
					XMVECTOR   vDiff2 = XMLoadFloat3(&vw3) - XMLoadFloat3(&vw2);

					vCross = XMVector3Cross(vDiff, vDiff2);
					final = XMVector3Normalize(vCross);

					XMFLOAT3 final2;
					XMStoreFloat3(&final2, final);

					workx = -final2.x;
					worky = -final2.y;
					workz = -final2.z;
				}
				src_v[cnt].nx = workx;
				src_v[cnt].ny = worky;
				src_v[cnt].nz = workz;

				cnt++;

				if (i == 2)
				{
					CalculateTangentBinormal(src_v[cnt - 3], src_v[cnt - 2], src_v[cnt - 1]);
				}

			}
		}

		qdist = FastDistance(
			m_vEyePt.x - centroidx,
			m_vEyePt.y - centroidy,
			m_vEyePt.z - centroidz);

		ObjectsToDraw[number_of_polys_per_frame].vert_index = number_of_polys_per_frame;
		ObjectsToDraw[number_of_polys_per_frame].dist = qdist;

		dp_commands[number_of_polys_per_frame] = poly_command;
		dp_command_index_mode[number_of_polys_per_frame] = USE_NON_INDEXED_DP;
		

		if ((poly_command == D3DPT_TRIANGLESTRIP) || (poly_command == D3DPT_TRIANGLEFAN)) {
			ConvertTraingleStrip(fan_cnt);

			//dp_commands[number_of_polys_per_frame] = D3DPT_TRIANGLELIST;

			dp_commands[number_of_polys_per_frame] = D3DPT_TRIANGLELIST;

			if (num_vert > 3) {
				num_vert = (num_vert - 3) * 3;
				verts_per_poly[number_of_polys_per_frame] = (num_vert + 3);
			}



			num_triangles_in_scene += (num_vert - 2);
		}
		
		if (poly_command == D3DPT_TRIANGLELIST)
			num_triangles_in_scene += (num_vert / 3);
		number_of_polys_per_frame++;
		num_verts_in_scene += num_vert;
		num_dp_commands_in_scene++;

		//if ((poly_command < 0) || (poly_command > 6))
			//PrintMessage(NULL, "CMyD3DApplication::ObjectToD3DVertList -  ERROR UNRECOGNISED COMMAND", NULL, LOGFILE_ONLY);
	}

	//121=pillar 58=torch 169=house1 170=house2

	//if (ob_type == 121 || ob_type == 169 || ob_type == 170 || ob_type == 58
	//	|| strstr(oblist[oblist_index].name, "door") != NULL) {
	//SmoothNormals(start_cnt);
	//}

	//return;
}

/*
void AddWorldLight(int ob_type, int angle, int oblist_index, IDirect3DDevice9* pd3dDevice) {

	if (oblist[oblist_index].light_source->command != 0)
	{
		// Set up the light structure
		D3DLIGHT9 light;
		ZeroMemory(&light, sizeof(D3DLIGHT9));

		light.Diffuse.r = .5f;
		light.Diffuse.g = .5f;
		light.Diffuse.b = .5f;

		light.Ambient.r = 0.3f;
		light.Ambient.g = 0.4f;
		light.Ambient.b = 0.6f;

		light.Specular.r = 0.2f;
		light.Specular.g = 0.2f;
		light.Specular.b = 0.2f;

		light.Range = 600.0f;
		light.Ambient.r = oblist[oblist_index].light_source->rcolour;
		light.Ambient.g = oblist[oblist_index].light_source->gcolour;
		light.Ambient.b = oblist[oblist_index].light_source->bcolour;


		float pos_x = oblist[oblist_index].light_source->position_x;
		float pos_y = oblist[oblist_index].light_source->position_y;
		float pos_z = oblist[oblist_index].light_source->position_z;

		float dir_x = oblist[oblist_index].light_source->direction_x;
		float dir_y = oblist[oblist_index].light_source->direction_y;
		float dir_z = oblist[oblist_index].light_source->direction_z;

		switch (oblist[oblist_index].light_source->command)
		{

		case POINT_LIGHT_SOURCE:
			light.Position = D3DXVECTOR3(pos_x, pos_y, pos_z);
			light.Attenuation0 = 1.0f;
			light.Type = D3DLIGHT_POINT;

			break;

		case DIRECTIONAL_LIGHT_SOURCE:
			light.Direction = D3DXVECTOR3(dir_x, dir_y, dir_z);
			light.Type = D3DLIGHT_DIRECTIONAL;

			break;

		case SPOT_LIGHT_SOURCE:

			light.Type = D3DLIGHT_SPOT;
			light.Position = D3DXVECTOR3(pos_x, pos_y, pos_z);
			light.Direction = D3DXVECTOR3(dir_x, dir_y, dir_z);
			light.Falloff = 250.0f;
			light.Range = (float)550.0f;

			light.Theta = 2.0f; // spotlight's inner cone
			light.Phi = 2.5f;	  // spotlight's outer cone

			light.Theta = 1.4f; // spotlight's inner cone
			light.Phi = 2.1f;	  // spotlight's outer cone

			light.Attenuation0 = 2.0f;

			light.Ambient.r = 0.5f;
			light.Ambient.g = 0.5f;
			light.Ambient.b = 0.5f;

			light.Diffuse.r = oblist[oblist_index].light_source->rcolour;
			light.Diffuse.g = oblist[oblist_index].light_source->gcolour;
			light.Diffuse.b = oblist[oblist_index].light_source->bcolour;
			break;

		case 900:
			light.Position = D3DXVECTOR3(pos_x, pos_y, pos_z);
			light.Type = D3DLIGHT_POINT;

			light.Attenuation0 = 1.0f;
			light.Range = 150.f;
			light.Ambient.r = 1.0f;
			light.Ambient.g = 1.0f;
			light.Ambient.b = 1.0f;

			light.Diffuse.r = 1.0f;
			light.Diffuse.g = 1.0f;
			light.Diffuse.b = 1.0f;


			break;
		}

		//if (oblist[oblist_index].light_source->command == 900) {
		pd3dDevice->SetLight(num_light_sources, &light);
		pd3dDevice->LightEnable((DWORD)num_light_sources, TRUE);
		//}

		num_light_sources++;



	}

	return;
}


*/



void DrawBoundingBox() {
	
	
	ObjectsToDraw[number_of_polys_per_frame].srcstart = cnt;
	ObjectsToDraw[number_of_polys_per_frame].objectId = -1;
	ObjectsToDraw[number_of_polys_per_frame].srcfstart = 0;

	ObjectsToDraw[number_of_polys_per_frame].vert_index = number_of_polys_per_frame;
	ObjectsToDraw[number_of_polys_per_frame].dist = 0;
	ObjectsToDraw[number_of_polys_per_frame].texture = 276;
	ObjectsToDraw[number_of_polys_per_frame].vertsperpoly = 3;
	ObjectsToDraw[number_of_polys_per_frame].facesperpoly = 1;

	texture_list_buffer[number_of_polys_per_frame] = 238;  //263


	int test = (countboundingbox / 4) * 6;
	verts_per_poly[number_of_polys_per_frame] = test;
	dp_command_index_mode[number_of_polys_per_frame] = USE_NON_INDEXED_DP;
	dp_commands[number_of_polys_per_frame] = D3DPT_TRIANGLELIST;



	//int num_vert = (countboundingbox - 3) * 3;
	//verts_per_poly[number_of_polys_per_frame] = (num_vert + 3);


	number_of_polys_per_frame++;

	
	int fan_cnt = cnt;


	int uvcount = 0;

	for (int i = 0; i < countboundingbox; i++)
	{
		src_v[cnt].x = boundingbox[i].x;
		src_v[cnt].y = boundingbox[i].y;
		src_v[cnt].z = boundingbox[i].z;



		if (uvcount == 0) {
			src_v[cnt].tu = 0.0f;
			src_v[cnt].tv = 0.0f;
			uvcount++;
		}
		else if (uvcount == 1) {
			src_v[cnt].tu = 0.0f;
			src_v[cnt].tv = 1.0f;
			uvcount++;
		}
		else if (uvcount == 2) {
			src_v[cnt].tu = 1.0f;
			src_v[cnt].tv = 0.0f;
			uvcount++;
		}
		else if (uvcount == 3) {
			src_v[cnt].tu = 1.0f;
			src_v[cnt].tv = 1.0f;

			uvcount = 0;
		}
		else {
			uvcount++;
		}


		cnt++;


	}

	ConvertQuad(fan_cnt);

}

void PlayerToD3DVertList(int pmodel_id, int curr_frame, float angle, int texture_alias, int tex_flag, float xt, float yt, float zt, int nextFrame)
{


	float qdist = 0;

	int i, j;
	int num_verts_per_poly;
	int num_poly;
	int i_count;
	short v_index;
	float x, y, z;
	float rx, ry, rz;
	float tx, ty;
	int count_v;

	vert_ptr tp;
	vert_ptr tpNextFrame;
	DWORD r, g, b;
	D3DPRIMITIVETYPE p_command;
	BOOL error = TRUE;
	float mx[224];
	float my[224];
	float mz[224];

	XMFLOAT3 vw1, vw2, vw3;
	float workx, worky, workz;

	if (angle >= 360)
		angle = angle - 360;
	if (angle < 0)
		angle += 360;

	float x_off = 0;
	float y_off = 0;
	float z_off = 0;

	float  wx = xt;
	float  wy = yt;
	float  wz = zt;

	if (pmdata[pmodel_id].use_indexed_primitive == TRUE)
	{
		//3ds models
		PlayerToD3DIndexedVertList(pmodel_id, 0, angle, texture_alias, tex_flag, wx, wy, wz);
		return;
	}


	//md2 models
	//float cosine = cos_table[angle];
	//float sine = sin_table[angle];

	float cosine = (float)cos(angle * k);
	float sine = (float)sin(angle * k);

	if (curr_frame >= pmdata[pmodel_id].num_frames)
		curr_frame = 0;

	i_count = 0;

	num_poly = pmdata[pmodel_id].num_polys_per_frame;

	int start_cnt = cnt;

	int startFrame = pmdata[pmodel_id].sequence_start_frame[0];
	int endFrame = pmdata[pmodel_id].sequence_stop_frame[0];


	for (i = 0; i < num_poly; i++)
	{
		p_command = pmdata[pmodel_id].poly_cmd[i];
		num_verts_per_poly = pmdata[pmodel_id].num_vert[i];

		count_v = 0;

		ObjectsToDraw[number_of_polys_per_frame].srcstart = cnt;

		ObjectsToDraw[number_of_polys_per_frame].objectId = -1;

		int counttri = 0;
		int firstcount = 0;
		int fan_cnt = -1;

		//if (p_command == D3DPT_TRIANGLEFAN) {
		fan_cnt = cnt;
		//}

		for (j = 0; j < num_verts_per_poly; j++)
		{
			v_index = pmdata[pmodel_id].f[i_count];

			tp = &pmdata[pmodel_id].w[curr_frame][v_index];


			if (nextFrame != -1) {

				tpNextFrame = &pmdata[pmodel_id].w[nextFrame][v_index];


				if (gametimerAnimation > 0.0f && gametimerAnimation < 1.0f ) {

					x = tp->x + gametimerAnimation * (tpNextFrame->x - tp->x);
					z = tp->y + gametimerAnimation * (tpNextFrame->y - tp->y);
					y = tp->z + gametimerAnimation * (tpNextFrame->z - tp->z);
				}
				else {
					x = tp->x + x_off;
					z = tp->y + y_off;
					y = (tp->z + z_off);

				}

				if (weapondrop == 1)
				{
					y = y - 40.0f;
				}

			}
			else {

				if (weapondrop == 1)
				{
					x = tp->x + x_off;
					z = tp->y + y_off;
					y = (tp->z + z_off) - 40.0f;
				}
				else
				{
					x = tp->x + x_off;
					z = tp->y + y_off;
					y = (tp->z + z_off);
				}

			}



			if (weapondrop == 0)
			{
				rx = wx + (x * cosine - z * sine);
				ry = wy + y;
				rz = wz + (x * sine + z * cosine);
			}
			else
			{
				rx = wx + x * sinf(fDot2 * k) * sinf(angle * k);
				ry = wy + y * cosf(fDot2 * k);
				rz = wz + (z)*sinf(fDot2 * k) * cosf(angle * k);
			}

			if (weapondrop == 1)
			{
				//pitch

				float newx, newy, newz;

				newx = x;
				newy = y;
				newz = z;

				rx = (newy * sinf(fDot2 * k) + newx * cosf(fDot2 * k));
				ry = (newy * cosf(fDot2 * k) - newx * sinf(fDot2 * k));
				rz = newz;

				newx = rx;
				newy = ry;
				newz = rz;

				rx = (newx * cosine - newz * sine);
				ry = newy;
				rz = (newx * sine + newz * cosine);

				newx = rx;
				newy = ry;
				newz = rz;
				rx = newx;
				ry = (newy * cosf(0.0f * k) - newz * sinf(0.0f * k));
				rz = (newy * sinf(0.0f * k) + newz * cosf(0.0f * k));


			}
			else
			{
				rx = (x * cosine - z * sine);
				ry = y;
				rz = (x * sine + z * cosine);
			}

			rx = rx + wx;
			ry = ry + wy;

			rz = rz + wz;

			tx = pmdata[pmodel_id].t[i_count].x * pmdata[pmodel_id].skx;
			ty = pmdata[pmodel_id].t[i_count].y * pmdata[pmodel_id].sky;

			r = 0;
			g = 0;
			b = 0;

			src_v[cnt].x = D3DVAL(rx);
			src_v[cnt].y = D3DVAL(ry);
			src_v[cnt].z = D3DVAL(rz);
			src_v[cnt].tu = D3DVAL(tx);
			src_v[cnt].tv = D3DVAL(ty);

			src_collide[cnt] = 1;

			mx[j] = D3DVAL(rx);
			my[j] = D3DVAL(ry);
			mz[j] = D3DVAL(rz);

			if (counttri == 2 && firstcount == 0 || firstcount == 1)
			{
				vw1.x = D3DVAL(mx[j - 2]);
				vw1.y = D3DVAL(my[j - 2]);
				vw1.z = D3DVAL(mz[j - 2]);

				vw2.x = D3DVAL(mx[j - 1]);
				vw2.y = D3DVAL(my[j - 1]);
				vw2.z = D3DVAL(mz[j - 1]);

				vw3.x = D3DVAL(mx[j]);
				vw3.y = D3DVAL(my[j]);
				vw3.z = D3DVAL(mz[j]);

				// calculate the NORMAL for the road using the CrossProduct <-important!

				//D3DXVECTOR3 vDiff = vw1 - vw2;
				//vDiff2 = vw3 - vw2;

				XMVECTOR   vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
				XMVECTOR   vDiff2 = XMLoadFloat3(&vw3) - XMLoadFloat3(&vw2);

				//D3DXVECTOR3 vCross, final;

				//D3DXVec3Cross(&vCross, &vDiff, &vDiff2);
				//D3DXVec3Normalize(&final, &vCross);

				XMVECTOR  vCross, final;
				vCross = XMVector3Cross(vDiff, vDiff2);
				final = XMVector3Normalize(vCross);

				XMFLOAT3 final2;
				XMStoreFloat3(&final2, final);

				//workx = -final2.x;
				//worky = -final2.y;
				//workz = -final2.z;

				//TODO: Why is this not negative now?
				workx = final2.x;
				worky = final2.y;
				workz = final2.z;


				if (firstcount == 1)
				{

					src_v[cnt - 2].nx = workx;
					src_v[cnt - 2].ny = worky;
					src_v[cnt - 2].nz = workz;


					src_v[cnt - 1].nx = workx;
					src_v[cnt - 1].ny = worky;
					src_v[cnt - 1].nz = workz;


					src_v[cnt].nx = workx;
					src_v[cnt].ny = worky;
					src_v[cnt].nz = workz;


					CalculateTangentBinormal(src_v[cnt - 2], src_v[cnt - 1], src_v[cnt]);

				}
				else {


					src_v[cnt - 2].nx = workx;
					src_v[cnt - 2].ny = worky;
					src_v[cnt - 2].nz = workz;

					src_v[cnt - 1].nx = workx;
					src_v[cnt - 1].ny = worky;
					src_v[cnt - 1].nz = workz;

					src_v[cnt].nx = workx;
					src_v[cnt].ny = worky;
					src_v[cnt].nz = workz;

					CalculateTangentBinormal(src_v[cnt - 2], src_v[cnt - 1], src_v[cnt]);
				}
				firstcount = 1;
				counttri = 0;
			}
			else {
				counttri++;
			}


			//if (j >= 2)
			//{
			//	src_v[cnt].nx = workx;
			//	src_v[cnt].ny = worky;
			//	src_v[cnt].nz = workz;
			//}


			cnt++;
			i_count++;
		}

		float centroidx = (mx[0] + mx[1] + mx[2]) * QVALUE;
		float centroidy = (my[0] + my[1] + my[2]) * QVALUE;
		float centroidz = (mz[0] + mz[1] + mz[2]) * QVALUE;

		//qdist = FastDistance(
		//	m_vEyePt.x - centroidx,
		//	m_vEyePt.y - centroidy,
		//	m_vEyePt.z - centroidz);

		ObjectsToDraw[number_of_polys_per_frame].vert_index = number_of_polys_per_frame;
		ObjectsToDraw[number_of_polys_per_frame].dist = qdist;
		ObjectsToDraw[number_of_polys_per_frame].texture = texture_alias;
		ObjectsToDraw[number_of_polys_per_frame].vertsperpoly = num_verts_per_poly;

		verts_per_poly[number_of_polys_per_frame] = num_verts_per_poly;
		dp_command_index_mode[number_of_polys_per_frame] = USE_NON_INDEXED_DP;
		dp_commands[number_of_polys_per_frame] = p_command;

		if (p_command == D3DPT_TRIANGLESTRIP) {
			num_triangles_in_scene += (num_verts_per_poly - 2);

			ConvertTraingleStrip(fan_cnt);
			dp_commands[number_of_polys_per_frame] = D3DPT_TRIANGLELIST;

			if (num_verts_per_poly > 3) {
				num_verts_per_poly = (num_verts_per_poly - 3) * 3;

				verts_per_poly[number_of_polys_per_frame] = (num_verts_per_poly + 3);
			}


		}


		if (p_command == D3DPT_TRIANGLELIST)
			num_triangles_in_scene += (num_verts_per_poly / 3);

		if (p_command == D3DPT_TRIANGLEFAN) {

			ConvertTraingleFan(fan_cnt);
			dp_commands[number_of_polys_per_frame] = D3DPT_TRIANGLELIST;
			//todo: fix this
			num_triangles_in_scene += (num_verts_per_poly - 2);

			if (num_verts_per_poly > 3) {
				num_verts_per_poly = (num_verts_per_poly - 3) * 3;

				verts_per_poly[number_of_polys_per_frame] = (num_verts_per_poly + 3);
			}

		}



		num_verts_in_scene += num_verts_per_poly;
		num_dp_commands_in_scene++;

		if (tex_flag == USE_PLAYERS_SKIN)
			texture_list_buffer[number_of_polys_per_frame] = texture_alias;
		else
			texture_list_buffer[number_of_polys_per_frame] = pmdata[pmodel_id].texture_list[i];

		texture_list_buffer[number_of_polys_per_frame] = texture_alias;

		number_of_polys_per_frame++;

	} // end for vert_cnt

	SmoothNormals(start_cnt);

	return;
}

int tracknormal[MAX_NUM_QUADS];

void SmoothNormals(int start_cnt) {
	// Smooth the vertex normals out so the models look less blocky.

	// Use a hash map to group vertices by position for O(n) performance.Add commentMore actions
	// This avoids the O(n^2) nested loop.
	struct Vec3Key {
		float x, y, z;
		bool operator==(const Vec3Key& other) const {
			constexpr float eps = 1e-5f;
			return fabs(x - other.x) < eps && fabs(y - other.y) < eps && fabs(z - other.z) < eps;
		}
	};
	struct Vec3KeyHash {
		std::size_t operator()(const Vec3Key& k) const {
			// Simple hash combining
			// Use bit_cast for better hashing if available, else stick to int cast
			std::size_t hx = std::hash<int>()(static_cast<int>(k.x * 10000));
			std::size_t hy = std::hash<int>()(static_cast<int>(k.y * 10000));
			std::size_t hz = std::hash<int>()(static_cast<int>(k.z * 10000));
			return hx ^ (hy << 1) ^ (hz << 2);
		}
	};

	// Map from vertex position to indices
	// Preallocate to avoid rehashing
	std::unordered_map<Vec3Key, std::vector<int>, Vec3KeyHash> vertMap;
	vertMap.reserve(static_cast<size_t>(cnt - start_cnt));

	// Group indices by position
	for (int i = start_cnt; i < cnt; ++i) {
		Vec3Key key{ src_v[i].x, src_v[i].y, src_v[i].z };
		vertMap[key].push_back(i);
	}

	// Use local variables to avoid repeated lookups
	for (auto& pair : vertMap) {
		const std::vector<int>& indices = pair.second;
		if (indices.size() > 1) {
			XMVECTOR sum = XMVectorZero();
			XMVECTOR sumtan = XMVectorZero();
			for (int idx : indices) {
				// Use pointer arithmetic to avoid repeated array lookups
				const D3DVERTEX2* v = &src_v[idx];
				XMFLOAT3 n{ v->nx, v->ny, v->nz };
				XMFLOAT3 t{ v->nmx, v->nmy, v->nmz };
				sum = XMVectorAdd(sum, XMLoadFloat3(&n));
				sumtan = XMVectorAdd(sumtan, XMLoadFloat3(&t));
			}
			XMFLOAT3 final2, finaltan;
			XMStoreFloat3(&final2, XMVector3Normalize(sum));
			XMStoreFloat3(&finaltan, XMVector3Normalize(sumtan));
			for (int idx : indices) {
				D3DVERTEX2* v = &src_v[idx];
				v->nx = final2.x;
				v->ny = final2.y;
				v->nz = final2.z;
				v->nmx = finaltan.x;
				v->nmy = finaltan.y;
				v->nmz = finaltan.z;
			}
		}
	}
}


void SmoothNormalsNoHash(int start_cnt) {

	//Smooth the vertex normals out so the models look less blocky.

	XMVECTOR sum, sumtan, average;
	XMFLOAT3 x1, xtan, final2, finaltan;

	int scount = 0;

	for (int i = start_cnt; i < cnt; i++) {
		tracknormal[i] = 0;
	}

	for (int i = start_cnt; i < cnt; i++) {

		if (tracknormal[i] == 0) {
			float x = src_v[i].x;
			float y = src_v[i].y;
			float z = src_v[i].z;

			scount = 0;

			//GitHub copilot fixed this! AI is the future.
			//old code: for (int j = start_cnt; j < cnt; j++)

			for (int j = i; j < cnt; j++) {
				if (tracknormal[j] == 0 && x == src_v[j].x && y == src_v[j].y && z == src_v[j].z) {
					//found shared vertex
					sharedv[scount] = j;
					scount++;
				}
			}

			if (scount > 1) {
				sum = XMVectorSet(0, 0, 0, 0);
				sumtan = XMVectorSet(0, 0, 0, 0);

				for (int k = 0; k < scount; k++) {
					x1.x = src_v[sharedv[k]].nx;
					x1.y = src_v[sharedv[k]].ny;
					x1.z = src_v[sharedv[k]].nz;
					sum = sum + XMLoadFloat3(&x1);

					xtan.x = src_v[sharedv[k]].nmx;
					xtan.y = src_v[sharedv[k]].nmy;
					xtan.z = src_v[sharedv[k]].nmz;
					sumtan = sumtan + XMLoadFloat3(&xtan);

				}

				average = XMVector3Normalize(sum);
				XMStoreFloat3(&final2, average);

				average = XMVector3Normalize(sumtan);
				XMStoreFloat3(&finaltan, average);

				for (int k = 0; k < scount; k++) {
					src_v[sharedv[k]].nx = final2.x;
					src_v[sharedv[k]].ny = final2.y;
					src_v[sharedv[k]].nz = final2.z;

					src_v[sharedv[k]].nmx = finaltan.x;
					src_v[sharedv[k]].nmy = finaltan.y;
					src_v[sharedv[k]].nmz = finaltan.z;

					tracknormal[sharedv[k]] = 1;
				}
			}
		}
	}
}



void ComputerWeightedAverages(int start_cnt);

void SmoothNormalsWeighted(int start_cnt) {

	for (int i = 0; i < MAX_NUM_QUADS; i++) {
		tracknormal[i] = 0;
	}

	ComputerWeightedAverages(start_cnt);

	//Smooth the vertex normals out so the models look less blocky.
	int scount = 0;

	for (int i = start_cnt; i < cnt; i++) {
		if (tracknormal[i] == 0) {
			float x = src_v[i].x;
			float y = src_v[i].y;
			float z = src_v[i].z;

			scount = 0;

			for (int j = start_cnt; j < cnt; j++) {
				//if (i != j) {
				if (tracknormal[j] == 0 && x == src_v[j].x && y == src_v[j].y && z == src_v[j].z) {

					//if (src_v[j].weight < 45.0f) {
						//found shared vertex
					sharedv[scount] = j;
					scount++;
					//}
				}
				//}
			}

			if (scount > 0) {
				XMVECTOR sum = XMVectorSet(0, 0, 0, 0);
				XMVECTOR sumtan = XMVectorSet(0, 0, 0, 0);

				XMFLOAT3 x1, xtan;
				XMVECTOR average;

				XMVECTOR work = XMVectorSet(0, 0, 0, 0);

				XMFLOAT3 finalweight;

				float weight = 0;
				float area = 0;

				for (int k = 0; k < scount; k++) {

					weight = src_v[sharedv[k]].weight;
					area = src_v[sharedv[k]].area;

					if (weight > 90.0f) {
						int a = 1;
					}

					//weight = (float)acos(weight) / (float)0.017453292;;

					work = XMVectorSet(src_v[sharedv[k]].nx, src_v[sharedv[k]].ny, src_v[sharedv[k]].nz, 0);
					work = work * (weight * area);
					//work = XMVector3Normalize(work);
					XMStoreFloat3(&finalweight, work);

					x1.x = finalweight.x;
					x1.y = finalweight.y;
					x1.z = finalweight.z;

					//x1.x = src_v[sharedv[k]].nx;
					//x1.y = src_v[sharedv[k]].ny;
					//x1.z = src_v[sharedv[k]].nz;
					sum = sum + XMLoadFloat3(&x1);


					work = XMVectorSet(src_v[sharedv[k]].nmx, src_v[sharedv[k]].nmy, src_v[sharedv[k]].nmz, 0);
					work = work * (weight * area);
					//work = XMVector3Normalize(work);
					XMStoreFloat3(&finalweight, work);

					xtan.x = finalweight.x;
					xtan.y = finalweight.y;
					xtan.z = finalweight.z;
					sumtan = sumtan + XMLoadFloat3(&xtan);
				}

				//sum = sum / (float)scount;

				XMFLOAT3 final2, finaltan;

				average = XMVector3Normalize(sum);
				XMStoreFloat3(&final2, average);

				average = XMVector3Normalize(sumtan);
				XMStoreFloat3(&finaltan, average);


				for (int k = 0; k < scount; k++) {
					src_v[sharedv[k]].nx = final2.x;
					src_v[sharedv[k]].ny = final2.y;
					src_v[sharedv[k]].nz = final2.z;

					src_v[sharedv[k]].nmx = finaltan.x;
					src_v[sharedv[k]].nmy = finaltan.y;
					src_v[sharedv[k]].nmz = finaltan.z;

					tracknormal[sharedv[k]] = 1;
				}
			}
		}
	}
}


void ComputerWeightedAverages(int start_cnt) {

	int count = 0;

	XMFLOAT3 vw1, vw2, vw3;

	XMVECTOR P1, P2;
	XMVECTOR vDiff;
	XMVECTOR vDiff2;

	XMVECTOR final;
	XMVECTOR final2;
	XMVECTOR fDotVector;
	float fDot;

	for (int i = start_cnt; i < cnt; i = i + 3) {

		vw1.x = src_v[i].x;
		vw1.y = src_v[i].y;
		vw1.z = src_v[i].z;
		//vw1.x = 0.0f;
		//vw1.y = 0;
		//vw1.z = 0;

		vw2.x = src_v[i + 1].x;
		vw2.y = src_v[i + 1].y;
		vw2.z = src_v[i + 1].z;
		//vw2.x = 0;
		//vw2.y = 0;
		//vw2.z = 50.0f;

		vw3.x = src_v[i + 2].x;
		vw3.y = src_v[i + 2].y;
		vw3.z = src_v[i + 2].z;
		//vw3.x = 50.0f;
		//vw3.y = 0;
		//vw3.z = 0;

		// v1, v2, v3 are the vertices of face A
		//if face B shares v1 {
		//	angle = angle_between_vectors(v1 - v2, v1 - v3)
		//		n += (face B facet normal) * (face B surface area) * angle // multiply by angle
		//}
		//if face B shares v2 {
		//	angle = angle_between_vectors(v2 - v1, v2 - v3)
		//		n += (face B facet normal) * (face B surface area) * angle // multiply by angle
		//}
		//if face B shares v3 {
		//	angle = angle_between_vectors(v3 - v1, v3 - v2)
		//		n += (face B facet normal) * (face B surface area) * angle // multiply by angle
		//}



		//VW1
		P1 = XMLoadFloat3(&vw1);
		P2 = XMLoadFloat3(&vw2);
		vDiff = P1 - P2;

		final = XMVector3Normalize(vDiff);

		P1 = XMLoadFloat3(&vw1);
		P2 = XMLoadFloat3(&vw3);
		vDiff2 = P1 - P2;
		final2 = XMVector3Normalize(vDiff2);


		fDotVector = XMVector3Dot(final, final2);
		fDot = XMVectorGetX(fDotVector);
		fDot = MathHelper::Clamp(fDot, -0.99999f, 0.99999f);


		fDot = (float)acos(fDot) / k;


		XMVECTOR vCross = XMVector3Cross(vDiff, vDiff2);
		XMFLOAT3 finalCross;
		XMStoreFloat3(&finalCross, vCross);

		//Set the area of the triangle
		src_v[i].area = .5f * sqrtf(fabsf(powf(finalCross.x,2.0f) ) + fabsf(powf(finalCross.y,2.0f)) + fabsf(powf(finalCross.z , 2.0f)));
		src_v[i + 1].area = src_v[i].area;
		src_v[i + 2].area = src_v[i].area;

		src_v[i].weight = fabsf(fDot);

		//VW2
		P1 = XMLoadFloat3(&vw2);
		P2 = XMLoadFloat3(&vw1);
		vDiff = P1 - P2;

		final = XMVector3Normalize(vDiff);

		P1 = XMLoadFloat3(&vw2);
		P2 = XMLoadFloat3(&vw3);
		vDiff2 = P1 - P2;
		final2 = XMVector3Normalize(vDiff2);

		fDotVector = XMVector3Dot(final, final2);
		fDot = XMVectorGetX(fDotVector);
		fDot = MathHelper::Clamp(fDot, -0.99999f, 0.99999f);
		fDot = (float)acos(fDot) / k;

		vCross = XMVector3Cross(vDiff, vDiff2);
		finalCross;
		XMStoreFloat3(&finalCross, vCross);
		//src_v[i + 1].area = .05f * sqrtf(fabsf(finalCross.x) + fabsf(finalCross.y) + fabsf(finalCross.z));

		src_v[i + 1].weight = fabsf(fDot);

		//VW3
		P1 = XMLoadFloat3(&vw3);
		P2 = XMLoadFloat3(&vw1);
		vDiff = P1 - P2;

		final = XMVector3Normalize(vDiff);

		P1 = XMLoadFloat3(&vw3);
		P2 = XMLoadFloat3(&vw2);
		vDiff2 = P1 - P2;
		final2 = XMVector3Normalize(vDiff2);


		fDotVector = XMVector3Dot(final, final2);
		fDot = XMVectorGetX(fDotVector);
		fDot = MathHelper::Clamp(fDot, -0.99999f, 0.99999f);
		fDot = (float)acos(fDot) / k;

		vCross = XMVector3Cross(vDiff, vDiff2);
		finalCross;
		XMStoreFloat3(&finalCross, vCross);
		//src_v[i +2].area = .05f * sqrtf(fabsf(finalCross.x) + fabsf(finalCross.y) + fabsf(finalCross.z));
		src_v[i + 2].weight = fabsf(fDot);
	}
}



void ConvertTraingleFan(int fan_cnt) {
    int counter = 0;

    for (int i = fan_cnt; i < cnt; i++) {
        if (counter < 3) {
            temp_v[counter] = src_v[i];
            counter++;
        } else {
            temp_v[counter] = src_v[fan_cnt];
            counter++;
            temp_v[counter] = src_v[i - 1];
            counter++;
            temp_v[counter] = src_v[i];
            counter++;
        }
    }

    int normal = 0;

    for (int i = 0; i < counter; i++) {
        src_v[fan_cnt + i] = temp_v[i];

        if (normal == 2) {
            normal = 0;
            XMFLOAT3 vw1 = {src_v[(fan_cnt + i) - 2].x, src_v[(fan_cnt + i) - 2].y, src_v[(fan_cnt + i) - 2].z};
            XMFLOAT3 vw2 = {src_v[(fan_cnt + i) - 1].x, src_v[(fan_cnt + i) - 1].y, src_v[(fan_cnt + i) - 1].z};
            XMFLOAT3 vw3 = {src_v[(fan_cnt + i)].x, src_v[(fan_cnt + i)].y, src_v[(fan_cnt + i)].z};

            XMVECTOR vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
            XMVECTOR vDiff2 = XMLoadFloat3(&vw3) - XMLoadFloat3(&vw2);
            XMVECTOR vCross = XMVector3Cross(vDiff, vDiff2);
            XMVECTOR final = XMVector3Normalize(vCross);

            XMFLOAT3 final2;
            XMStoreFloat3(&final2, final);

            float workx = -final2.x;
            float worky = -final2.y;
            float workz = -final2.z;

            src_v[(fan_cnt + i) - 2].nx = workx;
            src_v[(fan_cnt + i) - 2].ny = worky;
            src_v[(fan_cnt + i) - 2].nz = workz;

            src_v[(fan_cnt + i) - 1].nx = workx;
            src_v[(fan_cnt + i) - 1].ny = worky;
            src_v[(fan_cnt + i) - 1].nz = workz;

            src_v[(fan_cnt + i)].nx = workx;
            src_v[(fan_cnt + i)].ny = worky;
            src_v[(fan_cnt + i)].nz = workz;

            CalculateTangentBinormal(src_v[(fan_cnt + i) - 2], src_v[(fan_cnt + i) - 1], src_v[(fan_cnt + i)]);
        } else {
            normal++;
        }
    }
    cnt = fan_cnt + counter;
}

void ConvertTraingleStrip(int fan_cnt) {
	int counter = 0;
	int v = 0;

	// Combine loops to reduce redundant operations
	for (int i = fan_cnt; i < cnt; i++) {
		if (counter < 3) {
			temp_v[counter++] = src_v[i];
		}
		else {
			if (v == 0) {
				temp_v[counter++] = src_v[i];
				temp_v[counter++] = src_v[i - 1];
				temp_v[counter++] = src_v[i - 2];
				v = 1;
			}
			else {
				temp_v[counter++] = src_v[i - 2];
				temp_v[counter++] = src_v[i - 1];
				temp_v[counter++] = src_v[i];
				v = 0;
			}
		}
	}

	// Directly process vertices without intermediate copying
	for (int i = 0; i < counter; i += 3) {
		XMFLOAT3 vw1 = { temp_v[i].x, temp_v[i].y, temp_v[i].z };
		XMFLOAT3 vw2 = { temp_v[i + 1].x, temp_v[i + 1].y, temp_v[i + 1].z };
		XMFLOAT3 vw3 = { temp_v[i + 2].x, temp_v[i + 2].y, temp_v[i + 2].z };

		XMVECTOR vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
		XMVECTOR vDiff2 = XMLoadFloat3(&vw3) - XMLoadFloat3(&vw2);
		XMVECTOR vCross = XMVector3Cross(vDiff, vDiff2);
		XMVECTOR final = XMVector3Normalize(vCross);

		XMFLOAT3 final2;
		XMStoreFloat3(&final2, final);

		float workx = -final2.x;
		float worky = -final2.y;
		float workz = -final2.z;

		for (int j = 0; j < 3; j++) {
			temp_v[i + j].nx = workx;
			temp_v[i + j].ny = worky;
			temp_v[i + j].nz = workz;
		}

		CalculateTangentBinormal(temp_v[i], temp_v[i + 1], temp_v[i + 2]);
	}

	// Update src_v in bulk
	std::copy(temp_v, temp_v + counter, src_v + fan_cnt);
	cnt = fan_cnt + counter;
}


void ConvertQuad(int fan_cnt) {
    int counter = 0;
    int quad = 0;

    for (int i = fan_cnt; i < cnt; i++) {
        if (quad >= 3) {
            for (int j = 3; j >= 0; j--) {
                temp_v[counter] = src_v[i - j];
                counter++;
            }
            for (int j = 1; j >= 0; j--) {
                temp_v[counter] = src_v[i - j];
                counter++;
            }
            quad = 0;
        } else {
            quad++;
        }
    }

    for (int i = 0; i < counter; i++) {
        src_v[fan_cnt + i] = temp_v[i];
        src_collide[fan_cnt + i] = 0;
    }

    for (int i = 0; i < counter; i += 3) {
        XMFLOAT3 vw1 = {src_v[fan_cnt + i].x, src_v[fan_cnt + i].y, src_v[fan_cnt + i].z};
        XMFLOAT3 vw2 = {src_v[fan_cnt + i + 1].x, src_v[fan_cnt + i + 1].y, src_v[fan_cnt + i + 1].z};
        XMFLOAT3 vw3 = {src_v[fan_cnt + i + 2].x, src_v[fan_cnt + i + 2].y, src_v[fan_cnt + i + 2].z};

        XMVECTOR vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
        XMVECTOR vDiff2 = XMLoadFloat3(&vw3) - XMLoadFloat3(&vw2);
        XMVECTOR vCross = XMVector3Cross(vDiff, vDiff2);
        XMVECTOR final = XMVector3Normalize(vCross);

        XMFLOAT3 final2;
        XMStoreFloat3(&final2, final);

        float workx = -final2.x;
        float worky = -final2.y;
        float workz = -final2.z;

        for (int j = 0; j < 3; j++) {
            src_v[fan_cnt + i + j].nx = workx;
            src_v[fan_cnt + i + j].ny = worky;
            src_v[fan_cnt + i + j].nz = workz;
        }

        CalculateTangentBinormal(src_v[fan_cnt + i], src_v[fan_cnt + i + 1], src_v[fan_cnt + i + 2]);
    }

    cnt = fan_cnt + counter;
}
int GetNextFrame(int monsterId);

void DrawMonsters()
{
	int cullflag = 0;
	int merchantfound = 0;
	BOOL use_player_skins_flag = false;
	int getgunid = 0;
	int monsteron = 0;
	for (int i = 0; i < num_monsters; i++)
	{

		if (monster_list[i].bIsPlayerValid == TRUE && monster_list[i].bStopAnimating == FALSE ||
			monster_list[i].bIsPlayerValid == FALSE && monster_list[i].bStopAnimating == FALSE ||
			monster_list[i].bIsPlayerAlive == FALSE && monster_list[i].bStopAnimating == TRUE

			)
		{
			cullflag = 0;
			for (int cullloop = 0; cullloop < monstercount; cullloop++)
			{

				if (monstercull[cullloop] == monster_list[i].monsterid)
				{
					cullflag = 1;
					break;
				}
			}

			if (cullflag == 1)
			{
				float wx = monster_list[i].x;
				float wy = monster_list[i].y;
				float wz = monster_list[i].z;

				float qdist = FastDistance(player_list[trueplayernum].x - wx, player_list[trueplayernum].y - wy, player_list[trueplayernum].z - wz);

				//if (strcmp(monster_list[i].rname, "CENTAUR") == 0 && qdist <= 100.0f && merchantfound == 0)
				//{
				//	//just found
				//	merchantfound = 1;
				//	DisplayDialogText("Press SPACE to buy and sell items", 0.0f);
				//}

				XMFLOAT3 work1;
				work1.x = wx;
				work1.y = wy;
				work1.z = wz;


				monsteron = CalculateView(m_vEyePt, work1, cullAngle, true);
				if (monsteron)
				{

					use_player_skins_flag = 0;

					

					int nextFrame = GetNextFrame(i);

					PlayerToD3DVertList(monster_list[i].model_id,
						monster_list[i].current_frame, monster_list[i].rot_angle,
						monster_list[i].skin_tex_id,
						USE_PLAYERS_SKIN, monster_list[i].x, monster_list[i].y, monster_list[i].z, nextFrame);

					for (int q = 0; q < countmodellist; q++)
					{

						if (strcmp(model_list[q].name, monster_list[i].rname) == 0)
						{
							getgunid = q;
							break;
						}
					}

					if (strcmp(model_list[getgunid].monsterweapon, "NONE") != 0 && monster_list[i].bIsPlayerAlive == TRUE)
					{

						PlayerToD3DVertList(FindModelID(model_list[getgunid].monsterweapon),
							monster_list[i].current_frame, monster_list[i].rot_angle,
							FindGunTexture(model_list[getgunid].monsterweapon),
							USE_PLAYERS_SKIN, monster_list[i].x, monster_list[i].y, monster_list[i].z, nextFrame);
					}
				}
				else
				{
				}
			}
		}
	}
}

int GetNextFrame(int monsterId) {


	int mod_id = monster_list[monsterId].model_id;
	int curr_frame = monster_list[monsterId].current_frame;
	int sequence = monster_list[monsterId].current_sequence;
	int stop_frame = pmdata[mod_id].sequence_stop_frame[monster_list[monsterId].current_sequence];
	int startframe = pmdata[mod_id].sequence_start_frame[monster_list[monsterId].current_sequence];
	int nextFrame = 0;


	if (monster_list[monsterId].bStopAnimating)
		return -1;

	if (curr_frame >= stop_frame)
	{

		nextFrame = pmdata[mod_id].sequence_start_frame[sequence];

	}
	else
	{
		nextFrame = curr_frame + 1;

	}

	return nextFrame;
}


int GetNextFramePlayer() {
	
		int mod_id = player_list[trueplayernum].model_id;
		int curr_frame = player_list[trueplayernum].current_frame;
		int stop_frame = pmdata[mod_id].sequence_stop_frame[player_list[trueplayernum].current_sequence];
		int startframe = pmdata[mod_id].sequence_start_frame[player_list[trueplayernum].current_sequence];
		int nextFrame = 0;

		if (curr_frame >= stop_frame)
		{
			int curr_seq = player_list[trueplayernum].current_sequence;
			nextFrame = pmdata[mod_id].sequence_start_frame[curr_seq];
			player_list[trueplayernum].animationdir = 1;
		}
		else
		{
			nextFrame = curr_frame + 1;

		}
		return nextFrame;
	
}

int FindModelID(char* p)
{
	int i = 0;

	for (i = 0; i < countmodellist; i++)
	{
		if (strcmp(model_list[i].name, p) == 0)
		{
			return model_list[i].model_id;
		}
	}

	for (i = 0; i < num_your_guns; i++)
	{
		if (strcmp(your_gun[i].gunname, p) == 0)
		{
			return your_gun[i].model_id;
		}
	}

	return 0;
}

void AddItem(float x, float y, float z, float rot_angle, float monsterid, float monstertexture, float monnum, char modelid[80], char modeltexture[80], int ability)
{
	//if (monsterenable == 0)
		//return;

	item_list[itemlistcount].bIsPlayerValid = TRUE;
	item_list[itemlistcount].bIsPlayerAlive = TRUE;
	item_list[itemlistcount].x = x;
	item_list[itemlistcount].y = y;
	item_list[itemlistcount].z = z;
	item_list[itemlistcount].rot_angle = rot_angle;
	item_list[itemlistcount].model_id = (int)monsterid;
	item_list[itemlistcount].skin_tex_id = (int)monstertexture;

	item_list[itemlistcount].current_sequence = 0;
	item_list[itemlistcount].current_frame = 85;

	item_list[itemlistcount].draw_external_wep = FALSE;

	item_list[itemlistcount].monsterid = (int)monnum;

	item_list[itemlistcount].ability = (int)ability;
	item_list[itemlistcount].gold = (int)ability;
	strcpy_s(item_list[itemlistcount].rname, modelid);
	strcpy_s(item_list[itemlistcount].texturename, modeltexture);

	itemlistcount++;
}

void DrawItems(float fElapsedTime)
{
	BOOL use_player_skins_flag = false;
	int cullflag = 0;
	int monsteron = 0;
	float rotateSpeed = 100.0f * fElapsedTime;

	for (int i = 0; i < itemlistcount; i++)
	{
		if (item_list[i].bIsPlayerValid == TRUE)
		{

			float qdist = FastDistance(
				m_vEyePt.x - item_list[i].x,
				m_vEyePt.y - item_list[i].y,
				m_vEyePt.z - item_list[i].z);


			if (qdist < 1100.0f) {

				cullflag = 0;
				for (int cullloop = 0; cullloop < monstercount; cullloop++)
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

					XMFLOAT3 work1;
					work1.x = wx;
					work1.y = wy;
					work1.z = wz;

					monsteron = CalculateView(m_vEyePt, work1, cullAngle, true);

					if (monsteron)
					{

						use_player_skins_flag = 1;
						int mtexlookup;

						if (strcmp(item_list[i].rname, "COIN") == 0)
						{


							item_list[i].rot_angle = fixangle(item_list[i].rot_angle, rotateSpeed);


							PlayerToD3DVertList(item_list[i].model_id,
								item_list[i].current_frame, item_list[i].rot_angle,
								1,
								USE_DEFAULT_MODEL_TEX, item_list[i].x, item_list[i].y, item_list[i].z);
						}
						else if (strcmp(item_list[i].rname, "diamond") == 0)
						{
							item_list[i].rot_angle = fixangle(item_list[i].rot_angle, rotateSpeed);
							PlayerToD3DVertList(item_list[i].model_id,
								item_list[i].current_frame, item_list[i].rot_angle,
								1,
								USE_DEFAULT_MODEL_TEX, item_list[i].x, item_list[i].y, item_list[i].z);
						}
						else if (strcmp(item_list[i].rname, "SCROLL-MAGICMISSLE-") == 0 ||
							strcmp(item_list[i].rname, "SCROLL-FIREBALL-") == 0 ||
							strcmp(item_list[i].rname, "SCROLL-LIGHTNING-") == 0 ||
							strcmp(item_list[i].rname, "SCROLL-HEALING-") == 0)
						{
							if (item_list[i].monsterid == 9999)
								item_list[i].rot_angle = fixangle(item_list[i].rot_angle, rotateSpeed);

							PlayerToD3DVertList(item_list[i].model_id,
								item_list[i].current_frame, item_list[i].rot_angle,
								1,
								USE_DEFAULT_MODEL_TEX, item_list[i].x, item_list[i].y, item_list[i].z);
						}
						else if (strcmp(item_list[i].rname, "KEY2") == 0)
						{
							item_list[i].rot_angle = fixangle(item_list[i].rot_angle, rotateSpeed);
							PlayerToD3DVertList(item_list[i].model_id,
								item_list[i].current_frame, item_list[i].rot_angle,
								1,
								USE_DEFAULT_MODEL_TEX, item_list[i].x, item_list[i].y, item_list[i].z);
						}
						else if (strcmp(item_list[i].rname, "spellbook") == 0)
						{
							item_list[i].rot_angle = fixangle(item_list[i].rot_angle, rotateSpeed);
							PlayerToD3DVertList(item_list[i].model_id,
								item_list[i].current_frame, item_list[i].rot_angle,
								1,
								USE_DEFAULT_MODEL_TEX, item_list[i].x, item_list[i].y, item_list[i].z);
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
							strcmp(item_list[i].rname, "LIGHTNINGSWORD") == 0

							)
						{

							mtexlookup = FindGunTexture(item_list[i].rname);

							PlayerToD3DVertList(item_list[i].model_id,
								item_list[i].current_frame, item_list[i].rot_angle,
								mtexlookup,
								USE_DEFAULT_MODEL_TEX, item_list[i].x, item_list[i].y, item_list[i].z);
						}
						else if (strcmp(item_list[i].rname, "POTION") == 0)
						{
							//if (maingameloop)
							item_list[i].rot_angle = fixangle(item_list[i].rot_angle, rotateSpeed);

							PlayerToD3DVertList(item_list[i].model_id,
								item_list[i].current_frame, item_list[i].rot_angle,
								1,
								USE_DEFAULT_MODEL_TEX, item_list[i].x, item_list[i].y, item_list[i].z);
						}
						else
						{
							int displayitem = 1;
							if (strcmp(item_list[i].rname, "spiral") == 0)
							{
								displayitem = 0;
							}
							if (displayitem == 1)
							{
								PlayerToD3DVertList(item_list[i].model_id,
									item_list[i].current_frame, item_list[i].rot_angle,
									1,
									USE_DEFAULT_MODEL_TEX, item_list[i].x, item_list[i].y, item_list[i].z);
							}
						}
					}
				}
			}
		}
	}
}

void PlayerToD3DIndexedVertList(int pmodel_id, int curr_frame, float angle, int texture_alias, int tex_flag, float xt, float yt, float zt)
{

	float qdist = 0;
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
	float mx[6000];
	float my[6000];
	float mz[6000];
	float workx, worky, workz;
	XMFLOAT3 vw1, vw2, vw3;

	float x_off = 0;
	float y_off = 0;
	float z_off = 0;

	float wx = xt;
	float wy = yt;
	float wz = zt;

	if (curr_frame >= pmdata[pmodel_id].num_frames)
		curr_frame = 0;

	curr_frame = 0;
	//float cosine = cos_table[angle];
	//float sine = sin_table[angle];

	float cosine = (float)cos(angle * k);
	float sine = (float)sin(angle * k);

	i_count = 0;
	face_i_count = 0;

	/*if (rendering_first_frame == TRUE)
	{
		if (fopen_s(&fp, "ds.log", "a") != 0)
		{
		}
	}*/

	// process and transfer the model data from the pmdata structure
	// to the array of D3DVERTEX2 structures, src_v

	num_poly = pmdata[pmodel_id].num_polys_per_frame;

	ObjectsToDraw[number_of_polys_per_frame].srcstart = cnt;
	ObjectsToDraw[number_of_polys_per_frame].objectId = -1;

	int start_cnt = cnt;

	for (i = 0; i < num_poly; i++)
	{
		poly_command = pmdata[pmodel_id].poly_cmd[i];
		num_verts_per_poly = pmdata[pmodel_id].num_verts_per_object[i];
		num_faces_per_poly = pmdata[pmodel_id].num_faces_per_object[i];
		count_v = 0;

		ObjectsToDraw[number_of_polys_per_frame].srcstart = cnt;

		int counttri = 0;
		for (j = 0; j < num_verts_per_poly; j++)
		{

			x = pmdata[pmodel_id].w[curr_frame][i_count].x + x_off;
			z = pmdata[pmodel_id].w[curr_frame][i_count].y + y_off;
			y = pmdata[pmodel_id].w[curr_frame][i_count].z + z_off;

			rx = wx + (x * cosine - z * sine);
			ry = wy + y;
			rz = wz + (x * sine + z * cosine);

			if (fDot2 != 0.0f)
			{
				float newx, newy, newz;

				newx = x;
				newy = y;
				newz = z;

				rx = (newy * sinf(fDot2 * k) + newx * cosf(fDot2 * k));
				ry = (newy * cosf(fDot2 * k) - newx * sinf(fDot2 * k));
				rz = newz;

				newx = rx;
				newy = ry;
				newz = rz;

				rx = (newx * cosine - newz * sine);
				ry = newy;
				rz = (newx * sine + newz * cosine);

				newx = rx;
				newy = ry;
				newz = rz;
				rx = newx;
				ry = (newy * cosf(0.0f * k) - newz * sinf(0.0f * k));
				rz = (newy * sinf(0.0f * k) + newz * cosf(0.0f * k));
			}
			else
			{

				rx = (x * cosine - z * sine);
				ry = y;
				rz = (x * sine + z * cosine);
			}

			rx = rx + wx;
			ry = ry + wy;
			rz = rz + wz;

			tx = pmdata[pmodel_id].t[i_count].x * pmdata[pmodel_id].skx;
			ty = pmdata[pmodel_id].t[i_count].y * pmdata[pmodel_id].sky;
			ty = 1.0f - ty;

			src_v[cnt].x = D3DVAL(rx);
			src_v[cnt].y = D3DVAL(ry);
			src_v[cnt].z = D3DVAL(rz);
			src_v[cnt].tu = D3DVAL(tx);
			src_v[cnt].tv = D3DVAL(ty);

			src_collide[cnt] = 1;
			/*
			if (rendering_first_frame == TRUE)
			{
				fprintf(fp, "%f %f %f, ",
					src_v[cnt].x,
					src_v[cnt].y,
					src_v[cnt].z);
			}*/
			mx[j] = rx;
			my[j] = ry;
			mz[j] = rz;



			if (counttri == 2)
			{
				vw1.x = D3DVAL(mx[j - 2]);
				vw1.y = D3DVAL(my[j - 2]);
				vw1.z = D3DVAL(mz[j - 2]);

				vw2.x = D3DVAL(mx[j - 1]);
				vw2.y = D3DVAL(my[j - 1]);
				vw2.z = D3DVAL(mz[j - 1]);

				vw3.x = D3DVAL(mx[j]);
				vw3.y = D3DVAL(my[j]);
				vw3.z = D3DVAL(mz[j]);

				// calculate the NORMAL for the road using the CrossProduct <-important!

				XMVECTOR vDiff = XMLoadFloat3(&vw1) - XMLoadFloat3(&vw2);
				XMVECTOR vDiff2 = XMLoadFloat3(&vw3) - XMLoadFloat3(&vw2);
				XMVECTOR vCross, final;

				//D3DXVec3Cross(&vCross, &vDiff, &vDiff2);
				//D3DXVec3Normalize(&final, &vCross);

				vCross = XMVector3Cross(vDiff, vDiff2);
				final = XMVector3Normalize(vCross);

				//if (FastDistance(final.x,final.y,final.z) < 0.1f)
					//final = D3DXVECTOR3(0.0f, 0.0f, 0.0f);

				XMFLOAT3 final2;
				XMStoreFloat3(&final2, final);

				workx = (-final2.x);
				worky = (-final2.y);
				workz = (-final2.z);

				src_v[cnt - 2].nx = workx;
				src_v[cnt - 2].ny = worky;
				src_v[cnt - 2].nz = workz;

				src_v[cnt - 1].nx = workx;
				src_v[cnt - 1].ny = worky;
				src_v[cnt - 1].nz = workz;

				src_v[cnt].nx = workx;
				src_v[cnt].ny = worky;
				src_v[cnt].nz = workz;


				CalculateTangentBinormal(src_v[cnt -2 ], src_v[cnt - 1], src_v[cnt]);

				counttri = 0;
			}
			else {
				counttri++;
			}


			//if (j >= 2)
			//{

			//	src_v[cnt].nx = workx;
			//	src_v[cnt].ny = worky;
			//	src_v[cnt].nz = workz;
			//}


			//src_v[cnt].nx = 1.0f;
			//src_v[cnt].ny = 1.0f;
			//src_v[cnt].nz = 0.3f;

			cnt++;
			i_count++;

			

		} // end for j
		ObjectsToDraw[number_of_polys_per_frame].srcfstart = cnt_f;
		//			face_i_count = 0;
		for (j = 0; j < num_faces_per_poly * 3; j++)
		{
			src_f[cnt_f] = pmdata[pmodel_id].f[face_i_count];

			//if (rendering_first_frame == TRUE)
			//{
			//fprintf( fp, "%d ", src_f[cnt_f] );
			//}

			cnt_f++;
			face_i_count++;
		}

		float centroidx = (mx[0] + mx[1] + mx[2]) * QVALUE;
		float centroidy = (my[0] + my[1] + my[2]) * QVALUE;
		float centroidz = (mz[0] + mz[1] + mz[2]) * QVALUE;

		qdist = FastDistance(
			m_vEyePt.x - centroidx,
			m_vEyePt.y - centroidy,
			m_vEyePt.z - centroidz);

		ObjectsToDraw[number_of_polys_per_frame].vert_index = number_of_polys_per_frame;
		ObjectsToDraw[number_of_polys_per_frame].texture = texture_alias;

		ObjectsToDraw[number_of_polys_per_frame].dist = qdist;
		ObjectsToDraw[number_of_polys_per_frame].vertsperpoly = num_verts_per_poly;
		ObjectsToDraw[number_of_polys_per_frame].facesperpoly = num_faces_per_poly;

		verts_per_poly[number_of_polys_per_frame] = num_verts_per_poly;
		faces_per_poly[number_of_polys_per_frame] = num_faces_per_poly;

		dp_command_index_mode[number_of_polys_per_frame] = USE_INDEXED_DP;
		dp_commands[number_of_polys_per_frame] = D3DPT_TRIANGLELIST;

		num_triangles_in_scene += num_faces_per_poly;
		num_verts_in_scene += num_verts_per_poly;
		num_dp_commands_in_scene++;

		if (tex_flag == USE_PLAYERS_SKIN)
			texture_list_buffer[number_of_polys_per_frame] = texture_alias;
		else
			texture_list_buffer[number_of_polys_per_frame] = pmdata[pmodel_id].texture_list[i];

		number_of_polys_per_frame++;

	} // end for vert_cnt

	/*if (rendering_first_frame == TRUE)
	{
		fprintf(fp, " \n\n");
		fclose(fp);
	}*/





	return;
}

void AddModel(float x, float y, float z, float rot_angle, float monsterid, float monstertexture, float monnum, char modelid[80], char modeltexture[80], int ability)
{

	//if (monsterenable == 0)
	//	return;


	player_list2[num_players2].bIsPlayerValid = TRUE;

	player_list2[num_players2].x = x;
	player_list2[num_players2].y = y;
	player_list2[num_players2].z = z;
	player_list2[num_players2].rot_angle = rot_angle;
	player_list2[num_players2].model_id = (int)monsterid;
	player_list2[num_players2].skin_tex_id = (int)monstertexture;

	player_list2[num_players2].current_sequence = 0;
	player_list2[num_players2].ability = ability;
	player_list2[num_players2].draw_external_wep = FALSE;

	player_list2[num_players2].monsterid = (int)monnum;

	strcpy_s(player_list2[num_players2].rname, modelid);
	strcpy_s(player_list2[num_players2].texturename, modeltexture);


	if (strstr(modelid, "switch") != NULL)
	{

		switchmodify[countswitches].num = num_players2;
		switchmodify[countswitches].objectid = (int)monsterid;
		switchmodify[countswitches].active = 0;

		if (rot_angle >= 0.0f && rot_angle < 90.0f)
		{
			switchmodify[countswitches].direction = 1;
			switchmodify[countswitches].savelocation = x;
		}
		if (rot_angle >= 90.0f && rot_angle < 180.0f)
		{
			switchmodify[countswitches].direction = 2;
			switchmodify[countswitches].savelocation = z;
		}
		if (rot_angle >= 180.0f && rot_angle < 270.0f)
		{
			switchmodify[countswitches].direction = 3;
			switchmodify[countswitches].savelocation = x;
		}
		if (rot_angle >= 270.0f && rot_angle <= 360.0f)
		{
			switchmodify[countswitches].direction = 4;
			switchmodify[countswitches].savelocation = z;
		}
		switchmodify[countswitches].x = x;
		switchmodify[countswitches].y = y;
		switchmodify[countswitches].z = z;
		switchmodify[countswitches].count = 0;
		countswitches++;
	}

	num_players2++;

}

int FindGunTexture(char* p)
{
	int i = 0;

	for (i = 0; i < num_your_guns; i++)
	{
		if (strcmp(your_gun[i].gunname, p) == 0)
		{

			return your_gun[i].skin_tex_id;
		}
	}

	return 0;
}

int CycleBitMap(int i)
{

	char texname[200];
	char junk[200];
	char junk2[200];

	char* p;
	int talias;
	int tnum;
	int num = 0;
	int count = 0;
	int result;

	talias = i;

	strcpy_s(texname, TexMap[talias].tex_alias_name);

	p = strstr(texname, "@");

	if (p != NULL)
	{
		if (maingameloop2 == 0)
			return FindTextureAlias(texname);
		strcpy_s(junk, p + 1);

		while (texname[num] != '@')
			junk2[count++] = texname[num++];

		junk2[count] = '\0';

		tnum = atoi(junk);

		tnum++;

		sprintf_s(junk, "%s@%d", junk2, tnum);

		result = FindTextureAlias(junk);
		if (result == -1)
		{
			sprintf_s(junk, "%s@1", junk2);
			result = FindTextureAlias(junk);
		}

		return result;
	}

	return -1;
}

/*
void DrawBoundingBox(IDirect3DDevice9* pd3dDevice)
{

	pd3dDevice->SetTexture(0, g_pTextureList[157]); //set texture

	g_pVB->Lock(0, sizeof(&pBoundingBox), (void**)&pBoundingBox, 0);
	for (int i = 0; i < countboundingbox; i++)
	{
		D3DXVECTOR3 a = D3DXVECTOR3(boundingbox[i].x, boundingbox[i].y, boundingbox[i].z);
		pBoundingBox[i].position = a;
		pBoundingBox[i].color = D3DCOLOR_RGBA(105, 105, 105, 0); //0xffffffff;
	}
	g_pVB->Unlock();

	int g = 1;
	for (int i = 0; i < countboundingbox; i = i + 4)
	{
		pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, i, 2);
	}
}
*/

/*
void FlashLight(IDirect3DDevice9* pd3dDevice) {

	bool bIsFlashlightOn = true;
	float radius = 20.0f; // used for flashlight
	int lighttype = 1;
	// Set up the light structure
	D3DLIGHT9 light;
	ZeroMemory(&light, sizeof(D3DLIGHT9));

	light.Diffuse.r = 1.0f;
	light.Diffuse.g = 1.0f;
	light.Diffuse.b = 1.0f;

	light.Ambient.r = 0.3f;
	light.Ambient.g = 0.3f;
	light.Ambient.b = 0.3f;


	light.Range = 500.0f; // D3DLIGHT_RANGE_MAX

	// Calculate the flashlight's lookat point, from
	// the player's view direction angy.

	float lx = m_vEyePt.x + radius * sinf(angy * k);
	float ly = 0;
	float lz = m_vEyePt.z + radius * cosf(angy * k);

	// Calculate direction vector for flashlight
	float dir_x = lx - m_vEyePt.x;
	float dir_y = 0; //ly - m_vEyePt.y;
	float dir_z = lz - m_vEyePt.z;

	// set flashlight's position to player's position
	float pos_x = player_list[trueplayernum].x;
	float pos_y = player_list[trueplayernum].y;
	float pos_z = player_list[trueplayernum].z;

	if (lighttype == 0)
	{
		light.Position = D3DXVECTOR3(pos_x, pos_y, pos_z);
		light.Direction = D3DXVECTOR3(dir_x, dir_y, dir_z);
		light.Falloff = .1f;
		light.Theta = .6f; // spotlight's inner cone
		light.Phi = 1.3f;	 // spotlight's outer cone
		light.Attenuation0 = 1.0f;
		light.Type = D3DLIGHT_SPOT;
	}
	else
	{

		light.Type = D3DLIGHT_POINT;

		if (strstr(your_gun[current_gun].gunname, "LIGHTNINGSWORD"))
		{
			light.Ambient.r = 1.0f;
			light.Ambient.g = 1.0f;
			light.Ambient.b = 1.0f;
		}
		else if (strstr(your_gun[current_gun].gunname, "FLAME") != NULL)
		{
			light.Ambient.r = 1.0f;
			light.Ambient.g = 0.2f;
			light.Ambient.b = 0.3f;
		}
		else
		{
			light.Ambient.r = 0.4f;
			light.Ambient.g = 0.3f;
			light.Ambient.b = 1.0f;
		}

		light.Diffuse.r = light.Ambient.r;
		light.Diffuse.g = light.Ambient.g;
		light.Diffuse.b = light.Ambient.b;

		light.Specular.r = 0.0f;
		light.Specular.g = 0.0f;
		light.Specular.b = 0.0f;
		light.Range = 200.0f;
		light.Position.x = pos_x;
		light.Position.y = pos_y;
		light.Position.z = pos_z;

		light.Attenuation0 = 1.0f;
	}

	if (bIsFlashlightOn == TRUE)
	{
		pd3dDevice->SetLight(num_light_sources, &light);
		pd3dDevice->LightEnable((DWORD)num_light_sources, TRUE);
		num_light_sources++;
	}

}

void AddMissleLight(IDirect3DDevice9* pd3dDevice) {

	for (int misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{
		if (your_missle[misslecount].active == 1)
		{
			D3DVECTOR collidenow;
			D3DVECTOR saveeye;
			float qdist = FastDistance(
				player_list[trueplayernum].x - your_missle[misslecount].x,
				player_list[trueplayernum].y - your_missle[misslecount].y,
				player_list[trueplayernum].z - your_missle[misslecount].z);
			your_missle[misslecount].qdist = qdist;

			your_missle[misslecount].qdist = qdist;

			if (qdist > culldist)
			{
				your_missle[misslecount].active = 0;
			}
			else
			{
				// Set up the light structure
				D3DLIGHT9 light;
				ZeroMemory(&light, sizeof(D3DLIGHT9));


				if (current_gun == 18)
				{
					light.Ambient.r = 1.0f;
					light.Ambient.g = 1.0f;
					light.Ambient.b = 1.0f;
				}
				else if (current_gun == 19)
				{
					light.Ambient.r = 1.0f;
					light.Ambient.g = 0.2f;
					light.Ambient.b = 0.3f;
				}
				else
				{
					light.Ambient.r = 0.4f;
					light.Ambient.g = 0.3f;
					light.Ambient.b = 1.0f;
				}

				light.Diffuse.r = light.Ambient.r;
				light.Diffuse.g = light.Ambient.g;
				light.Diffuse.b = light.Ambient.b;

				light.Specular.r = 0.2f;
				light.Specular.g = 0.2f;
				light.Specular.b = 0.2f;

				light.Range = 100.0f;

				light.Position = D3DXVECTOR3(your_missle[misslecount].x,
					your_missle[misslecount].y, your_missle[misslecount].z);

				light.Attenuation0 = 1.0f;
				light.Type = D3DLIGHT_POINT;

				//if (num_light_sources < 7) {
				pd3dDevice->SetLight(num_light_sources, &light);
				pd3dDevice->LightEnable((DWORD)num_light_sources, TRUE);
				num_light_sources++;
				//}
				//num_light_sources++;
			}
		}
	}
}

*/