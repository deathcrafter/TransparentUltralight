#pragma once
#include <vector>

#include <Ultralight/platform/Platform.h>
#include <Ultralight/platform/Config.h>
#include <Ultralight/Renderer.h>
#include <Ultralight/String.h>

#include "gpu/GPUContext.h"
#include "gpu/GPUDriver.h"
#include "Window.h"
#include "WindowsUtil.h"
#include "ClipboardImpl.h"
#include "Monitor.h"
#include "FileLogger.h"
#include "DIBSurface.h"

using namespace ultralight;

struct Settings {
	String developer_name = "deathcrafter";
	String app_name = "TransparentUltralight";

	String file_system_base = "./assets/";
	bool load_shaders_from_file_system = false;

	bool force_cpu_render = false;
};

class Application final: RefCountedImpl<Application> {
public:
	static RefPtr<Application> Create(Settings settings = Settings(), Config config = Config());

	static Application* instance();

	const Settings& settings() const { return settings_; }

	bool is_running() const { return is_running_; }

	Monitor* main_monitor() { return main_monitor_.get(); }

	RefPtr<Renderer> renderer() { return renderer_; }

	void Run();

	void Quit();

	GPUContextD3D11* gpu_context() { return gpu_context_.get(); }
	GPUDriverD3D11* gpu_driver() { return gpu_driver_.get(); }

	REF_COUNTED_IMPL(Application);
protected:
	DISALLOW_COPY_AND_ASSIGN(Application);

	Application(Settings settings, Config config);
	~Application();

	void Update();

	std::vector<Window*> windows_;
	void AddWindow(Window* window) { windows_.push_back(window); }
	void RemoveWindow(Window* window) {
		windows_.erase(std::remove(windows_.begin(), windows_.end(), window), windows_.end());
	}

	Settings settings_;
	bool is_running_ = false;

	RefPtr<Renderer> renderer_;
	
	std::unique_ptr<WindowsUtil> windows_util_;
	std::unique_ptr<Monitor> main_monitor_;
	std::unique_ptr<ClipboardImpl> clipboard_;

	std::unique_ptr<GPUDriverD3D11> gpu_driver_;
	std::unique_ptr<GPUContextD3D11> gpu_context_;

	std::unique_ptr<DIBSurfaceFactory> surface_factory_;

	std::unique_ptr<FileLogger> logger_;

	friend class Window;
};