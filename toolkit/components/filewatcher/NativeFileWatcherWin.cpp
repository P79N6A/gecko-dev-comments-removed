






#include "NativeFileWatcherWin.h"

#include "mozilla/Services.h"
#include "mozilla/UniquePtr.h"
#include "nsClassHashtable.h"
#include "nsDataHashtable.h"
#include "nsILocalFile.h"
#include "nsIObserverService.h"
#include "nsProxyRelease.h"
#include "nsTArray.h"
#include "prlog.h"

namespace mozilla {


namespace {




class WatchedErrorEvent MOZ_FINAL : public nsRunnable
{
public:
  




  WatchedErrorEvent(const nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>& aOnError,
                    const nsresult& anError, const DWORD& osError)
    : mOnError(aOnError)
    , mError(anError)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_METHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    
    if (mOnError) {
      (void)mOnError->Complete(mError, mOsError);
    }

    return NS_OK;
  }

 private:
  nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback> mOnError;
  nsresult mError;
  DWORD mOsError;
};




class WatchedSuccessEvent MOZ_FINAL : public nsRunnable
{
public:
  




  WatchedSuccessEvent(const nsMainThreadPtrHandle<nsINativeFileWatcherSuccessCallback>& aOnSuccess,
                      const nsAString& aResourcePath)
    : mOnSuccess(aOnSuccess)
    , mResourcePath(aResourcePath)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_METHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    
    if (mOnSuccess) {
      (void)mOnSuccess->Complete(mResourcePath);
    }

    return NS_OK;
  }

 private:
  nsMainThreadPtrHandle<nsINativeFileWatcherSuccessCallback> mOnSuccess;
  nsString mResourcePath;
};





class WatchedChangeEvent MOZ_FINAL : public nsRunnable
{
public:
  



  WatchedChangeEvent(const nsMainThreadPtrHandle<nsINativeFileWatcherCallback>& aOnChange,
                     const nsAString& aChangedResource)
    : mOnChange(aOnChange)
    , mChangedResource(aChangedResource)
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_METHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    (void)mOnChange->Changed(mChangedResource, 0);
    return NS_OK;
  }

private:
  nsMainThreadPtrHandle<nsINativeFileWatcherCallback> mOnChange;
  nsString mChangedResource;
};

#if defined(PR_LOGGING)
static PRLogModuleInfo* GetFileWatcherContextLog()
{
  static PRLogModuleInfo *gNativeWatcherPRLog;
  if (!gNativeWatcherPRLog) {
    gNativeWatcherPRLog = PR_NewLogModule("NativeFileWatcherService");
  }
  return gNativeWatcherPRLog;
}
#endif

#define FILEWATCHERLOG(...) PR_LOG(GetFileWatcherContextLog(), PR_LOG_DEBUG, (__VA_ARGS__))




const unsigned int WATCHED_RES_MAXIMUM_NOTIFICATIONS = 100;



const size_t NOTIFICATION_BUFFER_SIZE =
  WATCHED_RES_MAXIMUM_NOTIFICATIONS * sizeof(FILE_NOTIFY_INFORMATION);




struct AutoCloseHandleTraits
{
  typedef HANDLE type;
  static type empty() { return INVALID_HANDLE_VALUE; }
  static void release(type anHandle)
  {
    if (anHandle != INVALID_HANDLE_VALUE) {
      
      
      (void)CancelIo(anHandle);
      (void)CloseHandle(anHandle);
    }
  }
};
typedef Scoped<AutoCloseHandleTraits> AutoCloseHandle;


typedef nsTArray<nsMainThreadPtrHandle<nsINativeFileWatcherCallback>> ChangeCallbackArray;
typedef nsTArray<nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>> ErrorCallbackArray;





struct WatchedResourceDescriptor {
  
  nsString mPath;

  
  
  
  UniquePtr<unsigned char> mNotificationBuffer;

  
  
  OVERLAPPED mOverlappedInfo;

  
  AutoCloseHandle mResourceHandle;

  WatchedResourceDescriptor(const nsAString& aPath, const HANDLE anHandle)
    : mPath(aPath)
    , mResourceHandle(anHandle)
  {
    memset(&mOverlappedInfo,  0, sizeof(OVERLAPPED));
    mNotificationBuffer.reset(new unsigned char[NOTIFICATION_BUFFER_SIZE]);
  }
};





struct PathRunnablesParametersWrapper {
  nsString mPath;
  nsMainThreadPtrHandle<nsINativeFileWatcherCallback> mChangeCallbackHandle;
  nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback> mErrorCallbackHandle;
  nsMainThreadPtrHandle<nsINativeFileWatcherSuccessCallback> mSuccessCallbackHandle;

  PathRunnablesParametersWrapper(
    const nsAString& aPath,
    const nsMainThreadPtrHandle<nsINativeFileWatcherCallback>& aOnChange,
    const nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>& aOnError,
    const nsMainThreadPtrHandle<nsINativeFileWatcherSuccessCallback>& aOnSuccess)
    : mPath(aPath)
    , mChangeCallbackHandle(aOnChange)
    , mErrorCallbackHandle(aOnError)
    , mSuccessCallbackHandle(aOnSuccess)
  {
  }
};





class NativeWatcherIOShutdownTask : public nsRunnable
{
public:
  NativeWatcherIOShutdownTask()
    : mWorkerThread(do_GetCurrentThread())
  {
    MOZ_ASSERT(!NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    mWorkerThread->Shutdown();
    return NS_OK;
  }

private:
  nsCOMPtr<nsIThread> mWorkerThread;
};





static PLDHashOperator
WatchedPathsInfoHashtableTraverser(nsVoidPtrHashKey::KeyType key,
                                   WatchedResourceDescriptor* watchedResource,
                                   void* userArg)
{
  FILEWATCHERLOG("NativeFileWatcherIOTask::DeactivateRunnableMethod - "
                 "%S is still being watched.", watchedResource->mPath);

  return PL_DHASH_NEXT;
}












class NativeFileWatcherIOTask : public nsRunnable
{
public:
  NativeFileWatcherIOTask(HANDLE aIOCompletionPort)
    : mIOCompletionPort(aIOCompletionPort)
    , mShuttingDown(false)
  {
  }

  NS_IMETHOD Run();
  nsresult AddPathRunnableMethod(PathRunnablesParametersWrapper* aWrappedParameters);
  nsresult RemovePathRunnableMethod(PathRunnablesParametersWrapper* aWrappedParameters);
  nsresult DeactivateRunnableMethod();

private:
  
  
  
  
  
  
  nsClassHashtable<nsStringHashKey, WatchedResourceDescriptor> mWatchedResourcesByPath;
  nsDataHashtable<nsVoidPtrHashKey, WatchedResourceDescriptor*> mWatchedResourcesByHandle;

  
  
  
  nsClassHashtable<nsStringHashKey, ChangeCallbackArray> mChangeCallbacksTable;
  nsClassHashtable<nsStringHashKey, ErrorCallbackArray> mErrorCallbacksTable;

  
  HANDLE mIOCompletionPort;

  
  bool mShuttingDown;

  nsresult RunInternal();

  nsresult DispatchChangeCallbacks(WatchedResourceDescriptor* aResourceDescriptor,
                                   const nsAString& aChangedResource);

  nsresult ReportChange(
    const nsMainThreadPtrHandle<nsINativeFileWatcherCallback>& aOnChange,
    const nsAString& aChangedResource);

  nsresult DispatchErrorCallbacks(WatchedResourceDescriptor* aResourceDescriptor,
                                  nsresult anError, DWORD anOSError);

  nsresult ReportError(
    const nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>& aOnError,
    nsresult anError, DWORD anOSError);

  nsresult ReportSuccess(
    const nsMainThreadPtrHandle<nsINativeFileWatcherSuccessCallback>& aOnSuccess,
    const nsAString& aResourcePath);

  nsresult AddDirectoryToWatchList(WatchedResourceDescriptor* aDirectoryDescriptor);

  void AppendCallbacksToHashtables(
    const nsAString& aPath,
    const nsMainThreadPtrHandle<nsINativeFileWatcherCallback>& aOnChange,
    const nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>& aOnError);

  void RemoveCallbacksFromHashtables(
    const nsAString& aPath,
    const nsMainThreadPtrHandle<nsINativeFileWatcherCallback>& aOnChange,
    const nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>& aOnError);

  nsresult MakeResourcePath(
    WatchedResourceDescriptor* changedDescriptor,
    const nsAString& resourceName,
    nsAString& nativeResourcePath);
};







nsresult
NativeFileWatcherIOTask::RunInternal()
{
  
  
  OVERLAPPED* overlappedStructure;

  
  
  DWORD transferredBytes = 0;

  
  
  ULONG_PTR changedResourceHandle = 0;

  
  
  
  
  
  if (!GetQueuedCompletionStatus(mIOCompletionPort, &transferredBytes,
                                 &changedResourceHandle, &overlappedStructure,
                                 INFINITE)) {
    
    DWORD errCode = GetLastError();
    switch (errCode) {
      case ERROR_NOTIFY_ENUM_DIR: {
        
        
        FILEWATCHERLOG("NativeFileWatcherIOTask::Run - Notification buffer has overflowed");

        WatchedResourceDescriptor* changedRes =
          mWatchedResourcesByHandle.Get((HANDLE)changedResourceHandle);

        nsresult rv = DispatchChangeCallbacks(changedRes, NS_LITERAL_STRING("*"));
        if (NS_FAILED(rv)) {
          
          
          FILEWATCHERLOG(
            "NativeFileWatcherIOTask::Run - Failed to dispatch change callbacks (%x).",
            rv);
          return rv;
        }

        return NS_OK;
      }
      case ERROR_ABANDONED_WAIT_0:
      case ERROR_INVALID_HANDLE: {
        
        
        
        FILEWATCHERLOG(
          "NativeFileWatcherIOTask::Run - The completion port was closed (%x).",
          errCode);
        return NS_ERROR_ABORT;
      }
      case ERROR_OPERATION_ABORTED: {
        
        
        
        FILEWATCHERLOG("NativeFileWatcherIOTask::Run - Path unwatched (%x).", errCode);

        WatchedResourceDescriptor* toFree =
          mWatchedResourcesByHandle.Get((HANDLE)changedResourceHandle);

        if (toFree) {
          

          mWatchedResourcesByHandle.Remove((HANDLE)changedResourceHandle);

          
          mWatchedResourcesByPath.Remove(toFree->mPath);
        }

        return NS_OK;
      }
      default: {
        
        FILEWATCHERLOG("NativeFileWatcherIOTask::Run - Unknown error (%x).", errCode);

        return NS_ERROR_FAILURE;
      }
    }
  }

  
  
  
  
  
  
  if (!transferredBytes &&
      (overlappedStructure ||
      (!overlappedStructure && !changedResourceHandle))) {
    
    
    
    
    return NS_OK;
  }

  
  WatchedResourceDescriptor* changedRes =
    mWatchedResourcesByHandle.Get((HANDLE)changedResourceHandle);
  MOZ_ASSERT(changedRes, "Could not find the changed resource in the list of watched ones.");

  
  const unsigned char* rawNotificationBuffer = changedRes->mNotificationBuffer.get();

  while (true) {
    FILE_NOTIFY_INFORMATION* notificationInfo =
      (FILE_NOTIFY_INFORMATION*)rawNotificationBuffer;

    
    
    nsAutoString resourceName(notificationInfo->FileName,
                              notificationInfo->FileNameLength / sizeof(WCHAR));

    
    nsString resourcePath;
    nsresult rv = MakeResourcePath(changedRes, resourceName, resourcePath);
    if (NS_SUCCEEDED(rv)) {
      rv = DispatchChangeCallbacks(changedRes, resourcePath);
      if (NS_FAILED(rv)) {
        
        FILEWATCHERLOG(
          "NativeFileWatcherIOTask::Run - Failed to dispatch change callbacks (%x).",
          rv);
        return rv;
      }
    }

    if (!notificationInfo->NextEntryOffset) {
      break;
    }

    rawNotificationBuffer += notificationInfo->NextEntryOffset;
  };

  
  nsresult rv = AddDirectoryToWatchList(changedRes);
  if (NS_FAILED(rv)) {
    
    if (rv == NS_ERROR_ABORT) {
      
      FILEWATCHERLOG(
        "NativeFileWatcherIOTask::Run - Failed to watch %s and"
        " to dispatch the related error callbacks", changedRes->mPath);
      return rv;
    }
  }

  return NS_OK;
}








NS_IMETHODIMP
NativeFileWatcherIOTask::Run()
{
  MOZ_ASSERT(!NS_IsMainThread());

  
  
  if (mShuttingDown) {
    return NS_OK;
  }

  nsresult rv = RunInternal();
  if (NS_FAILED(rv)) {
    
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::Run - Stopping the watcher loop (error %S)", rv);

    
    
    return NS_OK;
  }

  
  return NS_DispatchToCurrentThread(this);
}














nsresult
NativeFileWatcherIOTask::AddPathRunnableMethod(
  PathRunnablesParametersWrapper* aWrappedParameters)
{
  MOZ_ASSERT(!NS_IsMainThread());

  
  
  if (mShuttingDown) {
    return NS_OK;
  }

  nsAutoPtr<PathRunnablesParametersWrapper> wrappedParameters(aWrappedParameters);

  if (!wrappedParameters ||
      !wrappedParameters->mChangeCallbackHandle) {
    FILEWATCHERLOG("NativeFileWatcherIOTask::AddPathRunnableMethod - Invalid arguments.");
    return NS_ERROR_NULL_POINTER;
  }

  
  WatchedResourceDescriptor* watchedResource =
    mWatchedResourcesByPath.Get(wrappedParameters->mPath);
  if (watchedResource) {
    
    AppendCallbacksToHashtables(
      watchedResource->mPath,
      wrappedParameters->mChangeCallbackHandle,
      wrappedParameters->mErrorCallbackHandle);

    return NS_OK;
  }

  
  
  
  
  HANDLE resHandle = CreateFileW(wrappedParameters->mPath.get(),
                                 FILE_LIST_DIRECTORY, 
                                 FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, 
                                 nullptr, 
                                 OPEN_EXISTING, 
                                 FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
                                 nullptr); 
  if (resHandle == INVALID_HANDLE_VALUE) {
    DWORD dwError = GetLastError();
    nsresult rv;
    if (dwError == ERROR_FILE_NOT_FOUND) {
      rv = NS_ERROR_FILE_NOT_FOUND;
    } else if (dwError == ERROR_ACCESS_DENIED) {
      rv = NS_ERROR_FILE_ACCESS_DENIED;
    } else {
      rv = NS_ERROR_FAILURE;
    }

    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::AddPathRunnableMethod - CreateFileW failed (error %x) for %S.",
      dwError, wrappedParameters->mPath);

    rv = ReportError(wrappedParameters->mErrorCallbackHandle, rv, dwError);
    if (NS_FAILED(rv)) {
      FILEWATCHERLOG(
        "NativeFileWatcherIOTask::AddPathRunnableMethod - "
        "Failed to dispatch the error callback (%x).",
        rv);
      return rv;
    }

    
    return NS_OK;
  }

  
  UniquePtr<WatchedResourceDescriptor> resourceDesc(
    new WatchedResourceDescriptor(wrappedParameters->mPath, resHandle));

  
  if (!CreateIoCompletionPort(resourceDesc->mResourceHandle, mIOCompletionPort,
                              (ULONG_PTR)resourceDesc->mResourceHandle.get(), 0)) {
    DWORD dwError = GetLastError();

    FILEWATCHERLOG("NativeFileWatcherIOTask::AddPathRunnableMethod"
                   " - CreateIoCompletionPort failed (error %x) for %S.",
                   dwError, wrappedParameters->mPath);

    
    
    nsresult rv =
      ReportError(wrappedParameters->mErrorCallbackHandle, NS_ERROR_UNEXPECTED, dwError);
    if (NS_FAILED(rv)) {
      FILEWATCHERLOG(
        "NativeFileWatcherIOTask::AddPathRunnableMethod - "
        "Failed to dispatch the error callback (%x).",
        rv);
      return rv;
    }

    
    return NS_OK;
  }

  
  
  
  AppendCallbacksToHashtables(
    wrappedParameters->mPath,
    wrappedParameters->mChangeCallbackHandle,
    wrappedParameters->mErrorCallbackHandle);

  
  nsresult rv = AddDirectoryToWatchList(resourceDesc.get());
  if (NS_SUCCEEDED(rv)) {
    
    WatchedResourceDescriptor* resource = resourceDesc.release();
    mWatchedResourcesByPath.Put(wrappedParameters->mPath, resource);
    mWatchedResourcesByHandle.Put(resHandle, resource);

    
    nsresult rv =
      ReportSuccess(wrappedParameters->mSuccessCallbackHandle, wrappedParameters->mPath);
    if (NS_FAILED(rv)) {
      FILEWATCHERLOG(
        "NativeFileWatcherIOTask::AddPathRunnableMethod - "
        "Failed to dispatch the success callback (%x).",
        rv);
      return rv;
    }

    return NS_OK;
  }

  
  
  RemoveCallbacksFromHashtables(
    watchedResource->mPath,
    wrappedParameters->mChangeCallbackHandle,
    wrappedParameters->mErrorCallbackHandle);

  if (rv != NS_ERROR_ABORT) {
    
    return NS_OK;
  }

  
  FILEWATCHERLOG(
    "NativeFileWatcherIOTask::AddPathRunnableMethod - Failed to watch %s and"
    " to dispatch the related error callbacks", resourceDesc->mPath);

  return rv;
}




















nsresult
NativeFileWatcherIOTask::RemovePathRunnableMethod(
  PathRunnablesParametersWrapper* aWrappedParameters)
{
  MOZ_ASSERT(!NS_IsMainThread());

  
  
  if (mShuttingDown) {
    return NS_OK;
  }

  nsAutoPtr<PathRunnablesParametersWrapper> wrappedParameters(aWrappedParameters);

  if (!wrappedParameters ||
      !wrappedParameters->mChangeCallbackHandle) {
    return NS_ERROR_NULL_POINTER;
  }

  WatchedResourceDescriptor* toRemove =
    mWatchedResourcesByPath.Get(wrappedParameters->mPath);
  if (!toRemove) {
    
    
    nsresult rv =
      ReportSuccess(wrappedParameters->mSuccessCallbackHandle, wrappedParameters->mPath);
    if (NS_FAILED(rv)) {
      FILEWATCHERLOG(
        "NativeFileWatcherIOTask::RemovePathRunnableMethod - "
        "Failed to dispatch the success callback (%x).",
        rv);
      return rv;
    }
    return NS_OK;
  }

  ChangeCallbackArray* changeCallbackArray =
    mChangeCallbacksTable.Get(toRemove->mPath);

  
  MOZ_ASSERT(changeCallbackArray);

  bool removed =
    changeCallbackArray->RemoveElement(wrappedParameters->mChangeCallbackHandle);
  if (!removed) {
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::RemovePathRunnableMethod - Unable to remove the change "
      "callback from the change callback hash map for %S.",
      wrappedParameters->mPath);
    MOZ_CRASH();
  }

  ErrorCallbackArray* errorCallbackArray =
    mErrorCallbacksTable.Get(toRemove->mPath);

  MOZ_ASSERT(errorCallbackArray);

  removed =
    errorCallbackArray->RemoveElement(wrappedParameters->mErrorCallbackHandle);
  if (!removed) {
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::RemovePathRunnableMethod - Unable to remove the error "
      "callback from the error callback hash map for %S.",
      wrappedParameters->mPath);
    MOZ_CRASH();
  }

  
  
  
  if (changeCallbackArray->Length()) {
    
    nsresult rv =
      ReportSuccess(wrappedParameters->mSuccessCallbackHandle, wrappedParameters->mPath);
    if (NS_FAILED(rv)) {
      FILEWATCHERLOG(
        "NativeFileWatcherIOTask::RemovePathRunnableMethod - "
        "Failed to dispatch the success callback (%x).",
        rv);
      return rv;
    }
    return NS_OK;
  }

  
  
  
  
  
  
  
  

  
  
  
  
  toRemove->mResourceHandle.dispose();

  
  nsresult rv =
    ReportSuccess(wrappedParameters->mSuccessCallbackHandle, wrappedParameters->mPath);
  if (NS_FAILED(rv)) {
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::RemovePathRunnableMethod - "
      "Failed to dispatch the success callback (%x).",
      rv);
    return rv;
  }

  return NS_OK;
}





nsresult
NativeFileWatcherIOTask::DeactivateRunnableMethod()
{
  MOZ_ASSERT(!NS_IsMainThread());

  
  MOZ_ASSERT(!mWatchedResourcesByHandle.Count(),
             "Clients of the nsINativeFileWatcher must remove "
             "watches manually before quitting.");

  
  (void)mWatchedResourcesByHandle.EnumerateRead(
    &WatchedPathsInfoHashtableTraverser, nullptr);

  
  
  if (mShuttingDown) {
    
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::DeactivateRunnableMethod - We are already shutting down.");
    MOZ_CRASH();
    return NS_OK;
  }

  
  mShuttingDown = true;

  
  
  mWatchedResourcesByHandle.Clear();

  
  
  mWatchedResourcesByPath.Clear();

  
  mChangeCallbacksTable.Clear();
  mErrorCallbacksTable.Clear();

  
  
  if (mIOCompletionPort) {
    if (!CloseHandle(mIOCompletionPort)) {
      FILEWATCHERLOG(
        "NativeFileWatcherIOTask::DeactivateRunnableMethod - "
        "Failed to close the IO completion port HANDLE.");
    }
  }

  
  nsRefPtr<NativeWatcherIOShutdownTask> shutdownRunnable =
    new NativeWatcherIOShutdownTask();

  return NS_DispatchToMainThread(shutdownRunnable);
}










nsresult
NativeFileWatcherIOTask::DispatchChangeCallbacks(
  WatchedResourceDescriptor* aResourceDescriptor,
  const nsAString& aChangedResource)
{
  MOZ_ASSERT(aResourceDescriptor);

  
  ChangeCallbackArray* changeCallbackArray =
    mChangeCallbacksTable.Get(aResourceDescriptor->mPath);

  
  MOZ_ASSERT(changeCallbackArray);

  for (size_t i = 0; i < changeCallbackArray->Length(); i++) {
    nsresult rv =
      ReportChange((*changeCallbackArray)[i], aChangedResource);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}











nsresult
NativeFileWatcherIOTask::ReportChange(
  const nsMainThreadPtrHandle<nsINativeFileWatcherCallback>& aOnChange,
  const nsAString& aChangedResource)
{
  nsRefPtr<WatchedChangeEvent> changeRunnable =
    new WatchedChangeEvent(aOnChange, aChangedResource);
  return NS_DispatchToMainThread(changeRunnable);
}












nsresult
NativeFileWatcherIOTask::DispatchErrorCallbacks(
  WatchedResourceDescriptor* aResourceDescriptor,
  nsresult anError, DWORD anOSError)
{
  MOZ_ASSERT(aResourceDescriptor);

  
  ErrorCallbackArray* errorCallbackArray =
    mErrorCallbacksTable.Get(aResourceDescriptor->mPath);

  
  MOZ_ASSERT(errorCallbackArray);

  for (size_t i = 0; i < errorCallbackArray->Length(); i++) {
    nsresult rv =
      ReportError((*errorCallbackArray)[i], anError, anOSError);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return NS_OK;
}













nsresult
NativeFileWatcherIOTask::ReportError(
  const nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>& aOnError,
  nsresult anError, DWORD anOSError)
{
  nsRefPtr<WatchedErrorEvent> errorRunnable =
    new WatchedErrorEvent(aOnError, anError, anOSError);
  return NS_DispatchToMainThread(errorRunnable);
}











nsresult
NativeFileWatcherIOTask::ReportSuccess(
  const nsMainThreadPtrHandle<nsINativeFileWatcherSuccessCallback>& aOnSuccess,
  const nsAString& aResource)
{
  nsRefPtr<WatchedSuccessEvent> successRunnable =
    new WatchedSuccessEvent(aOnSuccess, aResource);
  return NS_DispatchToMainThread(successRunnable);
}













nsresult
NativeFileWatcherIOTask::AddDirectoryToWatchList(
  WatchedResourceDescriptor* aDirectoryDescriptor)
{
  MOZ_ASSERT(!mShuttingDown);

  DWORD dwPlaceholder;
  
  
  
  
  
  
  
  if (!ReadDirectoryChangesW(aDirectoryDescriptor->mResourceHandle,
                             aDirectoryDescriptor->mNotificationBuffer.get(),
                             NOTIFICATION_BUFFER_SIZE,
                             true, 
                             FILE_NOTIFY_CHANGE_LAST_WRITE
                             | FILE_NOTIFY_CHANGE_FILE_NAME
                             | FILE_NOTIFY_CHANGE_DIR_NAME,
                             &dwPlaceholder,
                             &aDirectoryDescriptor->mOverlappedInfo,
                             nullptr)) {
    
    
    
    
    
    DWORD dwError = GetLastError();

    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::AddDirectoryToWatchList "
      " - ReadDirectoryChangesW failed (error %x) for %S.",
      dwError, aDirectoryDescriptor->mPath);

    nsresult rv =
      DispatchErrorCallbacks(aDirectoryDescriptor, NS_ERROR_FAILURE, dwError);
    if (NS_FAILED(rv)) {
      
      
      return NS_ERROR_ABORT;
    }

    
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}











void
NativeFileWatcherIOTask::AppendCallbacksToHashtables(
  const nsAString& aPath,
  const nsMainThreadPtrHandle<nsINativeFileWatcherCallback>& aOnChangeHandle,
  const nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>& aOnErrorHandle)
{
  
  ChangeCallbackArray* callbacksArray = mChangeCallbacksTable.Get(aPath);
  if (!callbacksArray) {
    
    callbacksArray = new ChangeCallbackArray();
    mChangeCallbacksTable.Put(aPath, callbacksArray);
  }

  
  
  ChangeCallbackArray::index_type changeCallbackIndex =
    callbacksArray->IndexOf(aOnChangeHandle);

  
  if (changeCallbackIndex == ChangeCallbackArray::NoIndex) {
    callbacksArray->AppendElement(aOnChangeHandle);
  }

  
  ErrorCallbackArray* errorCallbacksArray = mErrorCallbacksTable.Get(aPath);
  if (!errorCallbacksArray) {
    
    errorCallbacksArray = new ErrorCallbackArray();
    mErrorCallbacksTable.Put(aPath, errorCallbacksArray);
  }

  ErrorCallbackArray::index_type errorCallbackIndex =
    errorCallbacksArray->IndexOf(aOnErrorHandle);

  if (errorCallbackIndex == ErrorCallbackArray::NoIndex) {
    errorCallbacksArray->AppendElement(aOnErrorHandle);
  }
}










void
NativeFileWatcherIOTask::RemoveCallbacksFromHashtables(
  const nsAString& aPath,
  const nsMainThreadPtrHandle<nsINativeFileWatcherCallback>& aOnChangeHandle,
  const nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback>& aOnErrorHandle)
{
  
  ChangeCallbackArray* callbacksArray = mChangeCallbacksTable.Get(aPath);
  if (callbacksArray) {
    
    callbacksArray->RemoveElement(aOnChangeHandle);
  }

  
  ErrorCallbackArray* errorCallbacksArray = mErrorCallbacksTable.Get(aPath);
  if (errorCallbacksArray) {
    
    errorCallbacksArray->RemoveElement(aOnErrorHandle);
  }
}













nsresult
NativeFileWatcherIOTask::MakeResourcePath(
  WatchedResourceDescriptor* changedDescriptor,
  const nsAString& resourceName,
  nsAString& nativeResourcePath)
{
  nsCOMPtr<nsILocalFile>
    localPath(do_CreateInstance("@mozilla.org/file/local;1"));
  if (!localPath) {
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::MakeResourcePath - Failed to create a nsILocalFile instance.");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = localPath->InitWithPath(changedDescriptor->mPath);
  if (NS_FAILED(rv)) {
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::MakeResourcePath - Failed to init nsILocalFile with %S (%x).",
      changedDescriptor->mPath, rv);
    return rv;
  }

  rv = localPath->AppendRelativePath(resourceName);
  if (NS_FAILED(rv)) {
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::MakeResourcePath - Failed to append %S to %S (%x).",
      resourceName, changedDescriptor->mPath, rv);
    return rv;
  }

  rv = localPath->GetPath(nativeResourcePath);
  if (NS_FAILED(rv)) {
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::MakeResourcePath - Failed to get native path from nsILocalFile (%x).",
      rv);
    return rv;
  }

  return NS_OK;
}

} 



NS_IMPL_ISUPPORTS(NativeFileWatcherService, nsINativeFileWatcherService, nsIObserver);

NativeFileWatcherService::NativeFileWatcherService()
{
}

NativeFileWatcherService::~NativeFileWatcherService()
{
}






nsresult
NativeFileWatcherService::Init()
{
  
  AutoCloseHandle completionPort(
    CreateIoCompletionPort(INVALID_HANDLE_VALUE, 
                           nullptr, 
                           0, 
                           2)); 
  if (!completionPort) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
  if (!observerService) {
    return NS_ERROR_FAILURE;
  }

  observerService->AddObserver(this, "xpcom-shutdown-threads", false);

  
  mWorkerIORunnable = new NativeFileWatcherIOTask(completionPort);
  nsresult rv = NS_NewThread(getter_AddRefs(mIOThread), mWorkerIORunnable);
  if (NS_FAILED(rv)) {
    FILEWATCHERLOG(
      "NativeFileWatcherIOTask::Init - Unable to create and dispatch the worker thread (%x).",
      rv);
    return rv;
  }

  
  NS_SetThreadName(mIOThread, "FileWatcher IO");

  mIOCompletionPort = completionPort.forget();

  return NS_OK;
}


















NS_IMETHODIMP
NativeFileWatcherService::AddPath(const nsAString& aPathToWatch,
                                  nsINativeFileWatcherCallback* aOnChange,
                                  nsINativeFileWatcherErrorCallback* aOnError,
                                  nsINativeFileWatcherSuccessCallback* aOnSuccess)
{
  
  if (!mIOThread) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  if (!aOnChange) {
    return NS_ERROR_NULL_POINTER;
  }

  nsMainThreadPtrHandle<nsINativeFileWatcherCallback> changeCallbackHandle(
    new nsMainThreadPtrHolder<nsINativeFileWatcherCallback>(aOnChange));

  nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback> errorCallbackHandle(
    new nsMainThreadPtrHolder<nsINativeFileWatcherErrorCallback>(aOnError));

  nsMainThreadPtrHandle<nsINativeFileWatcherSuccessCallback> successCallbackHandle(
    new nsMainThreadPtrHolder<nsINativeFileWatcherSuccessCallback>(aOnSuccess));

  
  UniquePtr<PathRunnablesParametersWrapper> wrappedCallbacks(
    new PathRunnablesParametersWrapper(
      aPathToWatch,
      changeCallbackHandle,
      errorCallbackHandle,
      successCallbackHandle));

  
  nsresult rv =
    mIOThread->Dispatch(
      NS_NewRunnableMethodWithArg<PathRunnablesParametersWrapper*>(
        static_cast<NativeFileWatcherIOTask*>(mWorkerIORunnable.get()),
        &NativeFileWatcherIOTask::AddPathRunnableMethod,
        wrappedCallbacks.get()),
      nsIEventTarget::DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  wrappedCallbacks.release();

  WakeUpWorkerThread();

  return NS_OK;
}


















NS_IMETHODIMP
NativeFileWatcherService::RemovePath(const nsAString& aPathToRemove,
                                     nsINativeFileWatcherCallback* aOnChange,
                                     nsINativeFileWatcherErrorCallback* aOnError,
                                     nsINativeFileWatcherSuccessCallback* aOnSuccess)
{
  
  if (!mIOThread) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  if (!aOnChange) {
    return NS_ERROR_NULL_POINTER;
  }

  nsMainThreadPtrHandle<nsINativeFileWatcherCallback> changeCallbackHandle(
    new nsMainThreadPtrHolder<nsINativeFileWatcherCallback>(aOnChange));

  nsMainThreadPtrHandle<nsINativeFileWatcherErrorCallback> errorCallbackHandle(
    new nsMainThreadPtrHolder<nsINativeFileWatcherErrorCallback>(aOnError));

  nsMainThreadPtrHandle<nsINativeFileWatcherSuccessCallback> successCallbackHandle(
    new nsMainThreadPtrHolder<nsINativeFileWatcherSuccessCallback>(aOnSuccess));

  
  UniquePtr<PathRunnablesParametersWrapper> wrappedCallbacks(
    new PathRunnablesParametersWrapper(
      aPathToRemove,
      changeCallbackHandle,
      errorCallbackHandle,
      successCallbackHandle));

  
  nsresult rv =
    mIOThread->Dispatch(
      NS_NewRunnableMethodWithArg<PathRunnablesParametersWrapper*>(
        static_cast<NativeFileWatcherIOTask*>(mWorkerIORunnable.get()),
        &NativeFileWatcherIOTask::RemovePathRunnableMethod,
        wrappedCallbacks.get()),
      nsIEventTarget::DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  wrappedCallbacks.release();

  WakeUpWorkerThread();

  return NS_OK;
}
















nsresult
NativeFileWatcherService::Uninit()
{
  
  if (!mIOThread) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIThread> ioThread;
  ioThread.swap(mIOThread);

  
  
  nsresult rv =
    ioThread->Dispatch(
      NS_NewRunnableMethod(
        static_cast<NativeFileWatcherIOTask*>(mWorkerIORunnable.get()),
        &NativeFileWatcherIOTask::DeactivateRunnableMethod),
      nsIEventTarget::DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    return rv;
  }

  WakeUpWorkerThread();

  return NS_OK;
}







void
NativeFileWatcherService::WakeUpWorkerThread()
{
  
  
  
  
  PostQueuedCompletionStatus(mIOCompletionPort, 0, 0, nullptr);
}





NS_IMETHODIMP
NativeFileWatcherService::Observe(nsISupports* aSubject, const char* aTopic,
                                  const char16_t* aData)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!strcmp("xpcom-shutdown-threads", aTopic)) {
    nsresult rv = Uninit();
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    return NS_OK;
  }

  MOZ_ASSERT(false, "NativeFileWatcherService got an unexpected topic!");

  return NS_ERROR_UNEXPECTED;
}

} 
