#pragma once

#include <map>
#include "CachedFunction.h"
#include <memory>

typedef std::map<int, std::shared_ptr<CachedFunction>> CachedFuncMap;

class MetadataHelper
{
private:
	ICorProfilerInfo7 *mProfilerInfo;
	CachedFuncMap mFuncMap;

	std::shared_ptr<CachedFunction> cacheFunction(int functionId, std::shared_ptr<CachedFunction> functionName, const CachedFuncMap::iterator &lb);

public:
	MetadataHelper(ICorProfilerInfo7 *profilerInfo);
	virtual ~MetadataHelper();

	std::shared_ptr<CachedFunction> getMappedFunction(int functionId);
	std::shared_ptr<CachedFunction> mapFunction(int functionId, std::string modName = "");
};

