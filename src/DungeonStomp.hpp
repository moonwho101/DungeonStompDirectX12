#include "Font.hpp"
#include "ShadowMap.h"
#include "Ssao.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

// Lightweight structure stores parameters to draw a shape.  This will
// vary from app-to-app.
typedef struct RenderItem
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
} RenderItemMain;

enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count,
	Sky
};

class DungeonStompApp : public D3DApp
{
public:
	DungeonStompApp(HINSTANCE hInstance);
	DungeonStompApp(const DungeonStompApp& rhs) = delete;
	DungeonStompApp& operator=(const DungeonStompApp& rhs) = delete;
	~DungeonStompApp();

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
	void UpdateDungeon(const GameTimer& gt);
	void UpdateShadowPassCB(const GameTimer& gt);
	void UpdateSsaoCB(const GameTimer& gt);

	void BuildRootSignature();
	void BuildSsaoRootSignature();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildDungeonGeometryBuffers();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, const GameTimer& gt);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index)const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const;
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const;

	void LoadTextures();
	void BuildDescriptorHeaps();
	BOOL LoadRRTextures11(char* filename);
	void ProcessLights11();

	void DrawRenderItemsFL(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);
	//void RenderText(Font font, std::wstring text, XMFLOAT2 pos, XMFLOAT2 scale = XMFLOAT2(1.0f, 1.0f), XMFLOAT2 padding = XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	void RenderText(Font font, std::wstring text, XMFLOAT2 pos, XMFLOAT2 scale = XMFLOAT2(1.0f, 1.0f), XMFLOAT2 padding = XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f));
	void RenderRectangle(Font font, int index, int textureid, XMFLOAT2 pos, XMFLOAT2 scale = XMFLOAT2(1.0f, 1.0f), XMFLOAT2 padding = XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f));
	void display_message3(float x, float y, char text[2048], int r, int g, int b, float fontx, float fonty, int fonttype);
	void SetDungeonText();
	void ScanMod(float fElapsedTime);
	void DisplayHud();
	void DisplayPlayerCaption();
	void DrawDungeon(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, BOOL isAlpha, bool isTorch = false, bool normalMap = false);
	void SetTextureNormalMap();
	void SetTextureNormalMapEmpty();
	void DrawSceneToShadowMap(const GameTimer& gt);
	void DrawNormalsAndDepth(const GameTimer& gt);
	void UpdateShadowTransform(const GameTimer& gt, int light);
	void CreateRtvAndDsvDescriptorHeaps();

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();

	float GetHillsHeight(float x, float z)const;
	XMFLOAT3 GetHillsNormal(float x, float z)const;

private:
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;
	FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12RootSignature> mSsaoRootSignature = nullptr;

	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> mTextures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

	RenderItem* mDungeonRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Dungeon> mDungeon;

	UINT mSkyTexHeapIndex = 0;
	UINT mShadowMapHeapIndex = 0;

	UINT mSsaoHeapIndexStart = 0;
	UINT mSsaoAmbientMapIndex = 0;

	UINT mNullCubeSrvIndex = 0;
	UINT mNullTexSrvIndex = 0;

	UINT mNullTexSrvIndex1 = 0;
	UINT mNullTexSrvIndex2 = 0;

	CD3DX12_GPU_DESCRIPTOR_HANDLE mNullSrv;

	PassConstants mMainPassCB;
	PassConstants mShadowPassCB;// index 1 of pass cbuffer.



	Light LightContainer[MaxLights];

	XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * XM_PI;
	float mPhi = XM_PIDIV2 - 0.1f;
	float mRadius = 50.0f;

	float mSunTheta = 1.25f * XM_PI;
	float mSunPhi = XM_PIDIV4;

	POINT mLastMousePos;

	ID3D12Resource* textVertexBuffer;
	UINT8* textVBGPUAddress;
	D3D12_VERTEX_BUFFER_VIEW textVertexBufferView; // a view for our text vertex buffer

	ID3D12Resource* rectangleVertexBuffer[MaxRectangle];
	UINT8* rectangleVBGPUAddress[MaxRectangle];
	D3D12_VERTEX_BUFFER_VIEW rectangleVertexBufferView[MaxRectangle]; // a view for our text vertex buffer

	std::unique_ptr<ShadowMap> mShadowMap;
	std::unique_ptr<Ssao> mSsao;

	DirectX::BoundingSphere mSceneBounds;

	float mLightNearZ = 0.0f;
	float mLightFarZ = 0.0f;
	XMFLOAT3 mLightPosW;
	XMFLOAT4X4 mLightView = MathHelper::Identity4x4();
	XMFLOAT4X4 mLightProj = MathHelper::Identity4x4();
	XMFLOAT4X4 mShadowTransform = MathHelper::Identity4x4();

	float mLightRotationAngle = 0.0f;
	XMFLOAT3 mBaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f)

	};
	XMFLOAT3 mRotatedLightDirections[3];


};

