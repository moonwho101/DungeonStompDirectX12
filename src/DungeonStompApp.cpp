//***************************************************************************************
// DungeonStompApp.cpp by Mark Longo (C) 2022 All Rights Reserved.
//***************************************************************************************

#include "../Common/d3dApp.h"
#include "../Common/MathHelper.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include "Dungeon.h"
#include <d3dtypes.h>
#include "world.hpp"
#include "GlobalSettings.hpp"
#include "Missle.hpp"
#include "GameLogic.hpp"
#include "DungeonStomp.hpp"
#include "Ssao.h"
#include "CameraBob.hpp"
#include <vector>

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")

// External variable declarations
extern PLAYER player_list[1];
extern OBJECTLIST* oblist;
extern int oblist_length;
extern D3DVERTEX2* src_v;
extern int cnt;
extern POLY_SORT ObjectsToDraw[MAX_NUM_QUADS];
extern int number_of_polys_per_frame;
extern int* texture_list_buffer;
extern TEXTUREMAPPING TexMap[MAX_NUM_TEXTURES];
extern int* verts_per_poly;
extern D3DPRIMITIVETYPE* dp_commands;
extern BOOL* dp_command_index_mode;
extern int playerGunObjectStart;
extern int playerObjectStart;
extern int playerObjectEnd;
extern int trueplayernum;
extern XMFLOAT3 m_vEyePt;
extern D3DVALUE angy;
extern D3DVALUE look_up_ang;
extern int playercurrentmove;
extern XMFLOAT3 GunTruesave;
extern SCROLLINFO gScrollText[MAX_SCROLL_TEXT];
extern int gCurrentScrollText;
extern int gMaxScrollText;
extern char gActionMessage[2048];
extern int displayCapture;
extern int displayCaptureCount[1000];
extern int displayCaptureIndex[1000];
extern int gravityon;
extern int outside;
extern void ScanMod(float);
extern void DisplayHud();
extern void SetDungeonText();
extern void DisplayPlayerCaption();
extern void UpdateScrollList(unsigned char r, unsigned char g, unsigned char b);
extern void InitDS();
extern void UpdateControls();
extern HRESULT FrameMove(double fTime, FLOAT fTimeKey);
extern void UpdateWorld(float fElapsedTime);
extern OBJECTDATA* obdata;
extern int obdata_length;
extern int* num_polys_per_object;
extern float k;
extern int MaxRectangle;
extern int MAX_NUM_QUADS;
extern Font arialFont;
extern GUNLIST* your_gun;
extern int current_gun;
extern int number_of_tex_aliases;
extern ID3D12PipelineState* textPSO;
extern ID3D12PipelineState* rectanglePSO[MaxRectangle];


Font LoadFont(LPCWSTR filename, int windowWidth, int windowHeight);

// Forward declarations for DXR geometry preprocessing helpers
// Adapting existing CalculateTangentBinormal from ProcessModel.cpp for use here if needed.
// For simplicity, new helper functions will be defined if direct adaptation is too complex.
static void CalculateFaceNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
static void ConvertTriangleStripForPreprocess(const std::vector<Vertex>& polygonVertices, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, UINT& baseVertexIndex)
{
    if (polygonVertices.size() < 3) return;

    for (size_t i = 0; i <= polygonVertices.size() - 3; ++i)
    {
        Vertex v0 = polygonVertices[i];
        Vertex v1 = polygonVertices[i + 1];
        Vertex v2 = polygonVertices[i + 2];

        if ((i % 2) == 1) // For odd triangles, reverse order to maintain winding
        {
            std::swap(v1, v2);
        }

        outVertices.push_back(v0);
        outVertices.push_back(v1);
        outVertices.push_back(v2);

        outIndices.push_back(baseVertexIndex);
        outIndices.push_back(baseVertexIndex + 1);
        outIndices.push_back(baseVertexIndex + 2);
        baseVertexIndex += 3;
    }
}

static void ConvertTriangleFanForPreprocess(const std::vector<Vertex>& polygonVertices, std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, UINT& baseVertexIndex)
{
    if (polygonVertices.size() < 3) return;

    const Vertex& commonVertex = polygonVertices[0];
    for (size_t i = 1; i < polygonVertices.size() - 1; ++i)
    {
        outVertices.push_back(commonVertex);
        outVertices.push_back(polygonVertices[i]);
        outVertices.push_back(polygonVertices[i + 1]);

        outIndices.push_back(baseVertexIndex);
        outIndices.push_back(baseVertexIndex + 1);
        outIndices.push_back(baseVertexIndex + 2);
        baseVertexIndex += 3;
    }
}

// Basic face normal calculation. Does not smooth.
// Assumes vertices are already in the correct (local) space.
static void CalculateFaceNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
{
    for (size_t i = 0; i < indices.size(); i += 3)
    {
        Vertex& v0 = vertices[indices[i]];
        Vertex& v1 = vertices[indices[i + 1]];
        Vertex& v2 = vertices[indices[i + 2]];

        XMVECTOR p0 = XMLoadFloat3(&v0.Pos);
        XMVECTOR p1 = XMLoadFloat3(&v1.Pos);
        XMVECTOR p2 = XMLoadFloat3(&v2.Pos);

        XMVECTOR e0 = XMVectorSubtract(p1, p0);
        XMVECTOR e1 = XMVectorSubtract(p2, p0);
        XMVECTOR faceNormal = XMVector3Normalize(XMVector3Cross(e0, e1));

        // For flat shading, assign same normal to all three vertices of the triangle
        XMStoreFloat3(&v0.Normal, faceNormal);
        XMStoreFloat3(&v1.Normal, faceNormal);
        XMStoreFloat3(&v2.Normal, faceNormal);

        // Basic Tangent calculation (can be improved, e.g. using MikkTSpace)
        // This is a simplified version.
        XMFLOAT3 tangent = {1.0f, 0.0f, 0.0f}; // Default tangent
        // More robust tangent calculation would involve UVs
        XMVECTOR t = XMLoadFloat3(&tangent);

        XMStoreFloat3(&v0.TangentU, t);
        XMStoreFloat3(&v1.TangentU, t);
        XMStoreFloat3(&v2.TangentU, t);
    }
}


void DungeonStompApp::CheckRaytracingSupport()
{
    HRESULT hr = md3dDevice.As(&mDxrDevice);
    if(FAILED(hr))
    {
        OutputDebugString(L"***ERROR: Failed to query ID3D12Device5. DXR support is unavailable.\n");
        mRaytracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
        mDxrDevice.Reset();
        return;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    ThrowIfFailed(mDxrDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)));
    mRaytracingTier = options5.RaytracingTier;

    if (mRaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    {
        OutputDebugString(L"***WARNING: DXR Tier 1.0 or higher is not supported on this hardware. Raytracing will be disabled.\n");
        mDxrDevice.Reset();
    }
    else
    {
        OutputDebugString(L"***DXR Tier 1.0+ is supported.\n");
        hr = mCommandList.As(&mDxrCommandList);
        if(FAILED(hr)) {
            OutputDebugString(L"***ERROR: Failed to query ID3D12GraphicsCommandList4. DXR support is unavailable.\n");
            mRaytracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
            mDxrDevice.Reset();
            mDxrCommandList.Reset();
        } else {
            OutputDebugString(L"***ID3D12GraphicsCommandList4 (or higher) is available.\n");
        }
    }
}

void DungeonStompApp::PreprocessStaticGeometriesForDXR()
{
    if (mRaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED || !mDxrDevice)
    {
        OutputDebugString(L"Skipping static geometry preprocessing for DXR due to no support.\n");
        return;
    }
    OutputDebugString(L"Preprocessing static geometries for DXR...\n");

    // This command list will be used for uploading geometry data.
    // It's assumed to be reset and available from Initialize().
    // A single mCommandList->Close()/ExecuteCommandLists()/FlushCommandQueue()
    // will be done at the end of this function.

    mStaticObjectGeometries.resize(obdata_length);
    // mStaticObjectBLASes will be resized in BuildStaticObjectBLASes

    std::vector<Vertex> tempVertices;
    std::vector<uint32_t> tempIndices;

    for (int ob_type = 0; ob_type < obdata_length; ++ob_type)
    {
        tempVertices.clear();
        tempIndices.clear();
        UINT currentVertexOffsetInObject = 0;
        int obdata_vert_idx = 0;

        if (!obdata || ob_type >= obdata_length) {
             OutputDebugString((L"Error: obdata is null or ob_type " + std::to_wstring(ob_type) + L" is out of bounds.\n").c_str());
            mStaticObjectGeometries[ob_type] = std::make_unique<MeshGeometry>(); // Empty geo
            mStaticObjectGeometries[ob_type]->Name = "error_obj_" + std::to_wstring(ob_type);
            continue;
        }
        if (!num_polys_per_object) {
            OutputDebugString(L"Error: num_polys_per_object is null.\n");
             mStaticObjectGeometries[ob_type] = std::make_unique<MeshGeometry>(); // Empty geo
            mStaticObjectGeometries[ob_type]->Name = "error_obj_" + std::to_wstring(ob_type);
            continue;
        }

        int num_polys = num_polys_per_object[ob_type];
        OutputDebugString((L"Processing ob_type: " + std::to_wstring(ob_type) + L", Name: " + s2ws(obdata[ob_type].name) + L", Polys: " + std::to_wstring(num_polys) + L"\n").c_str());


        for (int poly_idx = 0; poly_idx < num_polys; ++poly_idx)
        {
            // TODO: Adapt logic from ObjectToD3DVertList here
            // - Get num_poly_verts = obdata[ob_type].num_vert[poly_idx];
            // - Get poly_command = obdata[ob_type].poly_cmd[poly_idx];
            // - Get texture_alias = obdata[ob_type].tex[poly_idx];
            // - Create std::vector<Vertex> currentPolygonVertices;
            // - Loop v_idx from 0 to num_poly_verts:
            //   - Populate Vertex (Pos from obdata[ob_type].v[obdata_vert_idx], TexC from obdata or TexMap)
            //   - Increment obdata_vert_idx
            // - Call adapted ConvertTriangleStripForPreprocess or ConvertTriangleFanForPreprocess
            //   or directly add triangles for D3DPT_TRIANGLELIST.
        }

        if (tempVertices.empty() || tempIndices.empty()) {
            OutputDebugString((L"Warning: No valid triangle geometry generated for ob_type " + std::to_wstring(ob_type) + L". Creating dummy MeshGeometry.\n").c_str());
            mStaticObjectGeometries[ob_type] = std::make_unique<MeshGeometry>();
            mStaticObjectGeometries[ob_type]->Name = "dummy_static_obj_" + std::to_wstring(ob_type);
            mStaticObjectGeometries[ob_type]->VertexBufferByteSize = 0;
            mStaticObjectGeometries[ob_type]->IndexBufferByteSize = 0;
            continue;
        }

        // CalculateNormalsAndTangentsForMesh(tempVertices, tempIndices); // Call this once geometry is finalized for the object type

        auto meshGeo = std::make_unique<MeshGeometry>();
        meshGeo->Name = "static_obj_" + std::to_string(ob_type);
        // ... (Create and upload VB/IB to meshGeo->VertexBufferGPU/IndexBufferGPU) ...
        // ... (Set meshGeo properties: VertexByteStride, VertexBufferByteSize, IndexFormat, IndexBufferByteSize, DrawArgs) ...
        mStaticObjectGeometries[ob_type] = std::move(meshGeo);
    }

    // After loop, if any GPU uploads happened:
    // ThrowIfFailed(mCommandList->Close());
    // ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
    // mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    // FlushCommandQueue();
    // ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr)); // Reset for next init stage

    OutputDebugString(L"Preprocessing static geometries for DXR (Placeholder structure) done.\n");
}

void DungeonStompApp::BuildAccelerationStructures()
{
    if (mRaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED || !mDxrDevice || !mDxrCommandList)
    {
        OutputDebugString(L"Skipping Acceleration Structure build due to no DXR support or interfaces.\n");
        return;
    }

    if (!mCurrFrameResource) {
        OutputDebugString(L"***ERROR: mCurrFrameResource is null in BuildAccelerationStructures.\n");
        return;
    }
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    ThrowIfFailed(cmdListAlloc->Reset());
    ThrowIfFailed(mDxrCommandList->Reset(cmdListAlloc.Get(), nullptr));

    BuildStaticObjectBLASes();
    BuildSceneTLAS();

    ThrowIfFailed(mDxrCommandList->Close());
    ID3D12CommandList* cmdsLists[] = { mDxrCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);
    FlushCommandQueue();
}

void DungeonStompApp::BuildStaticObjectBLASes()
{
    OutputDebugString(L"Starting BuildStaticObjectBLASes (Placeholder - uses landGeo)...\n");

    MeshGeometry* currentGeo = nullptr;
    if (mStaticObjectGeometries.empty() || !mStaticObjectGeometries[0] || !mStaticObjectGeometries[0]->VertexBufferGPU) { // Check if preprocessed data is valid
        if(mGeometries.count("landGeo") && mGeometries["landGeo"]->VertexBufferGPU) { // Fallback to landGeo
            currentGeo = mGeometries["landGeo"].get();
            OutputDebugString(L"BuildStaticObjectBLASes: Using landGeo as placeholder.\n");
             if (mStaticObjectBLASes.empty()) mStaticObjectBLASes.resize(1); // Ensure space for this one BLAS
        } else {
             OutputDebugString(L"***ERROR: No suitable geometry for BuildStaticObjectBLASes (neither preprocessed nor landGeo).\n");
            mStaticObjectBLASes.clear();
            return;
        }
    } else { // Use the first preprocessed geometry
        currentGeo = mStaticObjectGeometries[0].get();
         mStaticObjectBLASes.resize(mStaticObjectGeometries.size()); // Resize to actual number of geos
         // This loop will eventually iterate all mStaticObjectGeometries
         // For now, it will only process the first one if mStaticObjectGeometries is populated
         // or the landGeo fallback.
    }


    if (!currentGeo || !currentGeo->VertexBufferGPU || !currentGeo->IndexBufferGPU)
    {
        OutputDebugString(L"***ERROR: Valid geometry not found for BLAS in BuildStaticObjectBLASes.\n");
        if (!mStaticObjectBLASes.empty()) mStaticObjectBLASes[0].Reset();
        return;
    }

    // This part will be looped for each item in mStaticObjectGeometries once Preprocess is full
    // For now, just doing for mStaticObjectBLASes[0] using currentGeo
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.VertexBuffer.StartAddress = currentGeo->VertexBufferGPU->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = currentGeo->VertexByteStride;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = currentGeo->VertexBufferByteSize / currentGeo->VertexByteStride;
    geometryDesc.Triangles.IndexBuffer = currentGeo->IndexBufferGPU->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexFormat = currentGeo->IndexFormat;
    geometryDesc.Triangles.IndexCount = currentGeo->IndexBufferByteSize / (currentGeo->IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4);
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
    buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildInputs.NumDescs = 1;
    buildInputs.pGeometryDescs = &geometryDesc;
    buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuildInfo = {};
    mDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &prebuildInfo);

    if (prebuildInfo.ResultDataMaxSizeInBytes == 0) {
        OutputDebugString(L"***ERROR: BLAS PrebuildInfo resulted in 0 size for result data. Skipping BLAS build.\n");
        if (!mStaticObjectBLASes.empty()) mStaticObjectBLASes[0].Reset();
        return;
    }

    ComPtr<ID3D12Resource> scratchBuffer;
    ThrowIfFailed(mDxrDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&scratchBuffer)));
    scratchBuffer->SetName(L"StaticBLAS_ScratchBuffer_0");

    ThrowIfFailed(mDxrDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(prebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(&mStaticObjectBLASes[0]))));
    mStaticObjectBLASes[0]->SetName(L"StaticBLAS_0_ResultBuffer");

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = buildInputs;
    buildDesc.ScratchAccelerationStructureData = scratchBuffer->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = mStaticObjectBLASes[0]->GetGPUVirtualAddress();

    mDxrCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(mStaticObjectBLASes[0].Get());
    mDxrCommandList->ResourceBarrier(1, &barrier);
    OutputDebugString(L"BuildStaticObjectBLASes (for one object) submitted.\n");
}

void DungeonStompApp::BuildSceneTLAS()
{
    if(mStaticObjectBLASes.empty() || !mStaticObjectBLASes[0].Get()) {
        OutputDebugString(L"***ERROR: Static BLAS mStaticObjectBLASes[0] is null. Skipping TLAS build.\n");
        mSceneTlas.Reset();
        mSceneTlasInstanceDesc.Reset();
        return;
    }
    OutputDebugString(L"Starting BuildSceneTLAS...\n");

    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1.0f;
    instanceDesc.InstanceMask = 0xFF;
    instanceDesc.InstanceID = 0;
    instanceDesc.InstanceContributionToHitGroupIndex = 0;
    instanceDesc.AccelerationStructure = mStaticObjectBLASes[0]->GetGPUVirtualAddress();
    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

    UINT64 instanceDescsBufferSize = sizeof(instanceDesc);

    if (!mSceneTlasInstanceDesc || mSceneTlasInstanceDesc->GetDesc().Width < instanceDescsBufferSize) {
         mSceneTlasInstanceDesc.Reset();
        ThrowIfFailed(mDxrDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(instanceDescsBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&mSceneTlasInstanceDesc)));
        mSceneTlasInstanceDesc->SetName(L"SceneTLAS_InstanceUploadBuffer");
    }

    UINT8* pHostInstanceDesc;
    ThrowIfFailed(mSceneTlasInstanceDesc->Map(0, nullptr, reinterpret_cast<void**>(&pHostInstanceDesc)));
    memcpy(pHostInstanceDesc, &instanceDesc, sizeof(instanceDesc));
    mSceneTlasInstanceDesc->Unmap(0, nullptr);

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
    buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    buildInputs.NumDescs = 1;
    buildInputs.InstanceDescs = mSceneTlasInstanceDesc->GetGPUVirtualAddress();
    buildInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuildInfo = {};
    mDxrDevice->GetRaytracingAccelerationStructurePrebuildInfo(&buildInputs, &tlasPrebuildInfo);

    if (tlasPrebuildInfo.ResultDataMaxSizeInBytes == 0) {
        OutputDebugString(L"***ERROR: TLAS PrebuildInfo resulted in 0 size for result data. Skipping TLAS build.\n");
        mSceneTlas.Reset();
        return;
    }

    ComPtr<ID3D12Resource> tlasScratchBuffer;
    ThrowIfFailed(mDxrDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(tlasPrebuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS, nullptr, IID_PPV_ARGS(&tlasScratchBuffer)));
    tlasScratchBuffer->SetName(L"SceneTLAS_ScratchBuffer");

    if (!mSceneTlas || mSceneTlas->GetDesc().Width < tlasPrebuildInfo.ResultDataMaxSizeInBytes) {
        mSceneTlas.Reset();
        ThrowIfFailed(mDxrDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(tlasPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, nullptr, IID_PPV_ARGS(&mSceneTlas)));
        mSceneTlas->SetName(L"SceneTLAS_ResultBuffer");
    }

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = buildInputs;
    buildDesc.DestAccelerationStructureData = mSceneTlas->GetGPUVirtualAddress();
    buildDesc.ScratchAccelerationStructureData = tlasScratchBuffer->GetGPUVirtualAddress();

    mDxrCommandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::UAV(mSceneTlas.Get());
    mDxrCommandList->ResourceBarrier(1, &barrier);
    OutputDebugString(L"BuildSceneTLAS (with placeholder single instance) submitted.\n");
}

void DungeonStompApp::CreateRaytracingOutputResource()
{
    if (!mDxrDevice) return;
    if (mClientWidth == 0 || mClientHeight == 0) return;

    mRaytracingOutput.Reset();

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = mClientWidth;
    texDesc.Height = mClientHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = mBackBufferFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ThrowIfFailed(mDxrDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        nullptr,
        IID_PPV_ARGS(&mRaytracingOutput)));
    mRaytracingOutput->SetName(L"RaytracingOutputUAV");

    if (mSrvDescriptorHeap) {
        if (mNullTexSrvIndex2 > 0 && (mNullTexSrvIndex2 + 1 < MAX_NUM_TEXTURES)) {
            mRaytracingOutputUavDescriptorHeapIndex = mNullTexSrvIndex2 + 1;
        } else {
            OutputDebugString(L"***ERROR: Cannot determine valid UAV descriptor index for Raytracing Output (mNullTexSrvIndex2 not valid or heap too small).\n");
            mRaytracingOutputUavDescriptorHeapIndex = MAX_NUM_TEXTURES -10;
             if(mRaytracingOutputUavDescriptorHeapIndex >= MAX_NUM_TEXTURES && MAX_NUM_TEXTURES > 0) mRaytracingOutputUavDescriptorHeapIndex = MAX_NUM_TEXTURES -1;
             else if (MAX_NUM_TEXTURES == 0) {ThrowIfFailed(E_FAIL); return;}
        }
        if (mRaytracingOutputUavDescriptorHeapIndex >= MAX_NUM_TEXTURES) {
             OutputDebugString(L"***ERROR: Calculated UAV descriptor index is out of bounds for Raytracing Output UAV.\n");
             ThrowIfFailed(E_FAIL);
        }

        D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
        uavDesc.Format = mBackBufferFormat;
        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        uavDesc.Texture2D.MipSlice = 0;

        CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        if (mRaytracingOutputUavDescriptorHeapIndex < MAX_NUM_TEXTURES) {
            uavHandle.Offset(mRaytracingOutputUavDescriptorHeapIndex, mCbvSrvDescriptorSize);
            mDxrDevice->CreateUnorderedAccessView(mRaytracingOutput.Get(), nullptr, &uavDesc, uavHandle);
            OutputDebugString(L"Raytracing Output UAV created.\n");
        } else {
            OutputDebugString(L"Raytracing Output UAV descriptor index is invalid, not creating view.\n");
        }
    } else {
        OutputDebugString(L"***ERROR: mSrvDescriptorHeap is NULL in CreateRaytracingOutputResource.\n");
    }
}

inline UINT AlignSBT(UINT size, UINT alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

void DungeonStompApp::BuildShaderBindingTable()
{
    if (mRaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED || !mDxrDevice || !mPSOs["raytracing"])
    {
        OutputDebugString(L"Skipping SBT build due to no DXR support or missing Raytracing PSO.\n");
        return;
    }
    OutputDebugString(L"Building Shader Binding Table...\n");

    ComPtr<ID3D12StateObjectProperties> stateObjectProps;
    ThrowIfFailed(mPSOs["raytracing"]->QueryInterface(IID_PPV_ARGS(&stateObjectProps)));

    void* rayGenShaderId = stateObjectProps->GetShaderIdentifier(L"raygeneration");
    void* missShaderId = stateObjectProps->GetShaderIdentifier(L"miss");
    void* hitGroupShaderId = stateObjectProps->GetShaderIdentifier(L"TestHitGroup");

    if (!rayGenShaderId || !missShaderId || !hitGroupShaderId) {
        OutputDebugString(L"***ERROR: Failed to get shader identifiers for SBT.\n");
        return;
    }

    UINT rayGenRecordSize = AlignSBT(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    UINT missRecordSize = AlignSBT(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    UINT hitGroupRecordSize = AlignSBT(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);

    UINT sbtSize = rayGenRecordSize + missRecordSize + hitGroupRecordSize;

    ThrowIfFailed(mDxrDevice->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(sbtSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mSBTBuffer)));
    mSBTBuffer->SetName(L"ShaderBindingTable");

    UINT8* pSBT;
    ThrowIfFailed(mSBTBuffer->Map(0, nullptr, reinterpret_cast<void**>(&pSBT)));

    memcpy(pSBT, rayGenShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    mRayGenSbtRecord.StartAddress = mSBTBuffer->GetGPUVirtualAddress();
    mRayGenSbtRecord.SizeInBytes = rayGenRecordSize;

    UINT8* pMissRecords = pSBT + rayGenRecordSize;
    memcpy(pMissRecords, missShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    mMissSbtRecord.StartAddress = mSBTBuffer->GetGPUVirtualAddress() + rayGenRecordSize;
    mMissSbtRecord.SizeInBytes = missRecordSize;
    mMissSbtRecord.StrideInBytes = missRecordSize;

    UINT8* pHitGroupRecords = pSBT + rayGenRecordSize + missRecordSize;
    memcpy(pHitGroupRecords, hitGroupShaderId, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    mHitGroupSbtRecord.StartAddress = mSBTBuffer->GetGPUVirtualAddress() + rayGenRecordSize + missRecordSize;
    mHitGroupSbtRecord.SizeInBytes = hitGroupRecordSize;
    mHitGroupSbtRecord.StrideInBytes = hitGroupRecordSize;

    mSBTBuffer->Unmap(0, nullptr);
    OutputDebugString(L"Shader Binding Table built successfully.\n");
}

void DungeonStompApp::BuildRaytracingPSO()
{
    if (mRaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED || !mDxrDevice)
    {
        OutputDebugString(L"Skipping Raytracing PSO build due to no DXR support.\n");
        return;
    }
    OutputDebugString(L"Building Raytracing PSO...\n");

    CD3DX12_DESCRIPTOR_RANGE UAVDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    CD3DX12_ROOT_PARAMETER rootParameters[3];
    rootParameters[0].InitAsShaderResourceView(0);
    rootParameters[1].InitAsDescriptorTable(1, &UAVDescriptorRange);
    rootParameters[2].InitAsConstantBufferView(0);

    CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(_countof(rootParameters), rootParameters);
    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&globalRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &error));
    if (error) OutputDebugStringA((char*)error->GetBufferPointer());
    ThrowIfFailed(mDxrDevice->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&mDxrGlobalRootSignature)));
    mDxrGlobalRootSignature->SetName(L"DxrGlobalRootSignature");

    std::vector<D3D12_STATE_SUBOBJECT> subobjects;

    auto rayGenLib = CD3DX12_DXIL_LIBRARY_SUBOBJECT();
    D3D12_SHADER_BYTECODE rayGenBytecode = { mShaders["rayGen"]->GetBufferPointer(), mShaders["rayGen"]->GetBufferSize() };
    rayGenLib.SetDXILLibrary(&rayGenBytecode);
    rayGenLib.DefineExport(L"raygeneration");
    subobjects.push_back(rayGenLib);

    auto missLib = CD3DX12_DXIL_LIBRARY_SUBOBJECT();
    D3D12_SHADER_BYTECODE missBytecode = { mShaders["miss"]->GetBufferPointer(), mShaders["miss"]->GetBufferSize() };
    missLib.SetDXILLibrary(&missBytecode);
    missLib.DefineExport(L"miss");
    subobjects.push_back(missLib);

    auto closestHitLib = CD3DX12_DXIL_LIBRARY_SUBOBJECT();
    D3D12_SHADER_BYTECODE closestHitBytecode = { mShaders["closestHit"]->GetBufferPointer(), mShaders["closestHit"]->GetBufferSize() };
    closestHitLib.SetDXILLibrary(&closestHitBytecode);
    closestHitLib.DefineExport(L"closesthit");
    subobjects.push_back(closestHitLib);

    auto hitGroup = CD3DX12_HIT_GROUP_SUBOBJECT();
    hitGroup.SetHitGroupExport(L"TestHitGroup");
    hitGroup.SetClosestHitShaderImport(L"closesthit");
    hitGroup.SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
    subobjects.push_back(hitGroup);

    auto shaderConfig = CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT();
    UINT payloadSize = sizeof(XMFLOAT4) + sizeof(UINT);
    UINT attributeSize = sizeof(XMFLOAT2);
    shaderConfig.Config(payloadSize, attributeSize);
    subobjects.push_back(shaderConfig);

    auto pipelineConfig = CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT();
    pipelineConfig.Config(1);
    subobjects.push_back(pipelineConfig);

    auto globalRootSigSubobject = CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT();
    globalRootSigSubobject.SetRootSignature(mDxrGlobalRootSignature.Get());
    subobjects.push_back(globalRootSigSubobject);

    CD3DX12_STATE_OBJECT_DESC dxrPipeline(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);
    dxrPipeline.NumSubobjects = static_cast<UINT>(subobjects.size());
    dxrPipeline.pSubobjects = subobjects.data();

    ComPtr<ID3D12StateObject> dxrStateObject;
    HRESULT hr = mDxrDevice->CreateStateObject(&dxrPipeline, IID_PPV_ARGS(&dxrStateObject));
    if (FAILED(hr)) {
        OutputDebugString(L"***ERROR: Failed to create DXR State Object.\n");
        if (hr == E_INVALIDARG) OutputDebugString(L"DXR PSO Create E_INVALIDARG\n");
        ComPtr<ID3D12StateObjectProperties> stateObjectProps;
        if(SUCCEEDED(dxrStateObject.As(&stateObjectProps)))
        {
            OutputDebugStringW(stateObjectProps->GetShaderIdentifier(L"RayGen") ? L"RayGen found\n" : L"RayGen NOT found\n");
        }
    } else {
         OutputDebugString(L"DXR State Object created successfully.\n");
         mPSOs["raytracing"] = dxrStateObject;
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance,
	PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		DungeonStompApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

DungeonStompApp::DungeonStompApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
	mSceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	float scale = 1415.0f;
	mSceneBounds.Radius = sqrtf((10.0f * 10.0f) * scale + (15.0f * 15.0f) * scale);
}

DungeonStompApp::~DungeonStompApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

bool DungeonStompApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	CheckRaytracingSupport();

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mDungeon = std::make_unique<Dungeon>(128, 128, 1.0f, 0.03f, 4.0f, 0.2f);
	mShadowMap = std::make_unique<ShadowMap>(md3dDevice.Get(), 2048, 2048);
	mSsao = std::make_unique<Ssao>(md3dDevice.Get(), mCommandList.Get(), mClientWidth, mClientHeight);

	InitDS();

	LoadTextures();
	BuildRootSignature();
	BuildSsaoRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildLandGeometry();
	BuildDungeonGeometryBuffers();
	BuildMaterials();
	BuildRenderItems();

	PreprocessStaticGeometriesForDXR();

	BuildFrameResources();
	BuildPSOs();
	mSsao->SetPSOs(mPSOs["ssao"].Get(), mPSOs["ssaoBlur"].Get());

	if(mRaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED && mDxrDevice.Get())
	{
		CreateRaytracingOutputResource();
		BuildRaytracingPSO();
		BuildShaderBindingTable();
		BuildAccelerationStructures();
	}

	bobX.SinWave(4.0f, 2.0f, 2.0f);
	bobY.SinWave(4.0f, 2.0f, 4.0f);

	arialFont = LoadFont(L"Arial.fnt", 800, 600);

	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	textVertexBufferView.BufferLocation = textVertexBuffer->GetGPUVirtualAddress();
	textVertexBufferView.StrideInBytes = sizeof(TextVertex);
	textVertexBufferView.SizeInBytes = maxNumTextCharacters * sizeof(TextVertex);

	for (int i = 0; i < MaxRectangle; ++i)
	{
		rectangleVertexBufferView[i].BufferLocation = rectangleVertexBuffer[i]->GetGPUVirtualAddress();
		rectangleVertexBufferView[i].StrideInBytes = sizeof(TextVertex);
		rectangleVertexBufferView[i].SizeInBytes = maxNumRectangleCharacters * sizeof(TextVertex);
	}
	FlushCommandQueue();

#if defined(DEBUG) || defined(_DEBUG) 
#else
	{ ShowWindow(mhMainWnd, SW_MAXIMIZE); mSwapChain->SetFullscreenState(1, nullptr); }
#endif

	return true;
}

void DungeonStompApp::OnResize()
{
	D3DApp::OnResize();

	float angle = 50.0f;
	float fov = angle * (MathHelper::Pi / 180.0f);
	cullAngle = 60.0f;
	XMMATRIX P = XMMatrixPerspectiveFovLH(fov, AspectRatio(), 1.0f, 10000.0f);
	XMStoreFloat4x4(&mProj, P);

	if (mSsao != nullptr)
	{
		mSsao->OnResize(mClientWidth, mClientHeight);
		mSsao->RebuildDescriptors(mDepthStencilBuffer.Get());
	}

    if(mRaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED && mDxrDevice.Get())
    {
        CreateRaytracingOutputResource();
    }
}

void DungeonStompApp::Update(const GameTimer& gt)
{
	float t = gt.DeltaTime();
	UpdateControls();
	FrameMove(0.0f, t);
	UpdateWorld(t);
	OnKeyboardInput(gt);

	bobY.update(t);
	bobX.update(t);

	UpdateCamera(gt);

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	XMMATRIX R = XMMatrixRotationY(mLightRotationAngle);
	for (int i = 0; i < 3; ++i)
	{
		XMVECTOR lightDir = XMLoadFloat3(&mBaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&mRotatedLightDirections[i], lightDir);
	}

	UpdateObjectCBs(gt);
	UpdateMaterialCBs(gt);
	UpdateShadowTransform(gt, 0);
	UpdateMainPassCB(gt);
	UpdateSsaoCB(gt);
	UpdateShadowPassCB(gt);
    UpdateRaytracingPassCB(gt);
	UpdateDungeon(gt);
}

void DungeonStompApp::Draw(const GameTimer& gt)
{
	auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
	ThrowIfFailed(cmdListAlloc->Reset());

    if (mRaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED && mDxrCommandList.Get() && mRaytracingOutput.Get() && mSceneTlas.Get() && mSBTBuffer.Get() && mPSOs["raytracing"].Get())
    {
        ThrowIfFailed(mDxrCommandList->Reset(cmdListAlloc.Get(), nullptr));

        ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
        mDxrCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        mDxrCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRaytracingOutput.Get(),
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

        mDxrCommandList->SetPipelineState1(mPSOs["raytracing"].Get());
        mDxrCommandList->SetComputeRootSignature(mDxrGlobalRootSignature.Get());

        mDxrCommandList->SetComputeRootShaderResourceView(0, mSceneTlas->GetGPUVirtualAddress());

        CD3DX12_GPU_DESCRIPTOR_HANDLE uavHandle(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        if (mRaytracingOutputUavDescriptorHeapIndex > 0 && mRaytracingOutputUavDescriptorHeapIndex < MAX_NUM_TEXTURES) {
             uavHandle.Offset(mRaytracingOutputUavDescriptorHeapIndex, mCbvSrvDescriptorSize);
             mDxrCommandList->SetComputeRootDescriptorTable(1, uavHandle);
        } else {
            OutputDebugString(L"***WARNING: Invalid Raytracing Output UAV descriptor index in Draw(). Skipping UAV bind.\n");
        }

        if(mCurrFrameResource && mCurrFrameResource->RayGenCB.get()) {
            auto currRayGenCB = mCurrFrameResource->RayGenCB.get();
            mDxrCommandList->SetComputeRootConstantBufferView(2, currRayGenCB->Resource()->GetGPUVirtualAddress());
        } else {
            OutputDebugString(L"***WARNING: RayGenCB is null in Draw(). Skipping CBV bind.\n");
        }

        D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
        dispatchDesc.RayGenerationShaderRecord = mRayGenSbtRecord;
        dispatchDesc.MissShaderTable = mMissSbtRecord;
        dispatchDesc.HitGroupTable = mHitGroupSbtRecord;
        dispatchDesc.Width = static_cast<UINT>(mScreenViewport.Width);
        dispatchDesc.Height = static_cast<UINT>(mScreenViewport.Height);
        dispatchDesc.Depth = 1;

        mDxrCommandList->DispatchRays(&dispatchDesc);

        mDxrCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mRaytracingOutput.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

        mDxrCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST));

        mDxrCommandList->CopyResource(CurrentBackBuffer(), mRaytracingOutput.Get());

        mDxrCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
            D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT));

        ThrowIfFailed(mDxrCommandList->Close());
    }
    else
    {
	    ThrowIfFailed(mCommandList->Reset(cmdListAlloc.Get(), mPSOs["opaque"].Get()));
	    ProcessLights11();
	    ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	    mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	    mCommandList->SetGraphicsRootDescriptorTable(5, mNullSrv);
	    DrawSceneToShadowMap(gt);
	    if (enableSSao) {
		    drawingSSAO = true;
		    DrawNormalsAndDepth(gt);
		    drawingSSAO = false;
		    mCommandList->SetGraphicsRootSignature(mSsaoRootSignature.Get());
		    mSsao->ComputeSsao(mCommandList.Get(), mCurrFrameResource, 3);
	    }
	    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	    mCommandList->RSSetViewports(1, &mScreenViewport);
	    mCommandList->RSSetScissorRects(1, &mScissorRect);
	    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		    D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&mMainPassCB.FogColor, 0, nullptr);
	    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	    auto passCB = mCurrFrameResource->PassCB->Resource();
	    mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	    DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque], gt);
	    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		    D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	    ThrowIfFailed(mCommandList->Close());
    }

	ID3D12CommandList* cmdsLists[] = { (mRaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED && mDxrCommandList.Get() && mRaytracingOutput.Get() && mSceneTlas.Get() && mSBTBuffer.Get() && mPSOs["raytracing"].Get()) ? mDxrCommandList.Get() : mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	if (enableVsync) {
		ThrowIfFailed(mSwapChain->Present(1, 0));
	}
	else {
		ThrowIfFailed(mSwapChain->Present(0, 0));
	}

	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;
	mCurrFrameResource->Fence = ++mCurrentFence;
	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void DungeonStompApp::OnMouseDown(WPARAM btnState, int x, int y)
{
}

void DungeonStompApp::OnMouseUp(WPARAM btnState, int x, int y)
{
}

void DungeonStompApp::OnMouseMove(WPARAM btnState, int x, int y)
{
}

void DungeonStompApp::OnKeyboardInput(const GameTimer& gt)
{
	if (player_list[trueplayernum].bIsPlayerAlive == FALSE) {
		if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
			player_list[trueplayernum].bIsPlayerAlive = TRUE;
			player_list[trueplayernum].health = player_list[trueplayernum].hp;
		}
	}

	if (GetAsyncKeyState('M') && !displayShadowMapKeyPress) {
		if (displayShadowMap)
			displayShadowMap = 0;
		else
			displayShadowMap = 1;
	}

	if (GetAsyncKeyState('M')) {
		displayShadowMapKeyPress = 1;
	}
	else {
		displayShadowMapKeyPress = 0;
	}

	if (GetAsyncKeyState('O') && !enableSSaoKey) {
		if (enableSSao) {
			enableSSao = 0;
			strcpy_s(gActionMessage, "SSAO Disabled");
			UpdateScrollList(0, 255, 255);
		}
		else {
			strcpy_s(gActionMessage, "SSAO Enabled");
			UpdateScrollList(0, 255, 255);
			enableSSao = 1;
		}
	}

	if (GetAsyncKeyState('O')) {
		enableSSaoKey = 1;
	}
	else {
		enableSSaoKey = 0;
	}

	if (GetAsyncKeyState('B') && !enableCameraBobKey) {
		if (enableCameraBob) {
			enableCameraBob = false;
			strcpy_s(gActionMessage, "Camera bob Disabled");
			UpdateScrollList(0, 255, 255);
		}
		else {
			strcpy_s(gActionMessage, "Camera bob Enabled");
			UpdateScrollList(0, 255, 255);
			enableCameraBob = true;
		}
	}

	if (GetAsyncKeyState('B')) {
		enableCameraBobKey = 1;
	}
	else {
		enableCameraBobKey = 0;
	}
	
	if (GetAsyncKeyState('N') && !enableNormalmapKey) {
		if (enableNormalmap) {
			enableNormalmap = false;
			SetTextureNormalMapEmpty();
			strcpy_s(gActionMessage, "Normal map Disabled");
			UpdateScrollList(0, 255, 255);
		}
		else {
			SetTextureNormalMap();
			strcpy_s(gActionMessage, "Normal map Enabled");
			UpdateScrollList(0, 255, 255);
			enableNormalmap = true;
		}
	}

	if (GetAsyncKeyState('N')) {
		enableNormalmapKey = 1;
	}
	else {
		enableNormalmapKey = 0;
	}

	if (GetAsyncKeyState('V') && !enableVsyncKey) {
		if (enableVsync) {
			enableVsync = false;
			strcpy_s(gActionMessage, "VSync Disabled");
			UpdateScrollList(0, 255, 255);
		}
		else {
			strcpy_s(gActionMessage, "VSync Enabled");
			UpdateScrollList(0, 255, 255);
			enableVsync = true;
		}
	}

	if (GetAsyncKeyState('V')) {
		enableVsyncKey = 1;
	}
	else {
		enableVsyncKey = 0;
	}
}

void DungeonStompApp::UpdateCamera(const GameTimer& gt)
{
	float adjust = 50.0f;
	float bx = 0.0f;
	float by = 0.0f;

	bx = bobX.getY();
	by = bobY.getY();

	if (player_list[trueplayernum].bIsPlayerAlive == FALSE) {
		adjust = 0.0f;
	}

	mEyePos.x = m_vEyePt.x;
	mEyePos.y = m_vEyePt.y + adjust;
	mEyePos.z = m_vEyePt.z;

	player_list[trueplayernum].x = m_vEyePt.x;
	player_list[trueplayernum].y = m_vEyePt.y + adjust;
	player_list[trueplayernum].z = m_vEyePt.z;

	XMVECTOR pos, target;
	XMFLOAT3 newspot;
	XMFLOAT3 newspot2;

	if (enableCameraBob) {
		float step_left_angy = 0;
		float r = 15.0f;
		step_left_angy = angy - 90;
		if (step_left_angy < 0) step_left_angy += 360;
		if (step_left_angy >= 360) step_left_angy = step_left_angy - 360;
		r = bx;

		if (playercurrentmove == 1 || playercurrentmove == 4) {
			if (!bobX.getIsBobbing()) bobX.SinWave(bobX.getSpeed(), bobX.getAmplitude(), bobX.getFrequency());
			if (!bobY.getIsBobbing()) bobY.SinWave(bobY.getSpeed(), bobY.getAmplitude(), bobY.getFrequency());
		} else if (playercurrentmove == 0) {
			bobX.stopBobbing();
			bobY.stopBobbing();
		}
		r = bx;
		newspot.x = player_list[trueplayernum].x + r * sinf(step_left_angy * k);
		newspot.y = player_list[trueplayernum].y + by;
		newspot.z = player_list[trueplayernum].z + r * cosf(step_left_angy * k);

		float cameradist = 50.0f;
		float newangle = 0;
		newangle = fixangle(look_up_ang, 90);
		newspot2.x = newspot.x + cameradist * sinf(newangle * k) * sinf(angy * k);
		newspot2.y = newspot.y + cameradist * cosf(newangle * k);
		newspot2.z = newspot.z + cameradist * sinf(newangle * k) * cosf(angy * k);
		mEyePos = newspot;
		GunTruesave = newspot;
		pos = XMVectorSet(newspot.x, newspot.y, newspot.z, 1.0f);
		target = XMVectorSet(newspot2.x, newspot2.y, newspot2.z, 1.0f);
	}
	else {
		pos = XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
		target = XMVectorSet(m_vLookatPt.x, m_vLookatPt.y + adjust, m_vLookatPt.z, 1.0f);
		GunTruesave = mEyePos;
	}

	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR EyeDirection = XMVectorSubtract(pos, target);
	if (XMVector3Equal(EyeDirection, XMVectorZero())) {
		return;
	}
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&mView, view);
	mSceneBounds.Center = XMFLOAT3(mEyePos.x, mEyePos.y, mEyePos.z);
}

void DungeonStompApp::UpdateObjectCBs(const GameTimer& gt)
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		if (e->NumFramesDirty > 0)
		{
			XMMATRIX world = XMLoadFloat4x4(&e->World);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);
			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(world));
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);
			e->NumFramesDirty--;
		}
	}
}

void DungeonStompApp::UpdateMaterialCBs(const GameTimer& gt)
{
	auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
	for (auto& e : mMaterials)
	{
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));
			matConstants.Metal = mat->Metal;
			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);
			mat->NumFramesDirty--;
		}
	}
}

void DungeonStompApp::UpdateShadowTransform(const GameTimer& gt, int light)
{
	XMVECTOR lightDir = XMLoadFloat3(&mRotatedLightDirections[0]);
	XMVECTOR lightPos = -2.0f * mSceneBounds.Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);
	XMStoreFloat3(&mLightPosW, lightPos);
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));
	float l = sphereCenterLS.x - (mSceneBounds.Radius);
	float b = sphereCenterLS.y - mSceneBounds.Radius;
	float n = sphereCenterLS.z - mSceneBounds.Radius;
	float r = sphereCenterLS.x + mSceneBounds.Radius;
	float t = sphereCenterLS.y + mSceneBounds.Radius;
	float f = sphereCenterLS.z + mSceneBounds.Radius;
	if ((angy >= 0.00 && angy <= 90.0f) || (angy >= 270.0f && angy <= 360.0f)) {
		l = sphereCenterLS.x - (mSceneBounds.Radius * 1.645f);
	}
	else {
		r = sphereCenterLS.x + (mSceneBounds.Radius * 1.645f);
	}
	mLightNearZ = n;
	mLightFarZ = f;
	XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	XMMATRIX S = lightView * lightProj * T;
	XMStoreFloat4x4(&mLightView, lightView);
	XMStoreFloat4x4(&mLightProj, lightProj);
	XMStoreFloat4x4(&mShadowTransform, S);
}

void DungeonStompApp::UpdateMainPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mView);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	XMMATRIX viewProjTex = XMMatrixMultiply(viewProj, T);
	XMMATRIX shadowTransform = XMLoadFloat4x4(&mShadowTransform);

	XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	XMStoreFloat4x4(&mMainPassCB.ViewProjTex, XMMatrixTranspose(viewProjTex));
	XMStoreFloat4x4(&mMainPassCB.ShadowTransform, XMMatrixTranspose(shadowTransform));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.TotalTime = gt.TotalTime();
	mMainPassCB.DeltaTime = gt.DeltaTime();
	mMainPassCB.AmbientLight = { 0.55f, 0.55f, 0.55f, 1.0f };
	for (int i = 0; i < MaxLights; i++) {
		mMainPassCB.Lights[i + 1].Direction = LightContainer[i].Direction;
		mMainPassCB.Lights[i + 1].Strength = LightContainer[i].Strength;
		mMainPassCB.Lights[i + 1].Position = LightContainer[i].Position;
		mMainPassCB.Lights[i + 1].FalloffEnd = LightContainer[i].FalloffEnd;
		mMainPassCB.Lights[i + 1].FalloffStart = LightContainer[i].FalloffStart;
		mMainPassCB.Lights[i + 1].SpotPower = LightContainer[i].SpotPower;
	}
	mMainPassCB.Lights[0].Strength =  { 0.15f, 0.15f, 0.15f };
	mMainPassCB.Lights[0].Direction = mRotatedLightDirections[0];
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void DungeonStompApp::UpdateShadowPassCB(const GameTimer& gt)
{
	XMMATRIX view = XMLoadFloat4x4(&mLightView);
	XMMATRIX proj = XMLoadFloat4x4(&mLightProj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX invProj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX invViewProj = XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);
	UINT w = mShadowMap->Width();
	UINT h = mShadowMap->Height();
	XMStoreFloat4x4(&mShadowPassCB.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&mShadowPassCB.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&mShadowPassCB.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&mShadowPassCB.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&mShadowPassCB.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&mShadowPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
	mShadowPassCB.EyePosW = mLightPosW;
	mShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	mShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / w, 1.0f / h);
	mShadowPassCB.NearZ = mLightNearZ;
	mShadowPassCB.FarZ = mLightFarZ;
	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mShadowPassCB);
}

void DungeonStompApp::UpdateRaytracingPassCB(const GameTimer& gt)
{
    if (mRaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED) return;
    if (!mCurrFrameResource || !mCurrFrameResource->RayGenCB.get()) return;

    RayGenConstants rgConstants;
    rgConstants.ProjectionToWorld = mMainPassCB.InvViewProj;
    rgConstants.CameraPosition = mEyePos;
    auto currRayGenCB = mCurrFrameResource->RayGenCB.get();
    currRayGenCB->CopyData(0, rgConstants);
}

void DungeonStompApp::UpdateSsaoCB(const GameTimer& gt)
{
	SsaoConstants ssaoCB;
	XMMATRIX P = XMLoadFloat4x4(&mProj);
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	ssaoCB.Proj = mMainPassCB.Proj;
	ssaoCB.InvProj = mMainPassCB.InvProj;
	XMStoreFloat4x4(&ssaoCB.ProjTex, XMMatrixTranspose(P * T));
	mSsao->GetOffsetVectors(ssaoCB.OffsetVectors);
	auto blurWeights = mSsao->CalcGaussWeights(2.5f);
	ssaoCB.BlurWeights[0] = XMFLOAT4(&blurWeights[0]);
	ssaoCB.BlurWeights[1] = XMFLOAT4(&blurWeights[4]);
	ssaoCB.BlurWeights[2] = XMFLOAT4(&blurWeights[8]);
	ssaoCB.InvRenderTargetSize = XMFLOAT2(1.0f / mSsao->SsaoMapWidth(), 1.0f / mSsao->SsaoMapHeight());
	ssaoCB.OcclusionRadius = 7.0f;
	ssaoCB.OcclusionFadeStart = 0.2f;
	ssaoCB.OcclusionFadeEnd = 0.4f;
	ssaoCB.SurfaceEpsilon = 0.20f;
	auto currSsaoCB = mCurrFrameResource->SsaoCB.get();
	currSsaoCB->CopyData(0, ssaoCB);
}

void DungeonStompApp::UpdateDungeon(const GameTimer& gt)
{
	DisplayPlayerCaption();
	auto currDungeonVB = mCurrFrameResource->DungeonVB.get();
	Vertex v;
	for (int j = 0; j < cnt; j++)
	{
		v.Pos.x = src_v[j].x;
		v.Pos.y = src_v[j].y;
		v.Pos.z = src_v[j].z;
		v.Normal.x = src_v[j].nx;
		v.Normal.y = src_v[j].ny;
		v.Normal.z = src_v[j].nz;
		v.TexC.x = src_v[j].tu;
		v.TexC.y = src_v[j].tv;
		v.TangentU.x = src_v[j].nmx;
		v.TangentU.y = src_v[j].nmy;
		v.TangentU.z = src_v[j].nmz;
		currDungeonVB->CopyData(j, v);
	}
	mDungeonRitem->Geo->VertexBufferGPU = currDungeonVB->Resource();
}

void DungeonStompApp::BuildRootSignature()
{
	const int rootItems = 8;
	CD3DX12_DESCRIPTOR_RANGE texTable0; texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);
	CD3DX12_DESCRIPTOR_RANGE texTable1; texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1, 0);
	CD3DX12_DESCRIPTOR_RANGE texTable2; texTable2.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
	CD3DX12_DESCRIPTOR_RANGE texTable3; texTable3.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 3, 0);
	CD3DX12_DESCRIPTOR_RANGE texTable4; texTable4.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 5, 0);
	CD3DX12_ROOT_PARAMETER slotRootParameter[rootItems];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstantBufferView(1);
	slotRootParameter[2].InitAsConstantBufferView(2);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable2, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[6].InitAsDescriptorTable(1, &texTable3, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[7].InitAsDescriptorTable(1, &texTable4, D3D12_SHADER_VISIBILITY_PIXEL);
	auto staticSamplers = GetStaticSamplers();
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(rootItems, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
	if (errorBlob != nullptr) { ::OutputDebugStringA((char*)errorBlob->GetBufferPointer()); }
	ThrowIfFailed(hr);
	ThrowIfFailed(md3dDevice->CreateRootSignature( 0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void DungeonStompApp::BuildSsaoRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE texTable0; texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);
	CD3DX12_DESCRIPTOR_RANGE texTable1; texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsConstantBufferView(0);
	slotRootParameter[1].InitAsConstants(1, 1);
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL);
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0.0f, 0, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers = { pointClamp, linearClamp, depthMapSam, linearWrap };
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter, (UINT)staticSamplers.size(), staticSamplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
	if (errorBlob != nullptr) { ::OutputDebugStringA((char*)errorBlob->GetBufferPointer()); }
	ThrowIfFailed(hr);
	ThrowIfFailed(md3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mSsaoRootSignature.GetAddressOf())));
}

void DungeonStompApp::DrawSceneToShadowMap(const GameTimer& gt)
{
	mCommandList->RSSetViewports(1, &mShadowMap->Viewport());
	mCommandList->RSSetScissorRects(1, &mShadowMap->ScissorRect());
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	mCommandList->ClearDepthStencilView(mShadowMap->Dsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(0, nullptr, false, &mShadowMap->Dsv());
	auto passCB = mCurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);
	mCommandList->SetPipelineState(mPSOs["shadow_opaque"].Get());
	drawingShadowMap = true;
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque], gt);
	drawingShadowMap = false;
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mShadowMap->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void DungeonStompApp::DrawNormalsAndDepth(const GameTimer& gt)
{
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);
	auto normalMap = mSsao->NormalMap();
	auto normalMapRtv = mSsao->NormalMapRtv();
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));
	float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	mCommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	mCommandList->OMSetRenderTargets(1, &normalMapRtv, true, &DepthStencilView());
	auto passCB = mCurrFrameResource->PassCB->Resource();
	mCommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	mCommandList->SetPipelineState(mPSOs["drawNormals"].Get());
	DrawRenderItems(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque], gt);
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(normalMap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> DungeonStompApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(0, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(1, D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(2, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(3, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(4, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.0f, 8);
	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5, D3D12_FILTER_ANISOTROPIC, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0.0f, 8);
	const CD3DX12_STATIC_SAMPLER_DESC shadow(6, D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER, 0.0f, 16, D3D12_COMPARISON_FUNC_LESS_EQUAL, D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);
	return { pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp, shadow };
}

void DungeonStompApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] = { "FOG", "1", "ALPHA_TEST", "1", NULL, NULL };
	const D3D_SHADER_MACRO sasoDefines[] = { "FOG", "1", "ALPHA_TEST", "1", "SSAO", "1", NULL, NULL };
	const D3D_SHADER_MACRO alphaTestDefines[] = { "FOG", "1", "ALPHA_TEST", "1", NULL, NULL };
	const D3D_SHADER_MACRO torchTestDefines[] = { "TORCH_TEST", "1", "ALPHA_TEST", "1", NULL, NULL };

	mShaders["standardVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["opaquePS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", defines, "PS", "ps_5_1");
	mShaders["opaqueSsaoPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", sasoDefines, "PS", "ps_5_1");
	mShaders["alphaTestedPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", alphaTestDefines, "PS", "ps_5_1");
	mShaders["torchPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Default.hlsl", torchTestDefines, "PS", "ps_5_1");
	mShaders["normalMapVS"] = d3dUtil::CompileShader(L"..\\Shaders\\NormalMap.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["normalMapPS"] = d3dUtil::CompileShader(L"..\\Shaders\\NormalMap.hlsl", defines, "PS", "ps_5_1");
	mShaders["normalMapSsaoPS"] = d3dUtil::CompileShader(L"..\\Shaders\\NormalMap.hlsl", sasoDefines, "PS", "ps_5_1");
	mShaders["shadowVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Shadows.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"..\\Shaders\\Shadows.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Shadows.hlsl", alphaTestDefines, "PS", "ps_5_1");
	mShaders["skyVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["skyPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["drawNormalsVS"] = d3dUtil::CompileShader(L"..\\Shaders\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["drawNormalsPS"] = d3dUtil::CompileShader(L"..\\Shaders\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["ssaoVS"] = d3dUtil::CompileShader(L"..\\Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoPS"] = d3dUtil::CompileShader(L"..\\Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");
	mShaders["ssaoBlurVS"] = d3dUtil::CompileShader(L"..\\Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["ssaoBlurPS"] = d3dUtil::CompileShader(L"..\\Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

	mShaders["rayGen"] = d3dUtil::CompileShader(L"..\\Shaders\\RayGeneration.hlsl", nullptr, "RayGen", "lib_6_3");
	mShaders["miss"] = d3dUtil::CompileShader(L"..\\Shaders\\Miss.hlsl", nullptr, "Miss", "lib_6_3");
	mShaders["closestHit"] = d3dUtil::CompileShader(L"..\\Shaders\\ClosestHit.hlsl", nullptr, "ClosestHit", "lib_6_3");

	ID3DBlob* errorBuff;
	ID3DBlob* textVertexShader;
	HRESULT hr = D3DCompileFromFile(L"..\\Shaders\\TextVertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &textVertexShader, &errorBuff);
	D3D12_SHADER_BYTECODE textVertexShaderBytecode = {};
	textVertexShaderBytecode.BytecodeLength = textVertexShader->GetBufferSize();
	textVertexShaderBytecode.pShaderBytecode = textVertexShader->GetBufferPointer();
	ID3DBlob* textPixelShader;
	hr = D3DCompileFromFile(L"..\\Shaders\\TextPixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &textPixelShader, &errorBuff);
	D3D12_SHADER_BYTECODE textPixelShaderBytecode = {};
	textPixelShaderBytecode.BytecodeLength = textPixelShader->GetBufferSize();
	textPixelShaderBytecode.pShaderBytecode = textPixelShader->GetBufferPointer();
	ID3DBlob* rectangleVertexShader;
	hr = D3DCompileFromFile(L"..\\Shaders\\RectangleVertexShader.hlsl", nullptr, nullptr, "main", "vs_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &rectangleVertexShader, &errorBuff);
	D3D12_SHADER_BYTECODE rectangleVertexShaderBytecode = {};
	rectangleVertexShaderBytecode.BytecodeLength = rectangleVertexShader->GetBufferSize();
	rectangleVertexShaderBytecode.pShaderBytecode = rectangleVertexShader->GetBufferPointer();
	ID3DBlob* rectanglePixelShader;
	hr = D3DCompileFromFile(L"..\\Shaders\\RectanglePixelShader.hlsl", nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &rectanglePixelShader, &errorBuff);
	D3D12_SHADER_BYTECODE rectanglePixelShaderBytecode = {};
	rectanglePixelShaderBytecode.BytecodeLength = rectanglePixelShader->GetBufferSize();
	rectanglePixelShaderBytecode.pShaderBytecode = rectanglePixelShader->GetBufferPointer();
	ID3DBlob* rectanglePixelMapShader;
	hr = D3DCompileFromFile(L"..\\Shaders\\rectanglePixelMapShader.hlsl", nullptr, nullptr, "main", "ps_5_0", D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, &rectanglePixelMapShader, &errorBuff);
	D3D12_SHADER_BYTECODE rectanglePixelMapShaderBytecode = {};
	rectanglePixelMapShaderBytecode.BytecodeLength = rectanglePixelMapShader->GetBufferSize();
	rectanglePixelMapShaderBytecode.pShaderBytecode = rectanglePixelMapShader->GetBufferPointer();

	mInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_INPUT_ELEMENT_DESC textInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};
	D3D12_INPUT_LAYOUT_DESC textInputLayoutDesc = {};
	textInputLayoutDesc.NumElements = sizeof(textInputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	textInputLayoutDesc.pInputElementDescs = textInputLayout;
	DXGI_SAMPLE_DESC sampleDesc = {};
	sampleDesc.Count = 1;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC textpsoDesc = {};
	textpsoDesc.InputLayout = textInputLayoutDesc;
	textpsoDesc.pRootSignature = mRootSignature.Get();
	textpsoDesc.VS = textVertexShaderBytecode;
	textpsoDesc.PS = textPixelShaderBytecode;
	textpsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	textpsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	textpsoDesc.SampleDesc = sampleDesc;
	textpsoDesc.SampleMask = 0xffffffff;
	textpsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_BLEND_DESC textBlendStateDesc = {};
	textBlendStateDesc.AlphaToCoverageEnable = FALSE;
	textBlendStateDesc.IndependentBlendEnable = FALSE;
	textBlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
	textBlendStateDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	textBlendStateDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	textBlendStateDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	textBlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	textBlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	textBlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	textBlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	textpsoDesc.BlendState = textBlendStateDesc;
	textpsoDesc.NumRenderTargets = 1;
	D3D12_DEPTH_STENCIL_DESC textDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	textDepthStencilDesc.DepthEnable = false;
	textpsoDesc.DepthStencilState = textDepthStencilDesc;
	hr = md3dDevice->CreateGraphicsPipelineState(&textpsoDesc, IID_PPV_ARGS(&textPSO));
	for (int i = 0; i < MaxRectangle; i++) {
		D3D12_GRAPHICS_PIPELINE_STATE_DESC rectanglepsoDesc = {};
		rectanglepsoDesc.InputLayout = textInputLayoutDesc;
		rectanglepsoDesc.pRootSignature = mRootSignature.Get();
		rectanglepsoDesc.VS = rectangleVertexShaderBytecode;
		rectanglepsoDesc.PS = rectanglePixelShaderBytecode;
		rectanglepsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		rectanglepsoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		rectanglepsoDesc.SampleDesc = sampleDesc;
		rectanglepsoDesc.SampleMask = 0xffffffff;
		rectanglepsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		D3D12_BLEND_DESC rectangleBlendStateDesc = {};
		rectangleBlendStateDesc.AlphaToCoverageEnable = FALSE;
		rectangleBlendStateDesc.IndependentBlendEnable = FALSE;
		rectangleBlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
		if (i == MaxRectangle - 1) {
			rectangleBlendStateDesc.RenderTarget[0].BlendEnable = FALSE;
			rectanglepsoDesc.PS = rectanglePixelMapShaderBytecode;
		}
		rectangleBlendStateDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		rectangleBlendStateDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
		rectangleBlendStateDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		if (i == 0) {
			rectangleBlendStateDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_COLOR;
			rectangleBlendStateDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_COLOR;
			rectangleBlendStateDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		}
		rectangleBlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
		rectangleBlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
		rectangleBlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		rectangleBlendStateDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
		rectanglepsoDesc.BlendState = rectangleBlendStateDesc;
		rectanglepsoDesc.NumRenderTargets = 1;
		D3D12_DEPTH_STENCIL_DESC rectangleDepthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		rectangleDepthStencilDesc.DepthEnable = false;
		rectanglepsoDesc.DepthStencilState = rectangleDepthStencilDesc;
		hr = md3dDevice->CreateGraphicsPipelineState(&rectanglepsoDesc, IID_PPV_ARGS(&rectanglePSO[i]));
	}
}

void DungeonStompApp::BuildLandGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateSphere(0.5f, 20, 20);
	std::vector<Vertex> vertices(grid.Vertices.size());
	for (size_t i = 0; i < grid.Vertices.size(); ++i)
	{
		vertices[i].Pos = grid.Vertices[i].Position;
		vertices[i].Normal = grid.Vertices[i].Normal;
		vertices[i].TexC = grid.Vertices[i].TexC;
	}
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "landGeo";
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, geo->VertexBufferUploader);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	geo->DrawArgs["grid"] = submesh;
	mGeometries["landGeo"] = std::move(geo);
}

void DungeonStompApp::BuildDungeonGeometryBuffers()
{
	std::vector<std::uint16_t> indices(3 * mDungeon->TriangleCount());
	assert(mDungeon->VertexCount() < 0x0000ffff);
	UINT vbByteSize = MAX_NUM_QUADS * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);
	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "waterGeo";
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, geo->IndexBufferUploader);
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;
	geo->DrawArgs["grid"] = submesh;
	mGeometries["waterGeo"] = std::move(geo);
}

void DungeonStompApp::DrawRenderItemsFL(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();
	CD3DX12_GPU_DESCRIPTOR_HANDLE tex3(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	tex3.Offset(484, mCbvSrvDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(6, tex3);
	auto ri = ritems[1];
	cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
	cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
	cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
	UINT materialIndex = mMaterials["flat"].get()->MatCBIndex;
	D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + materialIndex * matCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
	cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);
	cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
}

void DungeonStompApp::BuildPSOs()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize() };
	opaquePsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize() };
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueSsaoPsoDesc;
	ZeroMemory(&opaqueSsaoPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaqueSsaoPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaqueSsaoPsoDesc.pRootSignature = mRootSignature.Get();
	opaqueSsaoPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize() };
	opaqueSsaoPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["opaqueSsaoPS"]->GetBufferPointer()), mShaders["opaqueSsaoPS"]->GetBufferSize() };
	opaqueSsaoPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaqueSsaoPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaqueSsaoPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaqueSsaoPsoDesc.SampleMask = UINT_MAX;
	opaqueSsaoPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaqueSsaoPsoDesc.NumRenderTargets = 1;
	opaqueSsaoPsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaqueSsaoPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaqueSsaoPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaqueSsaoPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaqueSsaoPsoDesc, IID_PPV_ARGS(&mPSOs["opaqueSsao"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC smapPsoDesc = opaquePsoDesc;
	smapPsoDesc.RasterizerState.DepthBias = 100000;
	smapPsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
	smapPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	smapPsoDesc.pRootSignature = mRootSignature.Get();
	smapPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["shadowVS"]->GetBufferPointer()), mShaders["shadowVS"]->GetBufferSize() };
	smapPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["shadowOpaquePS"]->GetBufferPointer()), mShaders["shadowOpaquePS"]->GetBufferSize() };
	smapPsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	smapPsoDesc.NumRenderTargets = 0;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&smapPsoDesc, IID_PPV_ARGS(&mPSOs["shadow_opaque"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalMapPsoDesc;
	ZeroMemory(&normalMapPsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	normalMapPsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	normalMapPsoDesc.pRootSignature = mRootSignature.Get();
	normalMapPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["normalMapVS"]->GetBufferPointer()), mShaders["normalMapVS"]->GetBufferSize() };
	normalMapPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["normalMapPS"]->GetBufferPointer()), mShaders["normalMapPS"]->GetBufferSize() };
	normalMapPsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	normalMapPsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	normalMapPsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	normalMapPsoDesc.SampleMask = UINT_MAX;
	normalMapPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	normalMapPsoDesc.NumRenderTargets = 1;
	normalMapPsoDesc.RTVFormats[0] = mBackBufferFormat;
	normalMapPsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	normalMapPsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	normalMapPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&normalMapPsoDesc, IID_PPV_ARGS(&mPSOs["normalMap"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC normalMapSsaoPSoDesc;
	ZeroMemory(&normalMapSsaoPSoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	normalMapSsaoPSoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	normalMapSsaoPSoDesc.pRootSignature = mRootSignature.Get();
	normalMapSsaoPSoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["normalMapVS"]->GetBufferPointer()), mShaders["normalMapVS"]->GetBufferSize() };
	normalMapSsaoPSoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["normalMapSsaoPS"]->GetBufferPointer()), mShaders["normalMapSsaoPS"]->GetBufferSize() };
	normalMapSsaoPSoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	normalMapSsaoPSoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	normalMapSsaoPSoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	normalMapSsaoPSoDesc.SampleMask = UINT_MAX;
	normalMapSsaoPSoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	normalMapSsaoPSoDesc.NumRenderTargets = 1;
	normalMapSsaoPSoDesc.RTVFormats[0] = mBackBufferFormat;
	normalMapSsaoPSoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	normalMapSsaoPSoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	normalMapSsaoPSoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&normalMapSsaoPSoDesc, IID_PPV_ARGS(&mPSOs["normalMapSsao"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPsoDesc = opaquePsoDesc;
	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_COLOR;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_BLEND_FACTOR;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
	transparentPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparentPsoDesc, IID_PPV_ARGS(&mPSOs["transparent"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestedPsoDesc = opaquePsoDesc;
	alphaTestedPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["alphaTestedPS"]->GetBufferPointer()), mShaders["alphaTestedPS"]->GetBufferSize() };
	alphaTestedPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestedPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTested"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC torchPsoDesc = opaquePsoDesc;
	torchPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc; // Reuse transparencyBlendDesc
	torchPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["torchPS"]->GetBufferPointer()), mShaders["torchPS"]->GetBufferSize() };
	torchPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&torchPsoDesc, IID_PPV_ARGS(&mPSOs["torchTested"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC cubePsoDesc;
	ZeroMemory(&cubePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	cubePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	cubePsoDesc.pRootSignature = mRootSignature.Get();
	cubePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	cubePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	cubePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	cubePsoDesc.SampleMask = UINT_MAX;
	cubePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	cubePsoDesc.NumRenderTargets = 1;
	cubePsoDesc.RTVFormats[0] = mBackBufferFormat;
	cubePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	cubePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	cubePsoDesc.DSVFormat = mDepthStencilFormat;
	cubePsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	cubePsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	cubePsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["skyVS"]->GetBufferPointer()), mShaders["skyVS"]->GetBufferSize() };
	cubePsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["skyPS"]->GetBufferPointer()), mShaders["skyPS"]->GetBufferSize() };
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&cubePsoDesc, IID_PPV_ARGS(&mPSOs["sky"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC basePsoDesc;
	ZeroMemory(&basePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	basePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	basePsoDesc.pRootSignature = mRootSignature.Get();
	basePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	basePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	basePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	basePsoDesc.SampleMask = UINT_MAX;
	basePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	basePsoDesc.NumRenderTargets = 1;
	basePsoDesc.RTVFormats[0] = mBackBufferFormat;
	basePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	basePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	basePsoDesc.DSVFormat = mDepthStencilFormat;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawNormalsPsoDesc = basePsoDesc;
	drawNormalsPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["drawNormalsVS"]->GetBufferPointer()), mShaders["drawNormalsVS"]->GetBufferSize() };
	drawNormalsPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["drawNormalsPS"]->GetBufferPointer()), mShaders["drawNormalsPS"]->GetBufferSize() };
	drawNormalsPsoDesc.RTVFormats[0] = Ssao::NormalMapFormat;
	drawNormalsPsoDesc.SampleDesc.Count = 1;
	drawNormalsPsoDesc.SampleDesc.Quality = 0;
	drawNormalsPsoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawNormalsPsoDesc, IID_PPV_ARGS(&mPSOs["drawNormals"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPsoDesc = basePsoDesc;
	ssaoPsoDesc.InputLayout = { nullptr, 0 };
	ssaoPsoDesc.pRootSignature = mSsaoRootSignature.Get();
	ssaoPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["ssaoVS"]->GetBufferPointer()), mShaders["ssaoVS"]->GetBufferSize() };
	ssaoPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["ssaoPS"]->GetBufferPointer()), mShaders["ssaoPS"]->GetBufferSize() };
	ssaoPsoDesc.DepthStencilState.DepthEnable = false;
	ssaoPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPsoDesc.RTVFormats[0] = Ssao::AmbientMapFormat;
	ssaoPsoDesc.SampleDesc.Count = 1;
	ssaoPsoDesc.SampleDesc.Quality = 0;
	ssaoPsoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoPsoDesc, IID_PPV_ARGS(&mPSOs["ssao"])));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPsoDesc = ssaoPsoDesc;
	ssaoBlurPsoDesc.VS = { reinterpret_cast<BYTE*>(mShaders["ssaoBlurVS"]->GetBufferPointer()), mShaders["ssaoBlurVS"]->GetBufferSize() };
	ssaoBlurPsoDesc.PS = { reinterpret_cast<BYTE*>(mShaders["ssaoBlurPS"]->GetBufferPointer()), mShaders["ssaoBlurPS"]->GetBufferSize() };
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&ssaoBlurPsoDesc, IID_PPV_ARGS(&mPSOs["ssaoBlur"])));
}

void DungeonStompApp::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<FrameResource>(md3dDevice.Get(),
			2, (UINT)mAllRitems.size(), (UINT)mMaterials.size(), mDungeon->VertexCount()));
	}
}


void DungeonStompApp::BuildMaterials()
{
    std::ifstream infile("materials.txt");
    if (!infile.is_open()) {
        auto defaultMat = std::make_unique<Material>();
        defaultMat->Name = "default";
        defaultMat->MatCBIndex = 0;
        defaultMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
        defaultMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
        defaultMat->Roughness = 0.815f;
        defaultMat->Metal = 0.1f;
        mMaterials["default"] = std::move(defaultMat);
        return;
    }
    std::string line;
    while (std::getline(infile, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        std::string name;
        int matCBIndex = -1;
        float daR, daG, daB, daA;
        float frR, frG, frB;
        float roughness, metal;
        int diffuseSrvHeapIndex = -1;
        iss >> name >> matCBIndex >> daR >> daG >> daB >> daA >> frR >> frG >> frB >> roughness >> metal;
        if (!(iss >> diffuseSrvHeapIndex)) {
            diffuseSrvHeapIndex = 0;
        }
        auto mat = std::make_unique<Material>();
        mat->Name = name;
        mat->MatCBIndex = matCBIndex;
        mat->DiffuseAlbedo = XMFLOAT4(daR, daG, daB, daA);
        mat->FresnelR0 = XMFLOAT3(frR, frG, frB);
        mat->Roughness = roughness;
        mat->Metal = metal;
        mat->DiffuseSrvHeapIndex = diffuseSrvHeapIndex;
        mMaterials[name] = std::move(mat);
    }
    infile.close();
}

void DungeonStompApp::BuildRenderItems()
{
	auto dungeonRitem = std::make_unique<RenderItem>();
	dungeonRitem->World = MathHelper::Identity4x4();
	dungeonRitem->ObjCBIndex = 0;
	dungeonRitem->Mat = mMaterials["water"].get();
	dungeonRitem->Geo = mGeometries["waterGeo"].get();
	dungeonRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	dungeonRitem->IndexCount = dungeonRitem->Geo->DrawArgs["grid"].IndexCount;
	dungeonRitem->StartIndexLocation = dungeonRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	dungeonRitem->BaseVertexLocation = dungeonRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	mDungeonRitem = dungeonRitem.get();
	mRitemLayer[(int)RenderLayer::Opaque].push_back(dungeonRitem.get());

	auto gridRitem = std::make_unique<RenderItem>();
	gridRitem->World = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
	gridRitem->Mat = mMaterials["grass"].get();
	gridRitem->Geo = mGeometries["landGeo"].get();
	gridRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	mRitemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());

	mAllRitems.push_back(std::move(dungeonRitem));
	mAllRitems.push_back(std::move(gridRitem));
}

void DungeonStompApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, const GameTimer& gt)
{
	auto ri = ritems[0];
	cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
	cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
	cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
	CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	tex.Offset(1, mCbvSrvDescriptorSize);
	cmdList->SetGraphicsRootDescriptorTable(3, tex);
	bool enablePSO = true;
	if (drawingShadowMap || drawingSSAO) { enablePSO = false; }

	if (enablePSO) {
		if (enableSSao) { mCommandList->SetPipelineState(mPSOs["normalMapSsao"].Get()); }
		else { mCommandList->SetPipelineState(mPSOs["normalMap"].Get()); }
	}
	DrawDungeon(cmdList, ritems, false, false, true);

	if (enablePSO) {
		if (enableSSao) { mCommandList->SetPipelineState(mPSOs["opaqueSsao"].Get()); }
		else { mCommandList->SetPipelineState(mPSOs["opaque"].Get());}
	}
	DrawDungeon(cmdList, ritems, false, false, false);

	if (enablePSO) { mCommandList->SetPipelineState(mPSOs["transparent"].Get());}
	DrawDungeon(cmdList, ritems, true);

	if (enablePSO) { mCommandList->SetPipelineState(mPSOs["torchTested"].Get());}
	DrawDungeon(cmdList, ritems, true, true);

	if (enablePSO) {
		tex.Offset(377, mCbvSrvDescriptorSize);
		cmdList->SetGraphicsRootDescriptorTable(3, tex);
		cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		for (int i = 0; i < displayCapture; i++) {
			for (int j = 0; j < displayCaptureCount[i]; j++) {
				cmdList->DrawInstanced(4, 1, displayCaptureIndex[i] + (j * 4), 0);
			}
		}
		if (!gravityon || outside) {
			mCommandList->SetPipelineState(mPSOs["sky"].Get());
			DrawRenderItemsFL(mCommandList.Get(), mRitemLayer[(int)RenderLayer::Opaque]);
		}
		DisplayHud();
		SetDungeonText();
		ScanMod(gt.DeltaTime());
	}
	return;
}

extern bool ObjectHasShadow(int object_id);

void DungeonStompApp::DrawDungeon(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems, BOOL isAlpha, bool isTorch, bool normalMap) {
	auto ri = ritems[0];
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));
	auto objectCB = mCurrFrameResource->ObjectCB->Resource();
	auto matCB = mCurrFrameResource->MaterialCB->Resource();
	bool draw = true;
	int currentObject = 0;
	for (currentObject = 0; currentObject < number_of_polys_per_frame; currentObject++)
	{
		int i = ObjectsToDraw[currentObject].vert_index;
		int vert_index = ObjectsToDraw[currentObject].srcstart;
		int fperpoly = ObjectsToDraw[currentObject].srcfstart;
		int face_index = ObjectsToDraw[currentObject].srcfstart;
		int texture_alias_number = texture_list_buffer[i];
		int texture_number = TexMap[texture_alias_number].texture;
		int normal_map_texture = TexMap[texture_alias_number].normalmaptextureid;
		if (texture_alias_number == 104) { TexMap[texture_alias_number].is_alpha_texture = 1;}
		draw = true;
		if (isAlpha) {
			if (texture_number >= 94 && texture_number <= 101 ||
				texture_number >= 289 - 1 && texture_number <= 296 - 1 ||
				texture_number >= 279 - 1 && texture_number <= 288 - 1 ||
				texture_number >= 206 - 1 && texture_number <= 210 - 1 ||
				texture_number == 378) {
				if (isAlpha && !isTorch) { draw = false; }
				if (isAlpha && isTorch) { draw = true; }
			}
		}
		if (normal_map_texture == -1 && normalMap) { draw = false; }
		if (!normalMap && normal_map_texture != -1) { draw = false; }
		int oid = 0;
		if (drawingSSAO || drawingShadowMap) {
			oid = ObjectsToDraw[currentObject].objectId;
			if (oid == -99) { draw = false; }
		}
		if (drawingShadowMap) {
			if (oid == -1) { draw = true; } else { draw = false; }
			if (oid > 0) { if (ObjectHasShadow(oid)) { draw = true; } }
			if (ObjectsToDraw[currentObject].castshaddow == 0) { draw = false; }
		}
		if (currentObject >= playerGunObjectStart && currentObject < playerObjectStart && drawingShadowMap) { draw = false; }
		if (currentObject >= playerObjectStart && currentObject < playerObjectEnd && !drawingShadowMap) { draw = false; }
		if (currentObject >= playerObjectStart && currentObject < playerObjectEnd && drawingShadowMap) { draw = true; }
		if (draw) {
			auto textureType = TexMap[texture_number].material;
			UINT materialIndex = mMaterials[textureType].get()->MatCBIndex;
			D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
			D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + materialIndex * matCBByteSize;
			cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
			cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);
			CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex.Offset(texture_number, mCbvSrvDescriptorSize);
			cmdList->SetGraphicsRootDescriptorTable(3, tex);
			CD3DX12_GPU_DESCRIPTOR_HANDLE tex3(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex3.Offset(number_of_tex_aliases + 1, mCbvSrvDescriptorSize);
			cmdList->SetGraphicsRootDescriptorTable(5, tex3);
			CD3DX12_GPU_DESCRIPTOR_HANDLE tex4(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
			tex4.Offset(number_of_tex_aliases + 2, mCbvSrvDescriptorSize);
			cmdList->SetGraphicsRootDescriptorTable(7, tex4);
			if (normalMap && !drawingShadowMap && !drawingSSAO) {
				CD3DX12_GPU_DESCRIPTOR_HANDLE tex2(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
				tex2.Offset(normal_map_texture, mCbvSrvDescriptorSize);
				cmdList->SetGraphicsRootDescriptorTable(4, tex2);
			}
			if (dp_command_index_mode[i] == 1 && TexMap[texture_alias_number].is_alpha_texture == isAlpha) {
				int  v = verts_per_poly[currentObject];
				if (dp_commands[currentObject] == D3DPT_TRIANGLELIST) { cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); }
				else if (dp_commands[currentObject] == D3DPT_TRIANGLESTRIP) { cmdList->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); }
				cmdList->DrawInstanced(v, 1, vert_index, 0);
			}
		}
	}
	UINT materialIndex = mMaterials["default"].get()->MatCBIndex;
	D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
	D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress() + materialIndex * matCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
	cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);
}

float DungeonStompApp::GetHillsHeight(float x, float z)const
{
	return 0.3f * (z * sinf(0.1f * x) + x * cosf(0.1f * z));
}

XMFLOAT3 DungeonStompApp::GetHillsNormal(float x, float z)const
{
	XMFLOAT3 n(-0.03f * z * cosf(0.1f * x) - 0.3f * cosf(0.1f * z), 1.0f, -0.3f * sinf(0.1f * x) + 0.03f * x * sinf(0.1f * z));
	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);
	return n;
}

void DungeonStompApp::LoadTextures()
{
	auto woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../Textures/bricks2.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), woodCrateTex->Filename.c_str(), woodCrateTex->Resource, woodCrateTex->UploadHeap));
	auto grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../Textures/WoodCrate01.dds";
	ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), grassTex->Filename.c_str(), grassTex->Resource, grassTex->UploadHeap));
	mTextures[grassTex->Name] = std::move(grassTex);
	mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
}

void DungeonStompApp::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 3;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void DungeonStompApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = MAX_NUM_TEXTURES;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	auto woodCrateTex = mTextures["woodCrateTex"]->Resource;
	auto grassTex = mTextures["grassTex"]->Resource;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = woodCrateTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	md3dDevice->CreateShaderResourceView(woodCrateTex.Get(), &srvDesc, hDescriptor);
	hDescriptor.Offset(1, mCbvSrvDescriptorSize);
	srvDesc.Format = grassTex->GetDesc().Format;
	md3dDevice->CreateShaderResourceView(grassTex.Get(), &srvDesc, hDescriptor);

	LoadRRTextures11("textures.dat");

	HRESULT hr = md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(maxNumTextCharacters * sizeof(TextVertex)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textVertexBuffer));
	textVertexBuffer->SetName(L"Text Vertex Buffer Upload Resource Heap");
	CD3DX12_RANGE readRange(0, 0);
	hr = textVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&textVBGPUAddress));
	for (int i = 0; i < MaxRectangle; ++i)
	{
		hr = md3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE, &CD3DX12_RESOURCE_DESC::Buffer(maxNumRectangleCharacters * sizeof(TextVertex)), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&rectangleVertexBuffer[i]));
		rectangleVertexBuffer[i]->SetName(L"Rectangle Vertex Buffer Upload Resource Heap");
		CD3DX12_RANGE readRange2(0, 0);
		hr = rectangleVertexBuffer[i]->Map(0, &readRange2, reinterpret_cast<void**>(&rectangleVBGPUAddress[i]));
	}

	// mSkyTexHeapIndex is now managed within LoadRRTextures11 or by finding "sunsetcube1024"
	// The CreateShaderResourceView for skyCubeMap is effectively handled by LoadRRTextures11

	UINT nextFreeSrvIndex = 2 + number_of_tex_aliases;
	if (mTextures.count("sunsetcube1024") && mSkyTexHeapIndex == 0) {
		// If "sunsetcube1024" was loaded by LoadTextures() before LoadRRTextures11,
		// and if it's not in textures.dat, then mSkyTexHeapIndex needs to be set to its actual position.
		// This logic needs to be robust. For now, assuming LoadRRTextures11 sets mSkyTexHeapIndex if it loads it.
		// If LoadRRTextures11 loads it, number_of_tex_aliases would include it.
		// If mSkyTexHeapIndex is still 0 after LoadRRTextures11, it means it wasn't found/processed as expected.
		// This indicates a potential issue in descriptor heap indexing that needs careful review.
		// The +2 assumes woodCrate and grassTex are always at 0 and 1, and textures.dat starts after.
	}

	mShadowMapHeapIndex = nextFreeSrvIndex++;
	mSsaoHeapIndexStart = nextFreeSrvIndex;
    nextFreeSrvIndex += 3; // Placeholder for SSAO SRVs
	mNullCubeSrvIndex = nextFreeSrvIndex++;
	mNullTexSrvIndex1 = nextFreeSrvIndex++;
	mNullTexSrvIndex2 = nextFreeSrvIndex++;

	auto srvCpuStart = mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	auto srvGpuStart = mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC nullSrvDesc = {};
    nullSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    nullSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    nullSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    nullSrvDesc.Texture2D.MostDetailedMip = 0;
    nullSrvDesc.Texture2D.MipLevels = 1;
    nullSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	if(mNullTexSrvIndex1 < MAX_NUM_TEXTURES)
		md3dDevice->CreateShaderResourceView(nullptr, &nullSrvDesc, GetCpuSrv(mNullTexSrvIndex1));
    mNullSrv = GetGpuSrv(mNullTexSrvIndex1);

	mShadowMap->BuildDescriptors(GetCpuSrv(mShadowMapHeapIndex), GetGpuSrv(mShadowMapHeapIndex), GetDsv(1));
	mSsao->BuildDescriptors(mDepthStencilBuffer.Get(), GetCpuSrv(mSsaoHeapIndexStart), GetGpuSrv(mSsaoHeapIndexStart), GetRtv(SwapChainBufferCount), mCbvSrvUavDescriptorSize, mRtvDescriptorSize);
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DungeonStompApp::GetCpuSrv(int index)const
{
	auto srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	if(index < 0 || index >= MAX_NUM_TEXTURES) { OutputDebugString(L"GetCpuSrv: Invalid Index!\n"); return srv; /* Should handle error */ }
	srv.Offset(index, mCbvSrvDescriptorSize);
	return srv;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DungeonStompApp::GetGpuSrv(int index)const
{
	auto srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	if(index < 0 || index >= MAX_NUM_TEXTURES) { OutputDebugString(L"GetGpuSrv: Invalid Index!\n"); return srv; /* Should handle error */ }
	srv.Offset(index, mCbvSrvDescriptorSize);
	return srv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DungeonStompApp::GetDsv(int index)const
{
	auto dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart());
    // Assuming DSV heap has at least index+1 capacity (e.g. 2 for main DSV + shadow map DSV)
	dsv.Offset(index, mDsvDescriptorSize);
	return dsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DungeonStompApp::GetRtv(int index)const
{
	auto rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    // Assuming RTV heap has at least index+1 capacity
	rtv.Offset(index, mRtvDescriptorSize);
	return rtv;
}

BOOL DungeonStompApp::LoadRRTextures11(char* filename)
{
	FILE* fp;
	char s[256], p[256], f[256];
	int tex_alias_counter = 0;
	int tex_counter = 0;
	BOOL found;

	if (fopen_s(&fp, filename, "r") != 0) { return FALSE; }

	// Start hDescriptor after the two manually loaded textures (woodCrate, grassTex)
	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    hDescriptor.Offset(2, mCbvSrvDescriptorSize);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	int done = 0;
	while (done == 0)
	{
		found = FALSE;
		fscanf_s(fp, "%s", &s, 256);
		if (strcmp(s, "AddTexture") == 0) { fscanf_s(fp, "%s", &f, 256); tex_counter++; found = TRUE; }
		if (strcmp(s, "Alias") == 0)
		{
			char aliasName[100];
			fscanf_s(fp, "%s", &p, 256);
			fscanf_s(fp, "%s", &aliasName, 100);
			strcpy_s((char*)TexMap[tex_alias_counter].tex_alias_name, 100, aliasName);
			TexMap[tex_alias_counter].texture = tex_counter - 1;

			auto currentTex = std::make_unique<Texture>();
			currentTex->Name = aliasName; // Use the read alias name
			currentTex->Filename = charToWChar(f);

			HRESULT hr_tex = DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), currentTex->Filename.c_str(), currentTex->Resource, currentTex->UploadHeap);
			if (FAILED(hr_tex)) {
				currentTex->Filename = charToWChar("../Textures/WoodCrate01.dds");
				DirectX::CreateDDSTextureFromFile12(md3dDevice.Get(), mCommandList.Get(), currentTex->Filename.c_str(), currentTex->Resource, currentTex->UploadHeap);
			}

			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = currentTex->Resource->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = -1; // Use all mips
			srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

            UINT currentHeapIndex = 2 + tex_alias_counter;
            TexMap[tex_alias_counter].dxr_srv_heap_index = currentHeapIndex; // Store the intended heap index

			if (strcmp(aliasName, "sunsetcube1024") == 0) {
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
				srvDesc.TextureCube.MipLevels = -1; // Use all mips
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
                mSkyTexHeapIndex = currentHeapIndex;
			}

            CD3DX12_CPU_DESCRIPTOR_HANDLE specificHandle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
            if(currentHeapIndex < MAX_NUM_TEXTURES) { // Bounds check
                specificHandle.Offset(currentHeapIndex, mCbvSrvDescriptorSize);
			    md3dDevice->CreateShaderResourceView(currentTex->Resource.Get(), &srvDesc, specificHandle);
            } else {
                OutputDebugString(L"***ERROR: SRV Heap index out of bounds in LoadRRTextures11.\n");
            }

			mTextures[currentTex->Name] = std::move(currentTex);
			// Do not advance the local hDescriptor here, use specificHandle

			fscanf_s(fp, "%s", &p, 256); if (strcmp(p, "AlphaTransparent") == 0) TexMap[tex_alias_counter].is_alpha_texture = TRUE;
			int i_texmap = tex_alias_counter;
			fscanf_s(fp, "%s", &p, 256);
			if (strcmp(p, "WHOLE") == 0) { TexMap[i_texmap].tu[0] = 0.0f; TexMap[i_texmap].tv[0] = 1.0f; TexMap[i_texmap].tu[1] = 0.0f; TexMap[i_texmap].tv[1] = 0.0f; TexMap[i_texmap].tu[2] = 1.0f; TexMap[i_texmap].tv[2] = 1.0f; TexMap[i_texmap].tu[3] = 1.0f; TexMap[i_texmap].tv[3] = 0.0f; }
			else if (strcmp(p, "TL_QUAD") == 0) { TexMap[i_texmap].tu[0] = 0.0f; TexMap[i_texmap].tv[0] = 0.5f; TexMap[i_texmap].tu[1] = 0.0f; TexMap[i_texmap].tv[1] = 0.0f; TexMap[i_texmap].tu[2] = 0.5f; TexMap[i_texmap].tv[2] = 0.5f; TexMap[i_texmap].tu[3] = 0.5f; TexMap[i_texmap].tv[3] = 0.0f; }
			else if (strcmp(p, "TR_QUAD") == 0) { TexMap[i_texmap].tu[0] = 0.5f; TexMap[i_texmap].tv[0] = 0.5f; TexMap[i_texmap].tu[1] = 0.5f; TexMap[i_texmap].tv[1] = 0.0f; TexMap[i_texmap].tu[2] = 1.0f; TexMap[i_texmap].tv[2] = 0.5f; TexMap[i_texmap].tu[3] = 1.0f; TexMap[i_texmap].tv[3] = 0.0f; }
			else if (strcmp(p, "LL_QUAD") == 0) { TexMap[i_texmap].tu[0] = 0.0f; TexMap[i_texmap].tv[0] = 1.0f; TexMap[i_texmap].tu[1] = 0.0f; TexMap[i_texmap].tv[1] = 0.5f; TexMap[i_texmap].tu[2] = 0.5f; TexMap[i_texmap].tv[2] = 1.0f; TexMap[i_texmap].tu[3] = 0.5f; TexMap[i_texmap].tv[3] = 0.5f; }
			else if (strcmp(p, "LR_QUAD") == 0) { TexMap[i_texmap].tu[0] = 0.5f; TexMap[i_texmap].tv[0] = 1.0f; TexMap[i_texmap].tu[1] = 0.5f; TexMap[i_texmap].tv[1] = 0.5f; TexMap[i_texmap].tu[2] = 1.0f; TexMap[i_texmap].tv[2] = 1.0f; TexMap[i_texmap].tu[3] = 1.0f; TexMap[i_texmap].tv[3] = 0.5f; }
			else if (strcmp(p, "TOP_HALF") == 0) { TexMap[i_texmap].tu[0] = 0.0f; TexMap[i_texmap].tv[0] = 0.5f; TexMap[i_texmap].tu[1] = 0.0f; TexMap[i_texmap].tv[1] = 0.0f; TexMap[i_texmap].tu[2] = 1.0f; TexMap[i_texmap].tv[2] = 0.5f; TexMap[i_texmap].tu[3] = 1.0f; TexMap[i_texmap].tv[3] = 0.0f; }
			else if (strcmp(p, "BOT_HALF") == 0) { TexMap[i_texmap].tu[0] = 0.0f; TexMap[i_texmap].tv[0] = 1.0f; TexMap[i_texmap].tu[1] = 0.0f; TexMap[i_texmap].tv[1] = 0.5f; TexMap[i_texmap].tu[2] = 1.0f; TexMap[i_texmap].tv[2] = 1.0f; TexMap[i_texmap].tu[3] = 1.0f; TexMap[i_texmap].tv[3] = 0.5f; }
			else if (strcmp(p, "LEFT_HALF") == 0) { TexMap[i_texmap].tu[0] = 0.0f; TexMap[i_texmap].tv[0] = 1.0f; TexMap[i_texmap].tu[1] = 0.0f; TexMap[i_texmap].tv[1] = 0.0f; TexMap[i_texmap].tu[2] = 0.5f; TexMap[i_texmap].tv[2] = 1.0f; TexMap[i_texmap].tu[3] = 0.5f; TexMap[i_texmap].tv[3] = 0.0f; }
			else if (strcmp(p, "RIGHT_HALF") == 0) { TexMap[i_texmap].tu[0] = 0.5f; TexMap[i_texmap].tv[0] = 1.0f; TexMap[i_texmap].tu[1] = 0.5f; TexMap[i_texmap].tv[1] = 0.0f; TexMap[i_texmap].tu[2] = 1.0f; TexMap[i_texmap].tv[2] = 1.0f; TexMap[i_texmap].tu[3] = 1.0f; TexMap[i_texmap].tv[3] = 0.0f; }
			else if (strcmp(p, "TL_TRI") == 0) { TexMap[i_texmap].tu[0] = 0.0f; TexMap[i_texmap].tv[0] = 0.0f; TexMap[i_texmap].tu[1] = 1.0f; TexMap[i_texmap].tv[1] = 0.0f; TexMap[i_texmap].tu[2] = 0.0f; TexMap[i_texmap].tv[2] = 1.0f; }

			fscanf_s(fp, "%s", &p, 256);
			strcpy_s((char*)TexMap[tex_alias_counter].material, 100, (char*)&p);
			tex_alias_counter++;
			found = TRUE;
		}
		if (strcmp(s, "END_FILE") == 0) { number_of_tex_aliases = tex_alias_counter; found = TRUE; done = 1; }
		if (found == FALSE) { fclose(fp); return FALSE; }
	}
	fclose(fp);
	SetTextureNormalMap();
	return TRUE;
}

void DungeonStompApp::SetTextureNormalMap() {
	char junk[255];
	for (int i = 0; i < number_of_tex_aliases; i++) {
		sprintf_s(junk, "%s_nm", TexMap[i].tex_alias_name);
		TexMap[i].normalmaptextureid = -1;
		for (int j = 0; j < number_of_tex_aliases; j++) {
			if (strstr(TexMap[j].tex_alias_name, "_nm") != 0) {
				if (strcmp(TexMap[j].tex_alias_name, junk) == 0) {
					TexMap[i].normalmaptextureid = TexMap[j].dxr_srv_heap_index;
					break;
				}
			}
		}
	}
}

void DungeonStompApp::SetTextureNormalMapEmpty() {
	for (int i = 0; i < number_of_tex_aliases; i++) {
		TexMap[i].normalmaptextureid = -1;
	}
}

void DungeonStompApp::ProcessLights11()
{
	int sort[200]; float dist[200]; int obj[200]; int temp;
	for (int i = 0; i < MaxLights; i++) {
		LightContainer[i].Strength = { 1.0f, 1.0f, 1.0f }; LightContainer[i].FalloffStart = 80.0f;
		LightContainer[i].Direction = { 0.0f, -1.0f, 0.0f }; LightContainer[i].FalloffEnd = 120.0f;
		LightContainer[i].Position = DirectX::XMFLOAT3{ 0.0f,9000.0f,0.0f }; LightContainer[i].SpotPower = 90.0f;
	}
	int dcount = 0;
	for (int q = 0; q < oblist_length; q++) {
		int ob_type = oblist[q].type;
		float	qdist = FastDistance(m_vEyePt.x - oblist[q].x, m_vEyePt.y - oblist[q].y, m_vEyePt.z - oblist[q].z);
		if (ob_type == 6 && oblist[q].light_source->command == 900) {
			dist[dcount] = qdist; sort[dcount] = dcount; obj[dcount] = q; dcount++;
		}
	}
	for (int i = 0; i < dcount; i++) {
		for (int j = i + 1; j < dcount; j++) {
			if (dist[sort[i]] > dist[sort[j]]) { temp = sort[i]; sort[i] = sort[j]; sort[j] = temp; }
		}
	}
	if (dcount > 16) { dcount = 16; }
	for (int i = 0; i < dcount; i++) {
		int q = obj[sort[i]];
		LightContainer[i].Strength = { 9.0f, 9.0f, 9.0f };
		LightContainer[i].Position = DirectX::XMFLOAT3{ oblist[q].x,oblist[q].y + 50.0f, oblist[q].z };
	}
	int count = 0;
	for (int misslecount = 0; misslecount < MAX_MISSLE; misslecount++) {
		if (your_missle[misslecount].active == 1) {
			if (count < 4) {
				LightContainer[11 + count].Position = DirectX::XMFLOAT3{ your_missle[misslecount].x, your_missle[misslecount].y, your_missle[misslecount].z };
				LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f };
				LightContainer[11 + count].FalloffStart = 100.0f; LightContainer[11 + count].Direction = { 0.0f, -1.0f, 0.0f };
				LightContainer[11 + count].FalloffEnd = 200.0f; LightContainer[11 + count].SpotPower = 10.0f;
				if (your_missle[misslecount].model_id == 103) { LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 0.0f, 3.0f, 2.843f }; }
				else if (your_missle[misslecount].model_id == 104) { LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 3.0f, 0.396f, 0.5f }; }
				else if (your_missle[misslecount].model_id == 105) { LightContainer[11 + count].Strength = DirectX::XMFLOAT3{ 1.91f, 3.1f, 1.0f }; }
				count++;
			}
		}
	}
	bool flamesword = false;
	if (strstr(your_gun[current_gun].gunname, "FLAME") != NULL || strstr(your_gun[current_gun].gunname, "ICE") != NULL || strstr(your_gun[current_gun].gunname, "LIGHTNINGSWORD") != NULL) { flamesword = true;}
	if (flamesword) {
		int spot = 15;
		LightContainer[spot].Position = DirectX::XMFLOAT3{ m_vEyePt.x, m_vEyePt.y, m_vEyePt.z };
		LightContainer[spot].Strength = DirectX::XMFLOAT3{ 0.0f, 0.0f, 1.0f }; LightContainer[spot].FalloffStart = 200.0f;
		LightContainer[spot].Direction = { 0.0f, -1.0f, 0.0f }; LightContainer[spot].FalloffEnd = 300.0f; LightContainer[spot].SpotPower = 10.0f;
		if (strstr(your_gun[current_gun].gunname, "SUPERFLAME") != NULL) { LightContainer[spot].Strength = DirectX::XMFLOAT3{ 8.0f, 0.867f, 0.0f }; }
		else if (strstr(your_gun[current_gun].gunname, "FLAME") != NULL) { LightContainer[spot].Strength = DirectX::XMFLOAT3{ 7.0f, 0.369f, 0.0f }; }
		else if (strstr(your_gun[current_gun].gunname, "ICE") != NULL) { LightContainer[spot].Strength = DirectX::XMFLOAT3{ 0.0f, 0.796f, 5.0f }; }
		else if (strstr(your_gun[current_gun].gunname, "LIGHTNINGSWORD") != NULL) { LightContainer[spot].Strength = DirectX::XMFLOAT3{ 6.0f, 6.0f, 6.0f };}
	}
	count = 0; dcount = 0;
	for (int q = 0; q < oblist_length; q++) {
		int ob_type = oblist[q].type;
		float	qdist = FastDistance(m_vEyePt.x - oblist[q].x, m_vEyePt.y - oblist[q].y, m_vEyePt.z - oblist[q].z);
		if (ob_type == 6 && qdist < 2500 && oblist[q].light_source->command == 1) {
			dist[dcount] = qdist; sort[dcount] = dcount; obj[dcount] = q; dcount++;
		}
	}
	for (int i = 0; i < dcount; i++) {
		for (int j = i + 1; j < dcount; j++) {
			if (dist[sort[i]] > dist[sort[j]]) { temp = sort[i]; sort[i] = sort[j]; sort[j] = temp; }
		}
	}
	if (dcount > 10) { dcount = 10; }
	for (int i = 0; i < dcount; i++) {
		int q = obj[sort[i]];
		LightContainer[i + 16].Position = DirectX::XMFLOAT3{ oblist[q].x,oblist[q].y + 0.0f, oblist[q].z };
		LightContainer[i + 16].Strength = DirectX::XMFLOAT3{ (float)oblist[q].light_source->rcolour, (float)oblist[q].light_source->gcolour, (float)oblist[q].light_source->bcolour };
		LightContainer[i + 16].FalloffStart = 600.0f;
		LightContainer[i + 16].Direction = { oblist[q].light_source->direction_x, oblist[q].light_source->direction_y, oblist[q].light_source->direction_z };
		LightContainer[i + 16].FalloffEnd = 650.0f; LightContainer[i + 16].SpotPower = 1.9f;
	}
}

wchar_t* charToWChar(const char* text)
{
	const size_t size = strlen(text) + 1;
	wchar_t* wText = new wchar_t[size];
	size_t outSize;
	mbstowcs_s(&outSize, wText, size, text, size);
	return wText;
}
