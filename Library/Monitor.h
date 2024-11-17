#pragma once
#include <stdint.h>
#include <Windows.h>
#include "WindowsUtil.h"


class Monitor {
public:
	Monitor(WindowsUtil* util);
	virtual ~Monitor() {}

	virtual double scale() const;

	virtual uint32_t width() const;

	virtual uint32_t height() const;

protected:
	WindowsUtil* util_;
	HMONITOR monitor_;
};