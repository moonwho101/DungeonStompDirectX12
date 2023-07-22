//***************************************************************************************
// DungeonStompApp.cpp by Mark Longo (C) 2022 All Rights Reserved.
//***************************************************************************************

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include "Dungeon.h"
#include <d3dtypes.h>
#include "world.hpp"
#include "GlobalSettings.hpp"
#include "Missle.hpp"
#include "GameLogic.hpp"
#include "DungeonStomp.hpp"
#include "Ssao.h"
#include "CameraBob.hpp"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

Font LoadFont(LPCWSTR filename, int windowWidth, int windowHeight);

const int gNumFrameResources = 3;
extern int numCharacters;
extern char gfinaltext[2048];
int LevelUpXPNeeded(int xp);
extern Font arialFont; // this will store our arial font information
extern int maxNumTextCharacters;
extern int maxNumRectangleCharacters;

extern POLY_SORT ObjectsToDraw[MAX_NUM_QUADS];
extern BOOL* dp_command_index_mode;
extern int cnt;
extern D3DVERTEX2* src_v;
extern int number_of_polys_per_frame;
extern int savelastmove;

extern int displayCaptureIndex[1000];
extern int displayCaptureCount[1000];
extern int displayCapture;
extern int displayShadowMap;

extern int playercurrentmove;

int displayShadowMapKeyPress = 0;

bool enableSSao = false;
bool enableSSaoKey = false;

bool enableCameraBob = true;
bool enableCameraBobKey = false;

extern int playerObjectStart;
extern int playerGunObjectStart;
extern int playerObjectEnd;
extern int gravityon;
extern int outside;

CameraBob bobY;
CameraBob bobX;
float centrex = 0;
float centrey = 0;
bool centre = false;
bool stopx = false;
bool stopy = false;

bool drawingShadowMap = false;
bool drawingSSAO = false;

std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
extern int number_of_tex_aliases;

ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
extern ID3D12PipelineState* textPSO; // pso containing a pipeline state
extern ID3D12PipelineState* rectanglePSO[MaxRectangle]; // pso containing a pipeline state

VOID UpdateControls();
HRESULT FrameMove(double fTime, FLOAT fTimeKey);
void UpdateWorld(float fElapsedTime);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		DungeonStompApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

DungeonStompApp::DungeonStompApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{

	// Estimate the scene bounding sphere manually since we know how the scene was constructed.
	// The grid is the "widest object" with a width of 20 and depth of 30.0f, and centered at
	// the world space origin.  In general, you need to loop over every world space vertex
	// position and compute the bounding sphere.
	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	//mSceneBounds.Radius = sqrtf(110.0f * 110.0f + 115.0f * 115.0f);

	float scale = 1415.0f;
	mSceneBounds.Radius = sqrtf((10.0f * 10.0f) * scale + (15.0f * 15.0f) * scale);

}

DungeonStompApp::~DungeonStompApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool DungeonStompApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// Reset the command list to prep for initialization commands.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Get the increment size of a descriptor in this heap type.  This is hardware specific, so we have
	// to query this information.
	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mDungeon = std::make_unique<Dungeon>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	mShadowMap = std::make_unique<ShadowMap>(md3dDevice.Get(), 2048, 2048);

	mSsao = std::make_unique<Ssao>(
		md3dDevice.Get(),
		mCommandList.Get(),
		mClientWidth, mClientHeight);

	LoadTextures();
	BuildRootSignature();
	BuildSsaoRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildLandGeometry();
	BuildDungeonGeometryBuffers();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();
	mSsao->SetPSOs(mPSOs["ssao"].Get(), mPSOs["ssaoBlur"].Get());

	InitDS();

	//Set headbob
	bobX.SinWave(4.0f, 2.0f, 2.0f);
	bobY.SinWave(4.0f, 2.0f, 4.0f);

	arialFont = LoadFont(L"Arial.fnt", 800, 600);

	// Execute the initialization commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	//Set the Text Buffer
	textVertexBufferView.BufferLocation = textVertexBuffer->GetGPUVirtualAddress();
	textVertexBufferView.StrideInBytes = sizeof(TextVertex);
	textVertexBufferView.SizeInBytes = maxNumTextCharacters * sizeof(TextVertex);

	//Set the Rectangle Buffer
	for (int i = 0; i < MaxRectangle; ++i)
	{
		rectangleVertexBufferView[i].BufferLocation = rectangleVertexBuffer[i]->GetGPUVirtualAddress();
		rectangleVertexBufferView[i].StrideInBytes = sizeof(TextVertex);
		rectangleVertexBufferView[i].SizeInBytes = maxNumRectangleCharacters * sizeof(TextVertex);
	}
	// Wait until initialization is complete.
	FlushCommandQueue();

#if defined(DEBUG) || defined(_DEBUG) 
#else
	// Maximize window and go fullscreen in release mode.
	{
		ShowWindow(mhMainWnd, SW_MAXIMIZE);
		mSwapChain->SetFullscreenState(1, nullptr);
	}
#endif

	return true;
}

void DungeonStompApp::OnResize()
{
	D3DApp::OnResize();

	// The window resized, so update the aspect ratio and recompute the projection matrix.
	//XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 10000.0f);
	XMStoreFloat4x4(&mProj, P);

	if (mSsao != nullptr)
	{
		mSsao->OnResize(mClientWidth, mClientHeight);

		// Resources changed, so need to rebuild descriptors.
		mSsao->RebuildDescriptors(mDepthStencilBuffer.Get());
	}

}

void DungeonStompApp::Update(const GameTimer& gt)
{
	float t = gt.DeltaTime();
	UpdateControls();
	FrameMove(0.0f, t);
	UpdateWorld(t);
	OnKeyboardInput(gt);

	bobY.update(t);
	bobX.update(t);

	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	//mLightRotationAngle += 0.1f * gt.DeltaTime();
	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateShadowTransform(gt, 0);
	UpdateMainPassCB(gt);
	UpdateSsaoCB(gt);
	UpdateShadowPassCB(gt);

	UpdateDungeon(gt);

}

void DungeonStompApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	ProcessLights11();

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	// Bind null SRV for shadow map pass.
	mCommandList->SetGraphicsRootDescriptorTable(5, mNullSrv);

	//Render shadow map to texture.
	DrawSceneToShadowMap(gt);

	if (enableSSao) {

		drawingSSAO = true;
		// Normal/depth pass.
		DrawNormalsAndDepth(gt);
		drawingSSAO = false;
		// Compute SSAO.
		mCommandList->SetGraphicsRootSignature(mSsaoRootSignature.Get());
		mSsao->ComputeSsao(mCommandList.Get(), mCurrFrameResource, 3);
	}

	// Main rendering pass.

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//if (!enableSSao) {
		// SSAO - WE ALREADY WROTE THE DEPTH INFO TO THE DEPTH BUFFER IN DrawNormalsAndDepth,
		// SO DO NOT CLEAR DEPTH.

		// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);  //Colors::LightSteelBlue
	//}

	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	//Render the main scene
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque], gt);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers (vsync enable)
	//ThrowIfFailed(mSwapChain->Present(0, 0)); //set vsync off
	ThrowIfFailed(mSwapChain->Present(1, 0)); //set vsync on
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
	// Because we are on the GPU timeline, the new fence point won't be 
	// set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void DungeonStompApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	/* mLastMousePos.x = x;
	 mLastMousePos.y = y;

	 SetCapture(mhMainWnd);*/
}

void DungeonStompApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	//ReleaseCapture();
}

void DungeonStompApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	//if((btnState & MK_LBUTTON) != 0)
	//{
	//    // Make each pixel correspond to a quarter of a degree.
	//    float dx = XMConvertToRadians(0.25f*static_cast<float>(x - mLastMousePos.x));
	//    float dy = XMConvertToRadians(0.25f*static_cast<float>(y - mLastMousePos.y));

	//    // Update angles based on input to orbit camera around box.
	//    mTheta += dx;
	//    mPhi += dy;

	//    // Restrict the angle mPhi.
	//    mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f);
	//}
	//else if((btnState & MK_RBUTTON) != 0)
	//{
	//    // Make each pixel correspond to 0.2 unit in the scene.
	//    float dx = 0.2f*static_cast<float>(x - mLastMousePos.x);
	//    float dy = 0.2f*static_cast<float>(y - mLastMousePos.y);

	//    // Update the camera radius based on input.
	//    mRadius += dx - dy;

	//    // Restrict the radius.
	//    mRadius = MathHelper::Clamp(mRadius, 5.0f, 150.0f);
	//}

	//mLastMousePos.x = x;
	//mLastMousePos.y = y;
}

void DungeonStompApp::OnKeyboardInput(const GameTimer& gt)
{
	//rise from the dead
	if (player_list[trueplayernum].bIsPlayerAlive == FALSE) {
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			player_list[trueplayernum].bIsPlayerAlive = TRUE;
			player_list[trueplayernum].health = player_list[trueplayernum].hp;
		}
	}

	if (GetAsyncKeyState('M') && !displayShadowMapKeyPress) {

		if (displayShadowMap)
			displayShadowMap = 0;
		else
			displayShadowMap = 1;
	}

	if (GetAsyncKeyState('M')) {
		displayShadowMapKeyPress = 1;
	}
	else {
		displayShadowMapKeyPress = 0;
	}


	if (GetAsyncKeyState('O') && !enableSSaoKey) {

		if (enableSSao) {
			enableSSao = 0;
			strcpy_s(gActionMessage, "SSAO Disabled");
			UpdateScrollList(0, 255, 255);
		}
		else {
			strcpy_s(gActionMessage, "SSAO Enabled");
			UpdateScrollList(0, 255, 255);
			enableSSao = 1;
		}
	}

	if (GetAsyncKeyState('O')) {
		enableSSaoKey = 1;
	}
	else {
		enableSSaoKey = 0;
	}


	if (GetAsyncKeyState('B') && !enableCameraBobKey) {

		if (enableCameraBob) {
			enableCameraBob = false;
			strcpy_s(gActionMessage, "Camera bob Disabled");
			UpdateScrollList(0, 255, 255);
		}
		else {
			strcpy_s(gActionMessage, "Camera bob Enabled");
			UpdateScrollList(0, 255, 255);
			enableCameraBob = true;
		}
	}

	if (GetAsyncKeyState('B')) {
		enableCameraBobKey = 1;
	}
	else {
		enableCameraBobKey = 0;
	}


}

void DungeonStompApp::UpdateCamera(const GameTimer& gt)
{
	float adjust = 50.0f;
	float bx = 0.0f;
	float by = 0.0f;

	bx = bobX.getY();
	by = bobY.getY();

	if (player_list[trueplayernum].bIsPlayerAlive == FALSE) {
		//Dead on floor
		adjust = 0.0f;
	}

	mEyePos.x = m_vEyePt.x;
	mEyePos.y = m_vEyePt.y + adjust;
	mEyePos.z = m_vEyePt.z;

	player_list[trueplayernum].x = m_vEyePt.x;
	player_list[trueplayernum].y = m_vEyePt.y + adjust;
	player_list[trueplayernum].z = m_vEyePt.z;

	XMVECTOR pos, target;

	XMFLOAT3 newspot;
	XMFLOAT3 newspot2;

	if (enableCameraBob) {
		float step_left_angy = 0;
		float r = 15.0f;

		step_left_angy = angy - 90;

		if (step_left_angy < 0)
			step_left_angy += 360;

		if (step_left_angy >= 360)
			step_left_angy = step_left_angy - 360;

		r = bx;


		if (playercurrentmove == 1 || playercurrentmove == 4) {
			centre = false;
			stopx = false;
			stopy = false;
		}

		if (playercurrentmove == 0) {
			if (!centre) {
				centre = true;
				centrex = bobX.getY();
				centrey = bobY.getY();
			}
		}

		if (centre) {

			//X bob bring to centre
			if (centrex <= 0) {
				if (bobX.getY() >= 0) {
					stopx = true;
				}
			}
			else if (centrex > 0) {
				if (bobX.getY() <= 0) {
					stopx = true;
				}
			}

			//Y bob 
			if (centrey <= 0) {
				if (bobY.getY() >= 0) {
					stopy = true;
				}
			}
			else if (centrey > 0) {
				if (bobY.getY() <= 0) {
					stopy = true;
				}
			}

		}

		if (stopy) {
			by = 0.0f;
			bobY.setX(0);
			bobY.setY(0);
		}

		if (stopx) {
			r = 1.0f;
			bobX.setX(0);
			bobX.setY(0);
		}

		newspot.x = player_list[trueplayernum].x + r * sinf(step_left_angy * k);
		newspot.y = player_list[trueplayernum].y + by;
		newspot.z = player_list[trueplayernum].z + r * cosf(step_left_angy * k);

		float cameradist = 50.0f;

		float newangle = 0;
		newangle = fixangle(look_up_ang, 90);

		newspot2.x = newspot.x + cameradist * sinf(newangle * k) * sinf(angy * k);
		newspot2.y = newspot.y + cameradist * cosf(newangle * k);
		newspot2.z = newspot.z + cameradist * sinf(newangle * k) * cosf(angy * k);


		mEyePos = newspot;

		GunTruesave = newspot;


		// Build the view matrix.

		pos = XMVectorSet(newspot.x, newspot.y, newspot.z, 1.0f);
		target = XMVectorSet(newspot2.x, newspot2.y, newspot2.z, 1.0f);
	}
	else {
		// Build the view matrix.
		pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
		target = XMVectorSet(m_vLookatPt.x, m_vLookatPt.y + adjust, m_vLookatPt.z, 1.0f);

		GunTruesave = mEyePos;
	}



	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//Check for collision and nan errors
	XMVECTOR EyeDirection = XMVectorSubtract(pos, target);
	//assert(!XMVector3Equal(EyeDirection, XMVectorZero()));
	if (XMVector3Equal(EyeDirection, XMVectorZero())) {
		return;
	}

	//XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

	XMStoreFloat4x4(&mView, view);

	mSceneBounds.Center = XMFLOAT3(mEyePos.x, mEyePos.y, mEyePos.z);

}

void DungeonStompApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			//objConstants.MaterialIndex = e->Mat->MatCBIndex;

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}

}

void DungeonStompApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			//matConstants.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
			//matConstants.NormalMapIndex = mat->NormalSrvHeapIndex;

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}

void DungeonStompApp::UpdateShadowTransform(const GameTimer& gt, int light)
{
	// Only the first "main" light casts a shadow.
	XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
	XMVECTOR lightPos = -2.0f * mSceneBounds.Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

	XMStoreFloat3(&mLightPosW, lightPos);

	// Transform bounding sphere to light space.
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));

	// Ortho frustum in light space encloses scene.
	float l = sphereCenterLS.x - mSceneBounds.Radius;
	float b = sphereCenterLS.y - mSceneBounds.Radius;
	float n = sphereCenterLS.z - mSceneBounds.Radius;
	float r = sphereCenterLS.x + mSceneBounds.Radius;
	float t = sphereCenterLS.y + mSceneBounds.Radius;
	float f = sphereCenterLS.z + mSceneBounds.Radius;

	mLightNearZ = n;
	mLightFarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX S = lightView * lightProj * T;
	XMStoreFloat4x4(&mLightView, lightView);
	XMStoreFloat4x4(&mLightProj, lightProj);
	XMStoreFloat4x4(&mShadowTransform, S);
}

void DungeonStompApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);

	XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
	XMStoreFloat4x4(&mMainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));

	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();

	//mMainPassCB.AmbientLight = { 0.1f, 0.1f, 0.1f, 1.0f };
	//mMainPassCB.AmbientLight = { 1.00f, 1.00f, 1.00f, 1.00f };
	//mMainPassCB.AmbientLight = { 0.00f, 0.00f, 0.00f, 1.00f };
	mMainPassCB.AmbientLight = { 0.1f, 0.1f, 0.15f, 1.0f };

	//XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);
	//XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	//mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };

	for (int i = 0; i < MaxLights; i++) {
		mMainPassCB.Lights[i].Direction = LightContainer[i].Direction;
		mMainPassCB.Lights[i].Strength = LightContainer[i].Strength;
		mMainPassCB.Lights[i].Position = LightContainer[i].Position;
		mMainPassCB.Lights[i].FalloffEnd = LightContainer[i].FalloffEnd;
		mMainPassCB.Lights[i].FalloffStart = LightContainer[i].FalloffStart;
		mMainPassCB.Lights[i].SpotPower = LightContainer[i].SpotPower;
	}

	//mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	mMainPassCB.Lights[0].Direction = mRotatedLightDirections[0];
	//mMainPassCB.Lights[0].Strength = { 0.4f, 0.4f, 0.4f };
	//mMainPassCB.Lights[0].Strength = { 0.9f, 0.8f, 0.7f };
	//mMainPassCB.Lights[1].Direction = mRotatedLightDirections[1];
	//mMainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	//mMainPassCB.Lights[2].Direction = mRotatedLightDirections[2];
	//mMainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void DungeonStompApp::UpdateShadowPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mLightView);
	XMMATRIX proj = XMLoadFloat4x4(&mLightProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	UINT w = mShadowMap->Width();
	UINT h = mShadowMap->Height();

	XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mShadowPassCB.EyePosW = mLightPosW;
	mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
	mShadowPassCB.NearZ = mLightNearZ;
	mShadowPassCB.FarZ = mLightFarZ;

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mShadowPassCB);
}

void DungeonStompApp::UpdateSsaoCB(const GameTimer& gt)
{
	SsaoConstants ssaoCB;

	//XMMATRIX P = mCamera.GetProj();

	XMMATRIX P = XMLoadFloat4x4(&mProj);

	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	ssaoCB.Proj = mMainPassCB.Proj;
	ssaoCB.InvProj = mMainPassCB.InvProj;
	XMStoreFloat4x4(&ssaoCB.ProjTex, XMMatrixTranspose(P * T));

	mSsao->GetOffsetVectors(ssaoCB.OffsetVectors);

	auto blurWeights = mSsao->CalcGaussWeights(2.5f);
	ssaoCB.BlurWeights[0] = XMFLOAT4(&blurWeights[0]);
	ssaoCB.BlurWeights[1] = XMFLOAT4(&blurWeights[4]);
	ssaoCB.BlurWeights[2] = XMFLOAT4(&blurWeights[8]);

	ssaoCB.InvRenderTargetSize = XMFLOAT2(1.0f / mSsao->SsaoMapWidth(), 1.0f / mSsao->SsaoMapHeight());

	// Coordinates given in view space.
	ssaoCB.OcclusionRadius = 10.5f;
	ssaoCB.OcclusionFadeStart = 0.3f;
	ssaoCB.OcclusionFadeEnd = 2.0f;
	ssaoCB.SurfaceEpsilon = 0.05f;

	auto currSsaoCB = mCurrFrameResource->SsaoCB.get();
	currSsaoCB->CopyData(0, ssaoCB);
}

void DungeonStompApp::UpdateDungeon(const GameTimer& gt)
{

	DisplayPlayerCaption();

	// Update the dungeon vertex buffer with the new solution.
	auto currDungeonVB = mCurrFrameResource->DungeonVB.get();
	Vertex v;

	for (int j = 0; j < cnt; j++)
	{
		v.Pos.x = src_v[j].x;
		v.Pos.y = src_v[j].y;
		v.Pos.z = src_v[j].z;

		v.Normal.x = src_v[j].nx;
		v.Normal.y = src_v[j].ny;
		v.Normal.z = src_v[j].nz;

		v.TexC.x = src_v[j].tu;
		v.TexC.y = src_v[j].tv;

		//v.TangentU = XMFLOAT3(0.0f, 1.0f, 0.0f);

		v.TangentU.x = src_v[j].nmx;
		v.TangentU.y = src_v[j].nmy;
		v.TangentU.z = src_v[j].nmz;


		currDungeonVB->CopyData(j, v);
	}

	// Set the dynamic VB of the dungeon renderitem to the current frame VB.
	mDungeonRitem->Geo->VertexBufferGPU = currDungeonVB->Resource();
}

void DungeonStompApp::BuildRootSignature()
{

	const int rootItems = 8;

	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable2;
	texTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable3;
	texTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 3, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable4;
	texTable4.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[rootItems];

	// Create root CBV.
	slotRootParameter[0].InitAsConstantBufferView(0);  //cbuffer cbPerObject : register(b0)
	slotRootParameter[1].InitAsConstantBufferView(1);  //cbuffer cbMaterial : register(b1)
	slotRootParameter[2].InitAsConstantBufferView(2);  //cbuffer cbPass : register(b2) X 2 locations
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);  //Texture2D    gDiffuseMap : register(t0);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);  //Texture2D    gNormalMap  : register(t1);
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable2, D3D12_SHADER_VISIBILITY_PIXEL);  //Texture2D    gShadowMap  : register(t2);
	slotRootParameter[6].InitAsDescriptorTable(1, &texTable3, D3D12_SHADER_VISIBILITY_PIXEL);  //TextureCube  gCubeMap    : register(t4);
	slotRootParameter[7].InitAsDescriptorTable(1, &texTable4, D3D12_SHADER_VISIBILITY_PIXEL);  //Texture2D	 gSsaoMap    : register(t5);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(rootItems, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void DungeonStompApp::BuildSsaoRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstants(1, 1);
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers =
	{
		pointClamp, linearClamp, depthMapSam, linearWrap
	};

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mSsaoRootSignature.GetAddressOf())));
}

void DungeonStompApp::DrawSceneToShadowMap(const GameTimer& gt)
{
	mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());

	// Change to DEPTH_WRITE.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearDepthStencilView(mShadowMap->Dsv(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Set null render target because we are only going to draw to
	// depth buffer.  Setting a null render target will disable color writes.
	// Note the active PSO also must specify a render target count of 0.
	mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());

	// Bind the pass constant buffer for the shadow map pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

	mCommandList->SetPipelineState(mPSOs["shadow_opaque"].Get());

	drawingShadowMap = true;
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque], gt);
	drawingShadowMap = false;
	//DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void DungeonStompApp::DrawNormalsAndDepth(const GameTimer& gt)
{
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	auto normalMap = mSsao->NormalMap();
	auto normalMapRtv = mSsao->NormalMapRtv();

	// Change to RENDER_TARGET.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the screen normal map and depth buffer.
	float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	mCommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());

	// Bind the constant buffer for this pass.
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	mCommandList->SetPipelineState(mPSOs["drawNormals"].Get());

	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque], gt);

	// Change back to GENERIC_READ so we can read the texture in a shader.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> DungeonStompApp::GetStaticSamplers()
{
	// Applications usually only need a handful of samplers.  So just define them all up front
	// and keep them available as part of the root signature.  

	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC shadow(
		6, // shaderRegister
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                               // mipLODBias
		16,                                 // maxAnisotropy
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp,
		shadow
	};
}

void DungeonStompApp::BuildShadersAndInputLayout()
{

	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO sasoDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		"SSAO", "1",
		NULL, NULL
	};


	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO torchTestDefines[] =
	{
		"TORCH_TEST", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["opaqueSsaoPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", sasoDefines, "PS", "ps_5_1");

	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	mShaders["torchPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", torchTestDefines, "PS", "ps_5_1");

	mShaders["normalMapVS"] = d3dUtil::CompileShader(L"..\\Shaders\\NormalMap.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["normalMapPS"] = d3dUtil::CompileShader(L"..\\Shaders\\NormalMap.hlsl", defines, "PS", "ps_5_1");

	mShaders["normalMapSsaoPS"] = d3dUtil::CompileShader(L"..\\Shaders\\NormalMap.hlsl", sasoDefines, "PS", "ps_5_1");

	mShaders["shadowVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"..\\Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");

	mShaders["skyVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["drawNormalsVS"] = d3dUtil::CompileShader(L"..\\Shaders\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["drawNormalsPS"] = d3dUtil::CompileShader(L"..\\Shaders\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ssaoVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["ssaoBlurVS"] = d3dUtil::CompileShader(L"..\\Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoBlurPS"] = d3dUtil::CompileShader(L"..\\Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");


	// Text PSO
	ID3DBlob* errorBuff; // a buffer holding the error data if any
	// compile vertex shader
	ID3DBlob* textVertexShader; // d3d blob for holding vertex shader bytecode
	HRESULT hr = D3DCompileFromFile(L"..\\Shaders\\TextVertexShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&textVertexShader,
		&errorBuff);

	// fill out a shader bytecode structure, which is basically just a pointer
	// to the shader bytecode and the size of the shader bytecode
	D3D12_SHADER_BYTECODE textVertexShaderBytecode = {};
	textVertexShaderBytecode.BytecodeLength = textVertexShader->GetBufferSize();
	textVertexShaderBytecode.pShaderBytecode = textVertexShader->GetBufferPointer();

	// compile pixel shader
	ID3DBlob* textPixelShader;
	hr = D3DCompileFromFile(L"..\\Shaders\\TextPixelShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&textPixelShader,
		&errorBuff);

	// fill out shader bytecode structure for pixel shader
	D3D12_SHADER_BYTECODE textPixelShaderBytecode = {};
	textPixelShaderBytecode.BytecodeLength = textPixelShader->GetBufferSize();
	textPixelShaderBytecode.pShaderBytecode = textPixelShader->GetBufferPointer();

	// Rectangle PSO
	// compile vertex shader
	ID3DBlob* rectangleVertexShader; // d3d blob for holding vertex shader bytecode
	hr = D3DCompileFromFile(L"..\\Shaders\\RectangleVertexShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"vs_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&rectangleVertexShader,
		&errorBuff);

	// fill out a shader bytecode structure, which is basically just a pointer
	// to the shader bytecode and the size of the shader bytecode
	D3D12_SHADER_BYTECODE rectangleVertexShaderBytecode = {};
	rectangleVertexShaderBytecode.BytecodeLength = rectangleVertexShader->GetBufferSize();
	rectangleVertexShaderBytecode.pShaderBytecode = rectangleVertexShader->GetBufferPointer();

	// compile pixel shader
	ID3DBlob* rectanglePixelShader;
	hr = D3DCompileFromFile(L"..\\Shaders\\RectanglePixelShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&rectanglePixelShader,
		&errorBuff);

	// fill out shader bytecode structure for pixel shader
	D3D12_SHADER_BYTECODE rectanglePixelShaderBytecode = {};
	rectanglePixelShaderBytecode.BytecodeLength = rectanglePixelShader->GetBufferSize();
	rectanglePixelShaderBytecode.pShaderBytecode = rectanglePixelShader->GetBufferPointer();

	// compile pixel shader
	ID3DBlob* rectanglePixelMapShader;
	hr = D3DCompileFromFile(L"..\\Shaders\\rectanglePixelMapShader.hlsl",
		nullptr,
		nullptr,
		"main",
		"ps_5_0",
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
		0,
		&rectanglePixelMapShader,
		&errorBuff);

	// fill out shader bytecode structure for pixel shader
	D3D12_SHADER_BYTECODE rectanglePixelMapShaderBytecode = {};
	rectanglePixelMapShaderBytecode.BytecodeLength = rectanglePixelMapShader->GetBufferSize();
	rectanglePixelMapShaderBytecode.pShaderBytecode = rectanglePixelMapShader->GetBufferPointer();



	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_ELEMENT_DESC textInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};

	// fill out an input layout description structure
	D3D12_INPUT_LAYOUT_DESC textInputLayoutDesc = {};

	// we can get the number of elements in an array by "sizeof(array) / sizeof(arrayElementType)"
	textInputLayoutDesc.NumElements = sizeof(textInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	textInputLayoutDesc.pInputElementDescs = textInputLayout;

	// create the text pipeline state object (PSO)

	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1; // multisample count (no multisampling, so we just put 1, since we still need 1 sample)

	D3D12_GRAPHICS_PIPELINE_STATE_DESC textpsoDesc = {};
	textpsoDesc.InputLayout = textInputLayoutDesc;
	textpsoDesc.pRootSignature = mRootSignature.Get();
	textpsoDesc.VS = textVertexShaderBytecode;
	textpsoDesc.PS = textPixelShaderBytecode;
	textpsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	textpsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	textpsoDesc.SampleDesc = sampleDesc;
	textpsoDesc.SampleMask = 0xffffffff;
	textpsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	D3D12_BLEND_DESC textBlendStateDesc = {};
	textBlendStateDesc.AlphaToCoverageEnable = FALSE;
	textBlendStateDesc.IndependentBlendEnable = FALSE;
	textBlendStateDesc.RenderTarget[0].BlendEnable = TRUE;

	textBlendStateDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	textBlendStateDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	textBlendStateDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

	textBlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	textBlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	textBlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

	textBlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	textpsoDesc.BlendState = textBlendStateDesc;
	textpsoDesc.NumRenderTargets = 1;
	D3D12_DEPTH_STENCIL_DESC textDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	textDepthStencilDesc.DepthEnable = false;
	textpsoDesc.DepthStencilState = textDepthStencilDesc;

	// create the text pso
	hr = md3dDevice->CreateGraphicsPipelineState(&textpsoDesc, IID_PPV_ARGS(&textPSO));

	//create the rectangles for HUD
	for (int i = 0; i < MaxRectangle; i++) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC rectanglepsoDesc = {};
		rectanglepsoDesc.InputLayout = textInputLayoutDesc;
		rectanglepsoDesc.pRootSignature = mRootSignature.Get();
		rectanglepsoDesc.VS = rectangleVertexShaderBytecode;
		rectanglepsoDesc.PS = rectanglePixelShaderBytecode;
		rectanglepsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		rectanglepsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		rectanglepsoDesc.SampleDesc = sampleDesc;
		rectanglepsoDesc.SampleMask = 0xffffffff;
		rectanglepsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

		D3D12_BLEND_DESC rectangleBlendStateDesc = {};
		rectangleBlendStateDesc.AlphaToCoverageEnable = FALSE;
		rectangleBlendStateDesc.IndependentBlendEnable = FALSE;
		rectangleBlendStateDesc.RenderTarget[0].BlendEnable = TRUE;

		if (i == MaxRectangle - 1) {
			//shadowmap/ssoa texture - make it not transparent
			rectangleBlendStateDesc.RenderTarget[0].BlendEnable = FALSE;
			rectanglepsoDesc.PS = rectanglePixelMapShaderBytecode;
		}


		//if (i == MaxRectangle - 2 || i == MaxRectangle - 3) {
		//	//make the dice not transparent
		//	rectangleBlendStateDesc.RenderTarget[0].BlendEnable = FALSE;
		//}

		rectangleBlendStateDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rectangleBlendStateDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		rectangleBlendStateDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;

		if (i == 0) {
			rectangleBlendStateDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_COLOR;
			rectangleBlendStateDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
			rectangleBlendStateDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		}



		rectangleBlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		rectangleBlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		rectangleBlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;

		rectangleBlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		rectanglepsoDesc.BlendState = rectangleBlendStateDesc;
		rectanglepsoDesc.NumRenderTargets = 1;
		D3D12_DEPTH_STENCIL_DESC rectangleDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		rectangleDepthStencilDesc.DepthEnable = false;
		rectanglepsoDesc.DepthStencilState = rectangleDepthStencilDesc;

		// create the rectangle pso
		hr = md3dDevice->CreateGraphicsPipelineState(&rectanglepsoDesc, IID_PPV_ARGS(&rectanglePSO[i]));
	}
}

void DungeonStompApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateSphere(0.5f, 20, 20);

	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		//auto& p = grid.Vertices[i].Position;
		//vertices[i].Pos = p;
		//vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		//vertices[i].Normal = GetHillsNormal(p.x, p.z);
		vertices[i].Pos = grid.Vertices[i].Position;
		vertices[i].Normal = grid.Vertices[i].Normal;
		vertices[i].TexC = grid.Vertices[i].TexC;
	}


	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["landGeo"] = std::move(geo);
}

void DungeonStompApp::BuildDungeonGeometryBuffers()
{
	std::vector<std::uint16_t> indices(3 * mDungeon->TriangleCount()); // 3 indices per face
	assert(mDungeon->VertexCount() < 0x0000ffff);

	//// Iterate over each quad.
	//int m = mDungeon->RowCount();
	//int n = mDungeon->ColumnCount();
	//int k = 0;
	//for (int i = 0; i < m - 1; ++i)
	//{
	//	for (int j = 0; j < n - 1; ++j)
	//	{
	//		indices[k] = i * n + j;
	//		indices[k + 1] = i * n + j + 1;
	//		indices[k + 2] = (i + 1) * n + j;

	//		indices[k + 3] = (i + 1) * n + j;
	//		indices[k + 4] = i * n + j + 1;
	//		indices[k + 5] = (i + 1) * n + j + 1;

	//		k += 6; // next quad
	//	}
	//}

	//UINT vbByteSize = mDungeon->VertexCount() * sizeof(Vertex);
	UINT vbByteSize = MAX_NUM_QUADS * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";

	// Set dynamically.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(),
		mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	mGeometries["waterGeo"] = std::move(geo);
}

void DungeonStompApp::DrawRenderItemsFL(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();



	CD3DX12_GPU_DESCRIPTOR_HANDLE tex3(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	tex3.Offset(484, mCbvSrvDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(6, tex3); //Set the gCubeMap


	//auto ri = ritems[1];

	// For each render item...
	//for (size_t i = 0; i < ritems.size(); ++i)
	//{
	auto ri = ritems[1];

	cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
	cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
	cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;

	UINT materialIndex = mMaterials["flat"].get()->MatCBIndex;


	D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + materialIndex * matCBByteSize;


	cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
	cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);


	cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	//}
}

void DungeonStompApp::BuildPSOs()
{

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;

	//opaquePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_EQUAL;
	//opaquePsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueSsaoPsoDesc;

	//
	// PSO for opaqueSsao objects.
	//
	ZeroMemory(&opaqueSsaoPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaqueSsaoPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaqueSsaoPsoDesc.pRootSignature = mRootSignature.Get();
	opaqueSsaoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaqueSsaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaqueSsaoPS"]->GetBufferPointer()),
		mShaders["opaqueSsaoPS"]->GetBufferSize()
	};
	opaqueSsaoPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaqueSsaoPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaqueSsaoPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaqueSsaoPsoDesc.SampleMask = UINT_MAX;
	opaqueSsaoPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaqueSsaoPsoDesc.NumRenderTargets = 1;
	opaqueSsaoPsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaqueSsaoPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaqueSsaoPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaqueSsaoPsoDesc.DSVFormat = mDepthStencilFormat;


	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueSsaoPsoDesc, IID_PPV_ARGS(&mPSOs["opaqueSsao"])));



	//
	// PSO for shadow map pass.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.pRootSignature = mRootSignature.Get();
	smapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()),
		mShaders["shadowVS"]->GetBufferSize()
	};
	smapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["shadowOpaquePS"]->GetBufferPointer()),
		mShaders["shadowOpaquePS"]->GetBufferSize()
	};

	// Shadow map pass does not have a render target.
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["shadow_opaque"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalMapPsoDesc;

	//
	// PSO for normalMap objects.
	//
	ZeroMemory(&normalMapPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	normalMapPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	normalMapPsoDesc.pRootSignature = mRootSignature.Get();
	normalMapPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["normalMapVS"]->GetBufferPointer()),
		mShaders["normalMapVS"]->GetBufferSize()
	};
	normalMapPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["normalMapPS"]->GetBufferPointer()),
		mShaders["normalMapPS"]->GetBufferSize()
	};
	normalMapPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	normalMapPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	normalMapPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	normalMapPsoDesc.SampleMask = UINT_MAX;
	normalMapPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	normalMapPsoDesc.NumRenderTargets = 1;
	normalMapPsoDesc.RTVFormats[0] = mBackBufferFormat;
	normalMapPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	normalMapPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	normalMapPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&normalMapPsoDesc, IID_PPV_ARGS(&mPSOs["normalMap"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalMapSsaoPSoDesc;

	//
	// PSO for normalSasoMap objects.
	//
	ZeroMemory(&normalMapSsaoPSoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	normalMapSsaoPSoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	normalMapSsaoPSoDesc.pRootSignature = mRootSignature.Get();
	normalMapSsaoPSoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["normalMapVS"]->GetBufferPointer()),
		mShaders["normalMapVS"]->GetBufferSize()
	};
	normalMapSsaoPSoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["normalMapSsaoPS"]->GetBufferPointer()),
		mShaders["normalMapSsaoPS"]->GetBufferSize()
	};
	normalMapSsaoPSoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	normalMapSsaoPSoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	normalMapSsaoPSoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	normalMapSsaoPSoDesc.SampleMask = UINT_MAX;
	normalMapSsaoPSoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	normalMapSsaoPSoDesc.NumRenderTargets = 1;
	normalMapSsaoPSoDesc.RTVFormats[0] = mBackBufferFormat;
	normalMapSsaoPSoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	normalMapSsaoPSoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	normalMapSsaoPSoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&normalMapSsaoPSoDesc, IID_PPV_ARGS(&mPSOs["normalMapSsao"])));


	//
	// PSO for transparent objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_COLOR;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_BLEND_FACTOR;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	//
	// PSO for alpha tested objects
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()),
		mShaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));


	D3D12_GRAPHICS_PIPELINE_STATE_DESC torchPsoDesc = opaquePsoDesc;

	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_COLOR;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_BLEND_FACTOR;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	torchPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;

	torchPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["torchPS"]->GetBufferPointer()),
		mShaders["torchPS"]->GetBufferSize()
	};
	torchPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&torchPsoDesc, IID_PPV_ARGS(&mPSOs["torchTested"])));


	//
	// PSO for sky.
	//

	D3D12_GRAPHICS_PIPELINE_STATE_DESC cubePsoDesc;
	ZeroMemory(&cubePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	cubePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	cubePsoDesc.pRootSignature = mRootSignature.Get();
	cubePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	cubePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	cubePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	cubePsoDesc.SampleMask = UINT_MAX;
	cubePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	cubePsoDesc.NumRenderTargets = 1;
	cubePsoDesc.RTVFormats[0] = mBackBufferFormat;
	cubePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	cubePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	cubePsoDesc.DSVFormat = mDepthStencilFormat;


	// The camera is inside the sky sphere, so just turn off culling.
	cubePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	cubePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	cubePsoDesc.pRootSignature = mRootSignature.Get();
	cubePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()),
		mShaders["skyVS"]->GetBufferSize()
	};
	cubePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()),
		mShaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&cubePsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));



	D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;


	ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	basePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	basePsoDesc.pRootSignature = mRootSignature.Get();

	basePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	basePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	basePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	basePsoDesc.SampleMask = UINT_MAX;
	basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	basePsoDesc.NumRenderTargets = 1;
	basePsoDesc.RTVFormats[0] = mBackBufferFormat;
	basePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	basePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	basePsoDesc.DSVFormat = mDepthStencilFormat;

	//
  // PSO for drawing normals.
  //
	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawNormalsPsoDesc = basePsoDesc;
	drawNormalsPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["drawNormalsVS"]->GetBufferPointer()),
		mShaders["drawNormalsVS"]->GetBufferSize()
	};
	drawNormalsPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["drawNormalsPS"]->GetBufferPointer()),
		mShaders["drawNormalsPS"]->GetBufferSize()
	};
	drawNormalsPsoDesc.RTVFormats[0] = Ssao::NormalMapFormat;
	drawNormalsPsoDesc.SampleDesc.Count = 1;
	drawNormalsPsoDesc.SampleDesc.Quality = 0;
	drawNormalsPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawNormalsPsoDesc, IID_PPV_ARGS(&mPSOs["drawNormals"])));

	//
	// PSO for SSAO.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = basePsoDesc;
	ssaoPsoDesc.InputLayout = { nullptr, 0 };
	ssaoPsoDesc.pRootSignature = mSsaoRootSignature.Get();
	ssaoPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoVS"]->GetBufferPointer()),
		mShaders["ssaoVS"]->GetBufferSize()
	};
	ssaoPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoPS"]->GetBufferPointer()),
		mShaders["ssaoPS"]->GetBufferSize()
	};

	// SSAO effect does not need the depth buffer.
	ssaoPsoDesc.DepthStencilState.DepthEnable = false;
	ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPsoDesc.RTVFormats[0] = Ssao::AmbientMapFormat;
	ssaoPsoDesc.SampleDesc.Count = 1;
	ssaoPsoDesc.SampleDesc.Quality = 0;
	ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&mPSOs["ssao"])));

	//
	// PSO for SSAO blur.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPsoDesc = ssaoPsoDesc;
	ssaoBlurPsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoBlurVS"]->GetBufferPointer()),
		mShaders["ssaoBlurVS"]->GetBufferSize()
	};
	ssaoBlurPsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["ssaoBlurPS"]->GetBufferPointer()),
		mShaders["ssaoBlurPS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoBlurPsoDesc, IID_PPV_ARGS(&mPSOs["ssaoBlur"])));





}

void DungeonStompApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			2, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mDungeon->VertexCount()));
	}
}

void DungeonStompApp::BuildMaterials()
{
	//Water (0.02f, 0.02f, 0.02f);
	//Glass (0.08f, 0.08f, 0.08f);
	//Plastic (0.05f, 0.05f, 0.05f);
	//Gold (1.0f, 0.71f, 0.29f);
	//Silver (0.95f, 0.73f, 0.88f);
	//Copper (0.95f, 0.64f, 0.54f);
	//383 = tile
	//384 = tile normal


	auto default = std::make_unique<Material>();
	default->Name = "default";
	default->MatCBIndex = 0;
	default->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	default->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	default->Roughness = 0.815f;

	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 1;
	grass->DiffuseAlbedo = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.925f;

	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 2;
	water->DiffuseSrvHeapIndex = 0;
	water->DiffuseAlbedo = XMFLOAT4(0.5f, 0.5f, 1.0f, 0.5f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.326f;

	auto brick = std::make_unique<Material>();
	brick->Name = "brick";
	brick->MatCBIndex = 3;
	brick->DiffuseSrvHeapIndex = 0;
	brick->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	brick->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	brick->Roughness = 0.825f;

	auto stone = std::make_unique<Material>();
	stone->Name = "stone";
	stone->MatCBIndex = 4;
	stone->DiffuseSrvHeapIndex = 0;
	stone->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stone->FresnelR0 = XMFLOAT3(0.03f, 0.03f, 0.03f);
	stone->Roughness = 0.743f;

	auto tile = std::make_unique<Material>();
	tile->Name = "tile";
	tile->MatCBIndex = 5;
	tile->DiffuseSrvHeapIndex = 0;
	tile->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tile->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tile->Roughness = 0.32f;

	auto crate = std::make_unique<Material>();
	crate->Name = "crate";
	crate->MatCBIndex = 6;
	crate->DiffuseSrvHeapIndex = 0;
	crate->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	crate->FresnelR0 = XMFLOAT3(0.03f, 0.03f, 0.03f);
	crate->Roughness = 0.796f;

	auto ice = std::make_unique<Material>();
	ice->Name = "ice";
	ice->MatCBIndex = 7;
	ice->DiffuseSrvHeapIndex = 0;
	ice->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	ice->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	ice->Roughness = 0.415f;

	auto bone = std::make_unique<Material>();
	bone->Name = "bone";
	bone->MatCBIndex = 8;
	bone->DiffuseSrvHeapIndex = 0;
	bone->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	bone->FresnelR0 = XMFLOAT3(0.09f, 0.09f, 0.09f);
	bone->Roughness = 0.438f;

	auto metal = std::make_unique<Material>();
	metal->Name = "metal";
	metal->MatCBIndex = 9;
	metal->DiffuseSrvHeapIndex = 0;
	metal->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	metal->FresnelR0 = XMFLOAT3(0.12f, 0.12f, 0.12f);
	metal->Roughness = 0.314f;

	auto glass = std::make_unique<Material>();
	glass->Name = "glass";
	glass->MatCBIndex = 10;
	glass->DiffuseSrvHeapIndex = 0;
	glass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	glass->FresnelR0 = XMFLOAT3(0.06f, 0.06f, 0.06f);
	glass->Roughness = 0.224f;

	auto wood = std::make_unique<Material>();
	wood->Name = "wood";
	wood->MatCBIndex = 11;
	wood->DiffuseSrvHeapIndex = 0;
	wood->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	wood->FresnelR0 = XMFLOAT3(0.04f, 0.04f, 0.04f);
	wood->Roughness = 0.938f;

	auto flat = std::make_unique<Material>();
	flat->Name = "flat";
	flat->MatCBIndex = 12;
	flat->DiffuseSrvHeapIndex = 0;
	flat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	flat->FresnelR0 = XMFLOAT3(0.0f, 0.0f, 0.0f);
	flat->Roughness = 0.932f;

	auto tilebrown = std::make_unique<Material>();
	tilebrown->Name = "tilebrown";
	tilebrown->MatCBIndex = 13;
	tilebrown->DiffuseSrvHeapIndex = 0;
	tilebrown->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tilebrown->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tilebrown->Roughness = 0.324f;


	auto monster = std::make_unique<Material>();
	monster->Name = "monster";
	monster->MatCBIndex = 14;
	monster->DiffuseSrvHeapIndex = 0;
	monster->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	monster->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	monster->Roughness = 0.833f;


	auto monsterweapon = std::make_unique<Material>();
	monsterweapon->Name = "monsterweapon";
	monsterweapon->MatCBIndex = 15;
	monsterweapon->DiffuseSrvHeapIndex = 0;
	monsterweapon->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	monsterweapon->FresnelR0 = XMFLOAT3(0.06f, 0.06f, 0.06f);
	monsterweapon->Roughness = 0.702f;

	auto playerweapon = std::make_unique<Material>();
	playerweapon->Name = "playerweapon";
	playerweapon->MatCBIndex = 16;
	playerweapon->DiffuseSrvHeapIndex = 0;
	playerweapon->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	playerweapon->FresnelR0 = XMFLOAT3(0.07f, 0.07f, 0.07f);
	playerweapon->Roughness = 0.742f;

	//new material - increment MatCBIndex 

	mMaterials["default"] = std::move(default);
	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);

	mMaterials["brick"] = std::move(brick);
	mMaterials["stone"] = std::move(stone);
	mMaterials["tile"] = std::move(tile);

	mMaterials["crate"] = std::move(crate);
	mMaterials["ice"] = std::move(ice);
	mMaterials["bone"] = std::move(bone);
	mMaterials["metal"] = std::move(metal);

	mMaterials["glass"] = std::move(glass);
	mMaterials["wood"] = std::move(wood);
	mMaterials["flat"] = std::move(flat);
	
	mMaterials["tilebrown"] = std::move(tilebrown);
	mMaterials["monster"] = std::move(monster);
	mMaterials["monsterweapon"] = std::move(monsterweapon);
	
	mMaterials["playerweapon"] = std::move(playerweapon);

}

void DungeonStompApp::BuildRenderItems()
{
	auto dungeonRitem = std::make_unique<RenderItem>();


	dungeonRitem->World = MathHelper::Identity4x4();
	dungeonRitem->ObjCBIndex = 0;
	dungeonRitem->Mat = mMaterials["water"].get();
	dungeonRitem->Geo = mGeometries["waterGeo"].get();
	dungeonRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	dungeonRitem->IndexCount = dungeonRitem->Geo->DrawArgs["grid"].IndexCount;
	dungeonRitem->StartIndexLocation = dungeonRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	dungeonRitem->BaseVertexLocation = dungeonRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mDungeonRitem = dungeonRitem.get();

	mRitemLayer[(int)RenderLayer::Opaque].push_back(dungeonRitem.get());

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();


	//XMStoreFloat4x4(&gridRitem->World, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));


	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	mAllRitems.push_back(std::move(dungeonRitem));
	mAllRitems.push_back(std::move(gridRitem));
}

void DungeonStompApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, const GameTimer& gt)
{

	auto ri = ritems[0];

	cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
	cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
	cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	tex.Offset(1, mCbvSrvDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(3, tex);

	bool enablePSO = true;

	if (drawingShadowMap || drawingSSAO) {
		enablePSO = false;
	}

	if (enablePSO) {
		if (enableSSao) {
			mCommandList->SetPipelineState(mPSOs["normalMapSsao"].Get());
		}
		else {
			mCommandList->SetPipelineState(mPSOs["normalMap"].Get());
		}
	}
	//Draw dungeon, monsters and items with normal maps
	DrawDungeon(cmdList, ritems, false, false, true);

	if (enablePSO) {
		if (enableSSao) {
			mCommandList->SetPipelineState(mPSOs["opaqueSsao"].Get());
		}
		else {
			mCommandList->SetPipelineState(mPSOs["opaque"].Get());
		}
	}
	//Draw dungeon, monsters and items without normal maps
	DrawDungeon(cmdList, ritems, false, false, false);

	if (enablePSO) {
		mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	}
	//Draw alpha transparent items
	DrawDungeon(cmdList, ritems, true);


	if (enablePSO) {
		mCommandList->SetPipelineState(mPSOs["torchTested"].Get());
	}
	//Draw the torches and effects
	DrawDungeon(cmdList, ritems, true, true);

	if (enablePSO) {

		tex.Offset(377, mCbvSrvDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(3, tex);

		cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		//cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

		for (int i = 0; i < displayCapture; i++) {
			for (int j = 0; j < displayCaptureCount[i]; j++) {
				cmdList->DrawInstanced(4, 1, displayCaptureIndex[i] + (j * 4), 0);
			}
		}


		//Draw the skybox
		if (!gravityon || outside) {
			mCommandList->SetPipelineState(mPSOs["sky"].Get());
			DrawRenderItemsFL(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
		}

		DisplayHud();
		SetDungeonText();

		ScanMod(gt.DeltaTime());
	}

	return;
}

extern bool ObjectHasShadow(int object_id);

void DungeonStompApp::DrawDungeon(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, BOOL isAlpha, bool isTorch, bool normalMap) {

	auto ri = ritems[0];

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();

	bool draw = true;

	int currentObject = 0;
	for (currentObject = 0; currentObject < number_of_polys_per_frame; currentObject++)
	{
		int i = ObjectsToDraw[currentObject].vert_index;
		int vert_index = ObjectsToDraw[currentObject].srcstart;
		int fperpoly = ObjectsToDraw[currentObject].srcfstart;
		int face_index = ObjectsToDraw[currentObject].srcfstart;

		int texture_alias_number = texture_list_buffer[i];
		int texture_number = TexMap[texture_alias_number].texture;

		int normal_map_texture = TexMap[texture_alias_number].normalmaptextureid;


		if (texture_alias_number == 104) {
			TexMap[texture_alias_number].is_alpha_texture = 1;
		}


		draw = true;

		if (isAlpha) {
			if (texture_number >= 94 && texture_number <= 101 ||
				texture_number >= 289 - 1 && texture_number <= 296 - 1 ||
				texture_number >= 279 - 1 && texture_number <= 288 - 1 ||
				texture_number >= 206 - 1 && texture_number <= 210 - 1 ||
				texture_number == 378) {

				if (isAlpha && !isTorch) {
					draw = false;
				}

				if (isAlpha && isTorch) {
					draw = true;
				}
			}
		}



		if (normal_map_texture == -1 && normalMap) {
			draw = false;
		}

		if (!normalMap && normal_map_texture != -1) {
			draw = false;
		}



		int oid = 0;


		if (drawingSSAO || drawingShadowMap) {
			oid = ObjectsToDraw[currentObject].objectId;

			//Don't draw player captions
			if (oid == -99) {
				draw = false;
			}
		}

		if (drawingShadowMap) {

			if (oid == -1) {
				//Draw 3DS and MD2 Shadows
				draw = true;
			}
			else {
				draw = false;
			}

			if (oid > 0) {
				//Draw objects.dat that have SHADOW attribute set to 1
				if (ObjectHasShadow(oid)) {
					draw = true;
				}
			}
		}

		if (currentObject >= playerGunObjectStart && currentObject < playerObjectStart && drawingShadowMap) {
			//don't draw the onscreen player weapon
			draw = false;
		}

		if (currentObject >= playerObjectStart && currentObject < playerObjectEnd && !drawingShadowMap) {
			draw = false;
		}

		if (currentObject >= playerObjectStart && currentObject < playerObjectEnd && drawingShadowMap) {
			draw = true;
		}

		if (draw) {

			//default,grass,water,brick,stone,tile,crate,ice,bone,metal,wood
			auto textureType = TexMap[texture_number].material;

			UINT materialIndex = mMaterials[textureType].get()->MatCBIndex;

			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
			D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + materialIndex * matCBByteSize;

			cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress); //Set cbPerObject
			cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress); //Set cbMaterial

			CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex.Offset(texture_number, mCbvSrvDescriptorSize);
			cmdList->SetGraphicsRootDescriptorTable(3, tex); //Set gDiffuseMap

			CD3DX12_GPU_DESCRIPTOR_HANDLE tex3(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex3.Offset(number_of_tex_aliases + 1, mCbvSrvDescriptorSize);
			cmdList->SetGraphicsRootDescriptorTable(5, tex3); //Set gShadowMap

			CD3DX12_GPU_DESCRIPTOR_HANDLE tex4(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex4.Offset(number_of_tex_aliases + 2, mCbvSrvDescriptorSize);
			cmdList->SetGraphicsRootDescriptorTable(7, tex4); //Set gSsaoMap

			//CHECK THIS
			if (normalMap && !drawingShadowMap && !drawingSSAO) {
				CD3DX12_GPU_DESCRIPTOR_HANDLE tex2(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
				tex2.Offset(normal_map_texture, mCbvSrvDescriptorSize);
				cmdList->SetGraphicsRootDescriptorTable(4, tex2); //Set gNormalMap
			}

			if (dp_command_index_mode[i] == 1 && TexMap[texture_alias_number].is_alpha_texture == isAlpha) {  //USE_NON_INDEXED_DP
				int  v = verts_per_poly[currentObject];

				if (dp_commands[currentObject] == D3DPT_TRIANGLELIST)
				{
					cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				}
				else if (dp_commands[currentObject] == D3DPT_TRIANGLESTRIP)
				{
					cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				}
				cmdList->DrawInstanced(v, 1, vert_index, 0);
			}
		}
	}

	UINT materialIndex = mMaterials["default"].get()->MatCBIndex;

	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
	D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + materialIndex * matCBByteSize;

	cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
	cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);
}

float DungeonStompApp::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 DungeonStompApp::GetHillsNormal(float x, float z)const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z),
		1.0f,
		-0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void DungeonStompApp::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../Textures/bricks2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));


	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../Textures/WoodCrate01.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));


	mTextures[grassTex->Name] = std::move(grassTex);
	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);

}

void DungeonStompApp::CreateRtvAndDsvDescriptorHeaps()
{
	// Add +6 RTV for cube render target.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 3;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// Add +1 DSV for shadow map.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void DungeonStompApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = MAX_NUM_TEXTURES;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	//
	// Fill out the heap with actual descriptors.
	//
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto woodCrateTex = mTextures["woodCrateTex"]->Resource;
	auto grassTex = mTextures["grassTex"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = woodCrateTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);

	hDescriptor.Offset(1, mCbvSrvDescriptorSize);

	srvDesc.Format = grassTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);


	LoadRRTextures11("textures.dat");


	// create upload heap. We will fill this with data for our text

	HRESULT hr = md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
		D3D12_HEAP_FLAG_NONE, // no flags
		&CD3DX12_RESOURCE_DESC::Buffer(maxNumTextCharacters * sizeof(TextVertex)), // resource description for a buffer
		D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
		nullptr,
		IID_PPV_ARGS(&textVertexBuffer));

	textVertexBuffer->SetName(L"Text Vertex Buffer Upload Resource Heap");

	CD3DX12_RANGE readRange(0, 0);	// We do not intend to read from this resource on the CPU. (so end is less than or equal to begin)
	// map the resource heap to get a gpu virtual address to the beginning of the heap
	hr = textVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&textVBGPUAddress));

	for (int i = 0; i < MaxRectangle; ++i)
	{
		// create upload heap. We will fill this with data for our text
		hr = md3dDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), // upload heap
			D3D12_HEAP_FLAG_NONE, // no flags
			&CD3DX12_RESOURCE_DESC::Buffer(maxNumRectangleCharacters * sizeof(TextVertex)), // resource description for a buffer
			D3D12_RESOURCE_STATE_GENERIC_READ, // GPU will read from this buffer and copy its contents to the default heap
			nullptr,
			IID_PPV_ARGS(&rectangleVertexBuffer[i]));

		rectangleVertexBuffer[i]->SetName(L"Rectangle Vertex Buffer Upload Resource Heap");

		CD3DX12_RANGE readRange2(0, 0);
		// map the resource heap to get a gpu virtual address to the beginning of the heap
		hr = rectangleVertexBuffer[i]->Map(0, &readRange2, reinterpret_cast<void**>(&rectangleVBGPUAddress[i]));
	}

	//auto skyCubeMap = mTextures["skyCubeMap"]->Resource;
	auto skyCubeMap = mTextures["sunsetcube1024"]->Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDescSkyMap = {};
	srvDescSkyMap.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDescSkyMap.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDescSkyMap.Texture2D.MostDetailedMip = 0;
	srvDescSkyMap.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDescSkyMap.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDescSkyMap.TextureCube.MostDetailedMip = 0;
	srvDescSkyMap.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
	srvDescSkyMap.TextureCube.ResourceMinLODClamp = 0.0f;
	srvDescSkyMap.Format = skyCubeMap->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(skyCubeMap.Get(), &srvDescSkyMap, hDescriptor);

	int counttext = number_of_tex_aliases;

	mSkyTexHeapIndex = (UINT)number_of_tex_aliases;
	mShadowMapHeapIndex = mSkyTexHeapIndex + 1;

	mSsaoHeapIndexStart = mShadowMapHeapIndex + 1;
	mSsaoAmbientMapIndex = mSsaoHeapIndexStart + 3;

	//mNullCubeSrvIndex = mShadowMapHeapIndex + 1;
	//mNullTexSrvIndex = mNullCubeSrvIndex + 1;

	mNullCubeSrvIndex = mSsaoHeapIndexStart + 5;
	mNullTexSrvIndex1 = mNullCubeSrvIndex + 1;
	mNullTexSrvIndex2 = mNullTexSrvIndex1 + 1;


	auto srvCpuStart = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	auto dsvCpuStart = mDsvHeap->GetCPUDescriptorHandleForHeapStart();

	auto nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mNullCubeSrvIndex, mCbvSrvUavDescriptorSize);
	mNullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mNullCubeSrvIndex, mCbvSrvUavDescriptorSize);

	md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
	nullSrv.Offset(1, mCbvSrvUavDescriptorSize);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);


	mShadowMap->BuildDescriptors(
		GetCpuSrv(mShadowMapHeapIndex),
		GetGpuSrv(mShadowMapHeapIndex),
		GetDsv(1));

	/*mShadowMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, mShadowMapHeapIndex, mCbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, mDsvDescriptorSize));*/


	mSsao->BuildDescriptors(
		mDepthStencilBuffer.Get(),
		GetCpuSrv(mSsaoHeapIndexStart),
		GetGpuSrv(mSsaoHeapIndexStart),
		GetRtv(SwapChainBufferCount),
		mCbvSrvUavDescriptorSize,
		mRtvDescriptorSize);

	nullSrv.Offset(1, mCbvSrvUavDescriptorSize);
	md3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DungeonStompApp::GetCpuSrv(int index)const
{
	auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DungeonStompApp::GetGpuSrv(int index)const
{
	auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	srv.Offset(index, mCbvSrvUavDescriptorSize);
	return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DungeonStompApp::GetDsv(int index)const
{
	auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
	dsv.Offset(index, mDsvDescriptorSize);
	return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DungeonStompApp::GetRtv(int index)const
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtv.Offset(index, mRtvDescriptorSize);
	return rtv;
}

BOOL DungeonStompApp::LoadRRTextures11(char* filename)
{
	FILE* fp;
	char s[256];
	char p[256];
	char f[256];

	int y_count = 30;
	int done = 0;
	int object_count = 0;
	int vert_count = 0;
	int pv_count = 0;
	int poly_count = 0;
	int tex_alias_counter = 0;
	int tex_counter = 0;
	int i;
	BOOL start_flag = TRUE;
	BOOL found;

	if (fopen_s(&fp, filename, "r") != 0)
	{
		//PrintMessage(hwnd, "ERROR can't open ", filename, SCN_AND_FILE);
		//MessageBox(hwnd, filename, "Error can't open", MB_OK);
		//return FALSE;
	}

	//TODO: fix this
	//for (int i = 0;i < 400;i++) {
		//textures[i] = NULL;
	//}

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	int flip = 0;

	while (done == 0)
	{
		found = FALSE;
		fscanf_s(fp, "%s", &s, 256);

		if (strcmp(s, "AddTexture") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			//remember the file
			strcpy_s(f, 256, p);
			tex_counter++;
		}

		if (strcmp(s, "Alias") == 0)
		{
			fscanf_s(fp, "%s", &p, 256);
			fscanf_s(fp, "%s", &p, 256);
			strcpy_s((char*)TexMap[tex_alias_counter].tex_alias_name, 100, (char*)&p);

			TexMap[tex_alias_counter].texture = tex_counter - 1;

			bool exists = true;
			FILE* fp4 = NULL;
			fopen_s(&fp4, f, "rb");
			if (fp4 == NULL)
			{
				exists = false;
			}



			auto currentTex = std::make_unique<Texture>();
			currentTex->Name = p;

			if (exists) {
				//currentTex->Filename = charToWChar("../Textures/ruin1.dds");
				currentTex->Filename = charToWChar(f);
			}
			else {
				currentTex->Filename = charToWChar("../Textures/WoodCrate01.dds");
			}

			DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
				mCommandList.Get(), currentTex->Filename.c_str(),
				currentTex->Resource, currentTex->UploadHeap);

			//Default to woodcrate if the texture will not load
			if (currentTex->Resource == NULL) {
				currentTex->Filename = charToWChar("../Textures/WoodCrate01.dds");
				DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
					mCommandList.Get(), currentTex->Filename.c_str(),
					currentTex->Resource, currentTex->UploadHeap);
			}

			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = currentTex->Resource->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = currentTex->Resource->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			if (strcmp(p, "sunsetcube1024") == 0) {

				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MostDetailedMip = 0;
				srvDesc.TextureCube.MipLevels = currentTex->Resource->GetDesc().MipLevels;
				srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
				srvDesc.Format = currentTex->Resource->GetDesc().Format;
			}
			else {

			}


			srvDesc.Format = currentTex->Resource->GetDesc().Format;
			md3dDevice->CreateShaderResourceView(currentTex->Resource.Get(), &srvDesc, hDescriptor);


			//auto a = mTextures[currentTex->Name].get();

			mTextures[currentTex->Name] = std::move(currentTex);

			// next descriptor
			hDescriptor.Offset(1, mCbvSrvDescriptorSize);

			fscanf_s(fp, "%s", &p, 256);
			if (strcmp(p, "AlphaTransparent") == 0)
				TexMap[tex_alias_counter].is_alpha_texture = TRUE;

			i = tex_alias_counter;

			fscanf_s(fp, "%s", &p, 256);

			if (strcmp(p, "WHOLE") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.0;
			}

			if (strcmp(p, "TL_QUAD") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)0.5;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)0.5;
				TexMap[i].tv[2] = (float)0.5;
				TexMap[i].tu[3] = (float)0.5;
				TexMap[i].tv[3] = (float)0.0;
			}

			if (strcmp(p, "TR_QUAD") == 0)
			{
				TexMap[i].tu[0] = (float)0.5;
				TexMap[i].tv[0] = (float)0.5;
				TexMap[i].tu[1] = (float)0.5;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)0.5;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.0;
			}
			if (strcmp(p, "LL_QUAD") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.5;
				TexMap[i].tu[2] = (float)0.5;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)0.5;
				TexMap[i].tv[3] = (float)0.5;
			}
			if (strcmp(p, "LR_QUAD") == 0)
			{
				TexMap[i].tu[0] = (float)0.5;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.5;
				TexMap[i].tv[1] = (float)0.5;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.5;
			}
			if (strcmp(p, "TOP_HALF") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)0.5;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)0.5;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.0;
			}
			if (strcmp(p, "BOT_HALF") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.5;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.5;
			}
			if (strcmp(p, "LEFT_HALF") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)0.5;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)0.5;
				TexMap[i].tv[3] = (float)0.0;
			}
			if (strcmp(p, "RIGHT_HALF") == 0)
			{
				TexMap[i].tu[0] = (float)0.5;
				TexMap[i].tv[0] = (float)1.0;
				TexMap[i].tu[1] = (float)0.5;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)1.0;
				TexMap[i].tv[2] = (float)1.0;
				TexMap[i].tu[3] = (float)1.0;
				TexMap[i].tv[3] = (float)0.0;
			}
			if (strcmp(p, "TL_TRI") == 0)
			{
				TexMap[i].tu[0] = (float)0.0;
				TexMap[i].tv[0] = (float)0.0;
				TexMap[i].tu[1] = (float)1.0;
				TexMap[i].tv[1] = (float)0.0;
				TexMap[i].tu[2] = (float)0.0;
				TexMap[i].tv[2] = (float)1.0;
			}
			if (strcmp(p, "BR_TRI") == 0)
			{
			}

			fscanf_s(fp, "%s", &p, 256);
			strcpy_s((char*)TexMap[tex_alias_counter].material, 100, (char*)&p);

			tex_alias_counter++;
			found = TRUE;
		}

		if (strcmp(s, "END_FILE") == 0)
		{
			//PrintMessage(hwnd, "\n", NULL, LOGFILE_ONLY);
			number_of_tex_aliases = tex_alias_counter;
			found = TRUE;
			done = 1;
		}

		if (found == FALSE)
		{
			//PrintMessage(hwnd, "File Error: Syntax problem :", p, SCN_AND_FILE);
			//MessageBox(hwnd, "p", "File Error: Syntax problem ", MB_OK);
			//return FALSE;
		}
	}
	fclose(fp);

	SetTextureNormalMap();

	return TRUE;
}

void DungeonStompApp::SetTextureNormalMap() {

	char junk[255];

	for (int i = 0; i < number_of_tex_aliases; i++) {

		sprintf_s(junk, "%s_nm", TexMap[i].tex_alias_name);

		int normalmap = -1;

		for (int j = 0; j < number_of_tex_aliases; j++) {
			if (strstr(TexMap[j].tex_alias_name, "_nm") != 0) {

				if (strcmp(TexMap[j].tex_alias_name, junk) == 0) {
					TexMap[i].normalmaptextureid = j;
				}
			}
		}
	}
}

void DungeonStompApp::ProcessLights11()
{
	//P = pointlight, M = misslelight, C = sword light S = spotlight
	//12345678901234567890 
	//01234567890123456789
	//PPPPPPPPPPPMMMMCSSSS

	int sort[200];
	float dist[200];
	int obj[200];
	int temp;

	for (int i = 0; i < MaxLights; i++)
	{
		LightContainer[i].Strength = { 1.0f, 1.0f, 1.0f };
		LightContainer[i].FalloffStart = 200.0f;
		LightContainer[i].Direction = { 0.0f, -1.0f, 0.0f };
		LightContainer[i].FalloffEnd = 275.0f;
		LightContainer[i].Position = DirectX::XMFLOAT3{ 0.0f,9000.0f,0.0f };
		LightContainer[i].SpotPower = 90.0f;
	}

	int dcount = 0;
	//Find lights
	for (int q = 0; q < oblist_length; q++)
	{
		int ob_type = oblist[q].type;
		float	qdist = FastDistance(m_vEyePt.x - oblist[q].x,
			m_vEyePt.y - oblist[q].y,
			m_vEyePt.z - oblist[q].z);
		//if (ob_type == 57)
		if (ob_type == 6 && oblist[q].light_source->command == 900)
		//if (ob_type == 6 && qdist < 2500 && oblist[q].light_source->command == 900)
		{
			dist[dcount] = qdist;
			sort[dcount] = dcount;
			obj[dcount] = q;
			dcount++;
		}
	}
	//sorting - ASCENDING ORDER
	for (int i = 0; i < dcount; i++)
	{
		for (int j = i + 1; j < dcount; j++)
		{
			if (dist[sort[i]] > dist[sort[j]])
			{
				temp = sort[i];
				sort[i] = sort[j];
				sort[j] = temp;
			}
		}
	}

	if (dcount > 16) {
		dcount = 16;
	}

	for (int i = 0; i < dcount; i++)
	{
		int q = obj[sort[i]];
		float dist2 = dist[sort[i]];

		int angle = (int)oblist[q].rot_angle;
		int ob_type = oblist[q].type;
		//LightContainer[i+1].Strength = { 1.5f, 1.5f, 1.5f };
		LightContainer[i].Strength = { 1.0f, 1.0f, 1.0f };
		LightContainer[i].Position = DirectX::XMFLOAT3{ oblist[q].x,oblist[q].y + 50.0f, oblist[q].z };
	}

	int count = 0;

	for (int misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{
		if (your_missle[misslecount].active == 1)
		{
			if (count < 4) {

				float r = MathHelper::RandF(10.0f, 100.0f);

				LightContainer[11 + count].Position = DirectX::XMFLOAT3{ your_missle[misslecount].x, your_missle[misslecount].y, your_missle[misslecount].z };
				LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f };
				LightContainer[11 + count].FalloffStart = 200.0f;
				LightContainer[11 + count].Direction = { 0.0f, -1.0f, 0.0f };
				LightContainer[11 + count].FalloffEnd = 300.0f;
				LightContainer[11 + count].SpotPower = 10.0f;


				if (your_missle[misslecount].model_id == 103) {
					LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 0.0f, 1.0f, 0.843f };
				}
				else if (your_missle[misslecount].model_id == 104) {
					LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 1.0f, 0.396f, 0.0f };
				}
				else if (your_missle[misslecount].model_id == 105) {
					LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 0.91f, 1.0f, 0.0f };

				}
				count++;
			}
		}
	}

	bool flamesword = false;

	if (strstr(your_gun[current_gun].gunname, "FLAME") != NULL ||
		strstr(your_gun[current_gun].gunname, "ICE") != NULL ||
		strstr(your_gun[current_gun].gunname, "LIGHTNINGSWORD") != NULL)
	{
		flamesword = true;
	}

	if (flamesword) {

		int spot = 15;

		LightContainer[spot].Position = DirectX::XMFLOAT3{ m_vEyePt.x, m_vEyePt.y, m_vEyePt.z };
		LightContainer[spot].Strength = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f };
		LightContainer[spot].FalloffStart = 200.0f;
		LightContainer[spot].Direction = { 0.0f, -1.0f, 0.0f };
		LightContainer[spot].FalloffEnd = 300.0f;
		LightContainer[spot].SpotPower = 10.0f;

		if (strstr(your_gun[current_gun].gunname, "SUPERFLAME") != NULL) {
			LightContainer[spot].Strength = DirectX::XMFLOAT3{ 1.0f, 0.867f, 0.0f };
		}
		else if (strstr(your_gun[current_gun].gunname, "FLAME") != NULL) {
			LightContainer[spot].Strength = DirectX::XMFLOAT3{ 1.0f, 0.369f, 0.0f };
		}
		else if (strstr(your_gun[current_gun].gunname, "ICE") != NULL) {

			LightContainer[spot].Strength = DirectX::XMFLOAT3{ 0.0f, 0.796f, 1.0f };
		}
		else if (strstr(your_gun[current_gun].gunname, "LIGHTNINGSWORD") != NULL) {
			LightContainer[spot].Strength = DirectX::XMFLOAT3{ 1.0f, 1.0f, 1.0f };
		}
	}

	count = 0;
	dcount = 0;

	//Find lights SPOT
	for (int q = 0; q < oblist_length; q++)
	{
		int ob_type = oblist[q].type;
		float	qdist = FastDistance(m_vEyePt.x - oblist[q].x,
			m_vEyePt.y - oblist[q].y,
			m_vEyePt.z - oblist[q].z);
		//if (ob_type == 6)
		if (ob_type == 6 && qdist < 2500 && oblist[q].light_source->command == 1)
		{
			dist[dcount] = qdist;
			sort[dcount] = dcount;
			obj[dcount] = q;
			dcount++;
		}

	}

	//sorting - ASCENDING ORDER
	for (int i = 0; i < dcount; i++)
	{
		for (int j = i + 1; j < dcount; j++)
		{
			if (dist[sort[i]] > dist[sort[j]])
			{
				temp = sort[i];
				sort[i] = sort[j];
				sort[j] = temp;
			}
		}
	}

	if (dcount > 10) {
		dcount = 10;
	}

	for (int i = 0; i < dcount; i++)
	{
		int q = obj[sort[i]];
		float dist2 = dist[sort[i]];
		int angle = (int)oblist[q].rot_angle;
		int ob_type = oblist[q].type;
		float adjust = 0.4f;
		LightContainer[i + 16].Position = DirectX::XMFLOAT3{ oblist[q].x,oblist[q].y + 0.0f, oblist[q].z };
		LightContainer[i + 16].Strength = DirectX::XMFLOAT3{ (float)oblist[q].light_source->rcolour + adjust, (float)oblist[q].light_source->gcolour + adjust, (float)oblist[q].light_source->bcolour + adjust };
		LightContainer[i + 16].FalloffStart = 600.0f;
		LightContainer[i + 16].Direction = { oblist[q].light_source->direction_x, oblist[q].light_source->direction_y, oblist[q].light_source->direction_z };
		LightContainer[i + 16].FalloffEnd = 650.0f;
		LightContainer[i + 16].SpotPower = 1.9f;
	}
}

