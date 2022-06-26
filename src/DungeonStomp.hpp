#include "Font.hpp"
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
	Count
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

	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildLandGeometry();
	void BuildDungeonGeometryBuffers();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems);

	void LoadTextures();
	void BuildDescriptorHeaps();
	BOOL LoadRRTextures11(char* filename);
	void ProcessLights11();
	//void RenderText(Font font, std::wstring text, XMFLOAT2 pos, XMFLOAT2 scale = XMFLOAT2(1.0f, 1.0f), XMFLOAT2 padding = XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
	void RenderText(Font font, std::wstring text, XMFLOAT2 pos, XMFLOAT2 scale = XMFLOAT2(1.0f, 1.0f), XMFLOAT2 padding = XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f));
	void RenderRectangle(Font font, int index, int textureid, XMFLOAT2 pos, XMFLOAT2 scale = XMFLOAT2(1.0f, 1.0f), XMFLOAT2 padding = XMFLOAT2(0.5f, 0.0f), XMFLOAT4 color = XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f));
	void display_message3(float x, float y, char text[2048], int r, int g, int b, float fontx, float fonty, int fonttype);
	void SetDungeonText();
	void ScanMod(float fElapsedTime);
	void DisplayHud();
	void DisplayPlayerCaption();
	void DrawDungeon(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, BOOL isAlpha, bool isTorch = false);
	void SetTextureNormalMap();

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

	RenderItem* mDungeonRitem = nullptr;

	// List of all the render items.
	std::vector<std::unique_ptr<RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<RenderItem*> mRitemLayer[(int)RenderLayer::Count];

	std::unique_ptr<Dungeon> mDungeon;

	PassConstants mMainPassCB;

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
	

};

