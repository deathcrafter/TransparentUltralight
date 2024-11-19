#include "Window.h"

#include <ShellScalingAPI.h>
#include <tchar.h>
#include <windowsx.h>
#include <dwmapi.h>

#include "Application.h"
#include "DIBSurface.h"
#include "Monitor.h"
#include "Overlay.h"

#include "gpu/GPUDriver.h"
#include "gpu/GPUContext.h"

#pragma comment (lib, "Dwmapi.lib")

#define WINDOWDATA() ((WindowData*)GetWindowLongPtr(hWnd, GWLP_USERDATA))
#define WINDOW() (WINDOWDATA()->window)
#define WM_DPICHANGED_ 0x02E0

static HDC g_dc = 0;

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message) {
	case WM_PAINT:
		// Ignore WM_PAINT as we are creating a layered window

		/*g_dc = hdc = BeginPaint(hWnd, &ps);
		WINDOW()->InvalidateWindow();
		WINDOW()->Paint();
		EndPaint(hWnd, &ps);
		g_dc = 0;*/
		break;
	case WM_DESTROY:
		WINDOW()->OnClose();
		break;
	case WM_ENTERSIZEMOVE:
		WINDOWDATA()->is_resizing_modal = true;
		break;
	case WM_SIZE: {
		if (WINDOWDATA()) {
			WINDOW()->OnResize(WINDOW()->width(), WINDOW()->height());
			// This would normally be called when the message loop is idle
			// but during resize the window consumes all messages so we need
			// to force paints during the operation.
			// static_cast<AppWin*>(App::instance())->OnPaint();
			InvalidateRect(hWnd, nullptr, false);
		}
		break;
	}
	case WM_DPICHANGED_: {
		if (WINDOWDATA()) {
			double fscale = (double)HIWORD(wParam) / USER_DEFAULT_SCREEN_DPI;
			WINDOW()->OnChangeDPI(fscale, (RECT*)lParam);
			InvalidateRect(hWnd, nullptr, false);
		}
	}
	case WM_EXITSIZEMOVE:
		WINDOWDATA()->is_resizing_modal = false;
		WINDOW()->OnResize(WINDOW()->width(), WINDOW()->height());
		InvalidateRect(hWnd, nullptr, false);
		break;
	case WM_KEYDOWN:
		WINDOW()->FireKeyEvent(
			KeyEvent(KeyEvent::kType_RawKeyDown, (uintptr_t)wParam, (intptr_t)lParam, false));
		break;
	case WM_KEYUP:
		WINDOW()->FireKeyEvent(
			KeyEvent(KeyEvent::kType_KeyUp, (uintptr_t)wParam, (intptr_t)lParam, false));
		break;
	case WM_CHAR:
		WINDOW()->FireKeyEvent(
			KeyEvent(KeyEvent::kType_Char, (uintptr_t)wParam, (intptr_t)lParam, false));
		break;
	case WM_MOUSELEAVE:
		WINDOWDATA()->is_mouse_in_client = false;
		WINDOWDATA()->is_tracking_mouse = false;
		break;
	case WM_MOUSEMOVE: {
		if (!WINDOWDATA()->is_tracking_mouse) {
			// Need to install tracker to get WM_MOUSELEAVE events.
			WINDOWDATA()->track_mouse_event_data = { sizeof(WINDOWDATA()->track_mouse_event_data) };
			WINDOWDATA()->track_mouse_event_data.dwFlags = TME_LEAVE;
			WINDOWDATA()->track_mouse_event_data.hwndTrack = hWnd;
			TrackMouseEvent(&(WINDOWDATA()->track_mouse_event_data));
			WINDOWDATA()->is_tracking_mouse = true;
		}
		if (!WINDOWDATA()->is_mouse_in_client) {
			// We need to manually set the cursor when mouse enters client area
			WINDOWDATA()->is_mouse_in_client = true;
			WINDOW()->SetCursor(ultralight::kCursor_Pointer);
		}
		WINDOW()->FireMouseEvent(
			{ MouseEvent::kType_MouseMoved, WINDOW()->PixelsToScreen(GET_X_LPARAM(lParam)),
			  WINDOW()->PixelsToScreen(GET_Y_LPARAM(lParam)), WINDOWDATA()->cur_btn });
		break;
	}
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		SetCapture(WINDOW()->hwnd());
		WINDOWDATA()->cur_btn = MouseEvent::kButton_Left;
		WINDOW()->FireMouseEvent(
			{ MouseEvent::kType_MouseDown, WINDOW()->PixelsToScreen(GET_X_LPARAM(lParam)),
			  WINDOW()->PixelsToScreen(GET_Y_LPARAM(lParam)), WINDOWDATA()->cur_btn });
		break;
	case WM_MBUTTONDOWN:
	case WM_MBUTTONDBLCLK:
		SetCapture(WINDOW()->hwnd());
		WINDOWDATA()->cur_btn = MouseEvent::kButton_Middle;
		WINDOW()->FireMouseEvent(
			{ MouseEvent::kType_MouseDown, WINDOW()->PixelsToScreen(GET_X_LPARAM(lParam)),
			  WINDOW()->PixelsToScreen(GET_Y_LPARAM(lParam)), WINDOWDATA()->cur_btn });
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONDBLCLK:
		SetCapture(WINDOW()->hwnd());
		WINDOWDATA()->cur_btn = MouseEvent::kButton_Right;
		WINDOW()->FireMouseEvent(
			{ MouseEvent::kType_MouseDown, WINDOW()->PixelsToScreen(GET_X_LPARAM(lParam)),
			  WINDOW()->PixelsToScreen(GET_Y_LPARAM(lParam)), WINDOWDATA()->cur_btn });
		break;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		ReleaseCapture();
		WINDOW()->FireMouseEvent(
			{ MouseEvent::kType_MouseUp, WINDOW()->PixelsToScreen(GET_X_LPARAM(lParam)),
			  WINDOW()->PixelsToScreen(GET_Y_LPARAM(lParam)), WINDOWDATA()->cur_btn });
		WINDOWDATA()->cur_btn = MouseEvent::kButton_None;
		break;
	case WM_MOUSEWHEEL:
		WINDOW()->FireScrollEvent(
			{ ScrollEvent::kType_ScrollByPixel, 0,
			  static_cast<int>(WINDOW()->PixelsToScreen(GET_WHEEL_DELTA_WPARAM(wParam)) * 0.8) });
		break;
	case WM_SETFOCUS:
		WINDOW()->SetWindowFocused(true);
		break;
	case WM_KILLFOCUS:
		WINDOW()->SetWindowFocused(false);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

void CenterHwndOnMainMonitor(HWND hwnd) {
	RECT rect;
	GetWindowRect(hwnd, &rect);
	LPRECT prc = &rect;

	// Get main monitor
	HMONITOR hMonitor = MonitorFromPoint({ 1, 1 }, MONITOR_DEFAULTTONEAREST);

	MONITORINFO mi;
	RECT rc;
	int w = prc->right - prc->left;
	int h = prc->bottom - prc->top;

	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	rc = mi.rcMonitor;

	prc->left = rc.left + (rc.right - rc.left - w) / 2;
	prc->top = rc.top + (rc.bottom - rc.top - h) / 2;
	prc->right = prc->left + w;
	prc->bottom = prc->top + h;

	SetWindowPos(hwnd, NULL, rect.left, rect.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

bool g_window_class_initialized = false;

Window::Window(Monitor* monitor, uint32_t width, uint32_t height, bool fullscreen, DWORD window_flags)
	: monitor_(monitor), is_fullscreen_(fullscreen), style_(window_flags)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	std::wstring class_name = L"UltralightWindow";

	if (!g_window_class_initialized) {
		WNDCLASSEX wcex;
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = WndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_APPLICATION);
		wcex.hCursor = NULL;
		wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = class_name.c_str();
		wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_APPLICATION);
		if (!RegisterClassEx(&wcex)) {
			MessageBoxW(NULL, (LPCWSTR)L"RegisterClassEx failed", (LPCWSTR)L"Notification", MB_OK);
			exit(-1);
		}
		g_window_class_initialized = true;
	}

	scale_ = monitor_->scale();

	// Create window
	RECT rc = { 0, 0, (LONG)ScreenToPixels(width), (LONG)ScreenToPixels(height) };
	AdjustWindowRect(&rc, style_, FALSE);
	hwnd_ = ::CreateWindowEx(
		NULL, class_name.c_str(), _T(""), (fullscreen ? (WS_EX_TOPMOST | WS_POPUP) : style_) | WS_EX_LAYERED,
		fullscreen ? 0 : CW_USEDEFAULT, fullscreen ? 0 : CW_USEDEFAULT,
		fullscreen ? ScreenToPixels(width) : (rc.right - rc.left),
		fullscreen ? ScreenToPixels(height) : (rc.bottom - rc.top), NULL, NULL, hInstance, NULL);

	if (!hwnd_) {
		MessageBoxW(NULL, (LPCWSTR)L"CreateWindowEx failed", (LPCWSTR)L"Notification", MB_OK);
		exit(-1);
	}

	const MARGINS Margin = { -1 };
	DwmExtendFrameIntoClientArea(hwnd_, &Margin);

	window_data_.window = this;
	window_data_.cur_btn = ultralight::MouseEvent::kButton_None;
	window_data_.is_resizing_modal = false;
	window_data_.is_mouse_in_client = false;
	window_data_.is_tracking_mouse = false;

	SetWindowLongPtr(hwnd_, GWLP_USERDATA, (LONG_PTR)&window_data_);

	CenterHwndOnMainMonitor(hwnd_);

	if (!(window_flags & WS_VISIBLE))
		ShowWindow(hwnd_, SW_SHOW);

	// Set the thread affinity mask for better clock
	::SetThreadAffinityMask(::GetCurrentThread(), 1);

	cursor_hand_ = ::LoadCursor(NULL, IDC_HAND);
	cursor_arrow_ = ::LoadCursor(NULL, IDC_ARROW);
	cursor_ibeam_ = ::LoadCursor(NULL, IDC_IBEAM);
	cursor_size_all_ = ::LoadCursor(NULL, IDC_SIZEALL);
	cursor_size_north_east_ = ::LoadCursor(NULL, IDC_SIZENESW);
	cursor_size_north_south_ = ::LoadCursor(NULL, IDC_SIZENS);
	cursor_size_north_west_ = ::LoadCursor(NULL, IDC_SIZENWSE);
	cursor_size_west_east_ = ::LoadCursor(NULL, IDC_SIZEWE);
	cur_cursor_ = ultralight::kCursor_Pointer;

	SetWindowScale(scale());

	is_accelerated_ = false;

	assert(Application::instance());

	auto gpu_context = Application::instance()->gpu_context();
	auto gpu_driver = Application::instance()->gpu_driver();
	if (gpu_context && gpu_driver) {
		swap_chain_.reset(new SwapChainD3D11(
			gpu_context, gpu_driver, hwnd(),
			screen_width(), screen_height(),
			scale(), is_fullscreen(),
			true, true, 1
		));
		if (swap_chain_->swap_chain()) {
			is_accelerated_ = true;
			gpu_context->AddSwapChain(swap_chain_.get());
		}
		else {
			swap_chain_.reset();
		}
	}

	Application::instance()->AddWindow(this);
}

Window::~Window()
{
	DestroyCursor(cursor_hand_);
	DestroyCursor(cursor_arrow_);
	DestroyCursor(cursor_ibeam_);
	DestroyCursor(cursor_size_all_);
	DestroyCursor(cursor_size_north_east_);
	DestroyCursor(cursor_size_north_south_);
	DestroyCursor(cursor_size_north_west_);
	DestroyCursor(cursor_size_west_east_);

	if (Application::instance()) {
		Application::instance()->RemoveWindow(this);

		auto gpu_context = Application::instance()->gpu_context();
		if (is_accelerated_ && gpu_context && swap_chain_) {
			gpu_context->RemoveSwapChain(swap_chain_.get());
		}
	}
}

void Window::AddWindowExStyle(LONG_PTR flag)
{
	LONG_PTR style = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
	if ((style & flag) == 0)
	{
		SetWindowLongPtr(hwnd_, GWL_EXSTYLE, style | flag);
	}
}

void Window::RemoveWindowExStyle(LONG_PTR flag)
{
	LONG_PTR style = GetWindowLongPtr(hwnd_, GWL_EXSTYLE);
	if ((style & flag) != 0)
	{
		SetWindowLongPtr(hwnd_, GWL_EXSTYLE, style & ~flag);
	}
}

RefPtr<Window> Window::Create(Monitor* monitor, uint32_t width, uint32_t height, bool fullscreen, DWORD window_flags)
{
	return AdoptRef(*(new Window(monitor, width, height, fullscreen, window_flags)));
}

uint32_t Window::width() const
{
	RECT rc;
	::GetClientRect(hwnd_, &rc);
	return rc.right - rc.left;
}

uint32_t Window::height() const
{
	RECT rc;
	::GetClientRect(hwnd_, &rc);
	return rc.bottom - rc.top;
}

void Window::MoveTo(int x, int y)
{
	x = ScreenToPixels(x);
	y = ScreenToPixels(y);
	RECT rect = { x, y, x, y };
	AdjustWindowRect(&rect, style_, FALSE);
	SetWindowPos(hwnd_, NULL, rect.left, rect.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
}

void Window::MoveToCenter()
{
	CenterHwndOnMainMonitor(hwnd());
}

int Window::x() const
{
	POINT pos = { 0, 0 };
	ClientToScreen(hwnd_, &pos);

	return pos.x;
}

int Window::y() const
{
	POINT pos = { 0, 0 };
	ClientToScreen(hwnd_, &pos);

	return pos.y;
}

void Window::SetTitle(const char* title)
{
	String16 titleUtf16 = ultralight::String(title).utf16();
	SetWindowText(hwnd_, titleUtf16.data());
}

void Window::SetCursor(ultralight::Cursor cursor)
{
	switch (cursor) {
	case ultralight::kCursor_Hand: {
		::SetCursor(cursor_hand_);
		break;
	}
	case ultralight::kCursor_Pointer: {
		::SetCursor(cursor_arrow_);
		break;
	}
	case ultralight::kCursor_IBeam: {
		::SetCursor(cursor_ibeam_);
		break;
	}
	case ultralight::kCursor_Move: {
		::SetCursor(cursor_size_all_);
		break;
	}
	case ultralight::kCursor_NorthEastResize:
	case ultralight::kCursor_SouthWestResize:
	case ultralight::kCursor_NorthEastSouthWestResize: {
		::SetCursor(cursor_size_north_east_);
		break;
	}
	case ultralight::kCursor_NorthResize:
	case ultralight::kCursor_SouthResize:
	case ultralight::kCursor_NorthSouthResize: {
		::SetCursor(cursor_size_north_south_);
		break;
	}
	case ultralight::kCursor_NorthWestResize:
	case ultralight::kCursor_SouthEastResize:
	case ultralight::kCursor_NorthWestSouthEastResize: {
		::SetCursor(cursor_size_north_east_);
		break;
	}
	case ultralight::kCursor_WestResize:
	case ultralight::kCursor_EastResize:
	case ultralight::kCursor_EastWestResize: {
		::SetCursor(cursor_size_west_east_);
		break;
	}
	};

	cur_cursor_ = cursor;
}

void Window::Show() { ShowWindow(hwnd_, SW_SHOW); }

void Window::Hide() { ShowWindow(hwnd_, SW_HIDE); }

bool Window::is_visible() const { return IsWindowVisible(hwnd_); }

void Window::Close() { DestroyWindow(hwnd_); }

void Window::DrawSurface(int x, int y, Surface* surface) {
	DIBSurface* dibSurface = static_cast<DIBSurface*>(surface);
	PaintLayeredWindow(dibSurface->dc());
}

void Window::Paint()
{
	if (!is_accelerated()) {
		OverlayManager::Render();
		OverlayManager::Paint();
		return;
	}

	auto gpu_context = Application::instance()->gpu_context();
	auto gpu_driver = Application::instance()->gpu_driver();

	gpu_driver->BeginSynchronize();
	OverlayManager::Render();
	gpu_driver->EndSynchronize();

	if (gpu_driver->HasCommandsPending() || OverlayManager::NeedsRepaint()
		|| window_needs_repaint_) {
		gpu_driver->ClearRenderBuffer(swap_chain_->render_buffer_id());
		
		gpu_context->BeginDrawing();
		
		gpu_driver->DrawCommandList();
		OverlayManager::Paint();

		gpu_context->EndDrawing();

		PaintLayeredWindow(swap_chain_->GetDC());
		swap_chain_->ReleaseDC();

		if (is_first_paint_)
			is_first_paint_ = false;
	}

	window_needs_repaint_ = false;
}

void Window::PaintLayeredWindow(HDC dc)
{
	PAINTSTRUCT ps;
	BeginPaint(hwnd(), &ps);

	BLENDFUNCTION blendPixelFunction = { AC_SRC_OVER, 0, (BYTE)255, AC_SRC_ALPHA };
	POINT ptWindowScreenPosition = { x(), y() };
	POINT ptSrc = { 0 };
	SIZE szWindow = { width(), height() };

	HDC dcMemory = dc;
	if (!UpdateLayeredWindow(hwnd_, nullptr, &ptWindowScreenPosition, &szWindow, dcMemory, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA))
	{
		// After a call to SetLayerdWindowAttribute, UpdateLayeredWindow doesn't work unless WS_EX_LAYERED is re-set.
		// Retry after resetting WS_EX_LAYERED flag.
		RemoveWindowExStyle(WS_EX_LAYERED);
		AddWindowExStyle(WS_EX_LAYERED);
		UpdateLayeredWindow(hwnd_, nullptr, &ptWindowScreenPosition, &szWindow, dcMemory, &ptSrc, 0, &blendPixelFunction, ULW_ALPHA);
	}

	EndPaint(hwnd(), &ps);
}

void Window::FireKeyEvent(const ultralight::KeyEvent& evt)
{
	OverlayManager::FireKeyEvent(evt);
}

void Window::FireMouseEvent(const ultralight::MouseEvent& evt)
{
	OverlayManager::FireMouseEvent(evt);
}

void Window::FireScrollEvent(const ultralight::ScrollEvent& evt)
{
	OverlayManager::FireScrollEvent(evt);
}

void Window::OnClose() {
	// Keep window alive in case user-callbacks release our reference.
	RefPtr<Window> retain(this);
}

void Window::OnResize(uint32_t width, uint32_t height) {
	// Keep window alive in case user-callbacks release our reference.
	RefPtr<Window> retain(this);

	if (swap_chain_)
		swap_chain_->Resize(width, height);
}

void Window::OnChangeDPI(double scale, const RECT* suggested_rect) {
	scale_ = scale;

	SetWindowScale(scale_);

	if (is_accelerated()) {
		swap_chain_->set_scale(scale_);
	}

	for (auto overlay : overlays_) {
		overlay->view()->set_device_scale(scale_);
	}

	RECT* const prcNewWindow = (RECT*)suggested_rect;
	SetWindowPos(hwnd_, NULL, prcNewWindow->left, prcNewWindow->top,
		prcNewWindow->right - prcNewWindow->left, prcNewWindow->bottom - prcNewWindow->top,
		SWP_NOZORDER | SWP_NOACTIVATE);
}
