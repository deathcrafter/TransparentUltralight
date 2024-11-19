#pragma once
#include <Windows.h>
#include <tchar.h>

#include "Library\Application.h"
#include "Library\Overlay.h"

int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR lpCmdLine,
	_In_ int nCmdShow)
{
	Settings settings;
	settings.app_name = "UltralightTransparent";
	settings.developer_name = "deathcrafter";
	//settings.force_cpu_render = true;

	Config config;
	config.user_stylesheet = "html, body { background: #00000001; }";
	
	auto app = Application::Create(settings, config);

	auto window = Window::Create(app->main_monitor(), 400, 60, false, WS_POPUP);

	auto vwCfg = ViewConfig();
	vwCfg.is_transparent = true;

	auto overlay = Overlay::Create(window, window->width(), window->height(), 0, 0, vwCfg);

	overlay->view()->LoadURL("file:///apps/test/index.html");

	app->Run();

	return 0;
}