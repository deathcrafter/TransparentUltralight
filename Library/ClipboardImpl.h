#pragma once
#include <Ultralight/platform/Clipboard.h>

using namespace ultralight;

class ClipboardImpl: public Clipboard
{
public:
	ClipboardImpl() {}

	virtual void Clear() override;

	virtual String ReadPlainText() override;

	virtual void WritePlainText(const String& text) override;
};

