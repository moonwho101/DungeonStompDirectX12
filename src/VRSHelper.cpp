//***************************************************************************************
// VRSHelper.cpp - Variable Rate Shading (DX12 Ultimate) implementation
//***************************************************************************************

#include "VRSHelper.h"

using Microsoft::WRL::ComPtr;

void VRSHelper::Initialize(ID3D12Device* device, D3D12_VARIABLE_SHADING_RATE_TIER vrsTier, UINT vrsTileSize) {
	mTier = vrsTier;
	mTileSize = vrsTileSize;
	mSupported = (vrsTier != D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED);

	if (mSupported) {
		OutputDebugString(L"[VRS] Variable Rate Shading initialized.\n");
		if (mTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2) {
			OutputDebugString(L"[VRS] Tier 2 (image-based VRS) available.\n");
		}
	}
}

void VRSHelper::SetShadingRate(
	ID3D12GraphicsCommandList* cmdList,
	D3D12_SHADING_RATE rate,
	const D3D12_SHADING_RATE_COMBINER* combiners)
{
	if (!mSupported)
		return;

	// Query for the CommandList5 interface needed for VRS.
	ComPtr<ID3D12GraphicsCommandList5> cmdList5;
	if (SUCCEEDED(cmdList->QueryInterface(IID_PPV_ARGS(&cmdList5)))) {
		cmdList5->RSSetShadingRate(rate, combiners);
	}
}

void VRSHelper::SetFullRate(ID3D12GraphicsCommandList* cmdList) {
	D3D12_SHADING_RATE_COMBINER combiners[2] = {
		D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,
		D3D12_SHADING_RATE_COMBINER_PASSTHROUGH
	};
	SetShadingRate(cmdList, D3D12_SHADING_RATE_1X1, combiners);
}

void VRSHelper::SetReducedRate(ID3D12GraphicsCommandList* cmdList) {
	D3D12_SHADING_RATE_COMBINER combiners[2] = {
		D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,
		D3D12_SHADING_RATE_COMBINER_PASSTHROUGH
	};
	SetShadingRate(cmdList, D3D12_SHADING_RATE_2X2, combiners);
}

void VRSHelper::BuildShadingRateImage(
	ID3D12GraphicsCommandList* cmdList,
	ID3D12Resource* depthBuffer,
	UINT width, UINT height)
{
	// Tier 2 screen-space image generation would go here.
	// For now, per-draw VRS (Tier 1) is used in the rendering loop.
	// A compute shader would generate the VRS image from the depth buffer.
}
