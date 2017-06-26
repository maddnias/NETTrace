#pragma once
#include <boost/property_tree/ptree.hpp>
#include <vector>

class TraceSettings
{
public:
	TraceSettings();
	~TraceSettings();

	static std::string getSettingsFile();
	bool parseTracedModules();
	bool loadSettings();

	bool isModuleTraced(std::string moduleName);

private:
	boost::property_tree::ptree mSettingsRoot;
	std::vector<std::string> mTracedModules;
};

