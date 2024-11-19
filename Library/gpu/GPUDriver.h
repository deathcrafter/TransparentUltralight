#pragma  once
#include <d3d11.h>
#include <map>
#include <memory>
#include <vector>
#include <wrl/client.h>

#include <Ultralight/platform/GPUDriver.h>

#pragma comment (lib, "D3DCompiler.lib")

using namespace ultralight;
using Microsoft::WRL::ComPtr;

class GPUContextD3D11;

class GPUDriverD3D11 : public GPUDriver {
public:
	GPUDriverD3D11(GPUContextD3D11* context);
	virtual ~GPUDriverD3D11();

	virtual const char* name() { return "Direct3D 11"; };

	virtual void BeginDrawing() {};

	virtual void EndDrawing() {};

	virtual void BindTexture(uint8_t texture_unit, uint32_t texture_id);

	virtual void BindRenderBuffer(uint32_t render_buffer_id);

	virtual void ClearRenderBuffer(uint32_t render_buffer_id);

	virtual void DrawGeometry(uint32_t geometry_id,
		uint32_t indices_count,
		uint32_t indices_offset,
		const ultralight::GPUState& state);

	virtual bool HasCommandsPending() { return !command_list_.empty(); };

	virtual void DrawCommandList();

	virtual int batch_count() const { return batch_count_; };

	///
  /// Called before any state (eg, CreateTexture(), UpdateTexture(), DestroyTexture(), etc.) is
  /// updated during a call to Renderer::Render().
  ///
  /// This is a good time to prepare the GPU for any state updates.
  ///
	virtual void BeginSynchronize() override {};

	///
	/// Called after all state has been updated during a call to Renderer::Render().
	///
	virtual void EndSynchronize() override {};

	///
	/// Get the next available texture ID.
	///
	/// This is used to generate a unique texture ID for each texture created by the library. The
	/// GPU driver implementation is responsible for mapping these IDs to a native ID.
	///
	/// @note Numbering should start at 1, 0 is reserved for "no texture".
	///
	/// @return Returns the next available texture ID.
	///
	virtual uint32_t NextTextureId() override { return next_texture_id_++; };

	///
	/// Create a texture with a certain ID and optional bitmap.
	///
	/// @param texture_id  The texture ID to use for the new texture.
	///
	/// @param bitmap      The bitmap to initialize the texture with (can be empty).
	///
	/// @note If the Bitmap is empty (Bitmap::IsEmpty), then a RTT Texture should be created instead.
	///       This will be used as a backing texture for a new RenderBuffer.
	///
	/// @warning A deep copy of the bitmap data should be made if you are uploading it to the GPU
	///          asynchronously, it will not persist beyond this call.
	///
	virtual void CreateTexture(uint32_t texture_id, RefPtr<Bitmap> bitmap) override;

	///
	/// Update an existing non-RTT texture with new bitmap data.
	///
	/// @param texture_id  The texture to update.
	///
	/// @param bitmap      The new bitmap data.
	///
	/// @warning A deep copy of the bitmap data should be made if you are uploading it to the GPU
	///          asynchronously, it will not persist beyond this call.
	///
	virtual void UpdateTexture(uint32_t texture_id, RefPtr<Bitmap> bitmap) override;

	///
	/// Destroy a texture.
	///
	/// @param texture_id  The texture to destroy.
	///
	virtual void DestroyTexture(uint32_t texture_id) override;

	///
	/// Get the next available render buffer ID.
	///
	/// This is used to generate a unique render buffer ID for each render buffer created by the
	/// library. The GPU driver implementation is responsible for mapping these IDs to a native ID.
	///
	/// @note Numbering should start at 1, 0 is reserved for "no render buffer".
	///
	/// @return Returns the next available render buffer ID.
	///
	virtual uint32_t NextRenderBufferId() override { return next_render_buffer_id_++; };

	///
	/// Create a render buffer with certain ID and buffer description.
	///
	/// @param render_buffer_id  The render buffer ID to use for the new render buffer.
	///
	/// @param buffer           The render buffer description.
	///
	virtual void CreateRenderBuffer(uint32_t render_buffer_id, const RenderBuffer& buffer) override;

	///
	/// Destroy a render buffer.
	///
	/// @param render_buffer_id  The render buffer to destroy.
	///
	virtual void DestroyRenderBuffer(uint32_t render_buffer_id) override;

	///
	/// Get the next available geometry ID.
	///
	/// This is used to generate a unique geometry ID for each geometry created by the library. The
	/// GPU driver implementation is responsible for mapping these IDs to a native ID.
	///
	/// @note Numbering should start at 1, 0 is reserved for "no geometry".
	///
	/// @return Returns the next available geometry ID.
	///
	virtual uint32_t NextGeometryId() override { return next_geometry_id_++; };
	///
	/// Create geometry with certain ID and vertex/index data.
	///
	/// @param geometry_id  The geometry ID to use for the new geometry.
	///
	/// @param vertices     The vertex buffer data.
	///
	/// @param indices      The index buffer data.
	///
	/// @warning A deep copy of the vertex/index data should be made if you are uploading it to the
	///          GPU asynchronously, it will not persist beyond this call.
	///
	virtual void CreateGeometry(uint32_t geometry_id, const VertexBuffer& vertices,
		const IndexBuffer& indices) override;

	///
	/// Update existing geometry with new vertex/index data.
	///
	/// @param geometry_id  The geometry to update.
	///
	/// @param vertices     The new vertex buffer data.
	///
	/// @param indices      The new index buffer data.
	///
	/// @warning A deep copy of the vertex/index data should be made if you are uploading it to the
	///          GPU asynchronously, it will not persist beyond this call.
	///
	virtual void UpdateGeometry(uint32_t geometry_id, const VertexBuffer& vertices,
		const IndexBuffer& indices) override;

	///
	/// Destroy geometry.
	///
	/// @param geometry_id  The geometry to destroy.
	///
	virtual void DestroyGeometry(uint32_t geometry_id) override;

	///
	/// Update the pending command list with commands to execute on the GPU.
	///
	/// Commands are dispatched to the GPU driver asynchronously via this method. The GPU driver
	/// implementation should consume these commands and execute them at an appropriate time.
	///
	/// @param list  The list of commands to execute.
	///
	/// @warning Implementations should make a deep copy of the command list, it will not persist
	///          beyond this call.
	///
	virtual void UpdateCommandList(const CommandList& list) override;

protected:
	uint32_t next_texture_id_ = 1;
	uint32_t next_render_buffer_id_ = 1; // render buffer id 0 is reserved for default render target view.
	uint32_t next_geometry_id_ = 1;
	std::vector<ultralight::Command> command_list_;
	int batch_count_;

	void LoadVertexShader(const char* path, ID3D11VertexShader** ppVertexShader,
		const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs, UINT NumElements,
		ID3D11InputLayout** ppInputLayout);
	void LoadPixelShader(const char* path, ID3D11PixelShader** ppPixelShader);
	void LoadCompiledVertexShader(unsigned char* data, unsigned int len,
		ID3D11VertexShader** ppVertexShader,
		const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
		UINT NumElements, ID3D11InputLayout** ppInputLayout);
	void LoadCompiledPixelShader(unsigned char* data, unsigned int len,
		ID3D11PixelShader** ppPixelShader);
	void LoadShaders();
	void BindShader(ShaderType shader);
	void BindVertexLayout(VertexBufferFormat format);
	void BindGeometry(uint32_t id);
	ID3D11RenderTargetView* GetRenderTargetView(uint32_t render_buffer_id);
	ComPtr<ID3D11SamplerState> GetSamplerState();
	ComPtr<ID3D11Buffer> GetConstantBuffer();
	void SetViewport(uint32_t width, uint32_t height);
	void UpdateConstantBuffer(const GPUState& state);
	Matrix ApplyProjection(const Matrix4x4& transform, float screen_width, float screen_height);

	GPUContextD3D11* context_;
	ComPtr<ID3D11InputLayout> vertex_layout_2f_4ub_2f_;
	ComPtr<ID3D11InputLayout> vertex_layout_2f_4ub_2f_2f_28f_;
	ComPtr<ID3D11SamplerState> sampler_state_;
	ComPtr<ID3D11Buffer> constant_buffer_;

	struct GeometryEntry {
		VertexBufferFormat format;
		ComPtr<ID3D11Buffer> vertexBuffer;
		ComPtr<ID3D11Buffer> indexBuffer;
	};
	typedef std::map<uint32_t, GeometryEntry> GeometryMap;
	GeometryMap geometry_;

	struct TextureEntry {
		ComPtr<ID3D11Texture2D> texture;
		ComPtr<ID3D11ShaderResourceView> texture_srv;

		// These members are only used when MSAA is enabled
		bool is_msaa_render_target = false;
		bool needs_resolve = false;
		ComPtr<ID3D11Texture2D> resolve_texture;
		ComPtr<ID3D11ShaderResourceView> resolve_texture_srv;
	};

	typedef std::map<uint32_t, TextureEntry> TextureMap;
	TextureMap textures_;

	struct RenderTargetEntry {
		ComPtr<ID3D11RenderTargetView> render_target_view;
		uint32_t render_target_texture_id;
	};

	typedef std::map<uint32_t, RenderTargetEntry> RenderTargetMap;
	RenderTargetMap render_targets_;

	typedef std::map<ShaderType, std::pair<ComPtr<ID3D11VertexShader>, ComPtr<ID3D11PixelShader>>>
		ShaderMap;
	ShaderMap shaders_;
};