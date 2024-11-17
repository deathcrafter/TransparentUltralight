#pragma once
#include <Ultralight/platform/FontLoader.h>
#include <map>

#pragma comment (lib, "Dwrite.lib")

using namespace ultralight;

/**
 * FontLoader implementation for Windows.
 */
class FontLoaderImpl : public FontLoader {
public:
	FontLoaderImpl() {}
	virtual ~FontLoaderImpl() {}
	virtual String fallback_font() const override;
	virtual String fallback_font_for_characters(const String& characters, int weight, bool italic) const override;
	virtual RefPtr<FontFile> Load(const String& family, int weight, bool italic) override;
protected:
	std::map<uint32_t, RefPtr<Buffer>> fonts_;
};