//***************************************************************************************
// VRSHelper.h - Variable Rate Shading (DX12 Ultimate) helper
// Manages per-draw and screen-space image-based VRS when supported.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"

// Wraps VRS Tier 1 (per-draw) and optionally Tier 2 (screen-space image) support.
class VRSHelper {
public:
	VRSHelper() = default;
	~VRSHelper() = default;

	// Call after device creation to check support and store capabilities.
	void Initialize(ID3D12Device* device, D3D12_VARIABLE_SHADING_RATE_TIER vrsTier, UINT vrsTileSize);

	bool IsSupported() const { return mSupported; }
	bool IsTier2() const { return mTier >= D3D12_VARIABLE_SHADING_RATE_TIER_2; }

	// Set per-draw shading rate via ID3D12GraphicsCommandList5.
	// rate: D3D12_SHADING_RATE_1X1, _1X2, _2X1, _2X2, _2X4, _4X2, _4X4
	// combiners[2]: how per-draw and per-primitive/image rates combine.
	void SetShadingRate(
		ID3D12GraphicsCommandList* cmdList,
		D3D12_SHADING_RATE rate,
		const D3D12_SHADING_RATE_COMBINER* combiners = nullptr);

	// Set 1x1 (full rate) shading - resets to highest quality.
	void SetFullRate(ID3D12GraphicsCommandList* cmdList);

	// Set reduced shading rate for less important draws (e.g. distant fog, particles).
	void SetReducedRate(ID3D12GraphicsCommandList* cmdList);

	// Build the VRS screen-space image for Tier 2 (image-based VRS).
	// Uses a simple depth-based heuristic: close objects get 1x1, far objects get coarser rates.
	void BuildShadingRateImage(
		ID3D12GraphicsCommandList* cmdList,
		ID3D12Resource* depthBuffer,
		UINT width, UINT height);

	ID3D12Resource* GetShadingRateImage() const { return mShadingRateImage.Get(); }

private:
	bool mSupported = false;
	D3D12_VARIABLE_SHADING_RATE_TIER mTier = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
	UINT mTileSize = 0;

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList5> mCmdList5;
	Microsoft::WRL::ComPtr<ID3D12Resource> mShadingRateImage;
};
