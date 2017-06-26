#include "stdafx.h"
#include "CachedFunction.h"
#include <sstream>

using namespace std;

CachedFunction::CachedFunction()
	: mFunctionId(0),
	mMdTok(0) {
}

CachedFunction::CachedFunction(int functionId, mdToken mdTok, const string typeName, const string funcName, const string modName)
	: mFunctionId(functionId),
	mMdTok(mdTok),
	mTypeName(typeName),
	mFuncName(funcName),
	mModName(modName) {
}

CachedFunction::~CachedFunction() {
}

int CachedFunction::getFunctionId() const {
	return this->mFunctionId;
}

mdToken CachedFunction::getMdTok() const {
	return this->mMdTok;
}

string CachedFunction::getTypeName() const {
	return this->mTypeName;
}

string CachedFunction::getFuncName() const {
	return this->mFuncName;
}

string CachedFunction::getFullyQualifiedName() const {
	stringstream ss;
	ss << mTypeName << "." << mFuncName;
	return ss.str();
}

std::string CachedFunction::getModName() const {
	return this->mModName;
}
