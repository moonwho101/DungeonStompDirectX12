//***************************************************************************************
// d3dApp.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// Link necessary d3d12 libraries.
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// DX12 Ultimate feature support flags
struct DX12UltimateFeatures {
	bool RaytracingSupported = false;
	D3D12_RAYTRACING_TIER RaytracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
	bool VariableRateShadingSupported = false;
	D3D12_VARIABLE_SHADING_RATE_TIER VRSTier = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
	bool MeshShaderSupported = false;
	D3D12_MESH_SHADER_TIER MeshShaderTier = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
	bool SamplerFeedbackSupported = false;
	D3D12_SAMPLER_FEEDBACK_TIER SamplerFeedbackTier = D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
	bool EnhancedBarriersSupported = false;
	bool RelaxedFormatCastingSupported = false;
	bool AdvancedTextureOpsSupported = false;
	bool GPUUploadHeapSupported = false;
	D3D_SHADER_MODEL HighestShaderModel = D3D_SHADER_MODEL_5_1;
	bool ShaderModel6_6Supported = false;
	D3D_ROOT_SIGNATURE_VERSION HighestRootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
};

class D3DApp {
  protected:
	D3DApp(HINSTANCE hInstance);
	D3DApp(const D3DApp &rhs) = delete;
	D3DApp &operator=(const D3DApp &rhs) = delete;
	virtual ~D3DApp();

  public:
	static D3DApp *GetApp();

	HINSTANCE AppInst() const;
	HWND MainWnd() const;
	float AspectRatio() const;

	bool Get4xMsaaState() const;
	void Set4xMsaaState(bool value);

	int Run();

	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();
	virtual void Update(const GameTimer &gt) = 0;
	virtual void Draw(const GameTimer &gt) = 0;

	// Convenience overrides for handling mouse input.
	virtual void OnMouseDown(WPARAM btnState, int x, int y) {
	}
	virtual void OnMouseUp(WPARAM btnState, int x, int y) {
	}
	virtual void OnMouseMove(WPARAM btnState, int x, int y) {
	}

  protected:
	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();
	void CheckDX12UltimateFeatures();
	bool CheckTearingSupport();
	void ToggleBorderlessFullscreen();

	void FlushCommandQueue();

	ID3D12Resource *CurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView() const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView() const;

	void CalculateFrameStats();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter *adapter);
	void LogOutputDisplayModes(IDXGIOutput *output, DXGI_FORMAT format);

  protected:
	static D3DApp *mApp;

	HINSTANCE mhAppInst = nullptr; // application instance handle
	HWND mhMainWnd = nullptr;      // main window handle
	bool mAppPaused = false;       // is the application paused?
	bool mMinimized = false;       // is the application minimized?
	bool mMaximized = false;       // is the application maximized?
	bool mResizing = false;        // are the resize bars being dragged?
	bool mFullscreenState = false; // fullscreen enabled

	// Set true to use 4X MSAA (�4.1.8).  The default is false.
	bool m4xMsaaState = false; // 4X MSAA enabled
	UINT m4xMsaaQuality = 0;   // quality level of 4X MSAA

	// Used to keep track of the �delta-time� and game time (�4.4).
	GameTimer mTimer;

	Microsoft::WRL::ComPtr<IDXGIFactory6> mdxgiFactory;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;
	Microsoft::WRL::ComPtr<ID3D12Device5> md3dDevice;

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> mCommandList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	// Derived class should set these in derived constructor to customize starting values.
	std::wstring mMainWndCaption = L"Dungeon Stomp";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;

	bool mTearingSupported = false;
	bool mBorderlessFullscreen = false;
	WINDOWPLACEMENT mWindowPlacement = {};
	DX12UltimateFeatures mDX12UltimateFeatures;
	D3D_FEATURE_LEVEL mFeatureLevel = D3D_FEATURE_LEVEL_11_0;

	// Frame latency waitable object for low-latency presentation.
	HANDLE mFrameLatencyWaitableObject = nullptr;
};
