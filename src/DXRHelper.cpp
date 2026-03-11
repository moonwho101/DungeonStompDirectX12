//***************************************************************************************
// DXRHelper.cpp - DirectX Raytracing Helper Implementation
// Provides DXR acceleration structure building and ray dispatch functionality
//***************************************************************************************

#include "DXRHelper.h"
#include "GlobalSettings.hpp"
#include <d3dcompiler.h>
#include <dxcapi.h>
#include <vector>

using namespace DirectX;

// Shader export names
const wchar_t* DXRHelper::kRayGenShader = L"RayGen";
const wchar_t* DXRHelper::kMissShader = L"Miss";
const wchar_t* DXRHelper::kClosestHitShader = L"ClosestHit";
const wchar_t* DXRHelper::kHitGroup = L"HitGroup";

// Align to shader table record alignment (D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT)
inline UINT Align(UINT size, UINT alignment) {
	return (size + (alignment - 1)) & ~(alignment - 1);
}

DXRHelper::DXRHelper() {
}

DXRHelper::~DXRHelper() {
	for (UINT i = 0; i < kNumFrameResources; ++i) {
		if (mSceneCBMappedData[i] && mSceneConstantBuffer[i]) {
			mSceneConstantBuffer[i]->Unmap(0, nullptr);
			mSceneCBMappedData[i] = nullptr;
		}
	}
}

bool DXRHelper::Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList5* cmdList,
                           UINT width, UINT height, DXGI_FORMAT backBufferFormat) {
	mWidth = width;
	mHeight = height;

	// Check DXR support
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
	if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
		OutputDebugStringA("DXR: Failed to query Options5 features.\n");
		return false;
	}

	if (options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED) {
		OutputDebugStringA("DXR: Raytracing not supported on this device.\n");
		return false;
	}

	OutputDebugStringA("DXR: Raytracing supported! Initializing...\n");

	mCbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	CreateDescriptorHeap(device);
	CreateConstantBuffer(device);
	CreateRaytracingOutputResource(device, width, height, backBufferFormat);
	CreateRootSignatures(device);
	CreateRaytracingPipelineState(device);
	BuildShaderTables(device);

	mBackBufferFormat = backBufferFormat;
	mIsInitialized = true;
	OutputDebugStringA("DXR: Initialization complete!\n");
	return true;
}

void DXRHelper::CreateDescriptorHeap(ID3D12Device5* device) {
	// Create descriptor heap for DXR resources
	// Layout: 0 = Output UAV, 1-550 = Texture SRVs (copied from main heap)
	// Total: 1 + MAX_NUM_TEXTURES (550) descriptors
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1 + 550; // UAV + textures
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mDXRDescriptorHeap)));
	
	mTextureStartOffset = 1; // Textures start after UAV
}

void DXRHelper::CreateConstantBuffer(ID3D12Device* device) {
	UINT cbSize = d3dUtil::CalcConstantBufferByteSize(sizeof(DXRSceneConstants));

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);

	for (UINT i = 0; i < kNumFrameResources; ++i) {
		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps,
		    D3D12_HEAP_FLAG_NONE,
		    &bufferDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ,
		    nullptr,
		    IID_PPV_ARGS(&mSceneConstantBuffer[i])));

		ThrowIfFailed(mSceneConstantBuffer[i]->Map(0, nullptr, reinterpret_cast<void**>(&mSceneCBMappedData[i])));
	}

	// Create CBV in descriptor heap (slot 2) — points to frame 0 initially;
	// DispatchRays binds the correct frame's buffer via root CBV each frame.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = mSceneConstantBuffer[0]->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cbSize;

	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvHandle(mDXRDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	cbvHandle.Offset(2, mCbvSrvUavDescriptorSize);
	device->CreateConstantBufferView(&cbvDesc, cbvHandle);
}

void DXRHelper::CreateRaytracingOutputResource(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format) {
	// Create UAV for raytracing output
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(device->CreateCommittedResource(
	    &heapProps,
	    D3D12_HEAP_FLAG_NONE,
	    &texDesc,
	    D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
	    nullptr,
	    IID_PPV_ARGS(&mRaytracingOutput)));

	// Create UAV in descriptor heap (slot 0)
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	device->CreateUnorderedAccessView(mRaytracingOutput.Get(), nullptr, &uavDesc,
	                                  mDXRDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
}

void DXRHelper::CreateRootSignatures(ID3D12Device5* device) {
	// Global root signature
	// Slot 0: Output UAV (u0) - descriptor table
	// Slot 1: Acceleration Structure SRV (t0) - inline
	// Slot 2: Scene CBV (b0) - inline
	// Slot 3: Vertex Buffer SRV (t1) - inline
	// Slot 4: Texture Array SRV (t2-t551) - descriptor table
	// Slot 5: Primitive Texture Indices (t0, space1) - inline
	// Slot 6: Primitive Normal Map Indices (t1, space1) - inline
	CD3DX12_DESCRIPTOR_RANGE1 uavRange;
	uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // u0

	// Texture array range - 550 textures starting at t2
	CD3DX12_DESCRIPTOR_RANGE1 textureRange;
	textureRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 550, 2); // t2-t551

	CD3DX12_ROOT_PARAMETER1 rootParams[7];
	rootParams[0].InitAsDescriptorTable(1, &uavRange);                              // Output UAV
	rootParams[1].InitAsShaderResourceView(0);                                      // TLAS (t0)
	rootParams[2].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE); // Scene CB (b0)
	rootParams[3].InitAsShaderResourceView(1);                                      // Vertex buffer (t1)
	rootParams[4].InitAsDescriptorTable(1, &textureRange);                          // Texture array (t2+)
	rootParams[5].InitAsShaderResourceView(0, 1);                                   // Primitive indices (t0, space1)
	rootParams[6].InitAsShaderResourceView(1, 1);                                   // Normal map indices (t1, space1)

	// Static sampler for texture sampling
	D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
	samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 8;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	samplerDesc.ShaderRegister = 0;
	samplerDesc.RegisterSpace = 0;
	samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
	rootSigDesc.Init_1_1(_countof(rootParams), rootParams, 1, &samplerDesc, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &signature, &error);
	if (FAILED(hr)) {
		if (error) OutputDebugStringA((char*)error->GetBufferPointer());
		ThrowIfFailed(hr);
	}

	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
	                                          IID_PPV_ARGS(&mGlobalRootSignature)));

	// Local root signature (empty for now - no per-hit data)
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC localRootSigDesc;
	localRootSigDesc.Init_1_1(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	hr = D3D12SerializeVersionedRootSignature(&localRootSigDesc, &signature, &error);
	if (FAILED(hr)) {
		if (error) OutputDebugStringA((char*)error->GetBufferPointer());
		ThrowIfFailed(hr);
	}

	ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
	                                          IID_PPV_ARGS(&mLocalRootSignature)));
}

ComPtr<ID3DBlob> DXRHelper::CompileRaytracingShader(const std::wstring& filename, const wchar_t* entryPoint) {
	// Use DXC for raytracing library compilation
	ComPtr<IDxcLibrary> library;
	ComPtr<IDxcCompiler> compiler;
	ComPtr<IDxcBlobEncoding> sourceBlob;
	ComPtr<IDxcOperationResult> result;

	ThrowIfFailed(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
	ThrowIfFailed(library->CreateBlobFromFile(filename.c_str(), nullptr, &sourceBlob));

	// Compile as library (lib_6_5) - shader model 6.5 for DXR 1.1 inline raytracing
	std::vector<LPCWSTR> args;
#ifdef _DEBUG
	args.push_back(L"-Zi");  // Debug info
	args.push_back(L"-Od");  // Disable optimization
#else
	args.push_back(L"-O3");  // Optimize
#endif

	ThrowIfFailed(compiler->Compile(
	    sourceBlob.Get(),
	    filename.c_str(),
	    L"",            // Entry point (empty for library)
	    L"lib_6_5",     // Target profile
	    args.data(),
	    (UINT)args.size(),
	    nullptr,
	    0,
	    nullptr,
	    &result));

	HRESULT hr;
	result->GetStatus(&hr);

	if (FAILED(hr)) {
		ComPtr<IDxcBlobEncoding> errors;
		result->GetErrorBuffer(&errors);
		if (errors && errors->GetBufferSize() > 0) {
			OutputDebugStringA("DXR Shader Compilation Errors:\n");
			OutputDebugStringA((char*)errors->GetBufferPointer());
		}
		ThrowIfFailed(hr);
	}

	ComPtr<IDxcBlob> shaderBlob;
	result->GetResult(&shaderBlob);

	// Convert to ID3DBlob
	ComPtr<ID3DBlob> blob;
	D3DCreateBlob(shaderBlob->GetBufferSize(), &blob);
	memcpy(blob->GetBufferPointer(), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize());

	return blob;
}

void DXRHelper::CreateRaytracingPipelineState(ID3D12Device5* device) {
	// Compile raytracing shader library
	ComPtr<ID3DBlob> rtShaderBlob = CompileRaytracingShader(L"../Shaders/Raytracing.hlsl", nullptr);

	// Build state object
	CD3DX12_STATE_OBJECT_DESC stateObjectDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

	// DXIL Library
	auto lib = stateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	D3D12_SHADER_BYTECODE libDxil = { rtShaderBlob->GetBufferPointer(), rtShaderBlob->GetBufferSize() };
	lib->SetDXILLibrary(&libDxil);
	lib->DefineExport(kRayGenShader);
	lib->DefineExport(kMissShader);
	lib->DefineExport(kClosestHitShader);

	// Hit group
	auto hitGroup = stateObjectDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	hitGroup->SetClosestHitShaderImport(kClosestHitShader);
	hitGroup->SetHitGroupExport(kHitGroup);
	hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

	// Shader config
	auto shaderConfig = stateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT payloadSize = sizeof(XMFLOAT4) + sizeof(UINT); // float4 color + uint depth
	UINT attributeSize = sizeof(XMFLOAT2); // Barycentrics
	shaderConfig->Config(payloadSize, attributeSize);

	// Local root signature for hit group (empty)
	auto localRootSig = stateObjectDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
	localRootSig->SetRootSignature(mLocalRootSignature.Get());

	auto localRootSigAssoc = stateObjectDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
	localRootSigAssoc->SetSubobjectToAssociate(*localRootSig);
	localRootSigAssoc->AddExport(kHitGroup);

	// Global root signature
	auto globalRootSig = stateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	globalRootSig->SetRootSignature(mGlobalRootSignature.Get());

	// Pipeline config
	auto pipelineConfig = stateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	UINT maxRecursionDepth = 5; // Primary rays + up to 4 transparency continuation rays
	pipelineConfig->Config(maxRecursionDepth);

	// Create state object
	ThrowIfFailed(device->CreateStateObject(stateObjectDesc, IID_PPV_ARGS(&mRaytracingStateObject)));
	ThrowIfFailed(mRaytracingStateObject->QueryInterface(IID_PPV_ARGS(&mRaytracingStateObjectProperties)));
}

void DXRHelper::BuildShaderTables(ID3D12Device5* device) {
	UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	void* rayGenShaderIdentifier = mRaytracingStateObjectProperties->GetShaderIdentifier(kRayGenShader);
	void* missShaderIdentifier = mRaytracingStateObjectProperties->GetShaderIdentifier(kMissShader);
	void* hitGroupIdentifier = mRaytracingStateObjectProperties->GetShaderIdentifier(kHitGroup);

	// Ray generation shader table
	{
		UINT recordSize = Align(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		mRayGenShaderTableSize = recordSize;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mRayGenShaderTableSize);

		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mRayGenShaderTable)));

		void* pData;
		mRayGenShaderTable->Map(0, nullptr, &pData);
		memcpy(pData, rayGenShaderIdentifier, shaderIdentifierSize);
		mRayGenShaderTable->Unmap(0, nullptr);
	}

	// Miss shader table
	{
		UINT recordSize = Align(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		mMissShaderTableSize = recordSize;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mMissShaderTableSize);

		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mMissShaderTable)));

		void* pData;
		mMissShaderTable->Map(0, nullptr, &pData);
		memcpy(pData, missShaderIdentifier, shaderIdentifierSize);
		mMissShaderTable->Unmap(0, nullptr);
	}

	// Hit group shader table
	{
		UINT recordSize = Align(shaderIdentifierSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
		mHitGroupShaderTableSize = recordSize;

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(mHitGroupShaderTableSize);

		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mHitGroupShaderTable)));

		void* pData;
		mHitGroupShaderTable->Map(0, nullptr, &pData);
		memcpy(pData, hitGroupIdentifier, shaderIdentifierSize);
		mHitGroupShaderTable->Unmap(0, nullptr);
	}
}

void DXRHelper::BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList5* cmdList,
                          ID3D12Resource* vertexBuffer, UINT vertexCount, UINT vertexStride,
                          bool isUpdate) {
	// Force a full rebuild if the vertex count changed — DXR updates require
	// identical geometry topology.
	if (isUpdate && vertexCount != mCurrentVertexCount) {
		isUpdate = false;
	}

	// Store for rebuild
	mCurrentVertexBuffer = vertexBuffer;
	mCurrentVertexCount = vertexCount;
	mCurrentVertexStride = vertexStride;

	// Geometry description - triangle list (3 verts per triangle)
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
	geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->GetGPUVirtualAddress();
	geometryDesc.Triangles.VertexBuffer.StrideInBytes = vertexStride;
	geometryDesc.Triangles.VertexCount = vertexCount;
	geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT; // Position is first 3 floats
	geometryDesc.Triangles.IndexBuffer = 0; // No index buffer
	geometryDesc.Triangles.IndexCount = 0;
	geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
	geometryDesc.Triangles.Transform3x4 = 0;
	geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

	// Build inputs
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geometryDesc;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD |
	               D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	// Allocate scratch/result buffers once, sized for the worst-case geometry
	// (MAX_NUM_QUADS vertices).  This avoids ever reallocating while the GPU
	// may still reference the old buffers from in-flight frames.
	if (!mBLAS.Scratch) {
		// Query prebuild info for the maximum possible geometry size so the
		// buffers are guaranteed large enough for any future vertex count.
		D3D12_RAYTRACING_GEOMETRY_DESC maxGeomDesc = geometryDesc;
		maxGeomDesc.Triangles.VertexCount = MAX_NUM_QUADS;

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS maxInputs = inputs;
		maxInputs.pGeometryDescs = &maxGeomDesc;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO maxPrebuild = {};
		device->GetRaytracingAccelerationStructurePrebuildInfo(&maxInputs, &maxPrebuild);

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
		    maxPrebuild.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps, D3D12_HEAP_FLAG_NONE, &scratchDesc,
		    D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mBLAS.Scratch)));

		CD3DX12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(
		    maxPrebuild.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps, D3D12_HEAP_FLAG_NONE, &resultDesc,
		    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&mBLAS.Result)));
	}

	// Build BLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = mBLAS.Scratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = mBLAS.Result->GetGPUVirtualAddress();

	if (isUpdate) {
		buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		buildDesc.SourceAccelerationStructureData = mBLAS.Result->GetGPUVirtualAddress();
	}

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// UAV barrier
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(mBLAS.Result.Get());
	cmdList->ResourceBarrier(1, &barrier);
}

void DXRHelper::BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList5* cmdList, bool isUpdate) {
	// Instance description
	D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
	instanceDesc.Transform[0][0] = 1.0f;
	instanceDesc.Transform[1][1] = 1.0f;
	instanceDesc.Transform[2][2] = 1.0f;
	instanceDesc.InstanceID = 0;
	instanceDesc.InstanceMask = 1;
	instanceDesc.InstanceContributionToHitGroupIndex = 0;
	instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
	instanceDesc.AccelerationStructure = mBLAS.Result->GetGPUVirtualAddress();

	// Allocate instance buffer once — size is fixed (1 instance).
	if (!mTLAS.InstanceDesc) {
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps, D3D12_HEAP_FLAG_NONE, &bufferDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mTLAS.InstanceDesc)));
	}

	// Copy instance data
	void* pData;
	mTLAS.InstanceDesc->Map(0, nullptr, &pData);
	memcpy(pData, &instanceDesc, sizeof(instanceDesc));
	mTLAS.InstanceDesc->Unmap(0, nullptr);

	// Build inputs
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.NumDescs = 1;
	inputs.InstanceDescs = mTLAS.InstanceDesc->GetGPUVirtualAddress();
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD |
	               D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

	// Get prebuild info
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
	device->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &prebuildInfo);

	// Allocate scratch/result buffers once.  TLAS with 1 instance has a
	// fixed size, so no reallocation is ever needed.
	if (!mTLAS.Scratch) {
		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

		CD3DX12_RESOURCE_DESC scratchDesc = CD3DX12_RESOURCE_DESC::Buffer(
		    prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps, D3D12_HEAP_FLAG_NONE, &scratchDesc,
		    D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&mTLAS.Scratch)));

		CD3DX12_RESOURCE_DESC resultDesc = CD3DX12_RESOURCE_DESC::Buffer(
		    prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps, D3D12_HEAP_FLAG_NONE, &resultDesc,
		    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&mTLAS.Result)));
	}

	// Build TLAS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
	buildDesc.Inputs = inputs;
	buildDesc.ScratchAccelerationStructureData = mTLAS.Scratch->GetGPUVirtualAddress();
	buildDesc.DestAccelerationStructureData = mTLAS.Result->GetGPUVirtualAddress();

	if (isUpdate) {
		buildDesc.Inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
		buildDesc.SourceAccelerationStructureData = mTLAS.Result->GetGPUVirtualAddress();
	}

	cmdList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

	// UAV barrier
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(mTLAS.Result.Get());
	cmdList->ResourceBarrier(1, &barrier);
}

void DXRHelper::UpdateSceneConstants(const DirectX::XMFLOAT4X4& invViewProj,
                                     const DirectX::XMFLOAT3& cameraPos,
                                     const DirectX::XMFLOAT4& ambientLight,
                                     const Light* lights, UINT numLights,
                                     float totalTime, float roughness, float metallic) {
	mSceneConstants.InvViewProj = invViewProj;
	mSceneConstants.CameraPosition = cameraPos;
	mSceneConstants.AmbientLight = ambientLight;
	mSceneConstants.NumLights = min(numLights, MaxLights);
	mSceneConstants.TotalTime = totalTime;
	mSceneConstants.Roughness = roughness;
	mSceneConstants.Metallic = metallic;

	for (UINT i = 0; i < mSceneConstants.NumLights; ++i) {
		mSceneConstants.Lights[i] = lights[i];
	}

	memcpy(mSceneCBMappedData[mCurrentFrameIndex], &mSceneConstants, sizeof(DXRSceneConstants));
}

void DXRHelper::DispatchRays(ID3D12GraphicsCommandList5* cmdList, UINT width, UINT height) {
	// Set pipeline state
	cmdList->SetComputeRootSignature(mGlobalRootSignature.Get());
	cmdList->SetPipelineState1(mRaytracingStateObject.Get());

	// Set descriptor heaps
	ID3D12DescriptorHeap* heaps[] = { mDXRDescriptorHeap.Get() };
	cmdList->SetDescriptorHeaps(1, heaps);

	// Set root arguments
	cmdList->SetComputeRootDescriptorTable(0, mDXRDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); // UAV
	cmdList->SetComputeRootShaderResourceView(1, mTLAS.Result->GetGPUVirtualAddress()); // TLAS
	cmdList->SetComputeRootConstantBufferView(2, mSceneConstantBuffer[mCurrentFrameIndex]->GetGPUVirtualAddress()); // Scene CB
	if (mCurrentVertexBuffer) {
		cmdList->SetComputeRootShaderResourceView(3, mCurrentVertexBuffer->GetGPUVirtualAddress()); // Vertex buffer
	}
	
	// Set texture array descriptor table (slot 4) - starts at offset 1 in DXR heap
	if (mTextureCount > 0) {
		CD3DX12_GPU_DESCRIPTOR_HANDLE texHandle(mDXRDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		texHandle.Offset(mTextureStartOffset, mCbvSrvUavDescriptorSize);
		cmdList->SetComputeRootDescriptorTable(4, texHandle);
	}
	
	// Set primitive texture indices buffer (slot 5)
	if (mPrimitiveTextureBuffer[mCurrentFrameIndex]) {
		cmdList->SetComputeRootShaderResourceView(5, mPrimitiveTextureBuffer[mCurrentFrameIndex]->GetGPUVirtualAddress());
	}

	// Set primitive normal map indices buffer (slot 6)
	if (mPrimitiveNormalMapBuffer[mCurrentFrameIndex]) {
		cmdList->SetComputeRootShaderResourceView(6, mPrimitiveNormalMapBuffer[mCurrentFrameIndex]->GetGPUVirtualAddress());
	}

	// Dispatch rays
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	dispatchDesc.RayGenerationShaderRecord.StartAddress = mRayGenShaderTable->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = mRayGenShaderTableSize;
	dispatchDesc.MissShaderTable.StartAddress = mMissShaderTable->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes = mMissShaderTableSize;
	dispatchDesc.MissShaderTable.StrideInBytes = mMissShaderTableSize;
	dispatchDesc.HitGroupTable.StartAddress = mHitGroupShaderTable->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes = mHitGroupShaderTableSize;
	dispatchDesc.HitGroupTable.StrideInBytes = mHitGroupShaderTableSize;
	dispatchDesc.Width = width;
	dispatchDesc.Height = height;
	dispatchDesc.Depth = 1;

	cmdList->DispatchRays(&dispatchDesc);
}

void DXRHelper::CopyOutputToBackBuffer(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* backBuffer) {
	// Transition output to copy source
	D3D12_RESOURCE_BARRIER barriers[2];
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
	    mRaytracingOutput.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
	    backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->ResourceBarrier(2, barriers);

	// Copy
	cmdList->CopyResource(backBuffer, mRaytracingOutput.Get());

	// Transition back
	barriers[0] = CD3DX12_RESOURCE_BARRIER::Transition(
	    mRaytracingOutput.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	barriers[1] = CD3DX12_RESOURCE_BARRIER::Transition(
	    backBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->ResourceBarrier(2, barriers);
}

void DXRHelper::OnResize(ID3D12Device* device, UINT width, UINT height) {
	if (!mIsInitialized || (width == mWidth && height == mHeight))
		return;

	mWidth = width;
	mHeight = height;

	// Release old output resource
	mRaytracingOutput.Reset();

	// Recreate output resource with new dimensions
	CreateRaytracingOutputResource(device, width, height, mBackBufferFormat);

	char buf[256];
	sprintf_s(buf, "DXR: Resized output to %u x %u\n", width, height);
	OutputDebugStringA(buf);
}

void DXRHelper::CopyTextureDescriptors(ID3D12Device* device, ID3D12DescriptorHeap* srcHeap, UINT textureCount) {
	if (!srcHeap || textureCount == 0)
		return;

	mTextureCount = textureCount;

	// Get descriptor handles
	CD3DX12_CPU_DESCRIPTOR_HANDLE destHandle(mDXRDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	destHandle.Offset(mTextureStartOffset, mCbvSrvUavDescriptorSize); // Skip UAV slot

	CD3DX12_CPU_DESCRIPTOR_HANDLE srcHandle(srcHeap->GetCPUDescriptorHandleForHeapStart());

	// Copy all texture descriptors from source heap to DXR heap
	device->CopyDescriptorsSimple(
	    textureCount,
	    destHandle,
	    srcHandle,
	    D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	char buf[256];
	sprintf_s(buf, "DXR: Copied %u texture descriptors to DXR heap\n", textureCount);
	OutputDebugStringA(buf);
	
	// Initialize primitive texture indices buffers for all frames with default values (texture 0)
	// This ensures the shader has valid data until proper per-primitive mapping is set up
	UINT defaultPrimitiveCount = 200000; // Max expected primitives
	std::vector<UINT> defaultIndices(defaultPrimitiveCount, 0); // All primitives use texture 0
	UINT savedFrame = mCurrentFrameIndex;
	for (UINT i = 0; i < kNumFrameResources; ++i) {
		mCurrentFrameIndex = i;
		UpdatePrimitiveTextureIndices(device, defaultIndices.data(), defaultPrimitiveCount);
	}

	// Initialize primitive normal map indices buffers for all frames with -1 (no normal map)
	std::vector<INT> defaultNormalMapIndices(defaultPrimitiveCount, -1);
	for (UINT i = 0; i < kNumFrameResources; ++i) {
		mCurrentFrameIndex = i;
		UpdatePrimitiveNormalMapIndices(device, defaultNormalMapIndices.data(), defaultPrimitiveCount);
	}
	mCurrentFrameIndex = savedFrame;
}

void DXRHelper::UpdatePrimitiveTextureIndices(ID3D12Device* device, const UINT* textureIndices, UINT primitiveCount) {
	UINT fi = mCurrentFrameIndex;

	// Create or resize the primitive texture index buffer for this frame if needed
	if (!mPrimitiveTextureBuffer[fi] || primitiveCount > mMaxPrimitives[fi]) {
		// Release old buffer
		if (mPrimitiveTextureMappedData[fi]) {
			mPrimitiveTextureBuffer[fi]->Unmap(0, nullptr);
			mPrimitiveTextureMappedData[fi] = nullptr;
		}
		mPrimitiveTextureBuffer[fi].Reset();

		// Create new buffer with some headroom
		mMaxPrimitives[fi] = max(primitiveCount, mMaxPrimitives[fi] * 2);
		if (mMaxPrimitives[fi] == 0) mMaxPrimitives[fi] = 65536;

		UINT bufferSize = mMaxPrimitives[fi] * sizeof(UINT);

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps,
		    D3D12_HEAP_FLAG_NONE,
		    &bufferDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ,
		    nullptr,
		    IID_PPV_ARGS(&mPrimitiveTextureBuffer[fi])));

		// Map the buffer
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(mPrimitiveTextureBuffer[fi]->Map(0, &readRange, reinterpret_cast<void**>(&mPrimitiveTextureMappedData[fi])));
	}

	// Copy texture indices
	if (mPrimitiveTextureMappedData[fi] && textureIndices) {
		memcpy(mPrimitiveTextureMappedData[fi], textureIndices, primitiveCount * sizeof(UINT));
	}
}

void DXRHelper::UpdatePrimitiveNormalMapIndices(ID3D12Device* device, const INT* normalMapIndices, UINT primitiveCount) {
	UINT fi = mCurrentFrameIndex;

	// Create or resize the normal map index buffer for this frame if needed
	if (!mPrimitiveNormalMapBuffer[fi] || primitiveCount > mMaxNormalMapPrimitives[fi]) {
		// Release old buffer
		if (mPrimitiveNormalMapMappedData[fi]) {
			mPrimitiveNormalMapBuffer[fi]->Unmap(0, nullptr);
			mPrimitiveNormalMapMappedData[fi] = nullptr;
		}
		mPrimitiveNormalMapBuffer[fi].Reset();

		// Create new buffer with some headroom
		mMaxNormalMapPrimitives[fi] = max(primitiveCount, mMaxNormalMapPrimitives[fi] * 2);
		if (mMaxNormalMapPrimitives[fi] == 0) mMaxNormalMapPrimitives[fi] = 65536;

		UINT bufferSize = mMaxNormalMapPrimitives[fi] * sizeof(INT);

		CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

		ThrowIfFailed(device->CreateCommittedResource(
		    &heapProps,
		    D3D12_HEAP_FLAG_NONE,
		    &bufferDesc,
		    D3D12_RESOURCE_STATE_GENERIC_READ,
		    nullptr,
		    IID_PPV_ARGS(&mPrimitiveNormalMapBuffer[fi])));

		// Map the buffer
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(mPrimitiveNormalMapBuffer[fi]->Map(0, &readRange, reinterpret_cast<void**>(&mPrimitiveNormalMapMappedData[fi])));
	}

	// Copy normal map indices
	if (mPrimitiveNormalMapMappedData[fi] && normalMapIndices) {
		memcpy(mPrimitiveNormalMapMappedData[fi], normalMapIndices, primitiveCount * sizeof(INT));
	}
}
