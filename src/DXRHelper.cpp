//***************************************************************************************
// DXRHelper.cpp - DirectX Raytracing (DXR) acceleration structure builder
//***************************************************************************************

#include "DXRHelper.h"

using Microsoft::WRL::ComPtr;

// Helper to create a default-heap UAV buffer for acceleration structures.
static ComPtr<ID3D12Resource> CreateUAVBuffer(ID3D12Device* device, UINT64 size,
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON)
{
	ComPtr<ID3D12Resource> buffer;
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&buffer)));
	return buffer;
}

// Helper to create an upload buffer and map data into it.
static ComPtr<ID3D12Resource> CreateUploadBuffer(ID3D12Device* device, UINT64 size, const void* data)
{
	ComPtr<ID3D12Resource> buffer;
	auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto desc = CD3DX12_RESOURCE_DESC::Buffer(size);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer)));

	if (data) {
		void* mapped = nullptr;
		buffer->Map(0, nullptr, &mapped);
		memcpy(mapped, data, static_cast<size_t>(size));
		buffer->Unmap(0, nullptr);
	}
	return buffer;
}

bool DXRHelper::Initialize(ID3D12Device5* device, D3D12_RAYTRACING_TIER tier) {
	if (!device || tier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
		mSupported = false;
		OutputDebugString(L"[DXR] Raytracing not supported on this device.\n");
		return false;
	}

	mDevice = device;
	mTier = tier;
	mSupported = true;

	std::wstring msg = L"[DXR] Raytracing initialized (Tier " +
		std::to_wstring(static_cast<int>(tier)) + L").\n";
	if (tier >= D3D12_RAYTRACING_TIER_1_1) {
		msg += L"[DXR] Tier 1.1 - Inline Ray Queries (RayQuery) available.\n";
	}
	OutputDebugString(msg.c_str());

	return true;
}

void DXRHelper::BuildAccelerationStructures(
	ID3D12GraphicsCommandList4* cmdList,
	ID3D12Resource* vertexBuffer,
	UINT vertexCount,
	UINT vertexStride,
	ID3D12Resource* indexBuffer,
	UINT indexCount,
	DXGI_FORMAT indexFormat)
{
	if (!mSupported || !mDevice)
		return;

	BuildBLAS(cmdList, vertexBuffer, vertexCount, vertexStride, indexBuffer, indexCount, indexFormat);
	BuildTLAS(cmdList);

	mIsBuilt = true;
	OutputDebugString(L"[DXR] Acceleration structures built.\n");
}

void DXRHelper::BuildBLAS(
	ID3D12GraphicsCommandList4* cmdList,
	ID3D12Resource* vertexBuffer,
	UINT vertexCount,
	UINT vertexStride,
	ID3D12Resource* indexBuffer,
	UINT indexCount,
	DXGI_FORMAT indexFormat)
{
	// Describe the geometry for the BLAS.
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexStride;
	geometryDesc.Triangles.VertexCount = vertexCount;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT; // POSITION is first 3 floats
	geometryDesc.Triangles.IndexBuffer = indexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.IndexCount = indexCount;
	geometryDesc.Triangles.IndexFormat = indexFormat;

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInputs = {};
	blasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	blasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	blasInputs.NumDescs = 1;
	blasInputs.pGeometryDescs = &geometryDesc;
	blasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&blasInputs, &prebuildInfo);

	// Allocate scratch and result buffers.
	mBLASScratch = CreateUAVBuffer(mDevice, prebuildInfo.ScratchDataSizeInBytes,
		D3D12_RESOURCE_STATE_COMMON);
	mBottomLevelAS = CreateUAVBuffer(mDevice, prebuildInfo.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	// Build the BLAS.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc = {};
	blasDesc.Inputs = blasInputs;
	blasDesc.ScratchAccelerationStructureData = mBLASScratch->GetGPUVirtualAddress();
	blasDesc.DestAccelerationStructureData = mBottomLevelAS->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);

	// Barrier to ensure BLAS build completes before TLAS build.
	auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(mBottomLevelAS.Get());
	cmdList->ResourceBarrier(1, &uavBarrier);
}

void DXRHelper::BuildTLAS(ID3D12GraphicsCommandList4* cmdList) {
	// Create the instance descriptor. Single identity instance referencing BLAS.
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = 1.0f;
	instanceDesc.Transform[1][1] = 1.0f;
	instanceDesc.Transform[2][2] = 1.0f;
	instanceDesc.InstanceMask = 0xFF;
	instanceDesc.AccelerationStructure = mBottomLevelAS->GetGPUVirtualAddress();

	// Upload instance desc to GPU.
	mInstanceDescBuffer = CreateUploadBuffer(mDevice, sizeof(instanceDesc), &instanceDesc);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
	tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	tlasInputs.NumDescs = 1;
	tlasInputs.InstanceDescs = mInstanceDescBuffer->GetGPUVirtualAddress();
	tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	mDevice->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &prebuildInfo);

	mTLASScratch = CreateUAVBuffer(mDevice, prebuildInfo.ScratchDataSizeInBytes,
		D3D12_RESOURCE_STATE_COMMON);
	mTopLevelAS = CreateUAVBuffer(mDevice, prebuildInfo.ResultDataMaxSizeInBytes,
		D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
	tlasDesc.Inputs = tlasInputs;
	tlasDesc.ScratchAccelerationStructureData = mTLASScratch->GetGPUVirtualAddress();
	tlasDesc.DestAccelerationStructureData = mTopLevelAS->GetGPUVirtualAddress();

	cmdList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);

	// Barrier to ensure TLAS is ready.
	auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(mTopLevelAS.Get());
	cmdList->ResourceBarrier(1, &uavBarrier);
}

void DXRHelper::CreateTLASSrv(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle) {
	if (!mIsBuilt || !mTopLevelAS)
		return;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.RaytracingAccelerationStructure.Location = mTopLevelAS->GetGPUVirtualAddress();

	device->CreateShaderResourceView(nullptr, &srvDesc, srvHandle);
}
