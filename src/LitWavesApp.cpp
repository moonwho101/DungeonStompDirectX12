//***************************************************************************************
// LitWavesApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Use arrow keys to move light positions.
//
//***************************************************************************************

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


using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

const int gNumFrameResources = 3;

void Draw(int currentObject, ID3D11DeviceContext* pd3dImmediateContext, int vert_index);

std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
extern int number_of_tex_aliases;
static wchar_t* charToWChar(const char* text);
ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;
// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
struct RenderItem
{
	RenderItem() = default;

	// World matrix of the shape that describes the object's local space
	// relative to the world space, which defines the position, orientation,
	// and scale of the object in the world.
	XMFLOAT4X4 World = MathHelper::Identity4x4();

	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// Dirty flag indicating the object data has changed and we need to update the constant buffer.
	// Because we have an object cbuffer for each FrameResource, we have to apply the
	// update to each FrameResource.  Thus, when we modify obect data we should set 
	// NumFramesDirty = gNumFrameResources so that each frame resource gets the update.
	int NumFramesDirty = gNumFrameResources;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1;

	Material* Mat = nullptr;
	MeshGeometry* Geo = nullptr;

	// Primitive topology.
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced parameters.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count
};


class LitWavesApp : public D3DApp
{
public:
    LitWavesApp(HINSTANCE hInstance);
    LitWavesApp(const LitWavesApp& rhs) = delete;
    LitWavesApp& operator=(const LitWavesApp& rhs) = delete;
    ~LitWavesApp();

    virtual bool Initialize()override;

private:
    virtual void OnResize()override;
    virtual void Update(const GameTimer& gt)override;
    virtual void Draw(const GameTimer& gt)override;

    virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
    virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
    virtual void OnMouseMove(WPARAM btnState, int x, int y)override;

	void OnKeyboardInput(const GameTimer& gt);
	void UpdateCamera(const GameTimer& gt);
	void UpdateObjectCBs(const GameTimer& gt);
	void UpdateMaterialCBs(const GameTimer& gt);
	void UpdateMainPassCB(const GameTimer& gt);
	void UpdateWaves(const GameTimer& gt);

    void BuildRootSignature();
    void BuildShadersAndInputLayout();
    void BuildLandGeometry();
    void BuildWavesGeometryBuffers();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildMaterials();
    void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	void LoadTextures();
	void BuildDescriptorHeaps();
	BOOL LoadRRTextures11(char* filename);
	void ProcessLights11();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

    float GetHillsHeight(float x, float z)const;
    XMFLOAT3 GetHillsNormal(float x, float z)const;

private:

    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    FrameResource* mCurrFrameResource = nullptr;
    int mCurrFrameResourceIndex = 0;

    UINT mCbvSrvDescriptorSize = 0;

    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	RenderItem* mWavesRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Waves> mWaves;

    PassConstants mMainPassCB;

	Light LightContainer[MaxLights];

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

    float mTheta = 1.5f*XM_PI;
    float mPhi = XM_PIDIV2 - 0.1f;
    float mRadius = 50.0f;

	float mSunTheta = 1.25f*XM_PI;
	float mSunPhi = XM_PIDIV4;

    POINT mLastMousePos;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
    PSTR cmdLine, int showCmd)
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    try
    {
        LitWavesApp theApp(hInstance);
        if(!theApp.Initialize())
            return 0;

        return theApp.Run();
    }
    catch(DxException& e)
    {
        MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
        return 0;
    }
}

LitWavesApp::LitWavesApp(HINSTANCE hInstance)
    : D3DApp(hInstance)
{
}

LitWavesApp::~LitWavesApp()
{
    if(md3dDevice != nullptr)
        FlushCommandQueue();
}

bool LitWavesApp::Initialize()
{
    if(!D3DApp::Initialize())
        return false;

    // Reset the command list to prep for initialization commands.
    ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    // Get the increment size of a descriptor in this heap type.  This is hardware specific, so we have
    // to query this information.
    mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mWaves = std::make_unique<Waves>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);

	LoadTextures();
    BuildRootSignature();
	BuildDescriptorHeaps();
    BuildShadersAndInputLayout();
	BuildLandGeometry();
    BuildWavesGeometryBuffers();
	BuildMaterials();
    BuildRenderItems();
	BuildRenderItems();
    BuildFrameResources();
	BuildPSOs();

	LoadRRTextures11("textures.dat");
	
	InitDS();


    // Execute the initialization commands.
    ThrowIfFailed(mCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

    // Wait until initialization is complete.
    FlushCommandQueue();

    return true;
}
 
void LitWavesApp::OnResize()
{
    D3DApp::OnResize();

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    //XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f*MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 100000.0f);
    XMStoreFloat4x4(&mProj, P);
}

VOID UpdateControls();
HRESULT FrameMove(double fTime, FLOAT fTimeKey);
void UpdateWorld(float fElapsedTime);

void LitWavesApp::Update(const GameTimer& gt)
{
	float t = gt.DeltaTime();
	UpdateControls();
	FrameMove(0.0f, t);
	UpdateWorld(t);

	OnKeyboardInput(gt);
	UpdateCamera(gt);

	// Cycle through the circular frame resource array.
	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if(mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateMainPassCB(gt);
	UpdateWaves(gt);
}

void LitWavesApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	// A command list can be reset after it has been added to the command queue via ExecuteCommandList.
	// Reusing the command list reuses memory.
	ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	//mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// Specify the buffers we are going to render to.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	//opaque
	mCommandList->SetPipelineState(mPSOs["opaque"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);

	//mCommandList->SetPipelineState(mPSOs["alphaTested"].Get());
	//DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::AlphaTested]);

	//mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	//DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Transparent]);


	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Swap the back and front buffers
	//ThrowIfFailed(mSwapChain->Present(0, 0));
	ThrowIfFailed(mSwapChain->Present(1, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	// Advance the fence value to mark commands up to this fence point.
	mCurrFrameResource->Fence = ++mCurrentFence;

	// Add an instruction to the command queue to set a new fence point. 
    // Because we are on the GPU timeline, the new fence point won't be 
    // set until the GPU finishes processing all the commands prior to this Signal().
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void LitWavesApp::OnMouseDown(WPARAM btnState, int x, int y)
{
   /* mLastMousePos.x = x;
    mLastMousePos.y = y;

    SetCapture(mhMainWnd);*/
}

void LitWavesApp::OnMouseUp(WPARAM btnState, int x, int y)
{
    //ReleaseCapture();
}

void LitWavesApp::OnMouseMove(WPARAM btnState, int x, int y)
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
    //mLastMousePos.y = y;ssssss
}

void LitWavesApp::OnKeyboardInput(const GameTimer& gt)
{
	const float dt = gt.DeltaTime();

	if(GetAsyncKeyState(VK_LEFT) & 0x8000)
		mSunTheta -= 1.0f*dt;

	if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
		mSunTheta += 1.0f*dt;

	if(GetAsyncKeyState(VK_UP) & 0x8000)
		mSunPhi -= 1.0f*dt;

	if(GetAsyncKeyState(VK_DOWN) & 0x8000)
		mSunPhi += 1.0f*dt;

	mSunPhi = MathHelper::Clamp(mSunPhi, 0.1f, XM_PIDIV2);
}

void LitWavesApp::UpdateCamera(const GameTimer& gt)
{
	// Convert Spherical to Cartesian coordinates.
	//mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	//mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	//mEyePos.y = mRadius * cosf(mPhi);
	
	float adjust = 50.0f;

	mEyePos.x = m_vEyePt.x;
	mEyePos.y = m_vEyePt.y + adjust;
	mEyePos.z = m_vEyePt.z;


	player_list[trueplayernum].x = m_vEyePt.x;
	player_list[trueplayernum].y = m_vEyePt.y + adjust;
	player_list[trueplayernum].z = m_vEyePt.z;


	// Build the view matrix.
	XMVECTOR pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	//XMVECTOR target = XMVectorZero();
	XMVECTOR target =  XMVectorSet(m_vLookatPt.x, m_vLookatPt.y + adjust, m_vLookatPt.z, 1.0f);

	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);

	XMStoreFloat4x4(&mView, view);

	//D3DXVECTOR3 vEyePt(m_vEyePt.x, m_vEyePt.y + adjust, m_vEyePt.z);
	//D3DXVECTOR3 vLookatPt(m_vLookatPt.x, m_vLookatPt.y + adjust, m_vLookatPt.z);

	//D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
	//D3DXMATRIXA16 matView;
	//D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);


}
//
//void LitWavesApp::UpdateObjectCBs(const GameTimer& gt)
//{
//	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
//	for(auto& e : mAllRitems)
//	{
//		// Only update the cbuffer data if the constants have changed.  
//		// This needs to be tracked per frame resource.
//		if(e->NumFramesDirty > 0)
//		{
//			XMMATRIX world = XMLoadFloat4x4(&e->World);
//			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);
//
//			ObjectConstants objConstants;
//			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
//
//			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
//
//			// Next FrameResource need to be updated too.
//			e->NumFramesDirty--;
//		}
//	}
//}


void LitWavesApp::UpdateObjectCBs(const GameTimer& gt)
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

			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->NumFramesDirty--;
		}
	}
}


void LitWavesApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for(auto& e : mMaterials)
	{
		// Only update the cbuffer data if the constants have changed.  If the cbuffer
		// data changes, it needs to be updated for each FrameResource.
		Material* mat = e.second.get();
		if(mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			// Next FrameResource need to be updated too.
			mat->NumFramesDirty--;
		}
	}
}



void LitWavesApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);

	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	//mMainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	//mMainPassCB.AmbientLight = { 1.00f, 1.00f, 1.00f, 1.00f };
	mMainPassCB.AmbientLight = { 0.00f, 0.00f, 0.00f, 0.00f };

	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.0f, mSunTheta, mSunPhi);

	XMStoreFloat3(&mMainPassCB.Lights[0].Direction, lightDir);
	//mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };
	mMainPassCB.Lights[0].Strength = { 1.0f, 1.0f, 0.9f };

	for (int i = 0; i < MaxLights; i++) {
		mMainPassCB.Lights[i].Direction = LightContainer[i].Direction;
		mMainPassCB.Lights[i].Strength = LightContainer[i].Strength;
		mMainPassCB.Lights[i].Position = LightContainer[i].Position;
		mMainPassCB.Lights[i].FalloffEnd = LightContainer[i].FalloffEnd;
		mMainPassCB.Lights[i].FalloffStart = LightContainer[i].FalloffStart;
		mMainPassCB.Lights[i].SpotPower = LightContainer[i].SpotPower;
	}


	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

extern int cnt;
extern D3DVERTEX* src_v;

void LitWavesApp::UpdateWaves(const GameTimer& gt)
{
	// Every quarter second, generate a random wave.
	static float t_base = 0.0f;
	if((mTimer.TotalTime() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		int i = MathHelper::Rand(4, mWaves->RowCount() - 5);
		int j = MathHelper::Rand(4, mWaves->ColumnCount() - 5);

		float r = MathHelper::RandF(0.2f, 0.5f);

		mWaves->Disturb(i, j, r);
	}

	// Update the wave simulation.
	mWaves->Update(gt.DeltaTime());

	// Update the wave vertex buffer with the new solution.
	auto currWavesVB = mCurrFrameResource->WavesVB.get();
	for(int i = 0; i < mWaves->VertexCount(); ++i)
	{
		Vertex v;

		v.Pos = mWaves->Position(i);
		v.Normal = mWaves->Normal(i);


		//currWavesVB->CopyData(i, v);
	}

	Vertex v;// [20000] ;

	for (int j = 0; j < cnt; j++)
	{
		//D3DXVECTOR3 a = D3DXVECTOR3(src_v[j].x, src_v[j].y, src_v[j].z);

		v.Pos.x = src_v[j].x;
		v.Pos.y = src_v[j].y;
		v.Pos.z = src_v[j].z;

		v.Normal.x = src_v[j].nx;
		v.Normal.y = src_v[j].ny;
		v.Normal.z = src_v[j].nz;

		v.TexC.x = src_v[j].tu;
		v.TexC.y = src_v[j].tv;

		currWavesVB->CopyData(j, v);

		//v[j].tu = src_v[j].tu;
		//v[j].tv = src_v[j].tv;
	}


	// Set the dynamic VB of the wave renderitem to the current frame VB.
	mWavesRitem->Geo->VertexBufferGPU = currWavesVB->Resource();
}

void LitWavesApp::BuildRootSignature()
{

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,1, 0);

    // Root parameter can be a table, root descriptor or root constants.
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];

    // Create root CBV.
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // A root signature is an array of root parameters.
	//CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    // create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

    if(errorBlob != nullptr)
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


std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> LitWavesApp::GetStaticSamplers()
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

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}



void LitWavesApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "0",
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
		NULL, NULL
	};

	mShaders["standardVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_0");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", defines, "PS", "ps_5_0");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_0");
	mShaders["torchPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", torchTestDefines, "PS", "ps_5_0");



	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

}

void LitWavesApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.0f, 160.0f, 50, 50);

	//
	// Extract the vertex elements we are interested and apply the height function to
	// each vertex.  In addition, color the vertices based on their height so we have
	// sandy looking beaches, grassy low hills, and snow mountain peaks.
	//

	std::vector<Vertex> vertices(grid.Vertices.size());
	for(size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		auto& p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
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

void LitWavesApp::BuildWavesGeometryBuffers()
{
	std::vector<std::uint16_t> indices(3 * mWaves->TriangleCount()); // 3 indices per face
	assert(mWaves->VertexCount() < 0x0000ffff);

	// Iterate over each quad.
	int m = mWaves->RowCount();
	int n = mWaves->ColumnCount();
	int k = 0;
	for(int i = 0; i < m - 1; ++i)
	{
		for(int j = 0; j < n - 1; ++j)
		{
			indices[k] = i*n + j;
			indices[k + 1] = i*n + j + 1;
			indices[k + 2] = (i + 1)*n + j;

			indices[k + 3] = (i + 1)*n + j;
			indices[k + 4] = i*n + j + 1;
			indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	UINT vbByteSize = mWaves->VertexCount()*sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size()*sizeof(std::uint16_t);

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
/*
void LitWavesApp::BuildPSOs()
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
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));
}
*/

void LitWavesApp::BuildPSOs()
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
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

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

	/*blendStateDesc.AlphaToCoverageEnable = FALSE;
	blendStateDesc.IndependentBlendEnable = FALSE;
	blendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	blendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
	blendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_BLEND_FACTOR;
	blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;*/

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
}

void LitWavesApp::BuildFrameResources()
{
    for(int i = 0; i < gNumFrameResources; ++i)
    {
        mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
            1, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mWaves->VertexCount()));
    }
}

void LitWavesApp::BuildMaterials()
{
	auto grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

    // This is not a good water material definition, but we do not have all the rendering
    // tools we need (transparency, environment reflection), so we fake it for now.
	auto water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseSrvHeapIndex = 0;
    water->DiffuseAlbedo = XMFLOAT4(2.0f, 2.0f, 2.0f, 2.0f);
    water->FresnelR0 = XMFLOAT3(0.3f, 0.3f, 0.3f);

    water->Roughness = 0.5f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
}

void LitWavesApp::BuildRenderItems()
{
	auto wavesRitem = std::make_unique<RenderItem>();
	wavesRitem->World = MathHelper::Identity4x4();
	wavesRitem->ObjCBIndex = 0;
	wavesRitem->Mat = mMaterials["water"].get();
	wavesRitem->Geo = mGeometries["waterGeo"].get();
	wavesRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRitem->IndexCount = wavesRitem->Geo->DrawArgs["grid"].IndexCount;
	wavesRitem->StartIndexLocation = wavesRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRitem->BaseVertexLocation = wavesRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mWavesRitem = wavesRitem.get();

	mRitemLayer[(int)RenderLayer::Opaque].push_back(wavesRitem.get());

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	mAllRitems.push_back(std::move(wavesRitem));
	mAllRitems.push_back(std::move(gridRitem));
}

int FindTextureAlias(char* alias);

void LitWavesApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	ScanMod();

	ProcessLights11();

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();
	// i = 2 water
	// For each render item...
	//for(size_t i = 0; i < ritems.size(); ++i)
	//{
	//	auto ri = ritems[i];

	//	cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
	//	cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
	//	cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

	//	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
	//	D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

	//	cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
	//	cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);

	//	//cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);


	//}

	auto ri = ritems[2];

	cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
	cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
	cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	//tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvDescriptorSize);

	//int a = FindTextureAlias("goblin");

	tex.Offset(1, mCbvSrvDescriptorSize);

	//int size = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);;
	//tex.Offset(0, size);

	cmdList->SetGraphicsRootDescriptorTable(3, tex);

	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
	D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + ri->Mat->MatCBIndex * matCBByteSize;

	cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
	cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);


	int currentObject = 0;
	for (currentObject = 0; currentObject < number_of_polys_per_frame; currentObject++)
	{
		int i = ObjectsToDraw[currentObject].vert_index;
		int vert_index = ObjectsToDraw[currentObject].srcstart;
		int fperpoly = (float)ObjectsToDraw[currentObject].srcfstart;
		int face_index = ObjectsToDraw[currentObject].srcfstart;

		int texture_alias_number = texture_list_buffer[i];
		int texture_number = TexMap[texture_alias_number].texture;

		//if (texture_number == 111) {
		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(texture_number, mCbvSrvDescriptorSize);
		//tex.Offset(111, mCbvSrvDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(3, tex);
		//}

		if (dp_command_index_mode[i] == 1 && TexMap[texture_alias_number].is_alpha_texture == FALSE) {  //USE_NON_INDEXED_DP
			//pd3dImmediateContext->PSSetShaderResources(0, 1, &textures[texture_number]);
			//Draw(currentObject, pd3dImmediateContext, vert_index);

			int  v = verts_per_poly[currentObject];

			if (dp_commands[currentObject] == D3DPT_TRIANGLESTRIP)
			{
				cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
				//pd3dImmediateContext->Draw(v, vert_index);
			}
			else if (dp_commands[currentObject] == D3DPT_TRIANGLELIST)
			{
				cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				//pd3dImmediateContext->Draw(v, vert_index);
			}

			cmdList->DrawInstanced(v, 1, vert_index, 0);
		}
	}

	
	bool draw = false;
	mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	//mCommandList->SetPipelineState(mPSOs["torchTested"].Get());

	for (currentObject = 0; currentObject < number_of_polys_per_frame; currentObject++)
	{
		int i = ObjectsToDraw[currentObject].vert_index;
		int vert_index = ObjectsToDraw[currentObject].srcstart;
		int fperpoly = (float)ObjectsToDraw[currentObject].srcfstart;
		int face_index = ObjectsToDraw[currentObject].srcfstart;

		int texture_alias_number = texture_list_buffer[i];
		int texture_number = TexMap[texture_alias_number].texture;

		if (texture_number >= 94 && texture_number <= 101 ||
			texture_number >= 289 - 1 && texture_number <= 296 - 1 ||
			texture_number >= 279 - 1 && texture_number <= 288 - 1 ||
			texture_number >= 206 - 1 && texture_number <= 210 - 1) {
			draw = false;
		}
		else {
			draw = true;
		}

		if (draw) {
			//if (texture_number == 111) {
			CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex.Offset(texture_number, mCbvSrvDescriptorSize);
			//tex.Offset(111, mCbvSrvDescriptorSize);
			cmdList->SetGraphicsRootDescriptorTable(3, tex);
			//}

			if (dp_command_index_mode[i] == 1 && TexMap[texture_alias_number].is_alpha_texture == TRUE) {  //USE_NON_INDEXED_DP
				//pd3dImmediateContext->PSSetShaderResources(0, 1, &textures[texture_number]);
				//Draw(currentObject, pd3dImmediateContext, vert_index);

				int  v = verts_per_poly[currentObject];

				if (dp_commands[currentObject] == D3DPT_TRIANGLESTRIP)
				{
					cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
					//pd3dImmediateContext->Draw(v, vert_index);
				}
				else if (dp_commands[currentObject] == D3DPT_TRIANGLELIST)
				{
					cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					//pd3dImmediateContext->Draw(v, vert_index);
				}

				cmdList->DrawInstanced(v, 1, vert_index, 0);
			}
		}
	}

	//Draw Torch
	//mCommandList->SetPipelineState(mPSOs["transparent"].Get());
	mCommandList->SetPipelineState(mPSOs["torchTested"].Get());

	draw = false;

	for (currentObject = 0; currentObject < number_of_polys_per_frame; currentObject++)
	{
		int i = ObjectsToDraw[currentObject].vert_index;
		int vert_index = ObjectsToDraw[currentObject].srcstart;
		int fperpoly = (float)ObjectsToDraw[currentObject].srcfstart;
		int face_index = ObjectsToDraw[currentObject].srcfstart;

		int texture_alias_number = texture_list_buffer[i];
		int texture_number = TexMap[texture_alias_number].texture;

		if (texture_number >= 94 && texture_number <= 101 ||
			texture_number >= 289 - 1 && texture_number <= 296 - 1 ||
			texture_number >= 279 - 1 && texture_number <= 288 - 1 ||
			texture_number >= 206 - 1 && texture_number <= 210 - 1) {
			draw = true;
		}
		else {
			draw = false;
		}


		
		if (draw) {
			CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex.Offset(texture_number, mCbvSrvDescriptorSize);
			//tex.Offset(111, mCbvSrvDescriptorSize);
			cmdList->SetGraphicsRootDescriptorTable(3, tex);
			//}

			if (dp_command_index_mode[i] == 1 && TexMap[texture_alias_number].is_alpha_texture == TRUE) {  //USE_NON_INDEXED_DP
				//pd3dImmediateContext->PSSetShaderResources(0, 1, &textures[texture_number]);
				//Draw(currentObject, pd3dImmediateContext, vert_index);

				int  v = verts_per_poly[currentObject];

				if (dp_commands[currentObject] == D3DPT_TRIANGLESTRIP)
				{
					cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
					//pd3dImmediateContext->Draw(v, vert_index);
				}
				else if (dp_commands[currentObject] == D3DPT_TRIANGLELIST)
				{
					cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					//pd3dImmediateContext->Draw(v, vert_index);
				}

				cmdList->DrawInstanced(v, 1, vert_index, 0);
			}
		}
	}


}

float LitWavesApp::GetHillsHeight(float x, float z)const
{
    return 0.3f*(z*sinf(0.1f*x) + x*cosf(0.1f*z));
}

XMFLOAT3 LitWavesApp::GetHillsNormal(float x, float z)const
{
    // n = (-df/dx, 1, -df/dz)
    XMFLOAT3 n(
        -0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
        1.0f,
        -0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

    XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
    XMStoreFloat3(&n, unitNormal);

    return n;
}

extern int number_of_polys_per_frame;
extern POLY_SORT ObjectsToDraw[200000];
extern BOOL* dp_command_index_mode;





void LitWavesApp::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	//woodCrateTex->Filename = L"../../Textures/WoodCrate01.dds";
	//woodCrateTex->Filename = L"./Textures/eye.dds";
	//woodCrateTex->Filename = L"./Textures/bricks2.dds";
	woodCrateTex->Filename = L"../Textures/goblin.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource, woodCrateTex->UploadHeap));


	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	//grassTex->Filename = L"../../Textures/grass.dds";
	//grassTex->Filename = L"./Textures/bricks2.dds";
	//grassTex->Filename = L"./Textures/goblin.dds";
	grassTex->Filename = L"../Textures2/WoodCrate01.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
		mCommandList.Get(), grassTex->Filename.c_str(),
		grassTex->Resource, grassTex->UploadHeap));


	mTextures[grassTex->Name] = std::move(grassTex);
	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);

}


void LitWavesApp::BuildDescriptorHeaps()
{
	//
	// Create the SRV heap.
	//
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 412;
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


}


BOOL LitWavesApp::LoadRRTextures11(char* filename)
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
			strcpy(f, p);

		/*	if (hr == S_OK)
			{
				textures[tex_counter] = save_out_srv;
				tex_counter++;
				found = TRUE;
			}*/

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

			//if (tex_counter == 301) {
			//	int test = 1;
			//}

			auto woodCrateTex = std::make_unique<Texture>();
			woodCrateTex->Name = p;

			if (exists) {
				woodCrateTex->Filename = charToWChar(f);
			}else{
				woodCrateTex->Filename = charToWChar("../Textures2/WoodCrate01.dds");
				//woodCrateTex->Filename = charToWChar("./Textures/goblin.dds");
			}

			//if (flip == 0) {
			//	woodCrateTex->Filename = charToWChar("./Textures/goblin.dds");
			//	flip = 1;
			//}
			//else {
			//	woodCrateTex->Filename = charToWChar("../../Textures/WoodCrate01.dds");
			//	flip = 0;
			//}

			//if (strcmp(p, "goblin") == 0) {
			//	//woodCrateTex->Filename = charToWChar(f);
			//	woodCrateTex->Filename = charToWChar("./Textures/goblin.dds");
			//}
			//else {
			//	woodCrateTex->Filename = charToWChar("../../Textures/WoodCrate01.dds");
			//}

			//woodCrateTex->Filename = charToWChar(f);
			
			ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(),
				mCommandList.Get(), woodCrateTex->Filename.c_str(),
				woodCrateTex->Resource, woodCrateTex->UploadHeap));

			//auto woodCrateTex2 = woodCrateTex->Resource;

			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = woodCrateTex->Resource->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = woodCrateTex->Resource->GetDesc().MipLevels;
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

			srvDesc.Format = woodCrateTex->Resource->GetDesc().Format;
			md3dDevice->CreateShaderResourceView(woodCrateTex->Resource.Get(), &srvDesc, hDescriptor);

			mTextures[woodCrateTex->Name] = std::move(woodCrateTex);

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

	return TRUE;
}


static wchar_t* charToWChar(const char* text)
{
	const size_t size = strlen(text) + 1;
	wchar_t* wText = new wchar_t[size];
	mbstowcs(wText, text, size);
	return wText;
}


void LitWavesApp::ProcessLights11()
{
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
		if (ob_type == 6 && qdist < 2500 && oblist[q].light_source->command == 900)
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

		LightContainer[i].Position = DirectX::XMFLOAT3{ oblist[q].x,oblist[q].y + 10.0f, oblist[q].z };
	}

	//P = pointlight, M = misslelight, C = sword light S = spotlight
	//12345678901234567890 
	//01234567890123456789
	//PPPPPPPPPPPMMMMCSSSS

	int count = 0;

	for (int misslecount = 0; misslecount < MAX_MISSLE; misslecount++)
	{
		if (your_missle[misslecount].active == 1)
		{
			if (count < 4) {
				LightContainer[11 + count + 1].Position = DirectX::XMFLOAT3{ your_missle[misslecount].x, your_missle[misslecount].y, your_missle[misslecount].z };
				LightContainer[11 + count + 1].Strength = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f };
				LightContainer[11 + count + 1].FalloffStart = 200.0f;
				LightContainer[11 + count + 1].Direction = { 0.0f, -1.0f, 0.0f };
				LightContainer[11 + count + 1].FalloffEnd = 300.0f;
				LightContainer[11 + count + 1].SpotPower = 10.0f;


				if (your_missle[misslecount].model_id == 103) {
					LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f };
				}
				else if (your_missle[misslecount].model_id == 104) {
					LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 1.0f, 0.0f, 0.0f };
				}
				else if (your_missle[misslecount].model_id == 105) {
					LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 1.0f, 0.0f, 1.0f };

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

		if (strstr(your_gun[current_gun].gunname, "FLAME") != NULL) {
			LightContainer[spot].Strength = DirectX::XMFLOAT3{ 1.0f, 0.0f, 0.0f };
		}
		else if (strstr(your_gun[current_gun].gunname, "ICE") != NULL) {

			LightContainer[spot].Strength = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f };
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

	if (dcount > 5) {
		dcount = 5;
	}

	for (int i = 0; i < dcount; i++)
	{
		int q = obj[sort[i]];
		float dist2 = dist[sort[i]];
		int angle = (int)oblist[q].rot_angle;
		int ob_type = oblist[q].type;

		LightContainer[i + 16].Position = DirectX::XMFLOAT3{ oblist[q].x,oblist[q].y + 0.0f, oblist[q].z };
		LightContainer[i + 16].Strength = DirectX::XMFLOAT3{ (float)oblist[q].light_source->rcolour, (float)oblist[q].light_source->gcolour, (float)oblist[q].light_source->bcolour };
		LightContainer[i + 16].FalloffStart = 550.0f;
		LightContainer[i + 16].Direction = { oblist[q].light_source->direction_x, oblist[q].light_source->direction_y, oblist[q].light_source->direction_z };
		LightContainer[i + 16].FalloffEnd = 600.0f;
		LightContainer[i + 16].SpotPower = 1.0f;
	}
}
