#pragma once
#include <string>
#include <vector>

#include <PathCch.h>
#include <Windows.h>
#include <shlobj_core.h>

#include <Ultralight/String.h>

#pragma comment(lib, "Pathcch.lib")

using namespace ultralight;

class FileSystemHelpers
{
public:
	static String AppendPath(String original, String to_append) {
		wchar_t buffer[MAX_PATH];
		wcscpy_s(buffer, original.utf16().data());

		HRESULT hr = PathCchAppend(
			buffer,
			MAX_PATH,
			to_append.utf16().data()
		);

		if (SUCCEEDED(hr)) {
			return String16(buffer, MAX_PATH);
		}
		else {
			String msg = "Couldn't append paths: ";
			msg += original;
			msg += " + ";
			msg += to_append;
			MessageBox(NULL, msg.utf16().data(), L"Error", MB_OK);
			exit(hr);
		}
	}

	static void MakeAllDirectories(String path) {
		auto e = SHCreateDirectory(NULL, path.utf16().data());
		if (e != ERROR_SUCCESS)
		{
			String msg = "Couldn't create specified directory: ";
			msg += path;
			MessageBox(NULL, msg.utf16().data(), L"Error", MB_OK);
			exit(e);
		}
	}
};

