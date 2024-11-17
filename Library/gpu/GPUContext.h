#pragma once
#pragma once
#include <stdint.h>
#include <unordered_map>

#include <d3d11.h>
#include <wrl/client.h>

#include <Ultralight/Geometry.h>

#pragma comment (lib, "D3D11.lib")

#define ENABLE_MSAA 1

using Microsoft::WRL::ComPtr;

class SwapChainD3D11;

class GPUContextD3D11 {
public:
	GPUContextD3D11();

	virtual ~GPUContextD3D11();

	virtual void BeginDrawing();

	virtual void EndDrawing();

	virtual ID3D11Device* device();

	virtual ID3D11DeviceContext* immediate_context();

	virtual void EnableBlend();
	virtual void DisableBlend();

	virtual void EnableScissor();
	virtual void DisableScissor();

	void AddSwapChain(SwapChainD3D11* swap_chain);
	void RemoveSwapChain(SwapChainD3D11* swap_chain);

	SwapChainD3D11* GetSwapChainByRenderBufferId(uint32_t render_buffer_id);

private:
	ComPtr<ID3D11Device> device_;
	ComPtr<ID3D11DeviceContext> immediate_context_;
	ComPtr<ID3D11BlendState> blend_state_;
	ComPtr<ID3D11BlendState> disabled_blend_state_;
	ComPtr<ID3D11RasterizerState> rasterizer_state_;
	ComPtr<ID3D11RasterizerState> scissored_rasterizer_state_;
	D3D_FEATURE_LEVEL feature_level_;
	std::unordered_map<uint32_t, SwapChainD3D11*> swap_chains_by_render_buffer_id_;
};

