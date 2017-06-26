#include "stdafx.h"
#include "TraceSettings.h"
#include <boost/unordered/unordered_map.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include "Constants.h"

using namespace std;
using namespace boost::property_tree;

TraceSettings::TraceSettings() {
}

TraceSettings::~TraceSettings() {
}

string TraceSettings::getSettingsFile() {
	char *settingsFileBuf = nullptr;
	size_t strSize = 0;

	if (_dupenv_s(&settingsFileBuf, &strSize, settingsFileName) == 0 && settingsFileBuf)
		return string(settingsFileBuf, strSize);

	return string();
}

bool TraceSettings::parseTracedModules() {
	for (const auto &itm : mSettingsRoot.get_child("modules")) {
		string name = itm.second.get_value<string>();
		// Convert to lowercase just to be certain
		ConvertUtils::toLowerCase(name);
		mTracedModules.push_back(name);
	}

	return true;
}

bool TraceSettings::loadSettings() {
	string settingsFile = getSettingsFile();
	if (settingsFile.empty())
		return false;

	// Make sure file actually exists on disk and is valid
	DWORD fileAttrib = GetFileAttributesA(settingsFile.c_str());
	if (fileAttrib != INVALID_FILE_ATTRIBUTES &&
		!(fileAttrib & FILE_ATTRIBUTE_DIRECTORY))
		return false;

	// Trim off quotations (boost don't allow it like GetFileAttributes does)
	settingsFile = settingsFile.substr(1, settingsFile.length() - 3);

	try {
		read_json(settingsFile, this->mSettingsRoot);
	}
	catch (...) {
		// Unexpected error, abort tracing
		return false;
	}

	if (!parseTracedModules())
		return false;

	return true;
}

bool TraceSettings::isModuleTraced(string moduleName) {
	return std::find(mTracedModules.begin(), mTracedModules.end(), moduleName) != mTracedModules.end();
}
