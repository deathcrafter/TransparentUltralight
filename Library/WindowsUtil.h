#pragma once
#include <Windows.h>
#include <VersionHelpers.h>
#include <ShellScalingApi.h>

class WindowsUtil {
public:
	WindowsUtil() {
		shcore_lib_ = LoadLibraryA("shcore.dll");
		if (shcore_lib_) {
			setProcessDpiAwareness_ = (FN_SetProcessDpiAwareness)
				GetProcAddress(shcore_lib_, "SetProcessDpiAwareness");
			getDpiForMonitor_ = (FN_GetDpiForMonitor)
				GetProcAddress(shcore_lib_, "GetDpiForMonitor");
		}

		ntdll_lib_ = LoadLibraryA("ntdll.dll");
		if (ntdll_lib_) {
			rtlVerifyVersionInfo_
				= (FN_RtlVerifyVersionInfo)GetProcAddress(ntdll_lib_, "RtlVerifyVersionInfo");
		}
	};
	~WindowsUtil() {
		if (shcore_lib_)
			FreeLibrary(shcore_lib_);
		if (ntdll_lib_)
			FreeLibrary(ntdll_lib_);
	};

	void EnableDPIAwareness() {
		if (IsWindows8Point1OrGreater())
			setProcessDpiAwareness_(PROCESS_PER_MONITOR_DPI_AWARE);
		else if (IsWindowsVistaOrGreater())
			SetProcessDPIAware();
	};

	double GetMonitorDPI(HMONITOR monitor) {
		UINT xdpi, ydpi;

		if (IsWindows8Point1OrGreater()) {
			getDpiForMonitor_(monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
		}
		else {
			const HDC dc = GetDC(NULL);
			xdpi = GetDeviceCaps(dc, LOGPIXELSX);
			ydpi = GetDeviceCaps(dc, LOGPIXELSY);
			ReleaseDC(NULL, dc);
		}

		// We only care about DPI in x-dimension right now
		return xdpi / 96.f;
	};

protected:

	BOOL IsWindowsVersionOrGreaterWin32(WORD major, WORD minor, WORD sp) {
		if (!rtlVerifyVersionInfo_)
			return false;

		OSVERSIONINFOEXW osvi = { sizeof(osvi), major, minor, 0, 0, { 0 }, sp };
		DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR;
		ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
		cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
		cond = VerSetConditionMask(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
		// HACK: Use RtlVerifyVersionInfo instead of VerifyVersionInfoW as the
		//       latter lies unless the user knew to embed a non-default manifest
		//       announcing support for Windows 10 via supportedOS GUID
		return rtlVerifyVersionInfo_(&osvi, mask, cond) == 0;
	};

	BOOL IsWindows10BuildOrGreaterWin32(WORD build) {
		if (!rtlVerifyVersionInfo_)
			return false;

		OSVERSIONINFOEXW osvi = { sizeof(osvi), 10, 0, build };
		DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER;
		ULONGLONG cond = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
		cond = VerSetConditionMask(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
		cond = VerSetConditionMask(cond, VER_BUILDNUMBER, VER_GREATER_EQUAL);
		// HACK: Use RtlVerifyVersionInfo instead of VerifyVersionInfoW as the
		//       latter lies unless the user knew to embed a non-default manifest
		//       announcing support for Windows 10 via supportedOS GUID
		return rtlVerifyVersionInfo_(&osvi, mask, cond) == 0;
	};

	typedef HRESULT(WINAPI* FN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
	typedef HRESULT(WINAPI* FN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
	typedef LONG(WINAPI* FN_RtlVerifyVersionInfo)(OSVERSIONINFOEXW*, ULONG, ULONGLONG);
	FN_SetProcessDpiAwareness setProcessDpiAwareness_ = nullptr;
	FN_GetDpiForMonitor getDpiForMonitor_ = nullptr;
	FN_RtlVerifyVersionInfo rtlVerifyVersionInfo_ = nullptr;
	HMODULE shcore_lib_ = nullptr;
	HMODULE ntdll_lib_ = nullptr;
};
