#include "FrameResource.h"
#include "GlobalSettings.hpp"

FrameResource::FrameResource(ID3D12Device *device, UINT passCount, UINT objectCount, UINT materialCount, UINT dungeonVertCount) {
	ThrowIfFailed(device->CreateCommandAllocator(
	    D3D12_COMMAND_LIST_TYPE_DIRECT,
	    IID_PPV_ARGS(CmdListAlloc.GetAddressOf())));

	//  FrameCB = std::make_unique<UploadBuffer<FrameConstants>>(device, 1, true);
	PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	SsaoCB = std::make_unique<UploadBuffer<SsaoConstants>>(device, 1, true);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);

	// DungeonVB = std::make_unique<UploadBuffer<Vertex>>(device, dungeonVertCount, false);

	DungeonVB = std::make_unique<UploadBuffer<Vertex>>(device, MAX_NUM_QUADS, false);
}

FrameResource::~FrameResource() {
}