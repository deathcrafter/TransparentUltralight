#include "FileLogger.h"
#include <iostream>


FileLogger::FileLogger(const String& log_path) : log_file_(log_path.utf8().data()) {
	if (!log_file_.is_open()) {
		std::cerr << "Could not open log file for writing with path: " <<
			log_path.utf8().data() << std::endl;
	}
}

FileLogger::~FileLogger() {
}

void FileLogger::LogMessage(LogLevel log_level, const String& message) {
	if (!log_file_.is_open())
		return;

	log_file_ << "> " << String(message).utf8().data() << std::endl << std::endl;
}
