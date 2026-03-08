//***************************************************************************************
// VRSHelper.h - Variable Rate Shading (DX12 Ultimate) helper
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"

// Thin wrapper around ID3D12GraphicsCommandList5 VRS Tier 1 (per-draw shading rate).
// Tier 1 VRS allows setting a single shading rate per draw call.
class VRSHelper {
  public:
	VRSHelper() = default;

	// Call once after device creation. Returns true if VRS Tier 1+ is available.
	bool Initialize(ID3D12Device5 *device);

	// Set shading rate on the command list for subsequent draw calls.
	// Requires ID3D12GraphicsCommandList5 (DX12 Ultimate).
	void SetShadingRate(ID3D12GraphicsCommandList5 *cmdList, D3D12_SHADING_RATE rate) const;

	// Convenience: set full-rate shading (1x1).
	void SetFullRate(ID3D12GraphicsCommandList5 *cmdList) const;

	// Convenience: set reduced-rate shading (2x2) for performance.
	void SetReducedRate(ID3D12GraphicsCommandList5 *cmdList) const;

	bool IsSupported() const { return mSupported; }
	D3D12_VARIABLE_SHADING_RATE_TIER Tier() const { return mTier; }

  private:
	bool mSupported = false;
	D3D12_VARIABLE_SHADING_RATE_TIER mTier = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
};
