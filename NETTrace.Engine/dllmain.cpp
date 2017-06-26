#include "stdafx.h"
#include "ProfilerClassFactory.h"

const IID IID_IUnknown = {0x00000000, 0x0000, 0x0000,{0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
const IID IID_IClassFactory = {0x00000001, 0x0000, 0x0000,{0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

BOOL STDMETHODCALLTYPE DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
	return TRUE;
}

extern "C" HRESULT STDMETHODCALLTYPE DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
	// {8356E05C-ED29-43D2-8EAD-06BEBA7DF8A3}
	const GUID CLSID_CorProfiler = { 0x8356e05c, 0xed29, 0x43d2,{ 0x8e, 0xad, 0x6, 0xbe, 0xba, 0x7d, 0xf8, 0xa3 } };

	if (ppv == nullptr || rclsid != CLSID_CorProfiler) {
		return E_FAIL;
	}

	auto factory = new ProfilerClassFactory;
	if (factory == nullptr) {
		return E_FAIL;
	}

	return factory->QueryInterface(riid, ppv);
}

extern "C" HRESULT STDMETHODCALLTYPE DllCanUnloadNow() {
	return S_OK;
}
