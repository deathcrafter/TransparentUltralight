#pragma once
#include <memory>

#include <Windows.h>

#include <Ultralight/Bitmap.h>
#include <Ultralight/KeyEvent.h>
#include <Ultralight/Listener.h>
#include <Ultralight/MouseEvent.h>
#include <Ultralight/platform/Surface.h>
#include <Ultralight/RefPtr.h>
#include <Ultralight/ScrollEvent.h>

#include "gpu/SwapChain.h"
#include "Monitor.h"
#include "OverlayManager.h"
#include "RefCountedImpl.h"

using namespace ultralight;

struct WindowData {
	class Window* window;
	MouseEvent::Button cur_btn;
	bool is_resizing_modal;
	bool is_mouse_in_client;
	bool is_tracking_mouse;
	TRACKMOUSEEVENT track_mouse_event_data;
};

class Window : public OverlayManager, public RefCountedImpl<Window>
{
public:
	static RefPtr<Window> Create(Monitor* monitor, uint32_t width, uint32_t height, bool fullscreen,
		DWORD window_flags);

	virtual uint32_t screen_width() const { return PixelsToScreen(width()); }

	virtual uint32_t width() const;

	virtual uint32_t screen_height() const { return PixelsToScreen(height()); }

	virtual uint32_t height() const;

	virtual bool is_fullscreen() const { return is_fullscreen_; }

	virtual bool is_accelerated() const { return is_accelerated_; }

	virtual uint32_t render_buffer_id() const {
		return swap_chain_ ? swap_chain_->render_buffer_id() : NULL;
	}

	virtual double scale() const { return scale_; }

	virtual void MoveTo(int x, int y);

	virtual void MoveToCenter();

	virtual int x() const;

	virtual int y() const;

	virtual void SetTitle(const char* title);

	virtual void SetCursor(ultralight::Cursor cursor);

	virtual void Show();

	virtual void Hide();

	virtual bool is_visible() const;

	virtual void Close();

	virtual int ScreenToPixels(int val) const { return (int)round(val * scale()); }

	virtual int PixelsToScreen(int val) const { return (int)round(val / scale()); }

	virtual void DrawSurface(int x, int y, Surface* surface);

	virtual OverlayManager* overlay_manager() const { return const_cast<Window*>(this); }

	// Inherited from OverlayManager
	virtual void Paint() override;

	virtual void FireKeyEvent(const ultralight::KeyEvent& evt) override;

	virtual void FireMouseEvent(const ultralight::MouseEvent& evt) override;

	virtual void FireScrollEvent(const ultralight::ScrollEvent& evt) override;

	HWND hwnd() { return hwnd_; }

	// These are called by WndProc then forwarded to listener(s)
	void OnClose();
	void OnResize(uint32_t width, uint32_t height);
	void OnChangeDPI(double scale, const RECT* suggested_rect);

	void InvalidateWindow() {
		InvalidateRect(hwnd_, nullptr, false);
		window_needs_repaint_ = true;
	}

	void PaintLayeredWindow(HDC dc);

	REF_COUNTED_IMPL(Window);
protected:
	Window(Monitor* monitor, uint32_t width, uint32_t height, bool fullscreen,
		DWORD window_flags);
	~Window();

	void AddWindowExStyle(LONG_PTR flag);
	void RemoveWindowExStyle(LONG_PTR flag);

	void PaintTransparent(HDC source, int alpha = 0);

	DISALLOW_COPY_AND_ASSIGN(Window);

	bool is_first_paint_ = true;
	bool window_needs_repaint_ = false;
	Monitor* monitor_;
	double scale_;
	bool is_fullscreen_;
	bool is_accelerated_;
	HWND hwnd_;
	HCURSOR cursor_hand_;
	HCURSOR cursor_arrow_;
	HCURSOR cursor_ibeam_;
	HCURSOR cursor_size_all_;
	HCURSOR cursor_size_north_east_;
	HCURSOR cursor_size_north_south_;
	HCURSOR cursor_size_north_west_;
	HCURSOR cursor_size_west_east_;
	Cursor cur_cursor_;
	WindowData window_data_;
	DWORD style_;

	std::unique_ptr<SwapChainD3D11> swap_chain_;

	friend class Application;
	friend class Overlay;
};

