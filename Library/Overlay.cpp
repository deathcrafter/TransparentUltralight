#include "Overlay.h"

#include <Ultralight/platform/Platform.h>
#include <Ultralight/platform/Config.h>

#include "Application.h"

static IndexType patternCW[] = { 0, 1, 3, 1, 2, 3 };
static IndexType patternCCW[] = { 0, 3, 1, 1, 3, 2 };

Overlay::Overlay(RefPtr<Window> window, uint32_t width, uint32_t height, int x, int y) :
	window_(window), width_(width), height_(height), x_(x), y_(y), needs_update_(true),
	use_gpu_(Platform::instance().gpu_driver()) {
	if (use_gpu_)
		driver_.reset((GPUDriverD3D11*)Platform::instance().gpu_driver());

	ViewConfig view_config;
	view_config.initial_device_scale = window_->scale();
	view_config.is_accelerated = use_gpu_;

	view_ = Application::instance()->renderer()->CreateView(width, height, view_config, nullptr);

	window_->overlay_manager()->Add(this);
}

Overlay::Overlay(RefPtr<Window> window, RefPtr<View> view, int x, int y) :
	window_(window), view_(view), width_(view->width()),
	height_(view->height()), x_(x), y_(y), needs_update_(true),
	use_gpu_(Platform::instance().gpu_driver()) {
	if (use_gpu_)
		driver_.reset((GPUDriverD3D11*)Platform::instance().gpu_driver());

	window_->overlay_manager()->Add(this);
}

Overlay::~Overlay()
{
	if (Application::instance()) {
		window_->overlay_manager()->Remove(this);

		if (use_gpu_ && vertices_.size() && driver_)
			driver_->DestroyGeometry(geometry_id_);
	}
}

RefPtr<Overlay> Overlay::Create(RefPtr<Window> window, uint32_t width, uint32_t height, int x, int y)
{
	return AdoptRef(*(new Overlay(window, width, height, x, y)));
}

RefPtr<Overlay> Overlay::Create(RefPtr<Window> window, RefPtr<View> view, int x, int y)
{
	return AdoptRef(*(new Overlay(window, view, x, y)));
}

void Overlay::Paint()
{
	if (is_hidden_)
		return;

	if (use_gpu_) {
		UpdateGeometry();

		driver_->DrawGeometry(geometry_id_, 6, 0, gpu_state_);
	}
	else if (view()->surface()) {
		Surface* surface = view()->surface();
		window_->DrawSurface(x_, y_, surface);
	}

	needs_update_ = false;
}

void Overlay::Resize(uint32_t width, uint32_t height)
{
	// Clamp each dimension to 2
	width = width <= 2 ? 2 : width;
	height = height <= 2 ? 2 : height;

	if (width == width_ && height == height_)
		return;

	view_->Resize(width, height);

	width_ = width;
	height_ = height;
	needs_update_ = true;

	if (use_gpu_) {
		UpdateGeometry();

		// Update these now since they were invalidated
		RenderTarget target = view_->render_target();
		gpu_state_.texture_1_id = target.texture_id;
		gpu_state_.viewport_width = window_->width();
		gpu_state_.viewport_height = window_->height();
	}
}

void Overlay::UpdateGeometry()
{
	if (!use_gpu_)
		return;

	bool initial_creation = false;
	RenderTarget target = view_->render_target();

	if (vertices_.empty()) {
		vertices_.resize(4);
		indices_.resize(6);

		auto& config = Platform::instance().config();
		if (config.face_winding == FaceWinding::Clockwise)
			memcpy(indices_.data(), patternCW, sizeof(IndexType) * indices_.size());
		else
			memcpy(indices_.data(), patternCCW, sizeof(IndexType) * indices_.size());

		memset(&gpu_state_, 0, sizeof(gpu_state_));
		Matrix identity;
		identity.SetIdentity();

		gpu_state_.viewport_width = window_->width();
		gpu_state_.viewport_height = window_->height();
		gpu_state_.transform = identity.GetMatrix4x4();
		gpu_state_.enable_blend = true;
		gpu_state_.enable_texturing = true;
		gpu_state_.shader_type = ShaderType::Fill;
		gpu_state_.render_buffer_id = window_->render_buffer_id();
		gpu_state_.texture_1_id = target.texture_id;

		initial_creation = true;
	}

	if (!needs_update_)
		return;

	Vertex_2f_4ub_2f_2f_28f v;
	memset(&v, 0, sizeof(v));

	v.data0[0] = 1; // Fill Type: Image

	v.color[0] = 255;
	v.color[1] = 255;
	v.color[2] = 255;
	v.color[3] = 255;

	float left = static_cast<float>(x_);
	float top = static_cast<float>(y_);
	float right = static_cast<float>(x_ + width());
	float bottom = static_cast<float>(y_ + height());

	// TOP LEFT
	v.pos[0] = v.obj[0] = left;
	v.pos[1] = v.obj[1] = top;
	v.tex[0] = target.uv_coords.left;
	v.tex[1] = target.uv_coords.top;

	vertices_[0] = v;

	// TOP RIGHT
	v.pos[0] = v.obj[0] = right;
	v.pos[1] = v.obj[1] = top;
	v.tex[0] = target.uv_coords.right;
	v.tex[1] = target.uv_coords.top;

	vertices_[1] = v;

	// BOTTOM RIGHT
	v.pos[0] = v.obj[0] = right;
	v.pos[1] = v.obj[1] = bottom;
	v.tex[0] = target.uv_coords.right;
	v.tex[1] = target.uv_coords.bottom;

	vertices_[2] = v;

	// BOTTOM LEFT
	v.pos[0] = v.obj[0] = left;
	v.pos[1] = v.obj[1] = bottom;
	v.tex[0] = target.uv_coords.left;
	v.tex[1] = target.uv_coords.bottom;

	vertices_[3] = v;

	ultralight::VertexBuffer vbuffer;
	vbuffer.format = ultralight::VertexBufferFormat::_2f_4ub_2f_2f_28f;
	vbuffer.size = static_cast<uint32_t>(sizeof(ultralight::Vertex_2f_4ub_2f_2f_28f) * vertices_.size());
	vbuffer.data = (uint8_t*)vertices_.data();

	ultralight::IndexBuffer ibuffer;
	ibuffer.size = static_cast<uint32_t>(sizeof(ultralight::IndexType) * indices_.size());
	ibuffer.data = (uint8_t*)indices_.data();

	if (initial_creation) {
		geometry_id_ = driver_->NextGeometryId();
		driver_->CreateGeometry(geometry_id_, vbuffer, ibuffer);
	}
	else {
		driver_->UpdateGeometry(geometry_id_, vbuffer, ibuffer);
	}

	needs_update_ = false;
}

void Overlay::Hide()
{
	is_hidden_ = true;
}

void Overlay::Show()
{
	is_hidden_ = false;
	needs_update_ = true;
}

void Overlay::Focus()
{
	window_->overlay_manager()->FocusOverlay((Overlay*)this);
}

void Overlay::Unfocus()
{
	if (has_focus())
		window_->overlay_manager()->UnfocusAll();
}

void Overlay::MoveTo(int x, int y)
{
	x_ = x;
	y_ = y;
	needs_update_ = true;
}

bool Overlay::NeedsRepaint()
{
	return needs_update_ || view_->needs_paint();
}
