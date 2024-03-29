





#include "MiniShmChild.h"

#include <limits>
#include <sstream>

namespace mozilla {
namespace plugins {

MiniShmChild::MiniShmChild()
  : mParentEvent(nullptr),
    mParentGuard(nullptr),
    mChildEvent(nullptr),
    mChildGuard(nullptr),
    mFileMapping(nullptr),
    mRegWait(nullptr),
    mView(nullptr),
    mTimeout(INFINITE)
{}

MiniShmChild::~MiniShmChild()
{
  if (mRegWait) {
    ::UnregisterWaitEx(mRegWait, INVALID_HANDLE_VALUE);
  }
  if (mParentGuard) {
    
    ::WaitForSingleObject(mParentGuard, mTimeout);
    ::CloseHandle(mParentGuard);
  }
  if (mParentEvent) {
    ::CloseHandle(mParentEvent);
  }
  if (mChildEvent) {
    ::CloseHandle(mChildEvent);
  }
  if (mChildGuard) {
    ::CloseHandle(mChildGuard);
  }
  if (mView) {
    ::UnmapViewOfFile(mView);
  }
  if (mFileMapping) {
    ::CloseHandle(mFileMapping);
  }
}

nsresult
MiniShmChild::Init(MiniShmObserver* aObserver, const std::wstring& aCookie,
                   const DWORD aTimeout)
{
  if (aCookie.empty() || !aTimeout) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  if (mFileMapping) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  std::wistringstream iss(aCookie);
  HANDLE mapHandle = nullptr;
  iss >> mapHandle;
  if (!iss) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  ScopedMappedFileView view(::MapViewOfFile(mapHandle,
                                            FILE_MAP_WRITE,
                                            0, 0, 0));
  if (!view.IsValid()) {
    return NS_ERROR_FAILURE;
  }
  MEMORY_BASIC_INFORMATION memInfo = {0};
  SIZE_T querySize = ::VirtualQuery(view, &memInfo, sizeof(memInfo));
  unsigned int mappingSize = 0;
  if (querySize) {
    if (memInfo.RegionSize <= std::numeric_limits<unsigned int>::max()) {
      mappingSize = static_cast<unsigned int>(memInfo.RegionSize);
    }
  }
  if (!querySize || !mappingSize) {
    return NS_ERROR_FAILURE;
  }
  nsresult rv = SetView(view, mappingSize, true);
  if (NS_FAILED(rv)) {
    return rv;
  }

  const MiniShmInit* initStruct = nullptr;
  rv = GetReadPtr(initStruct);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!initStruct->mParentEvent || !initStruct->mParentGuard ||
      !initStruct->mChildEvent || !initStruct->mChildGuard) {
    return NS_ERROR_FAILURE;
  }
  rv = SetGuard(initStruct->mParentGuard, aTimeout);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!::RegisterWaitForSingleObject(&mRegWait,
                                     initStruct->mChildEvent,
                                     &SOnEvent,
                                     this,
                                     INFINITE,
                                     WT_EXECUTEDEFAULT)) {
    return NS_ERROR_FAILURE;
  }

  MiniShmInitComplete* initCompleteStruct = nullptr;
  rv = GetWritePtrInternal(initCompleteStruct);
  if (NS_FAILED(rv)) {
    ::UnregisterWaitEx(mRegWait, INVALID_HANDLE_VALUE);
    mRegWait = nullptr;
    return NS_ERROR_FAILURE;
  }

  initCompleteStruct->mSucceeded = true;

  
  mFileMapping = mapHandle;
  mView = view.Take();
  mParentEvent = initStruct->mParentEvent;
  mParentGuard = initStruct->mParentGuard;
  mChildEvent = initStruct->mChildEvent;
  mChildGuard = initStruct->mChildGuard;
  SetObserver(aObserver);
  mTimeout = aTimeout;

  rv = Send();
  if (NS_FAILED(rv)) {
    initCompleteStruct->mSucceeded = false;
    mFileMapping = nullptr;
    view.Set(mView);
    mView = nullptr;
    mParentEvent = nullptr;
    mParentGuard = nullptr;
    mChildEvent = nullptr;
    mChildGuard = nullptr;
    ::UnregisterWaitEx(mRegWait, INVALID_HANDLE_VALUE);
    mRegWait = nullptr;
    return rv;
  }

  OnConnect();
  return NS_OK;
}

nsresult
MiniShmChild::Send()
{
  if (!mParentEvent) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  if (!::SetEvent(mParentEvent)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

void
MiniShmChild::OnEvent()
{
  MiniShmBase::OnEvent();
  ::SetEvent(mChildGuard);
}

} 
} 

