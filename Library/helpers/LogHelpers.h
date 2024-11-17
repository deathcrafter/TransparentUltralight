#pragma once
#define UL_LOG_INFO(msg) Platform::instance().logger()->LogMessage(LogLevel::Info, String(msg));
#define UL_LOG_ERROR(msg) Platform::instance().logger()->LogMessage(LogLevel::Error, String(msg));
#define UL_LOG_WARN(msg) Platform::instance().logger()->LogMessage(LogLevel::Warning, String(msg))