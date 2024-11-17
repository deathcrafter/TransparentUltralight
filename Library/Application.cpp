#include "Application.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <sstream>

#include "FileSystemImpl.h"
#include "FontLoaderImpl.h"
#include "helpers/FileSystemHelpers.h"
#include "helpers/LogHelpers.h"

static String GetModulePath() {
	HMODULE hModule = GetModuleHandleW(NULL);
	WCHAR module_path[MAX_PATH];
	GetModuleFileNameW(hModule, module_path, MAX_PATH);
	PathRemoveFileSpecW(module_path);
	return String16(module_path, lstrlenW(module_path));
}

static String GetRoamingAppDataPath() {
	PWSTR appDataPath = NULL;
	SHGetKnownFolderPath(FOLDERID_RoamingAppData, NULL, NULL, &appDataPath);
	String result = String16(appDataPath, lstrlenW(appDataPath));
	::CoTaskMemFree(static_cast<void*>(appDataPath));
	return result;
}

static Application* instance_;

Application::Application(Settings settings, Config config): settings_(settings)
{
	if (Application::instance()) {
		MessageBox(NULL, L"Applicatoin instance is already created.", L"Error", MB_OK);
		exit(1);
	}
	windows_util_.reset(new WindowsUtil());
	windows_util_->EnableDPIAwareness();

	main_monitor_.reset(new Monitor(windows_util_.get()));

	// Generate cache path. Required for persisted sessions
	String cache_path = GetRoamingAppDataPath();
	cache_path = FileSystemHelpers::AppendPath(cache_path, settings_.developer_name);
	cache_path = FileSystemHelpers::AppendPath(cache_path, settings_.app_name);

	int res = SHCreateDirectory(NULL, cache_path.utf16().data());

	if (!(res == ERROR_SUCCESS || res == ERROR_FILE_EXISTS || res == ERROR_ALREADY_EXISTS)) {
		MessageBox(NULL, L"Couldn't create cache path.", L"Error", MB_OK);
		exit(1);
	}

	std::ostringstream info;

	if (!Platform::instance().logger()) {
		String log_path = FileSystemHelpers::AppendPath(cache_path, "ultralight.log");

		logger_.reset(new FileLogger(log_path));
		Platform::instance().set_logger(logger_.get());

		info << "Writing log to: " << log_path.utf8().data() << std::endl;
		OutputDebugStringA(info.str().c_str());
	}

	String module_path = GetModulePath();
	config.cache_path = cache_path.utf16();
	config.face_winding = FaceWinding::Clockwise;
	Platform::instance().set_config(config);

	if (!Platform::instance().font_loader()) {
		Platform::instance().set_font_loader(new FontLoaderImpl());
	}

	if (!Platform::instance().file_system()) {
		// Replace forward slashes with backslashes for proper path
	// resolution on Windows
		std::wstring fs_str = settings_.file_system_base.utf16().data();
		std::replace(fs_str.begin(), fs_str.end(), L'/', L'\\');

		String file_system_path = FileSystemHelpers::AppendPath(module_path, String16(fs_str.data(), fs_str.length()));

		Platform::instance().set_file_system(new FileSystemImpl(file_system_path.utf16().data()));

		info.clear();
		info << "File system base directory resolved to: " << file_system_path.utf8().data();
		UL_LOG_INFO(info.str().c_str());
	}

	clipboard_.reset(new ClipboardImpl());
	Platform::instance().set_clipboard(clipboard_.get());

	if (settings_.force_cpu_render) {
		surface_factory_.reset(new DIBSurfaceFactory(GetDC(NULL)));
		Platform::instance().set_surface_factory(surface_factory_.get());
	}
	else {
		gpu_context_.reset(new GPUContextD3D11());
		if (gpu_context_->device()) {
			gpu_driver_.reset(new GPUDriverD3D11(gpu_context_.get()));
			Platform::instance().set_gpu_driver(gpu_driver_.get());
		}
		else {
			gpu_context_.reset();
		}
	}

	renderer_ = Renderer::Create();

	instance_ = this;
}

void Application::Run()
{
	if (is_running_)
		return;

	std::chrono::milliseconds interval_ms(2);
	std::chrono::steady_clock::time_point next_paint = std::chrono::steady_clock::now();

	is_running_ = true;
	while (is_running_) {
		auto timeout_ns = next_paint - std::chrono::steady_clock::now();
		long long timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout_ns).count();
		DWORD timeout = timeout_ms <= 0 ? 0 : (DWORD)timeout_ms;
		DWORD result = (timeout
			? MsgWaitForMultipleObjects(0, 0, TRUE, timeout, QS_ALLEVENTS)
			: WAIT_TIMEOUT);
		if (result == WAIT_TIMEOUT) {
			Update();
			for (auto window : windows_) {
				if (window->NeedsRepaint())
					InvalidateRect(window->hwnd(), nullptr, false);
			}
			next_paint = std::chrono::steady_clock::now() + interval_ms;
		}

		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				return;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void Application::Quit()
{
	is_running_ = false;
}

Application::~Application()
{
	Platform::instance().set_gpu_driver(nullptr);
	Platform::instance().set_clipboard(nullptr);
	Platform::instance().set_file_system(nullptr);
	Platform::instance().set_font_loader(nullptr);
	Platform::instance().set_logger(nullptr);
	Platform::instance().set_surface_factory(nullptr);
	gpu_driver_.reset();
	gpu_context_.reset();
}

RefPtr<Application> Application::Create(Settings settings, Config config)
{
	return AdoptRef(*(new Application(settings, config)));
}

Application* Application::instance()
{
	return instance_;
}

void Application::Update()
{
	renderer()->Update();
}

