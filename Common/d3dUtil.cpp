#include "d3dUtil.h"
#include <comdef.h>
#include <fstream>
#include <atlbase.h> // For CComPtr

using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring &functionName, const std::wstring &filename, int lineNumber) : ErrorCode(hr),
                                                                                                                       FunctionName(functionName),
                                                                                                                       Filename(filename),
                                                                                                                       LineNumber(lineNumber) {
}

bool d3dUtil::IsKeyDown(int vkeyCode) {
	return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring &filename) {
	std::ifstream fin(filename, std::ios::binary);

	fin.seekg(0, std::ios_base::end);
	std::ifstream::pos_type size = (int)fin.tellg();
	fin.seekg(0, std::ios_base::beg);

	ComPtr<ID3DBlob> blob;
	ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

	fin.read((char *)blob->GetBufferPointer(), size);
	fin.close();

	return blob;
}

Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device *device,
    ID3D12GraphicsCommandList *cmdList,
    const void *initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource> &uploadBuffer) {
	ComPtr<ID3D12Resource> defaultBuffer;

	// Create the actual default buffer resource.
	ThrowIfFailed(device->CreateCommittedResource(
	    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
	    D3D12_HEAP_FLAG_NONE,
	    &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
	    D3D12_RESOURCE_STATE_COMMON,
	    nullptr,
	    IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

	// In order to copy CPU memory data into our default buffer, we need to create
	// an intermediate upload heap.
	ThrowIfFailed(device->CreateCommittedResource(
	    &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
	    D3D12_HEAP_FLAG_NONE,
	    &CD3DX12_RESOURCE_DESC::Buffer(byteSize),
	    D3D12_RESOURCE_STATE_GENERIC_READ,
	    nullptr,
	    IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

	// Describe the data we want to copy into the default buffer.
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	// Schedule to copy the data to the default buffer resource.  At a high level, the helper function UpdateSubresources
	// will copy the CPU memory into the intermediate upload heap.  Then, using ID3D12CommandList::CopySubresourceRegion,
	// the intermediate upload heap data will be copied to mBuffer.
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
	                                                                  D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
	                                                                  D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));

	// Note: uploadBuffer has to be kept alive after the above function calls because
	// the command list has not been executed yet that performs the actual copy.
	// The caller can Release the uploadBuffer after it knows the copy has been executed.

	return defaultBuffer;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
    const std::wstring &filename,
    const D3D_SHADER_MACRO *defines,
    const std::string &entrypoint,
    const std::string &target) {
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
	                        entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char *)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

std::wstring DxException::ToString() const {
	// Get the string description of the error code.
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}

ComPtr<ID3DBlob> d3dUtil::CompileShaderDXC(
    const std::wstring &filename,
    const D3D_SHADER_MACRO *defines,
    const std::string &entrypoint,
    const std::string &target) {
	
	// Create DXC compiler instance
	CComPtr<IDxcUtils> pUtils;
	CComPtr<IDxcCompiler3> pCompiler;
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils)));
	ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler)));

	// Create default include handler
	CComPtr<IDxcIncludeHandler> pIncludeHandler;
	ThrowIfFailed(pUtils->CreateDefaultIncludeHandler(&pIncludeHandler));

	// Load the source file
	CComPtr<IDxcBlobEncoding> pSource;
	ThrowIfFailed(pUtils->LoadFile(filename.c_str(), nullptr, &pSource));

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = pSource->GetBufferPointer();
	sourceBuffer.Size = pSource->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_ACP;

	// Convert entrypoint and target to wide strings
	std::wstring wEntrypoint(entrypoint.begin(), entrypoint.end());
	std::wstring wTarget(target.begin(), target.end());

	// Build arguments
	std::vector<LPCWSTR> arguments;
	arguments.push_back(filename.c_str());
	arguments.push_back(L"-E");
	arguments.push_back(wEntrypoint.c_str());
	arguments.push_back(L"-T");
	arguments.push_back(wTarget.c_str());

#if defined(DEBUG) || defined(_DEBUG)
	arguments.push_back(L"-Zi");  // Debug info
	arguments.push_back(L"-Od");  // Disable optimizations
#endif

	// Add defines
	std::vector<std::wstring> defineStrings;
	if (defines) {
		for (const D3D_SHADER_MACRO* p = defines; p->Name != nullptr; ++p) {
			std::wstring defineName(p->Name, p->Name + strlen(p->Name));
			std::wstring defineValue(p->Definition, p->Definition + strlen(p->Definition));
			defineStrings.push_back(L"-D");
			defineStrings.push_back(defineName + L"=" + defineValue);
		}
		for (const auto& def : defineStrings) {
			arguments.push_back(def.c_str());
		}
	}

	// Compile
	CComPtr<IDxcResult> pResults;
	ThrowIfFailed(pCompiler->Compile(
		&sourceBuffer,
		arguments.data(),
		(UINT32)arguments.size(),
		pIncludeHandler,
		IID_PPV_ARGS(&pResults)));

	// Check for errors
	CComPtr<IDxcBlobUtf8> pErrors;
	pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
	if (pErrors && pErrors->GetStringLength() > 0) {
		OutputDebugStringA((char*)pErrors->GetStringPointer());
	}

	HRESULT hrStatus;
	ThrowIfFailed(pResults->GetStatus(&hrStatus));
	ThrowIfFailed(hrStatus);

	// Get compiled shader
	CComPtr<IDxcBlob> pShader;
	ThrowIfFailed(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr));

	// Copy to ID3DBlob for compatibility
	ComPtr<ID3DBlob> byteCode;
	ThrowIfFailed(D3DCreateBlob(pShader->GetBufferSize(), byteCode.GetAddressOf()));
	memcpy(byteCode->GetBufferPointer(), pShader->GetBufferPointer(), pShader->GetBufferSize());

	return byteCode;
}
