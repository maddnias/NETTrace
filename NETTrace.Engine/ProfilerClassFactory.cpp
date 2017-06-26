#include "stdafx.h"
#include "ProfilerClassFactory.h"
#include "ProfilerCallbackHandler.h"

ProfilerClassFactory::ProfilerClassFactory()
	: mRefCount(0) {
}

ProfilerClassFactory::~ProfilerClassFactory() {
}

HRESULT STDMETHODCALLTYPE ProfilerClassFactory::QueryInterface(REFIID riid, void **ppvObject) {
	if (riid == IID_IUnknown || riid == IID_IClassFactory) {
		*ppvObject = this;
		this->AddRef();
		return S_OK;
	}

	*ppvObject = nullptr;
	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE ProfilerClassFactory::AddRef() {
	return atomic_fetch_add(&this->mRefCount, 1) + 1;
}

ULONG STDMETHODCALLTYPE ProfilerClassFactory::Release() {
	int count = atomic_fetch_sub(&this->mRefCount, 1) - 1;
	if (count <= 0) {
		delete this;
	}

	return count;
}

HRESULT STDMETHODCALLTYPE ProfilerClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject) {
	if (pUnkOuter != nullptr) {
		*ppvObject = nullptr;
		return CLASS_E_NOAGGREGATION;
	}

	ProfilerCallbackHandler *profiler = new ProfilerCallbackHandler();
	if (profiler == nullptr) {
		return E_FAIL;
	}

	return profiler->QueryInterface(riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE ProfilerClassFactory::LockServer(BOOL fLock) {
	return S_OK;
}