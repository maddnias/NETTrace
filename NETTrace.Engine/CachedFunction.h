#pragma once
#include <string>

class CachedFunction {
public:
	CachedFunction();
	CachedFunction(int functionId, mdToken mdTok, const std::string typeName, const std::string funcName,
		const std::string modName);
	~CachedFunction();

	int getFunctionId() const;
	mdToken getMdTok() const;
	std::string getTypeName() const;
	std::string getFuncName() const;
	std::string getFullyQualifiedName() const;
	std::string getModName() const;

private:
	// We'll see if these need to be mutable at some point
	int mFunctionId;
	mdToken mMdTok;
	std::string mTypeName;
	std::string mFuncName;
	std::string mModName;
};

