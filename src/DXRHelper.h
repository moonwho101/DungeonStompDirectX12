#pragma once

#include "../Common/d3dUtil.h"
#include "FrameResource.h"
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

// DXR acceleration structure buffer
struct AccelerationStructureBuffers {
	ComPtr<ID3D12Resource> Scratch;
	ComPtr<ID3D12Resource> Result;
	ComPtr<ID3D12Resource> InstanceDesc; // Only for TLAS
};

// Shader record for shader table entries
struct ShaderRecord {
	void* ShaderIdentifier;
	size_t ShaderIdentifierSize;
	void* LocalRootArguments;
	size_t LocalRootArgumentsSize;
};

// DXR constants matching shader cbuffer
struct DXRSceneConstants {
	DirectX::XMFLOAT4X4 InvViewProj;
	DirectX::XMFLOAT3 CameraPosition;
	float Pad0;
	DirectX::XMFLOAT4 AmbientLight;
	Light Lights[MaxLights];
	UINT NumLights;
	float TotalTime;
	float Roughness;
	float Metallic;
};

class DXRHelper {
public:
	DXRHelper();
	~DXRHelper();

	// Initialize DXR pipeline
	bool Initialize(ID3D12Device5* device, ID3D12GraphicsCommandList5* cmdList,
	                UINT width, UINT height, DXGI_FORMAT backBufferFormat);

	// Build Bottom-Level Acceleration Structure from dynamic vertex buffer
	void BuildBLAS(ID3D12Device5* device, ID3D12GraphicsCommandList5* cmdList,
	               ID3D12Resource* vertexBuffer, UINT vertexCount, UINT vertexStride,
	               bool isUpdate = false);

	// Build Top-Level Acceleration Structure
	void BuildTLAS(ID3D12Device5* device, ID3D12GraphicsCommandList5* cmdList,
	               bool isUpdate = false);

	// Dispatch rays for raytracing
	void DispatchRays(ID3D12GraphicsCommandList5* cmdList, UINT width, UINT height);

	// Update scene constants (camera, lights, materials)
	void UpdateSceneConstants(const DirectX::XMFLOAT4X4& invViewProj,
	                          const DirectX::XMFLOAT3& cameraPos,
	                          const DirectX::XMFLOAT4& ambientLight,
	                          const Light* lights, UINT numLights,
	                          float totalTime, float roughness, float metallic);

	// Copy raytracing output to back buffer
	void CopyOutputToBackBuffer(ID3D12GraphicsCommandList* cmdList,
	                            ID3D12Resource* backBuffer);

	// Handle window resize - recreate output texture
	void OnResize(ID3D12Device* device, UINT width, UINT height);

	// Copy texture descriptors from main app's SRV heap to DXR heap
	void CopyTextureDescriptors(ID3D12Device* device, ID3D12DescriptorHeap* srcHeap, UINT textureCount);

	// Update per-primitive texture indices
	void UpdatePrimitiveTextureIndices(ID3D12Device* device, const UINT* textureIndices, UINT primitiveCount);

	// Update per-primitive normal map texture indices
	void UpdatePrimitiveNormalMapIndices(ID3D12Device* device, const INT* normalMapIndices, UINT primitiveCount);

	// Set current frame resource index (call once per frame before any DXR updates)
	void SetFrameIndex(UINT frameIndex) { mCurrentFrameIndex = frameIndex % kNumFrameResources; }

	// Check if DXR is ready
	bool IsReady() const { return mIsInitialized; }

	// Get output UAV resource
	ID3D12Resource* GetOutputResource() const { return mRaytracingOutput.Get(); }

	// Get descriptor heap for binding
	ID3D12DescriptorHeap* GetDescriptorHeap() const { return mDXRDescriptorHeap.Get(); }

private:
	void CreateRaytracingOutputResource(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format);
	void CreateRootSignatures(ID3D12Device5* device);
	void CreateRaytracingPipelineState(ID3D12Device5* device);
	void BuildShaderTables(ID3D12Device5* device);
	void CreateDescriptorHeap(ID3D12Device5* device);
	void CreateConstantBuffer(ID3D12Device* device);

	ComPtr<ID3DBlob> CompileRaytracingShader(const std::wstring& filename, const wchar_t* entryPoint);

private:
	static const UINT kNumFrameResources = 3;

	bool mIsInitialized = false;
	UINT mWidth = 0;
	UINT mHeight = 0;
	UINT mCurrentFrameIndex = 0;

	// Acceleration structures
	AccelerationStructureBuffers mBLAS;
	AccelerationStructureBuffers mTLAS;

	// Root signatures
	ComPtr<ID3D12RootSignature> mGlobalRootSignature;
	ComPtr<ID3D12RootSignature> mLocalRootSignature;

	// Pipeline state
	ComPtr<ID3D12StateObject> mRaytracingStateObject;
	ComPtr<ID3D12StateObjectProperties> mRaytracingStateObjectProperties;

	// Shader tables
	ComPtr<ID3D12Resource> mRayGenShaderTable;
	ComPtr<ID3D12Resource> mMissShaderTable;
	ComPtr<ID3D12Resource> mHitGroupShaderTable;

	UINT mRayGenShaderTableSize = 0;
	UINT mMissShaderTableSize = 0;
	UINT mHitGroupShaderTableSize = 0;

	// Raytracing output
	ComPtr<ID3D12Resource> mRaytracingOutput;

	// Descriptor heap for DXR resources
	ComPtr<ID3D12DescriptorHeap> mDXRDescriptorHeap;
	UINT mCbvSrvUavDescriptorSize = 0;

	// Scene constant buffer (per-frame to avoid CPU/GPU race)
	ComPtr<ID3D12Resource> mSceneConstantBuffer[kNumFrameResources];
	DXRSceneConstants mSceneConstants;
	UINT8* mSceneCBMappedData[kNumFrameResources] = {};

	// Store vertex buffer reference for BLAS rebuild
	ID3D12Resource* mCurrentVertexBuffer = nullptr;
	UINT mCurrentVertexCount = 0;
	UINT mCurrentVertexStride = 0;

	// Back buffer format for resize
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Texture support
	UINT mTextureStartOffset = 1; // Offset in DXR heap where textures start (after UAV)
	UINT mTextureCount = 0;

	// Per-primitive texture index buffer (per-frame to avoid CPU/GPU race)
	ComPtr<ID3D12Resource> mPrimitiveTextureBuffer[kNumFrameResources];
	UINT8* mPrimitiveTextureMappedData[kNumFrameResources] = {};
	UINT mMaxPrimitives[kNumFrameResources] = {};

	// Per-primitive normal map index buffer (per-frame to avoid CPU/GPU race)
	ComPtr<ID3D12Resource> mPrimitiveNormalMapBuffer[kNumFrameResources];
	UINT8* mPrimitiveNormalMapMappedData[kNumFrameResources] = {};
	UINT mMaxNormalMapPrimitives[kNumFrameResources] = {};

	// Shader identifiers
	static const wchar_t* kRayGenShader;
	static const wchar_t* kMissShader;
	static const wchar_t* kClosestHitShader;
	static const wchar_t* kHitGroup;
};
