//////////////////////////////////////////////////////////////////////////////
// d3dx12.h - Redirect to DirectX 12 Agility SDK's modern d3dx12 helpers.
// The Agility SDK NuGet package provides the authoritative headers.
// This file exists only so that #include "d3dx12.h" resolves locally.
//////////////////////////////////////////////////////////////////////////////
#pragma once

// The Agility SDK's d3dx12.h is in the include path added by the NuGet targets.
// We include the individual headers that the old monolithic d3dx12.h provided,
// using the Agility SDK's modular layout.
#include <d3d12.h>

#if defined(__cplusplus)

// Core helpers (CD3DX12_RESOURCE_BARRIER, CD3DX12_HEAP_PROPERTIES, etc.)
#include <d3dx12_core.h>
#include <d3dx12_default.h>
#include <d3dx12_barriers.h>
#include <d3dx12_root_signature.h>
#include <d3dx12_resource_helpers.h>
#include <d3dx12_pipeline_state_stream.h>
#include <d3dx12_render_pass.h>

#ifndef D3DX12_NO_STATE_OBJECT_HELPERS
#include <d3dx12_state_object.h>
#endif

#ifndef D3DX12_NO_CHECK_FEATURE_SUPPORT_CLASS
#include <d3dx12_check_feature_support.h>
#endif

#endif // __cplusplus
