//***************************************************************************************
// DXRHelper.h - DirectX Raytracing (DXR) helper for DX12 Ultimate
// Manages bottom/top-level acceleration structures for inline ray queries
// used as a ray-traced shadow alternative to traditional shadow mapping.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"
#include "FrameResource.h"

class DXRHelper {
public:
	DXRHelper() = default;
	~DXRHelper() = default;

	// Initialize DXR resources. Returns true if DXR is available.
	bool Initialize(ID3D12Device5* device, D3D12_RAYTRACING_TIER tier);

	bool IsSupported() const { return mSupported; }
	D3D12_RAYTRACING_TIER GetTier() const { return mTier; }

	// Build all acceleration structures from the current vertex/index data.
	// Call once after geometry is loaded, and again if geometry changes.
	void BuildAccelerationStructures(
		ID3D12GraphicsCommandList4* cmdList,
		ID3D12Resource* vertexBuffer,
		UINT vertexCount,
		UINT vertexStride,
		ID3D12Resource* indexBuffer,
		UINT indexCount,
		DXGI_FORMAT indexFormat);

	// Get the TLAS SRV for binding to shaders (for RayQuery in pixel shaders).
	ID3D12Resource* GetTLAS() const { return mTopLevelAS.Get(); }
	D3D12_GPU_VIRTUAL_ADDRESS GetTLASAddress() const {
		return mTopLevelAS ? mTopLevelAS->GetGPUVirtualAddress() : 0;
	}

	// Create an SRV for the TLAS in the given descriptor heap location.
	void CreateTLASSrv(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);

	bool IsBuilt() const { return mIsBuilt; }

private:
	void BuildBLAS(
		ID3D12GraphicsCommandList4* cmdList,
		ID3D12Resource* vertexBuffer,
		UINT vertexCount,
		UINT vertexStride,
		ID3D12Resource* indexBuffer,
		UINT indexCount,
		DXGI_FORMAT indexFormat);

	void BuildTLAS(ID3D12GraphicsCommandList4* cmdList);

	bool mSupported = false;
	bool mIsBuilt = false;
	D3D12_RAYTRACING_TIER mTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;

	ID3D12Device5* mDevice = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> mBottomLevelAS;
	Microsoft::WRL::ComPtr<ID3D12Resource> mTopLevelAS;

	// Scratch buffers (needed during build, can be released after GPU finishes).
	Microsoft::WRL::ComPtr<ID3D12Resource> mBLASScratch;
	Microsoft::WRL::ComPtr<ID3D12Resource> mTLASScratch;
	Microsoft::WRL::ComPtr<ID3D12Resource> mInstanceDescBuffer;
};
