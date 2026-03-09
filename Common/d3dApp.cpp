//***************************************************************************************
// d3dApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "d3dApp.h"
#include <WindowsX.h>

// ---- DirectX 12 Agility SDK opt-in ----
// These exports tell the D3D12 loader to use the Agility SDK runtime (D3D12Core.dll)
// from the D3D12\ subdirectory next to the executable.
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

namespace {

constexpr D3D_FEATURE_LEVEL kFeatureLevels[] = {
	D3D_FEATURE_LEVEL_12_2,
	D3D_FEATURE_LEVEL_12_1,
	D3D_FEATURE_LEVEL_12_0,
	D3D_FEATURE_LEVEL_11_1,
	D3D_FEATURE_LEVEL_11_0
};

bool IsSoftwareAdapter(IDXGIAdapter1 *adapter) {
	DXGI_ADAPTER_DESC1 desc = {};
	if (FAILED(adapter->GetDesc1(&desc))) {
		return true;
	}
	return (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0;
}

bool TryCreateDeviceForAdapter(IDXGIAdapter1 *adapter, ComPtr<ID3D12Device> &outDevice,
	                           D3D_FEATURE_LEVEL &outFeatureLevel) {
	for (auto fl : kFeatureLevels) {
		ComPtr<ID3D12Device> candidate;
		if (SUCCEEDED(D3D12CreateDevice(adapter, fl, IID_PPV_ARGS(&candidate)))) {
			outDevice = candidate;
			outFeatureLevel = fl;
			return true;
		}
	}

	return false;
}

bool SelectBestHardwareAdapter(IDXGIFactory6 *factory,
	                           ComPtr<IDXGIAdapter1> &outAdapter,
	                           ComPtr<ID3D12Device> &outDevice,
	                           D3D_FEATURE_LEVEL &outFeatureLevel,
	                           std::wstring &outAdapterName) {
	ComPtr<IDXGIAdapter1> bestAdapter;
	ComPtr<ID3D12Device> bestDevice;
	D3D_FEATURE_LEVEL bestFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	SIZE_T bestDedicatedMemory = 0;
	bool found = false;

	for (UINT i = 0;; ++i) {
		ComPtr<IDXGIAdapter1> adapter;
		if (FAILED(factory->EnumAdapterByGpuPreference(
		    i,
		    DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		    IID_PPV_ARGS(&adapter)))) {
			break;
		}

		if (IsSoftwareAdapter(adapter.Get())) {
			continue;
		}

		ComPtr<ID3D12Device> device;
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
		if (!TryCreateDeviceForAdapter(adapter.Get(), device, featureLevel)) {
			continue;
		}

		DXGI_ADAPTER_DESC1 desc = {};
		adapter->GetDesc1(&desc);

		const bool isBetter = !found
		    || (featureLevel > bestFeatureLevel)
		    || (featureLevel == bestFeatureLevel && desc.DedicatedVideoMemory > bestDedicatedMemory);

		if (isBetter) {
			found = true;
			bestAdapter = adapter;
			bestDevice = device;
			bestFeatureLevel = featureLevel;
			bestDedicatedMemory = desc.DedicatedVideoMemory;
			outAdapterName = desc.Description;
		}
	}

	if (!found) {
		for (UINT i = 0;; ++i) {
			ComPtr<IDXGIAdapter1> adapter;
			if (FAILED(factory->EnumAdapters1(i, &adapter))) {
				break;
			}

			if (IsSoftwareAdapter(adapter.Get())) {
				continue;
			}

			ComPtr<ID3D12Device> device;
			D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
			if (!TryCreateDeviceForAdapter(adapter.Get(), device, featureLevel)) {
				continue;
			}

			DXGI_ADAPTER_DESC1 desc = {};
			adapter->GetDesc1(&desc);

			const bool isBetter = !found
			    || (featureLevel > bestFeatureLevel)
			    || (featureLevel == bestFeatureLevel && desc.DedicatedVideoMemory > bestDedicatedMemory);

			if (isBetter) {
				found = true;
				bestAdapter = adapter;
				bestDevice = device;
				bestFeatureLevel = featureLevel;
				bestDedicatedMemory = desc.DedicatedVideoMemory;
				outAdapterName = desc.Description;
			}
		}
	}

	if (!found) {
		return false;
	}

	outAdapter = bestAdapter;
	outDevice = bestDevice;
	outFeatureLevel = bestFeatureLevel;
	return true;
}

const wchar_t *FeatureLevelToString(D3D_FEATURE_LEVEL fl) {
	switch (fl) {
	case D3D_FEATURE_LEVEL_12_2:
		return L"12_2";
	case D3D_FEATURE_LEVEL_12_1:
		return L"12_1";
	case D3D_FEATURE_LEVEL_12_0:
		return L"12_0";
	case D3D_FEATURE_LEVEL_11_1:
		return L"11_1";
	default:
		return L"11_0";
	}
}

const char *ShaderModelToString(D3D_SHADER_MODEL model) {
	switch (model) {
	#if defined(D3D_SHADER_MODEL_6_8)
	case D3D_SHADER_MODEL_6_8:
		return "6.8";
	#endif
	#if defined(D3D_SHADER_MODEL_6_7)
	case D3D_SHADER_MODEL_6_7:
		return "6.7";
	#endif
	case D3D_SHADER_MODEL_6_6:
		return "6.6";
	case D3D_SHADER_MODEL_6_5:
		return "6.5";
	case D3D_SHADER_MODEL_6_4:
		return "6.4";
	case D3D_SHADER_MODEL_6_3:
		return "6.3";
	case D3D_SHADER_MODEL_6_2:
		return "6.2";
	case D3D_SHADER_MODEL_6_1:
		return "6.1";
	case D3D_SHADER_MODEL_6_0:
		return "6.0";
	default:
		return "5.1";
	}
}

} // namespace

void ShutDownSound();
extern float gFps;
extern float gMspf;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp *D3DApp::mApp = nullptr;
D3DApp *D3DApp::GetApp() {
	return mApp;
}

D3DApp::D3DApp(HINSTANCE hInstance)
    : mhAppInst(hInstance) {
	// Only one D3DApp can be constructed.
	assert(mApp == nullptr);
	mApp = this;
}

D3DApp::~D3DApp() {
	if (md3dDevice != nullptr)
		FlushCommandQueue();

	if (mFrameLatencyWaitableObject) {
		CloseHandle(mFrameLatencyWaitableObject);
		mFrameLatencyWaitableObject = nullptr;
	}

	ShutDownSound();
}

HINSTANCE D3DApp::AppInst() const {
	return mhAppInst;
}

HWND D3DApp::MainWnd() const {
	return mhMainWnd;
}

float D3DApp::AspectRatio() const {
	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState() const {
	return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value) {
	if (m4xMsaaState != value) {
		m4xMsaaState = value;

		// Recreate the swapchain and buffers with new multisample settings.
		CreateSwapChain();
		OnResize();
	}
}

int D3DApp::Run() {
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT) {
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else {
			// Wait on the swap chain's frame latency waitable object before starting CPU work.
			// This ensures we don't queue too many frames ahead.
			if (mFrameLatencyWaitableObject) {
				WaitForSingleObjectEx(mFrameLatencyWaitableObject, 0, TRUE);
			}

			mTimer.Tick();

			if (!mAppPaused) {
				CalculateFrameStats();
				Update(mTimer);
				Draw(mTimer);
			} else {
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool D3DApp::Initialize() {
	if (!InitMainWindow())
		return false;

	if (!InitDirect3D())
		return false;

	// Do the initial resize code.
	OnResize();

	return true;
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps() {
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
	    &rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
	    &dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::OnResize() {
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// Flush before changing any resources.
	FlushCommandQueue();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	// Resize the swap chain (flags must match those used at creation).
	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	if (mTearingSupported)
		swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
	ThrowIfFailed(mSwapChain->ResizeBuffers(
	    SwapChainBufferCount,
	    mClientWidth, mClientHeight,
	    mBackBufferFormat,
	    swapChainFlags));

	if (mBorderlessFullscreen) {
		ShowCursor(FALSE);
	}

	mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++) {
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// Create the depth/stencil buffer and view.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;
	depthStencilDesc.Height = mClientHeight;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = mDepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
	    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	    D3D12_HEAP_FLAG_NONE,
	    &depthStencilDesc,
	    D3D12_RESOURCE_STATE_COMMON,
	    &optClear,
	    IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// Create descriptor to mip level 0 of entire resource using the format of the resource.
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

	// Transition the resource from its initial state to be used as a depth buffer.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
	                                                                       D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// Execute the resize commands.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList *cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// Wait until resize is complete.
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	// WM_ACTIVATE is sent when the window is activated or deactivated.
	// We pause the game when the window is deactivated and unpause it
	// when it becomes active.
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			mAppPaused = true;
			mTimer.Stop();
		} else {
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

	// WM_SIZE is sent when the user resizes the window.
	case WM_SIZE:
		// Save the new client area dimensions.
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice) {
			if (wParam == SIZE_MINIMIZED) {
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			} else if (wParam == SIZE_MAXIMIZED) {
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			} else if (wParam == SIZE_RESTORED) {

				// Restoring from minimized state?
				if (mMinimized) {
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized) {
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				} else if (mResizing) {
					// If user is dragging the resize bars, we do not resize
					// the buffers here because as the user continuously
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is
					// done resizing the window and releases the resize bars, which
					// sends a WM_EXITSIZEMOVE message.
				} else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

	// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

	// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
	// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

	// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	// The WM_MENUCHAR message is sent when a menu is active and the user presses
	// a key that does not correspond to any mnemonic or accelerator key.
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

	// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO *)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO *)lParam)->ptMinTrackSize.y = 200;
		return 0;

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
		} else if ((int)wParam == VK_F2) {
			// more work is needed
			// Set4xMsaaState(!m4xMsaaState);
		} else if ((int)wParam == VK_F11) {
			ToggleBorderlessFullscreen();
		}

		return 0;

	// Handle ALT+ENTER for borderless fullscreen toggle
	case WM_SYSKEYDOWN:
		if (wParam == VK_RETURN && (lParam & (1 << 29))) {
			ToggleBorderlessFullscreen();
			return 0;
		}
		break;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int SoundInit();
HRESULT CreateDInput(HWND hWnd);
void InitDS();

bool D3DApp::InitMainWindow() {
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc)) {
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
	                         WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd) {
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);
	srand((unsigned int)time(NULL));
	SoundInit();
	CreateDInput(mhMainWnd);

	return true;
}

bool D3DApp::InitDirect3D() {
	UINT dxgiFactoryFlags = 0;

#if defined(DEBUG) || defined(_DEBUG)
	// Enable the D3D12 debug layer.
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mdxgiFactory)));

	ComPtr<ID3D12Device> baseDevice;
	ComPtr<IDXGIAdapter1> bestAdapter;
	std::wstring bestAdapterName;
	HRESULT hardwareResult = E_FAIL;

	if (SelectBestHardwareAdapter(mdxgiFactory.Get(), bestAdapter, baseDevice, mFeatureLevel, bestAdapterName)) {
		hardwareResult = S_OK;
		OutputDebugString((L"Using adapter: " + bestAdapterName + L" (FL "
		                   + FeatureLevelToString(mFeatureLevel) + L")\n").c_str());
	}

	// Fallback to WARP device.
	if (FAILED(hardwareResult)) {
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));
		ThrowIfFailed(D3D12CreateDevice(pWarpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&baseDevice)));
		mFeatureLevel = D3D_FEATURE_LEVEL_11_0;
		OutputDebugStringA("Using WARP adapter fallback (no hardware DX12 device found).\n");
	}

	// Query ID3D12Device5 (required for DXR Tier 1.1, VRS)
	ThrowIfFailed(baseDevice.As(&md3dDevice));

	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
	                                      IID_PPV_ARGS(&mFence)));

	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Check 4X MSAA quality support for our back buffer format.
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
	    D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
	    &msQualityLevels,
	    sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

	// Check tearing support
	mTearingSupported = CheckTearingSupport();

	// Check DX12 Ultimate features (VRS, DXR, Mesh Shaders, etc.)
	CheckDX12UltimateFeatures();

#ifdef _DEBUG
	LogAdapters();
#endif

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void D3DApp::CheckDX12UltimateFeatures() {
	// DXR & VRS (Options5)
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	if (SUCCEEDED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
		mDX12UltimateFeatures.RaytracingTier = options5.RaytracingTier;
		mDX12UltimateFeatures.RaytracingSupported = (options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
	}

	// VRS (Options6)
	D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
	if (SUCCEEDED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
		mDX12UltimateFeatures.VRSTier = options6.VariableShadingRateTier;
		mDX12UltimateFeatures.VariableRateShadingSupported = (options6.VariableShadingRateTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1);
	}

	// Mesh Shaders & Sampler Feedback (Options7)
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
	if (SUCCEEDED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7)))) {
		mDX12UltimateFeatures.MeshShaderTier = options7.MeshShaderTier;
		mDX12UltimateFeatures.MeshShaderSupported = (options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1);
		mDX12UltimateFeatures.SamplerFeedbackTier = options7.SamplerFeedbackTier;
		mDX12UltimateFeatures.SamplerFeedbackSupported = (options7.SamplerFeedbackTier >= D3D12_SAMPLER_FEEDBACK_TIER_0_9);
	}

	// Enhanced Barriers (Options12)
	D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};
	if (SUCCEEDED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &options12, sizeof(options12)))) {
		mDX12UltimateFeatures.EnhancedBarriersSupported = options12.EnhancedBarriersSupported;
		mDX12UltimateFeatures.RelaxedFormatCastingSupported = options12.RelaxedFormatCastingSupported;
	}

#ifdef D3D12_FEATURE_D3D12_OPTIONS14
	D3D12_FEATURE_DATA_D3D12_OPTIONS14 options14 = {};
	if (SUCCEEDED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS14, &options14, sizeof(options14)))) {
		mDX12UltimateFeatures.AdvancedTextureOpsSupported = options14.AdvancedTextureOpsSupported;
	}
#endif

#ifdef D3D12_FEATURE_D3D12_OPTIONS16
	D3D12_FEATURE_DATA_D3D12_OPTIONS16 options16 = {};
	if (SUCCEEDED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &options16, sizeof(options16)))) {
		mDX12UltimateFeatures.GPUUploadHeapSupported = options16.GPUUploadHeapSupported;
	}
#endif

	D3D12_FEATURE_DATA_SHADER_MODEL shaderModel = {};
#if defined(D3D_SHADER_MODEL_6_8)
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_8;
#elif defined(D3D_SHADER_MODEL_6_7)
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_7;
#else
	shaderModel.HighestShaderModel = D3D_SHADER_MODEL_6_6;
#endif
	if (SUCCEEDED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shaderModel, sizeof(shaderModel)))) {
		mDX12UltimateFeatures.HighestShaderModel = shaderModel.HighestShaderModel;
	}
	mDX12UltimateFeatures.ShaderModel6_6Supported = (mDX12UltimateFeatures.HighestShaderModel >= D3D_SHADER_MODEL_6_6);

	// Root Signature version
	D3D12_FEATURE_DATA_ROOT_SIGNATURE rootSigFeature = {};
	rootSigFeature.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (SUCCEEDED(md3dDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &rootSigFeature, sizeof(rootSigFeature)))) {
		mDX12UltimateFeatures.HighestRootSignatureVersion = rootSigFeature.HighestVersion;
	} else {
		mDX12UltimateFeatures.HighestRootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Log detected features
	OutputDebugStringA("=== DX12 Ultimate Feature Detection ===\n");

	OutputDebugString((L"Feature Level: " + std::wstring(FeatureLevelToString(mFeatureLevel)) + L"\n").c_str());

	OutputDebugStringA(mDX12UltimateFeatures.RaytracingSupported ? "DXR: Supported\n" : "DXR: Not supported\n");
	OutputDebugStringA(mDX12UltimateFeatures.VariableRateShadingSupported ? "VRS: Supported\n" : "VRS: Not supported\n");
	OutputDebugStringA(mDX12UltimateFeatures.MeshShaderSupported ? "Mesh Shaders: Supported\n" : "Mesh Shaders: Not supported\n");
	OutputDebugStringA(mDX12UltimateFeatures.SamplerFeedbackSupported ? "Sampler Feedback: Supported\n" : "Sampler Feedback: Not supported\n");
	OutputDebugStringA(mDX12UltimateFeatures.EnhancedBarriersSupported ? "Enhanced Barriers: Supported\n" : "Enhanced Barriers: Not supported\n");
	OutputDebugStringA(mDX12UltimateFeatures.AdvancedTextureOpsSupported ? "Advanced Texture Ops: Supported\n" : "Advanced Texture Ops: Not supported\n");
	OutputDebugStringA(mDX12UltimateFeatures.GPUUploadHeapSupported ? "GPU Upload Heap: Supported\n" : "GPU Upload Heap: Not supported\n");
	OutputDebugStringA((std::string("Shader Model: ") + ShaderModelToString(mDX12UltimateFeatures.HighestShaderModel) + "\n").c_str());

	const bool dx12UltimateReady = (mFeatureLevel >= D3D_FEATURE_LEVEL_12_2)
	    && mDX12UltimateFeatures.RaytracingSupported
	    && mDX12UltimateFeatures.VariableRateShadingSupported
	    && mDX12UltimateFeatures.MeshShaderSupported
	    && mDX12UltimateFeatures.SamplerFeedbackSupported;
	OutputDebugStringA(dx12UltimateReady ? "DX12 Ultimate Baseline: Ready\n" : "DX12 Ultimate Baseline: Partial\n");

	OutputDebugStringA(mTearingSupported ? "Tearing: Supported\n" : "Tearing: Not supported\n");
	OutputDebugStringA("========================================\n");
}

bool D3DApp::CheckTearingSupport() {
	ComPtr<IDXGIFactory5> factory5;
	if (SUCCEEDED(mdxgiFactory.As(&factory5))) {
		BOOL allowTearing = FALSE;
		if (SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing)))) {
			return allowTearing == TRUE;
		}
	}
	return false;
}

void D3DApp::ToggleBorderlessFullscreen() {
	if (!mBorderlessFullscreen) {
		// Save current window placement
		GetWindowPlacement(mhMainWnd, &mWindowPlacement);

		// Get the monitor info for the monitor the window is on
		HMONITOR hMon = MonitorFromWindow(mhMainWnd, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO mi = { sizeof(mi) };
		GetMonitorInfo(hMon, &mi);

		// Switch to borderless fullscreen
		SetWindowLongPtr(mhMainWnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
		SetWindowPos(mhMainWnd, HWND_TOP,
		             mi.rcMonitor.left, mi.rcMonitor.top,
		             mi.rcMonitor.right - mi.rcMonitor.left,
		             mi.rcMonitor.bottom - mi.rcMonitor.top,
		             SWP_FRAMECHANGED | SWP_NOACTIVATE);
		ShowWindow(mhMainWnd, SW_MAXIMIZE);
		ShowCursor(FALSE);
		mBorderlessFullscreen = true;
	} else {
		// Restore window style
		SetWindowLongPtr(mhMainWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
		SetWindowPlacement(mhMainWnd, &mWindowPlacement);
		SetWindowPos(mhMainWnd, nullptr, 0, 0, 0, 0,
		             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		ShowCursor(TRUE);
		mBorderlessFullscreen = false;
	}
}

void D3DApp::CreateCommandObjects() {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
	    D3D12_COMMAND_LIST_TYPE_DIRECT,
	    IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	// Create command list as ID3D12GraphicsCommandList5 for DX12 Ultimate (VRS, DXR).
	ComPtr<ID3D12GraphicsCommandList> baseCmdList;
	ThrowIfFailed(md3dDevice->CreateCommandList(
	    0,
	    D3D12_COMMAND_LIST_TYPE_DIRECT,
	    mDirectCmdListAlloc.Get(),
	    nullptr,
	    IID_PPV_ARGS(baseCmdList.GetAddressOf())));
	ThrowIfFailed(baseCmdList.As(&mCommandList));

	// Start off in a closed state.  This is because the first time we refer
	// to the command list we will Reset it, and it needs to be closed before
	// calling Reset.
	mCommandList->Close();
}

void D3DApp::CreateSwapChain() {
	// Release the previous swapchain we will be recreating.
	if (mFrameLatencyWaitableObject) {
		CloseHandle(mFrameLatencyWaitableObject);
		mFrameLatencyWaitableObject = nullptr;
	}
	mSwapChain.Reset();

	// Build swap chain flags: tearing + frame latency waitable object for low-latency presentation.
	UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
	if (mTearingSupported)
		swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

	// Use modern CreateSwapChainForHwnd (DXGI 1.2+)
	DXGI_SWAP_CHAIN_DESC1 sd = {};
	sd.Width = mClientWidth;
	sd.Height = mClientHeight;
	sd.Format = mBackBufferFormat;
	sd.Stereo = FALSE;
	sd.SampleDesc.Count = 1; // flip model requires count=1
	sd.SampleDesc.Quality = 0;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = SwapChainBufferCount;
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	sd.Flags = swapChainFlags;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(mdxgiFactory->CreateSwapChainForHwnd(
	    mCommandQueue.Get(),
	    mhMainWnd,
	    &sd,
	    nullptr, // no fullscreen desc (borderless approach)
	    nullptr,
	    swapChain1.GetAddressOf()));

	// Disable DXGI's built-in ALT+ENTER handling; we use borderless fullscreen instead.
	ThrowIfFailed(mdxgiFactory->MakeWindowAssociation(mhMainWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&mSwapChain));

	// Set maximum frame latency to allow multiple frames in flight for throughput.
	// The waitable object signals when a back buffer is available.
	mSwapChain->SetMaximumFrameLatency(3);
	mFrameLatencyWaitableObject = mSwapChain->GetFrameLatencyWaitableObject();
}

void D3DApp::FlushCommandQueue() {
	// Advance the fence value to mark commands up to this fence point.
	mCurrentFence++;

	// Add an instruction to the command queue to set a new fence point.  Because we
	// are on the GPU timeline, the new fence point won't be set until the GPU finishes
	// processing all the commands prior to this Signal().
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// Wait until the GPU has completed commands up to this fence point.
	if (mFence->GetCompletedValue() < mCurrentFence) {
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// Wait until the GPU hits current fence event is fired.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource *D3DApp::CurrentBackBuffer() const {
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView() const {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
	    mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
	    mCurrBackBuffer,
	    mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView() const {
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats() {
	// Code computes the average frames per second, and also the
	// average time it takes to render one frame.  These stats
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f) {
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		gFps = fps;
		gMspf = mspf;

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = mMainWndCaption +
		                     L"    fps: " + fpsStr +
		                     L"   mspf: " + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void D3DApp::LogAdapters() {
	UINT i = 0;
	IDXGIAdapter *adapter = nullptr;
	std::vector<IDXGIAdapter *> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i) {
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter *adapter) {
	UINT i = 0;
	IDXGIOutput *output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND) {
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput *output, DXGI_FORMAT format) {
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto &x : modeList) {
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
		    L"Width = " + std::to_wstring(x.Width) + L" " +
		    L"Height = " + std::to_wstring(x.Height) + L" " +
		    L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
		    L"\n";

		::OutputDebugString(text.c_str());
	}
}