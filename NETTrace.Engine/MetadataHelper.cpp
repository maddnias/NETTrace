#include "stdafx.h"
#include "MetadataHelper.h"
#include "Constants.h"

using namespace std;

shared_ptr<CachedFunction> MetadataHelper::cacheFunction(int functionId, shared_ptr<CachedFunction> func,
	const CachedFuncMap::iterator &lb) {
	mFuncMap.insert(lb, CachedFuncMap::value_type(functionId, func));
	return func;
}

MetadataHelper::MetadataHelper(ICorProfilerInfo7 *profilerInfo)
	: mProfilerInfo(profilerInfo) {
}

MetadataHelper::~MetadataHelper() {
}

shared_ptr<CachedFunction> MetadataHelper::getMappedFunction(int functionId) {
	CachedFuncMap::iterator lb = mFuncMap.lower_bound(functionId);
	if (lb != mFuncMap.end() && !mFuncMap.key_comp()(functionId, lb->first))
		return mFuncMap[functionId];
	return nullptr;
}

// Could just return it as const if it's not gonna be mutable
shared_ptr<CachedFunction> MetadataHelper::mapFunction(int functionId, string modName) {
	// Check if it's already cached
	CachedFuncMap::iterator lb = mFuncMap.lower_bound(functionId);
	if (lb != mFuncMap.end() && !mFuncMap.key_comp()(functionId, lb->first)) 
		return mFuncMap[functionId];

	IMetaDataImport *pIMetaDataImport = nullptr;
	HRESULT hr;
	mdToken funcToken = 0;

	wstring fullMethodName(510, '0');
	wstring funcName(255, '0');
	wstring className(255, '0');

	// get the token for the function which we will use to get its name
	hr = this->mProfilerInfo->GetTokenAndMetaDataFromFunction(functionId, IID_IMetaDataImport,
		reinterpret_cast<LPUNKNOWN *>(&pIMetaDataImport), &funcToken);

	if (FAILED(hr))
		return nullptr;

	mdTypeDef classTypeDef;
	ULONG cchFunction;
	ULONG cchClass;

	// retrieve the function properties based on the token
	hr = pIMetaDataImport->GetMethodProps(funcToken, &classTypeDef,
		reinterpret_cast<LPWSTR>(&funcName[0]), funcName.length(), &cchFunction, 0, 0, 0, 0, 0);

	if (FAILED(hr)) {
		pIMetaDataImport->Release();
		return nullptr;
	}

	// get the function name
	hr = pIMetaDataImport->GetTypeDefProps(classTypeDef, reinterpret_cast<LPWSTR>(&className[0]),
		className.length(), &cchClass, 0, 0);

	if (FAILED(hr)) {
		pIMetaDataImport->Release();
		return nullptr;
	}

	// release our reference to the metadata
	pIMetaDataImport->Release();

	// Cache the function for potential later use
	auto cachedFunc = make_shared<CachedFunction>(functionId, funcToken, string(className.begin(),
		className.end()), string(funcName.begin(), funcName.end()), modName);

	if (SUCCEEDED(hr)) {
		return this->cacheFunction(functionId, cachedFunc, lb);
	}

	return nullptr;
}
