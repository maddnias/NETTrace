#pragma once

#include "unknwn.h"
#include <atomic>

class ProfilerClassFactory
	: public IClassFactory {
private:
	std::atomic<int> mRefCount;

public:
	ProfilerClassFactory();
	virtual ~ProfilerClassFactory();

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override;
	ULONG   STDMETHODCALLTYPE AddRef() override;
	ULONG   STDMETHODCALLTYPE Release() override;
	HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject) override;
	HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock) override;
};