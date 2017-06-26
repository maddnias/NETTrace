#include "stdafx.h"
#include "ProfilerCallbackHandler.h"
#include "ConvertUtils.h"

using std::wstring;
using std::string;
using std::shared_ptr;
using boost::asio::io_service;

void ProfilerCallbackHandler::releaseProfilerInfo() {
	if (this->mProfilerInfo)
		this->mProfilerInfo->Release();

	this->mCommClient.release();
	
	if (this->mWorkThread.native_handle()) {
		TerminateThread(this->mWorkThread.native_handle(), 0);
		// Also kill io_service here
		//TODO: wrong order?
		if (!this->mIoSvc.stopped())
			this->mIoSvc.stop();
	}
}

//WARNING: mIoWork and mWorkThread must be declared after mIoSvc in the class
ProfilerCallbackHandler::ProfilerCallbackHandler()
	: mRefCount(0),
	mProfilerInfo(nullptr),
	mIoSvc(),
	mIoWork(mIoSvc),
	mWorkThread(boost::bind(&io_service::run, &mIoSvc)),
	mCommClient(mIoSvc),
	mTraceSettings() {
	mCommClient.initialize();
	if (!mTraceSettings.loadSettings()) {
		PipeMessage msg;
		msg.serialize(PipeMessage::MessageType::TRACER_ERROR, { make_pair("error", 
			std::to_string(PipeMessage::PipeErrorCode::TRACER_ERR_SETTINGS_INVALID)) });

		// Send message synchronously so we can exit after without waiting for potential async callback
		mCommClient.sendMessageSync(msg);
		// We can't trace without any settings
		exit(-1);
	}
}


ProfilerCallbackHandler::~ProfilerCallbackHandler() {
	releaseProfilerInfo();
}

HRESULT ProfilerCallbackHandler::QueryInterface(REFIID riid, void **ppvObject) {
	// Return S_OK if riid is any descendant of this
	if (riid == __uuidof(ICorProfilerCallback7) ||
		riid == __uuidof(ICorProfilerCallback6) ||
		riid == __uuidof(ICorProfilerCallback5) ||
		riid == __uuidof(ICorProfilerCallback4) ||
		riid == __uuidof(ICorProfilerCallback3) ||
		riid == __uuidof(ICorProfilerCallback2) ||
		riid == __uuidof(ICorProfilerCallback) ||
		riid == IID_IUnknown) {
		*ppvObject = this;
		this->AddRef();
		return S_OK;
	}

	*ppvObject = nullptr;
	return E_NOINTERFACE;
}

ULONG ProfilerCallbackHandler::AddRef() {
	return atomic_fetch_add(&this->mRefCount, 1) + 1;
}

ULONG ProfilerCallbackHandler::Release() {
	int currCount = atomic_fetch_sub(&this->mRefCount, 1) - 1;

	if (currCount <= 0)
		delete this;

	return currCount;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::Initialize(IUnknown *pIProfilerCallbackHandlerInfoUnk) {
	HRESULT queryInterfaceResult = pIProfilerCallbackHandlerInfoUnk->QueryInterface(__uuidof(ICorProfilerInfo7), reinterpret_cast<void **>(&this->mProfilerInfo));

	if (FAILED(queryInterfaceResult))
		return E_FAIL;

	DWORD eventMask = COR_PRF_ALL;

	HRESULT hr = this->mProfilerInfo->SetEventMask(eventMask);
	this->mMdHelper = std::make_shared<MetadataHelper>(this->mProfilerInfo);
	return hr;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::Shutdown() {
	releaseProfilerInfo();

	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::AppDomainCreationStarted(AppDomainID appDomainId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::AppDomainCreationFinished(AppDomainID appDomainId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::AppDomainShutdownStarted(AppDomainID appDomainId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::AppDomainShutdownFinished(AppDomainID appDomainId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::AssemblyLoadStarted(AssemblyID assemblyId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::AssemblyLoadFinished(AssemblyID assemblyId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::AssemblyUnloadStarted(AssemblyID assemblyId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::AssemblyUnloadFinished(AssemblyID assemblyId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ModuleLoadStarted(ModuleID moduleId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ModuleLoadFinished(ModuleID moduleId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ModuleUnloadStarted(ModuleID moduleId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ModuleUnloadFinished(ModuleID moduleId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ModuleAttachedToAssembly(ModuleID moduleId, AssemblyID AssemblyId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ClassLoadStarted(ClassID classId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ClassLoadFinished(ClassID classId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ClassUnloadStarted(ClassID classId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ClassUnloadFinished(ClassID classId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::FunctionUnloadStarted(FunctionID functionId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::JITCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock) {
	ClassID classId;
	ModuleID moduleId;
	mdToken methodTok;

	JsonStringMap dataMap;

	HRESULT hr = mProfilerInfo->GetFunctionInfo(functionId, &classId, &moduleId, &methodTok);
	if (FAILED(hr))
		// Should not fail
		return S_OK;

	LPCBYTE modLoadAddr;
	ULONG modNameLen;
	AssemblyID asmId;

	// First call is just used to retrieve length of module 
	// (Remarks @ https://docs.microsoft.com/en-us/dotnet/framework/unmanaged-api/profiling/icorprofilerinfo-getmoduleinfo-method)
	hr = mProfilerInfo->GetModuleInfo(moduleId, &modLoadAddr, 0, &modNameLen, nullptr, &asmId);

	if (FAILED(hr))
		return S_OK;

	wstring modFullName(modNameLen, '0');
	hr = mProfilerInfo->GetModuleInfo(moduleId, &modLoadAddr, modFullName.length(), &modNameLen, &modFullName[0], &asmId);

	// Extract only module name without full path
	size_t found = modFullName.find_last_of(L"\\");
	string modName = string(modFullName.begin() + found + 1, modFullName.end());
	ConvertUtils::toLowerCase(modName);

	// Remove null-terminator
	modName.pop_back();

	// If module is not traced just return
	if (!mTraceSettings.isModuleTraced(modName))
		return S_OK;

	if (FAILED(hr))
		return S_OK;

	shared_ptr<CachedFunction> cachedFunc = mMdHelper->mapFunction(functionId, string(modFullName.begin(), modFullName.end()));

	if (cachedFunc) {
		dataMap.insert(make_pair("MetadataToken", std::to_string(cachedFunc->getMdTok())));
		dataMap.insert(make_pair("TypeName", cachedFunc->getTypeName()));
		dataMap.insert(make_pair("MethodName", cachedFunc->getFuncName()));
		dataMap.insert(make_pair("FullyQualifiedName", cachedFunc->getFullyQualifiedName()));
		dataMap.insert(make_pair("ModuleName", cachedFunc->getModName()));
	}

	PipeMessage msg;
	msg.serialize(PipeMessage::MessageType::TRACER_JIT_COMPILATION_STARTED, dataMap);
	this->mCommClient.sendMessage(msg);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::JITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock) {
	auto cachedFunc = mMdHelper->getMappedFunction(functionId);
	if (cachedFunc == nullptr)
		// I don't think we need to worry about non-cached functions
		return S_OK;

	JsonStringMap dataMap;
	dataMap.insert(make_pair("MetadataToken", std::to_string(cachedFunc->getMdTok())));
	dataMap.insert(make_pair("TypeName", cachedFunc->getTypeName()));
	dataMap.insert(make_pair("MethodName", cachedFunc->getFuncName()));
	dataMap.insert(make_pair("FullyQualifiedName", cachedFunc->getFullyQualifiedName()));
	dataMap.insert(make_pair("ModuleName", cachedFunc->getModName()));

	PipeMessage msg;
	msg.serialize(PipeMessage::MessageType::TRACER_JIT_COMPILATION_FINISHED, dataMap);
	this->mCommClient.sendMessage(msg);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::JITCachedFunctionSearchStarted(FunctionID functionId, BOOL *pbUseCachedFunction) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::JITCachedFunctionSearchFinished(FunctionID functionId, COR_PRF_JIT_CACHE result) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::JITFunctionPitched(FunctionID functionId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::JITInlining(FunctionID callerId, FunctionID calleeId, BOOL *pfShouldInline) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ThreadCreated(ThreadID threadId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ThreadDestroyed(ThreadID threadId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ThreadAssignedToOSThread(ThreadID managedThreadId, DWORD osThreadId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RemotingClientInvocationStarted() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RemotingClientSendingMessage(GUID *pCookie, BOOL fIsAsync) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RemotingClientReceivingReply(GUID *pCookie, BOOL fIsAsync) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RemotingClientInvocationFinished() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RemotingServerReceivingMessage(GUID *pCookie, BOOL fIsAsync) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RemotingServerInvocationStarted() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RemotingServerInvocationReturned() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RemotingServerSendingReply(GUID *pCookie, BOOL fIsAsync) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::UnmanagedToManagedTransition(FunctionID functionId, COR_PRF_TRANSITION_REASON reason) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ManagedToUnmanagedTransition(FunctionID functionId, COR_PRF_TRANSITION_REASON reason) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RuntimeSuspendFinished() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RuntimeSuspendAborted() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RuntimeResumeStarted() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RuntimeResumeFinished() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RuntimeThreadSuspended(ThreadID threadId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RuntimeThreadResumed(ThreadID threadId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::MovedReferences(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], ULONG cObjectIDRangeLength[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ObjectAllocated(ObjectID objectId, ClassID classId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ObjectsAllocatedByClass(ULONG cClassCount, ClassID classIds[], ULONG cObjects[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ObjectReferences(ObjectID objectId, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RootReferences(ULONG cRootRefs, ObjectID rootRefIds[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionThrown(ObjectID thrownObjectId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionSearchFunctionEnter(FunctionID functionId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionSearchFunctionLeave() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionSearchFilterEnter(FunctionID functionId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionSearchFilterLeave() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionSearchCatcherFound(FunctionID functionId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionOSHandlerEnter(UINT_PTR __unused) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionOSHandlerLeave(UINT_PTR __unused) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionUnwindFunctionEnter(FunctionID functionId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionUnwindFunctionLeave() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionUnwindFinallyEnter(FunctionID functionId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionUnwindFinallyLeave() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionCatcherEnter(FunctionID functionId, ObjectID objectId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionCatcherLeave() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::COMClassicVTableCreated(ClassID wrappedClassId, REFGUID implementedIID, void *pVTable, ULONG cSlots) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::COMClassicVTableDestroyed(ClassID wrappedClassId, REFGUID implementedIID, void *pVTable) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionCLRCatcherFound() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ExceptionCLRCatcherExecute() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ThreadNameChanged(ThreadID threadId, ULONG cchName, WCHAR name[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::GarbageCollectionStarted(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::SurvivingReferences(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], ULONG cObjectIDRangeLength[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::GarbageCollectionFinished() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::FinalizeableObjectQueued(DWORD finalizerFlags, ObjectID objectID) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::RootReferences2(ULONG cRootRefs, ObjectID rootRefIds[], COR_PRF_GC_ROOT_KIND rootKinds[], COR_PRF_GC_ROOT_FLAGS rootFlags[], UINT_PTR rootIds[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::HandleCreated(GCHandleID handleId, ObjectID initialObjectId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::HandleDestroyed(GCHandleID handleId) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::InitializeForAttach(IUnknown *pProfilerCallbackHandlerInfoUnk, void *pvClientData, UINT cbClientData) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ProfilerAttachComplete() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ProfilerDetachSucceeded() {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ReJITCompilationStarted(FunctionID functionId, ReJITID rejitId, BOOL fIsSafeToBlock) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ReJITCompilationFinished(FunctionID functionId, ReJITID rejitId, HRESULT hrStatus, BOOL fIsSafeToBlock) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ReJITError(ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::MovedReferences2(ULONG cMovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], SIZE_T cObjectIDRangeLength[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::SurvivingReferences2(ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], SIZE_T cObjectIDRangeLength[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ConditionalWeakTableElementReferences(ULONG cRootRefs, ObjectID keyRefIds[], ObjectID valueRefIds[], GCHandleID rootIds[]) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::GetAssemblyReferences(const WCHAR *wszAssemblyPath, ICorProfilerAssemblyReferenceProvider *pAsmRefProvider) {
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ProfilerCallbackHandler::ModuleInMemorySymbolsUpdated(ModuleID moduleId) {
	return S_OK;
}
