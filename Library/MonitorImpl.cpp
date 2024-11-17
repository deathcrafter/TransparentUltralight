#include "Monitor.h"

Monitor::Monitor(WindowsUtil* util) : util_(util) {
	monitor_ = MonitorFromPoint({ 1,1 }, MONITOR_DEFAULTTONEAREST);
}

double Monitor::scale() const {
	return util_->GetMonitorDPI(monitor_);
}

uint32_t Monitor::width() const {
	MONITORINFO info;
	info.cbSize = sizeof(info);
	if (!GetMonitorInfo(monitor_, &info))
		MessageBoxW(NULL, (LPCWSTR)L"GetMonitorInfo failed", (LPCWSTR)L"Notification", MB_OK);

	return (uint32_t)abs(info.rcMonitor.left - info.rcMonitor.right);
}

uint32_t Monitor::height() const {
	MONITORINFO info;
	info.cbSize = sizeof(info);
	if (!GetMonitorInfo(monitor_, &info))
		MessageBoxW(NULL, (LPCWSTR)L"GetMonitorInfo failed", (LPCWSTR)L"Notification", MB_OK);

	return (uint32_t)abs(info.rcMonitor.top - info.rcMonitor.bottom);
}