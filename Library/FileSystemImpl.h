#pragma once
#include <Ultralight/platform/FileSystem.h>
#include <Windows.h>
#include <memory>

#pragma comment (lib, "shlwapi.lib")

using namespace ultralight;

class FileSystemImpl : public ultralight::FileSystem {
public:
	// Construct FileSystem instance.
	// 
	// @note You can pass a valid baseDir here which will be prepended to
	//       all file paths. This is useful for making all File URLs relative
	//       to your HTML asset directory.
	FileSystemImpl(LPCWSTR baseDir);

	virtual ~FileSystemImpl();

	virtual bool FileExists(const String& file_path) override;

	virtual String GetFileMimeType(const String& file_path) override;

	virtual String GetFileCharset(const String& file_path) override;

	virtual RefPtr<Buffer> OpenFile(const String& file_path) override;

protected:
	std::unique_ptr<WCHAR[]> GetRelative(const String& path);

	std::unique_ptr<WCHAR[]> baseDir_;
};