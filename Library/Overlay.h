#pragma once
#include <Ultralight/View.h>
#include <Ultralight/RefPtr.h>

#include "Window.h"
#include "gpu/GPUDriver.h"
#include "RefCountedImpl.h"

using namespace ultralight;

class Overlay: public RefCountedImpl<Overlay>
{
public:
	static RefPtr<Overlay> Create(RefPtr<Window> window, uint32_t width, uint32_t height, int x, int y, ViewConfig cfg = ViewConfig());
	static RefPtr<Overlay> Create(RefPtr<Window> window, RefPtr<View> view, int x, int y);

	void Paint();
	void Resize(uint32_t width, uint32_t height);
	void UpdateGeometry();

	void Hide();
	void Show();

	void Focus();
	void Unfocus();

	void MoveTo(int x, int y);

	bool NeedsRepaint();

	RefPtr<View> view() { return view_; }

	uint32_t width() const { return width_; }

	uint32_t height() const { return height_; }

	int x() const { return x_; }

	int y() const { return y_; }

	bool is_hidden() const { return is_hidden_; }

	bool has_focus() const {
		return window_->overlay_manager()->IsOverlayFocused((Overlay*)this);
	}

	REF_COUNTED_IMPL(Overlay);

protected:
	Overlay(RefPtr<Window> window, uint32_t width, uint32_t height, int x, int y, ViewConfig cfg);
	Overlay(RefPtr<Window> window, RefPtr<View> view, int x, int y);
	~Overlay();

	DISALLOW_COPY_AND_ASSIGN(Overlay);

	RefPtr<Window> window_;

	uint32_t width_;
	uint32_t height_;
	int x_;
	int y_;
	bool is_hidden_ = false;
	bool use_gpu_ = true;

	RefPtr<View> view_;

	std::unique_ptr<GPUDriverD3D11> driver_;
	std::vector<Vertex_2f_4ub_2f_2f_28f> vertices_;
	std::vector<IndexType> indices_;
	bool needs_update_;
	uint32_t geometry_id_;
	GPUState gpu_state_;
};

