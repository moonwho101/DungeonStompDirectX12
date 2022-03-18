#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include "Waves.h"
#include <d3dtypes.h>
#include "world.hpp"
#include "GlobalSettings.hpp"
#include "Missle.hpp"
#include "GameLogic.hpp"
#include "DungeonStomp.hpp"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

extern int textcounter;
extern char gfinaltext[2048];
int numCharacters = 0;
extern ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap;
ID3D12PipelineState* textPSO; // pso containing a pipeline state

Font arialFont; // this will store our arial font information
static wchar_t* charToWChar(const char* text);

struct gametext
{
	int textnum;
	int type;
	char text[2048];
	int shown;
};

extern gametext gtext[200];
int maxNumTextCharacters = 1024; // the maximum number of characters you can render during a frame. This is just used to make sure
								// there is enough memory allocated for the text vertex buffer each frame

extern FLOAT LevelModTime;
extern int totalmod;
extern LEVELMOD* levelmodify;
void AddTreasureDrop(float x, float y, float z, int raction);
void ScanModJump(int jump);
extern int countmodtime;
extern FLOAT LevelModLastTime;
extern int countswitches;
extern SWITCHMOD* switchmodify;


Font LoadFont(LPCWSTR filename, int windowWidth, int windowHeight)
{
	std::wifstream fs;
	fs.open(filename);

	Font font;
	std::wstring tmp;
	int startpos;

	// extract font name
	fs >> tmp >> tmp; // info face="Arial"
	startpos = tmp.find(L"\"") + 1;
	font.name = tmp.substr(startpos, tmp.size() - startpos - 1);

	// get font size
	fs >> tmp; // size=73
	startpos = tmp.find(L"=") + 1;
	font.size = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// bold, italic, charset, unicode, stretchH, smooth, aa, padding, spacing
	fs >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp >> tmp; // bold=0 italic=0 charset="" unicode=0 stretchH=100 smooth=1 aa=1 

	// get padding
	fs >> tmp; // padding=5,5,5,5 
	startpos = tmp.find(L"=") + 1;
	tmp = tmp.substr(startpos, tmp.size() - startpos); // 5,5,5,5

	// get up padding
	startpos = tmp.find(L",") + 1;
	font.toppadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get right padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	startpos = tmp.find(L",") + 1;
	font.rightpadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get down padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	startpos = tmp.find(L",") + 1;
	font.bottompadding = std::stoi(tmp.substr(0, startpos)) / (float)windowWidth;

	// get left padding
	tmp = tmp.substr(startpos, tmp.size() - startpos);
	font.leftpadding = std::stoi(tmp) / (float)windowWidth;

	fs >> tmp; // spacing=0,0

	// get lineheight (how much to move down for each line), and normalize (between 0.0 and 1.0 based on size of font)
	fs >> tmp >> tmp; // common lineHeight=95
	startpos = tmp.find(L"=") + 1;
	font.lineHeight = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

	// get base height (height of all characters), and normalize (between 0.0 and 1.0 based on size of font)
	fs >> tmp; // base=68
	startpos = tmp.find(L"=") + 1;
	font.baseHeight = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

	// get texture width
	fs >> tmp; // scaleW=512
	startpos = tmp.find(L"=") + 1;
	font.textureWidth = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// get texture height
	fs >> tmp; // scaleH=512
	startpos = tmp.find(L"=") + 1;
	font.textureHeight = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// get pages, packed, page id
	fs >> tmp >> tmp; // pages=1 packed=0
	fs >> tmp >> tmp; // page id=0

	// get texture filename
	std::wstring wtmp;
	fs >> wtmp; // file="Arial.png"
	startpos = wtmp.find(L"\"") + 1;
	font.fontImage = wtmp.substr(startpos, wtmp.size() - startpos - 1);

	// get number of characters
	fs >> tmp >> tmp; // chars count=97
	startpos = tmp.find(L"=") + 1;
	font.numCharacters = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// initialize the character list
	font.CharList = new FontChar[font.numCharacters];

	for (int c = 0; c < font.numCharacters; ++c)
	{
		// get unicode id
		fs >> tmp >> tmp; // char id=0
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].id = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

		// get x
		fs >> tmp; // x=392
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].u = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)font.textureWidth;

		// get y
		fs >> tmp; // y=340
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].v = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)font.textureHeight;

		// get width
		fs >> tmp; // width=47
		startpos = tmp.find(L"=") + 1;
		tmp = tmp.substr(startpos, tmp.size() - startpos);
		font.CharList[c].width = (float)std::stoi(tmp) / (float)windowWidth;
		font.CharList[c].twidth = (float)std::stoi(tmp) / (float)font.textureWidth;

		// get height
		fs >> tmp; // height=57
		startpos = tmp.find(L"=") + 1;
		tmp = tmp.substr(startpos, tmp.size() - startpos);
		font.CharList[c].height = (float)std::stoi(tmp) / (float)windowHeight;
		font.CharList[c].theight = (float)std::stoi(tmp) / (float)font.textureHeight;

		// get xoffset
		fs >> tmp; // xoffset=-6
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].xoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowWidth;

		// get yoffset
		fs >> tmp; // yoffset=16
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].yoffset = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowHeight;

		// get xadvance
		fs >> tmp; // xadvance=65
		startpos = tmp.find(L"=") + 1;
		font.CharList[c].xadvance = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos)) / (float)windowWidth;

		// get page
		// get channel
		fs >> tmp >> tmp; // page=0    chnl=0
	}

	// get number of kernings
	fs >> tmp >> tmp; // kernings count=96
	startpos = tmp.find(L"=") + 1;
	font.numKernings = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

	// initialize the kernings list
	font.KerningsList = new FontKerning[font.numKernings];

	for (int k = 0; k < font.numKernings; ++k)
	{
		// get first character
		fs >> tmp >> tmp; // kerning first=87
		startpos = tmp.find(L"=") + 1;
		font.KerningsList[k].firstid = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

		// get second character
		fs >> tmp; // second=45
		startpos = tmp.find(L"=") + 1;
		font.KerningsList[k].secondid = std::stoi(tmp.substr(startpos, tmp.size() - startpos));

		// get amount
		fs >> tmp; // amount=-1
		startpos = tmp.find(L"=") + 1;
		int t = (float)std::stoi(tmp.substr(startpos, tmp.size() - startpos));
		font.KerningsList[k].amount = (float)t / (float)windowWidth;
	}

	return font;
}


void DungeonStompApp::RenderText(Font font, std::wstring text, XMFLOAT2 pos, XMFLOAT2 scale, XMFLOAT2 padding, XMFLOAT4 color)
{
	// clear the depth buffer so we can draw over everything
	//mCommandList->ClearDepthStencilView(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	//mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// set the text pipeline state object
	mCommandList->SetPipelineState(textPSO);

	// this way we only need 4 vertices per quad rather than 6 if we were to use a triangle list topology
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	//mCommandList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);


	// set the text vertex buffer
	mCommandList->IASetVertexBuffers(0, 1, &textVertexBufferView);

	// bind the text srv. We will assume the correct descriptor heap and table are currently bound and set
	//mCommandList->SetGraphicsRootDescriptorTable(3, font.srvHandle);

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	tex.Offset(103, mCbvSrvDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(3, tex);

	float topLeftScreenX = (pos.x * 2.0f) - 1.0f;
	float topLeftScreenY = ((1.0f - pos.y) * 2.0f) - 1.0f;

	float x = topLeftScreenX;
	float y = topLeftScreenY;

	float horrizontalPadding = (font.leftpadding + font.rightpadding) * padding.x;
	float verticalPadding = (font.toppadding + font.bottompadding) * padding.y;

	// cast the gpu virtual address to a textvertex, so we can directly store our vertices there
	TextVertex* vert = (TextVertex*)textVBGPUAddress;

	wchar_t lastChar = -1; // no last character to start with

	for (int i = 0; i < text.size(); ++i)
	{
		wchar_t c = text[i];

		FontChar* fc = font.GetChar(c);

		// character not in font char set
		if (fc == nullptr)
			continue;

		// end of string
		if (c == L'\0')
			break;

		// new line
		if (c == L'\n')
		{
			x = topLeftScreenX;
			y -= (font.lineHeight + verticalPadding) * scale.y;
			continue;
		}

		// don't overflow the buffer. In your app if this is true, you can implement a resize of your text vertex buffer
		if (numCharacters >= maxNumTextCharacters)
			break;

		float kerning = 0.0f;
		if (i > 0)
			kerning = font.GetKerning(lastChar, c);

		vert[numCharacters] = TextVertex(color.x,
			color.y,
			color.z,
			color.w,
			fc->u,
			fc->v,
			fc->twidth,
			fc->theight,
			x + ((fc->xoffset + kerning) * scale.x),
			y - (fc->yoffset * scale.y),
			fc->width * scale.x,
			fc->height * scale.y);

		numCharacters++;

		// remove horrizontal padding and advance to next char position
		x += (fc->xadvance - horrizontalPadding) * scale.x;

		lastChar = c;
	}

	// we are going to have 4 vertices per character (trianglestrip to make quad), and each instance is one character
	mCommandList->DrawInstanced(4, numCharacters, 0, 0);
}


void DungeonStompApp::DisplayHud() {


	numCharacters = 0;

	//RenderText(arialFont, std::wstring(L"Dungeon Stomp Direct12 by Mark Longo"), XMFLOAT2(0.01f, 0.0f), XMFLOAT2(0.30f, 0.30f));
	//RenderText(arialFont, std::wstring(L"12345"), XMFLOAT2(0.5f, 0.5f), XMFLOAT2(1.0f, 1.0f));

	char junk[255];

	float adjust = 170.0f;

	//DisplayHud();

	//sprintf_s(junk, "Dungeon Stomp 1.90");
	//display_message(5.0f, (FLOAT)wHeight - adjust - 14.0f, junk, 255, 255, 0, 12.5, 16, 0);



	//sprintf_s(junk, "Dungeon Stomp 1.90 %llu " , gametimer);
	sprintf_s(junk, "J");
	//display_message(5.0f, (FLOAT)wHeight - adjust - 14.0f, junk, 255, 255, 0, 12.5, 16, 0);
	//RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.0f, 0.8f), XMFLOAT2(0.30f, 0.30f)); //, XMFLOAT2(0.5f, 0.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f));
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(-0.45f, 0.35f), XMFLOAT2(34.00f, 34.00f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));

	//sprintf_s(junk, "Area: ");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 10.0f, junk, 255, 255, 0, 12.5, 16, 0);
	//RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.0f, 0.82f), XMFLOAT2(0.30f, 0.30f));
	//sprintf_s(junk, "%s", gfinaltext);
	//RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.05f, 0.82f), XMFLOAT2(0.30f, 0.30f));
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 10.0f, junk, 0, 245, 255, 12.5, 16, 0);

	//statusbardisplay((float)player_list[trueplayernum].hp, (float)player_list[trueplayernum].hp, 1);

	sprintf_s(junk, "Health");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 24.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.0f, 0.82f), XMFLOAT2(0.30f, 0.30f));

	sprintf_s(junk, "%d/%d", player_list[trueplayernum].health, player_list[trueplayernum].hp);
	//display_message(0.0f + 110.0f, (FLOAT)wHeight - adjust + 24.0f, junk, 255, 255, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.82f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	sprintf_s(junk, "Weapon");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 38.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.84f), XMFLOAT2(0.30f, 0.30f));

	char junk3[255];
	if (strstr(your_gun[current_gun].gunname, "SCROLL-MAGICMISSLE") != NULL)
	{
		strcpy_s(junk3, "MAGIC MISSLE");
		sprintf_s(junk, "%s: %d", junk3, (int)your_gun[current_gun].x_offset);
	}
	else if (strstr(your_gun[current_gun].gunname, "SCROLL-FIREBALL") != NULL)
	{
		strcpy_s(junk3, "FIREBALL");
		sprintf_s(junk, "%s: %d", junk3, (int)your_gun[current_gun].x_offset);
	}
	else if (strstr(your_gun[current_gun].gunname, "SCROLL-LIGHTNING") != NULL)
	{
		strcpy_s(junk3, "LIGHTNING");
		sprintf_s(junk, "%s: %d", junk3, (int)your_gun[current_gun].x_offset);
	}
	else if (strstr(your_gun[current_gun].gunname, "SCROLL-HEALING") != NULL)
	{
		strcpy_s(junk3, "HEALING");
		sprintf_s(junk, "%s: %d", junk3, (int)your_gun[current_gun].x_offset);
	}
	else
	{
		sprintf_s(junk, "%s", your_gun[current_gun].gunname);
	}
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 38.0f, junk, 0, 245, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.84f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	sprintf_s(junk, "Damage");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 52.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.86f), XMFLOAT2(0.30f, 0.30f));
	sprintf_s(junk, "%dD%d", player_list[trueplayernum].damage1, player_list[trueplayernum].damage2);
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 52.0f, junk, 0, 245, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.86f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	int attackbonus = your_gun[current_gun].sattack;
	int damagebonus = your_gun[current_gun].sdamage;

	sprintf_s(junk, "Bonus");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 66.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.88f), XMFLOAT2(0.30f, 0.30f));
	sprintf_s(junk, "+%d/%+d", attackbonus, damagebonus);
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 66.0f, junk, 0, 245, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.88f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	int nextlevelxp = LevelUpXPNeeded(player_list[trueplayernum].xp) + 1;

	sprintf_s(junk, "XP");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 80.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.90f), XMFLOAT2(0.30f, 0.30f));
	sprintf_s(junk, "%d", player_list[trueplayernum].xp);
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 80.0f, junk, 0, 245, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.90f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	sprintf_s(junk, "Level");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 94.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.92f), XMFLOAT2(0.30f, 0.30f));
	sprintf_s(junk, "%d (%d)", player_list[trueplayernum].hd, nextlevelxp);
	//sprintf_s(junk, "%d (%d)", player_list[trueplayernum].hd, 0);
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 94.0f, junk, 0, 245, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.92f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	sprintf_s(junk, "Armour");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 108.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.94f), XMFLOAT2(0.30f, 0.30f));
	sprintf_s(junk, "%d", player_list[trueplayernum].ac);
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 108.0f, junk, 0, 245, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.94f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	//sprintf_s(junk, "THAC: ");
	////display_message(0.0f, (FLOAT)wHeight - adjust + 122.0f, junk, 255, 255, 0, 12.5, 16, 0);
	//RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.96f), XMFLOAT2(0.30f, 0.30f));
	//sprintf_s(junk, "%d", player_list[trueplayernum].thaco);
	////display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 122.0f, junk, 0, 245, 255, 12.5, 16, 0);
	//RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.96f), XMFLOAT2(0.30f, 0.30f));

	sprintf_s(junk, "Gold");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 136.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.96f), XMFLOAT2(0.30f, 0.30f));
	sprintf_s(junk, "%d", player_list[trueplayernum].gold);
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 136.0f, junk, 0, 245, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.96f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

	sprintf_s(junk, "Keys");
	//display_message(0.0f, (FLOAT)wHeight - adjust + 150.0f, junk, 255, 255, 0, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.00f, 0.98f), XMFLOAT2(0.30f, 0.30f));
	sprintf_s(junk, "%d", player_list[trueplayernum].keys);
	//display_message(0.0f + 60.0f, (FLOAT)wHeight - adjust + 150.0f, junk, 0, 245, 255, 12.5, 16, 0);
	RenderText(arialFont, charToWChar(junk), XMFLOAT2(0.08f, 0.98f), XMFLOAT2(0.30f, 0.30f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));


	int flag = 1;
	float scrollmessage1 = 60;
	int count = 0;
	int scount = 0;
	char junk2[2048];
	int scrolllistnum = 6;

	scount = sliststart;
	//scrollmessage1 = 14.0f * (scrolllistnum + 2);
	scrollmessage1 = 0;

	while (flag)
	{
		sprintf_s(junk2, "%s", scrolllist1[scount].text);
		//display_message(0.0f, scrollmessage1, junk2, scrolllist1[scount].r, scrolllist1[scount].g, scrolllist1[scount].b, 12.5, 16, 0);

		RenderText(arialFont, charToWChar(junk2), XMFLOAT2(0.0f, 0.1f + scrollmessage1), XMFLOAT2(0.30f, 0.30f));


		scrollmessage1 -= 0.02f;

		count++;
		scount--;

		if (scount < 0)
			scount = scrolllistnum - 1;

		if (count >= scrolllistnum)
			flag = 0;
	}

}

void DungeonStompApp::ScanMod()
{
	int i = 0;
	int j = 0;
	int gotone = 0;

	int counter = 0;
	float qdist = 0;
	LevelModTime = timeGetTime() * 0.001f;

	for (i = 0; i < totalmod; i++)
	{

		for (j = 0; j < num_monsters; j++)
		{
			if (monster_list[j].monsterid == levelmodify[counter].objectid &&
				levelmodify[counter].active == 1)
			{
				//DropTreasure
				if (strstr(levelmodify[counter].Function, "DropTreasure") != NULL &&
					levelmodify[counter].active == 1)
				{
					if (monster_list[j].bIsPlayerAlive == FALSE)
					{
						levelmodify[counter].active = 2;
						AddTreasureDrop(monster_list[j].x, monster_list[j].y, monster_list[j].z, atoi(levelmodify[counter].Text1));
					}
				}
				//MonsterActive
				if (strstr(levelmodify[counter].Function, "MonsterActive") != NULL &&
					levelmodify[counter].active == 1)
				{
					if (monster_list[j].ability == 0)
					{
						if (gotone == 0)
						{
							//DisplayDialogText(levelmodify[counter].Text1, -20.0f);
							int len = strlen(levelmodify[counter].Text1);
							len = len / 2;
							RenderText(arialFont, charToWChar(levelmodify[counter].Text1), XMFLOAT2(0.5f - (len * 0.005f), 0.5f), XMFLOAT2(0.20f, 0.20f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

						}
						gotone = 1;

						ScanModJump(levelmodify[counter].jump);
						if (countmodtime == 0)
						{
							LevelModLastTime = timeGetTime() * 0.001f;
							countmodtime = 1;
						}

						if (LevelModTime - LevelModLastTime >= 5.0f)
						{
							levelmodify[counter].active = 0;
							countmodtime = 0;
						}
					}
				}
				//XP
				if (strstr(levelmodify[counter].Function, "XPPoints") != NULL &&
					levelmodify[counter].active == 1)
				{
					if (monster_list[j].bIsPlayerAlive == FALSE)
					{
						levelmodify[counter].active = 0;
						int xp = atoi(levelmodify[counter].Text1);
						sprintf_s(gActionMessage, "You got %d XP!.", xp);
						UpdateScrollList(0, 245, 255);
						player_list[trueplayernum].xp += xp;
						//LevelUp(player_list[trueplayernum].xp);
					}
				}

				//SetHitPoints

				if (strstr(levelmodify[counter].Function, "SetHitPoints") != NULL &&
					levelmodify[counter].active == 1)
				{

					levelmodify[counter].active = 0;
					monster_list[j].health = atoi(levelmodify[counter].Text1);
					monster_list[j].hp = atoi(levelmodify[counter].Text1);
				}

				//IsDeadText
				if (strstr(levelmodify[counter].Function, "IsDeadText") != NULL &&
					levelmodify[counter].active == 1)
				{
					if (monster_list[j].bIsPlayerAlive == FALSE)
					{
						if (gotone == 0)
						{
							//DisplayDialogText(levelmodify[counter].Text1, -20.0f);
							int len = strlen(levelmodify[counter].Text1);
							len = len / 2;
							RenderText(arialFont, charToWChar(levelmodify[counter].Text1), XMFLOAT2(0.5f - (len * 0.005f), 0.5f), XMFLOAT2(0.20f, 0.20f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
						}
						gotone = 1;

						ScanModJump(levelmodify[counter].jump);
						if (countmodtime == 0)
						{
							LevelModLastTime = timeGetTime() * 0.001f;
							countmodtime = 1;
						}

						if (LevelModTime - LevelModLastTime >= 5.0f)
						{
							levelmodify[counter].active = 0;
							countmodtime = 0;
						}
					}
				}
				//isNear
				if (strstr(levelmodify[counter].Function, "IsNear") != NULL &&
					levelmodify[counter].active == 1)
				{
					qdist = FastDistance(
						player_list[trueplayernum].x - monster_list[j].x,
						player_list[trueplayernum].y - monster_list[j].y,
						player_list[trueplayernum].z - monster_list[j].z);

					if (qdist < 300.0f)
					{
						if (gotone == 0)
						{
							//DisplayDialogText(levelmodify[counter].Text1, -20.0f);
							int len = strlen(levelmodify[counter].Text1);
							len = len / 2;
							RenderText(arialFont, charToWChar(levelmodify[counter].Text1), XMFLOAT2(0.5f - (len * 0.005f), 0.5f), XMFLOAT2(0.20f, 0.20f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

						}
						gotone = 1;

						ScanModJump(levelmodify[counter].jump);
						if (countmodtime == 0)
						{
							LevelModLastTime = timeGetTime() * 0.001f;
							countmodtime = 1;
						}

						if (LevelModTime - LevelModLastTime >= 5.0f)
						{
							levelmodify[counter].active = 0;
							countmodtime = 0;
						}
					}
				}
			}
			// 	break;
		}

		for (j = 0; j < num_players2; j++)
		{
			if (player_list2[j].monsterid == levelmodify[counter].objectid &&
				levelmodify[counter].active == 1)
			{
				//Moveup
				if (strstr(levelmodify[counter].Function, "MoveUp") != NULL &&
					levelmodify[counter].active == 1)
				{

					if (levelmodify[counter].currentheight == -9999)
						levelmodify[counter].currentheight = player_list2[j].y;

					ScanModJump(levelmodify[counter].jump);
					if (player_list2[j].y - levelmodify[counter].currentheight <= atoi(levelmodify[counter].Text1))
					{
						player_list2[j].y++;
						qdist = FastDistance(
							player_list[trueplayernum].x - player_list2[j].x,
							player_list[trueplayernum].y - player_list2[j].y,
							player_list[trueplayernum].z - player_list2[j].z);
						//closesoundid[3] = qdist;
					}
					else
					{
						levelmodify[counter].active = 0;
					}
				}
				//isswitch
				if (strstr(levelmodify[counter].Function, "IsSwitch") != NULL)
				{
					for (int t = 0; t < countswitches; t++)
					{
						if (j == switchmodify[t].num)
						{
							if (switchmodify[t].active == 2)
							{
								levelmodify[levelmodify[counter].jump - 1].active = 1;
							}
						}
					}
				}

				//isNear
				if (strstr(levelmodify[counter].Function, "IsNear") != NULL &&
					levelmodify[counter].active == 1)
				{
					qdist = FastDistance(
						player_list[trueplayernum].x - player_list2[j].x,
						player_list[trueplayernum].y - player_list2[j].y,
						player_list[trueplayernum].z - player_list2[j].z);

					if (qdist < 300.0f)
					{
						if (gotone == 0)
						{
							//DisplayDialogText(levelmodify[counter].Text1, -20.0f);
							int len = strlen(levelmodify[counter].Text1);
							len = len / 2;
							RenderText(arialFont, charToWChar(levelmodify[counter].Text1), XMFLOAT2(0.5f - (len * 0.005f), 0.5f), XMFLOAT2(0.20f, 0.20f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));

						}
						gotone = 1;

						ScanModJump(levelmodify[counter].jump);
						if (countmodtime == 0)
						{
							LevelModLastTime = timeGetTime() * 0.001f;
							countmodtime = 1;
						}

						if (LevelModTime - LevelModLastTime >= 5.0f)
						{
							levelmodify[counter].active = 0;
							countmodtime = 0;
						}
					}
				}
			}
		}

		for (j = 0; j < itemlistcount; j++)
		{
			if (item_list[j].monsterid == levelmodify[counter].objectid &&
				levelmodify[counter].active == 1)
			{
				//Moveup
				if (strstr(levelmodify[counter].Function, "MoveUp") != NULL &&
					levelmodify[counter].active == 1)
				{

					if (levelmodify[counter].currentheight == -9999)
						levelmodify[counter].currentheight = item_list[j].y;

					ScanModJump(levelmodify[counter].jump);
					if (item_list[j].y - levelmodify[counter].currentheight <= atoi(levelmodify[counter].Text1))
					{
						item_list[j].y++;
						qdist = FastDistance(
							player_list[trueplayernum].x - item_list[j].x,
							player_list[trueplayernum].y - item_list[j].y,
							player_list[trueplayernum].z - item_list[j].z);
						//closesoundid[3] = qdist;
					}
					else
					{
						levelmodify[counter].active = 0;
					}
				}

				//TreasueAmount
				if (strstr(levelmodify[counter].Function, "TreasureAmount") != NULL &&
					levelmodify[counter].active == 1)
				{
					levelmodify[counter].active = 0;

					item_list[j].gold = atoi(levelmodify[counter].Text1);
				}
			}
		}
		counter++;
	}
}

static wchar_t* charToWChar(const char* text)
{
	const size_t size = strlen(text) + 1;
	wchar_t* wText = new wchar_t[size];

	//TODO: Fix this
	mbstowcs(wText, text, size);
	return wText;
}


void DungeonStompApp::SetDungeonText()
{

	for (int q = 0; q < oblist_length; q++)
	{
		int angle = (int)oblist[q].rot_angle;
		int ob_type = oblist[q].type;
		if (ob_type == 120)
		{
			float	qdist = FastDistance(m_vEyePt.x - oblist[q].x, m_vEyePt.y - oblist[q].y, m_vEyePt.z - oblist[q].z);
			if (qdist < 500.0f) {
				if (strstr(oblist[q].name, "text") != NULL)
				{
					for (int il = 0; il < textcounter; il++)
					{
						if (gtext[il].textnum == q)
						{
							if (gtext[il].type == 0)
							{
								strcpy_s(gfinaltext, gtext[il].text);
							}
							else if (gtext[il].type == 1 || gtext[il].type == 2)
							{
								if (qdist < 200.0f)
								{

									//DisplayDialogText(gtext[il].text, 0.0f);
									//XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f));

									int len = strlen(gtext[il].text);
									len = len / 2;
									RenderText(arialFont, charToWChar(gtext[il].text), XMFLOAT2(0.5f - (len * 0.005f), 0.5f), XMFLOAT2(0.20f, 0.20f), XMFLOAT2(0.5f, 0.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f));
								}
							}
						}
					}
				}
			}
		}
	}
}

