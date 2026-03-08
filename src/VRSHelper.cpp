//***************************************************************************************
// VRSHelper.cpp - Variable Rate Shading (DX12 Ultimate) implementation
//***************************************************************************************

#include "VRSHelper.h"

bool VRSHelper::Initialize(ID3D12Device5 *device) {
	if (!device)
		return false;

	D3D12_FEATURE_DATA_D3D12_OPTIONS6 options6 = {};
	if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &options6, sizeof(options6)))) {
		mTier = options6.VariableShadingRateTier;
		mSupported = (mTier >= D3D12_VARIABLE_SHADING_RATE_TIER_1);
	}

	if (mSupported) {
		OutputDebugStringA("VRSHelper: Variable Rate Shading Tier 1+ enabled.\n");
	} else {
		OutputDebugStringA("VRSHelper: VRS not supported on this device.\n");
	}

	return mSupported;
}

void VRSHelper::SetShadingRate(ID3D12GraphicsCommandList5 *cmdList, D3D12_SHADING_RATE rate) const {
	if (!mSupported || !cmdList)
		return;

	D3D12_SHADING_RATE_COMBINER combiners[2] = {
		D3D12_SHADING_RATE_COMBINER_PASSTHROUGH,
		D3D12_SHADING_RATE_COMBINER_PASSTHROUGH
	};
	cmdList->RSSetShadingRate(rate, combiners);
}

void VRSHelper::SetFullRate(ID3D12GraphicsCommandList5 *cmdList) const {
	SetShadingRate(cmdList, D3D12_SHADING_RATE_1X1);
}

void VRSHelper::SetReducedRate(ID3D12GraphicsCommandList5 *cmdList) const {
	SetShadingRate(cmdList, D3D12_SHADING_RATE_2X2);
}
