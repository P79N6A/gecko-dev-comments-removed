





#include "mozilla/RefPtr.h"
#include "pk11pub.h"
#include "ScopedNSSTypes.h"
#include "secoidt.h"

#include "nsIFile.h"
#include "nsIPipe.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsXPCOMStrings.h"

#include "BackgroundFileSaver.h"
#include "mozilla/Telemetry.h"

namespace mozilla {
namespace net {







#define BUFFERED_OUTPUT_SIZE (1024 * 32)




#define REQUEST_SUSPEND_AT (1024 * 1024 * 4)




#define REQUEST_RESUME_AT (1024 * 1024 * 2)








class NotifyTargetChangeRunnable MOZ_FINAL : public nsRunnable
{
public:
  NotifyTargetChangeRunnable(BackgroundFileSaver *aSaver, nsIFile *aTarget)
  : mSaver(aSaver)
  , mTarget(aTarget)
  {
  }

  NS_IMETHODIMP Run()
  {
    return mSaver->NotifyTargetChange(mTarget);
  }

private:
  nsCOMPtr<BackgroundFileSaver> mSaver;
  nsCOMPtr<nsIFile> mTarget;
};




uint32_t BackgroundFileSaver::sThreadCount = 0;
uint32_t BackgroundFileSaver::sTelemetryMaxThreadCount = 0;

BackgroundFileSaver::BackgroundFileSaver()
: mControlThread(nullptr)
, mWorkerThread(nullptr)
, mPipeOutputStream(nullptr)
, mPipeInputStream(nullptr)
, mObserver(nullptr)
, mLock("BackgroundFileSaver.mLock")
, mWorkerThreadAttentionRequested(false)
, mFinishRequested(false)
, mComplete(false)
, mStatus(NS_OK)
, mAssignedTarget(nullptr)
, mAssignedTargetKeepPartial(false)
, mAsyncCopyContext(nullptr)
, mSha256Enabled(false)
, mActualTarget(nullptr)
, mActualTargetKeepPartial(false)
, mDigestContext(nullptr)
{
}

BackgroundFileSaver::~BackgroundFileSaver()
{
  destructorSafeDestroyNSSReference();
  shutdown(calledFromObject);
}

void
BackgroundFileSaver::destructorSafeDestroyNSSReference()
{
  nsNSSShutDownPreventionLock lock;
  if (isAlreadyShutDown()) {
    return;
  }
  if (mDigestContext) {
    mozilla::psm::PK11_DestroyContext_true(mDigestContext.forget());
    mDigestContext = nullptr;
  }
}

void
BackgroundFileSaver::virtualDestroyNSSReference()
{
  destructorSafeDestroyNSSReference();
}


nsresult
BackgroundFileSaver::Init()
{
  MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

  nsresult rv;

  rv = NS_NewPipe2(getter_AddRefs(mPipeInputStream),
                   getter_AddRefs(mPipeOutputStream), true, true, 0,
                   HasInfiniteBuffer() ? UINT32_MAX : 0, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_GetCurrentThread(getter_AddRefs(mControlThread));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewThread(getter_AddRefs(mWorkerThread));
  NS_ENSURE_SUCCESS(rv, rv);

  sThreadCount++;
  if (sThreadCount > sTelemetryMaxThreadCount) {
    sTelemetryMaxThreadCount = sThreadCount;
  }

  return NS_OK;
}


NS_IMETHODIMP
BackgroundFileSaver::GetObserver(nsIBackgroundFileSaverObserver **aObserver)
{
  NS_ENSURE_ARG_POINTER(aObserver);
  *aObserver = mObserver;
  NS_IF_ADDREF(*aObserver);
  return NS_OK;
}


NS_IMETHODIMP
BackgroundFileSaver::SetObserver(nsIBackgroundFileSaverObserver *aObserver)
{
  mObserver = aObserver;
  return NS_OK;
}


NS_IMETHODIMP
BackgroundFileSaver::SetTarget(nsIFile *aTarget, bool aKeepPartial)
{
  NS_ENSURE_ARG(aTarget);
  {
    MutexAutoLock lock(mLock);
    aTarget->Clone(getter_AddRefs(mAssignedTarget));
    mAssignedTargetKeepPartial = aKeepPartial;
  }

  
  
  return GetWorkerThreadAttention(true);
}


NS_IMETHODIMP
BackgroundFileSaver::Finish(nsresult aStatus)
{
  nsresult rv;

  
  
  rv = mPipeOutputStream->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  {
    MutexAutoLock lock(mLock);
    mFinishRequested = true;
    if (NS_SUCCEEDED(mStatus)) {
      mStatus = aStatus;
    }
  }

  
  
  
  
  
  return GetWorkerThreadAttention(NS_FAILED(aStatus));
}

NS_IMETHODIMP
BackgroundFileSaver::EnableSha256()
{
  MOZ_ASSERT(NS_IsMainThread(),
             "Can't enable sha256 or initialize NSS off the main thread");
  mSha256Enabled = true;
  
  
  nsresult rv;
  nsCOMPtr<nsISupports> nssDummy = do_GetService("@mozilla.org/psm;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
BackgroundFileSaver::GetSha256Hash(nsACString& aHash)
{
  MOZ_ASSERT(NS_IsMainThread(), "Can't inspect sha256 off the main thread");
  
  MutexAutoLock lock(mLock);
  if (mSha256.IsEmpty()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  aHash = mSha256;
  return NS_OK;
}


nsresult
BackgroundFileSaver::GetWorkerThreadAttention(bool aShouldInterruptCopy)
{
  nsresult rv;

  MutexAutoLock lock(mLock);

  
  
  
  
  
  
  
  
  
  if (mWorkerThreadAttentionRequested) {
    return NS_OK;
  }

  if (!mAsyncCopyContext) {
    
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &BackgroundFileSaver::ProcessAttention);
    NS_ENSURE_TRUE(event, NS_ERROR_FAILURE);

    rv = mWorkerThread->Dispatch(event, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (aShouldInterruptCopy) {
    
    
    NS_CancelAsyncCopy(mAsyncCopyContext, NS_ERROR_ABORT);
  }

  
  
  mWorkerThreadAttentionRequested = true;

  return NS_OK;
}



void
BackgroundFileSaver::AsyncCopyCallback(void *aClosure, nsresult aStatus)
{
  BackgroundFileSaver *self = (BackgroundFileSaver *)aClosure;
  {
    MutexAutoLock lock(self->mLock);

    
    
    self->mAsyncCopyContext = nullptr;

    
    if (NS_FAILED(aStatus) && aStatus != NS_ERROR_ABORT &&
        NS_SUCCEEDED(self->mStatus)) {
      self->mStatus = aStatus;
    }
  }

  (void)self->ProcessAttention();

  
  
  
  NS_RELEASE(self);
}


nsresult
BackgroundFileSaver::ProcessAttention()
{
  nsresult rv;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mAsyncCopyContext) {
    NS_CancelAsyncCopy(mAsyncCopyContext, NS_ERROR_ABORT);
    return NS_OK;
  }
  
  rv = ProcessStateChange();
  if (NS_FAILED(rv)) {
    
    {
      MutexAutoLock lock(mLock);

      if (NS_SUCCEEDED(mStatus)) {
        mStatus = rv;
      }
    }
    
    CheckCompletion();
  }

  return NS_OK;
}


nsresult
BackgroundFileSaver::ProcessStateChange()
{
  nsresult rv;

  
  if (CheckCompletion()) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIFile> target;
  bool targetKeepPartial;
  bool sha256Enabled = false;
  {
    MutexAutoLock lock(mLock);

    target = mAssignedTarget;
    targetKeepPartial = mAssignedTargetKeepPartial;

    
    mWorkerThreadAttentionRequested = false;

    sha256Enabled = mSha256Enabled;
  }

  
  
  if (!target) {
    return NS_OK;
  }

  
  bool equalToCurrent = false;
  int32_t creationIoFlags = PR_CREATE_FILE | PR_TRUNCATE;
  if (mActualTarget) {
    creationIoFlags = PR_APPEND;

    
    rv = mActualTarget->Equals(target, &equalToCurrent);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!equalToCurrent)
    {
      
      nsCOMPtr<nsIFile> targetParentDir;
      rv = target->GetParent(getter_AddRefs(targetParentDir));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString targetName;
      rv = target->GetLeafName(targetName);
      NS_ENSURE_SUCCESS(rv, rv);

      
      bool exists = false;
      rv = target->Exists(&exists);
      NS_ENSURE_SUCCESS(rv, rv);
      if (exists) {
        rv = target->Remove(false);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      
      
      
      rv = mActualTarget->MoveTo(targetParentDir, targetName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  mActualTarget = target;
  mActualTargetKeepPartial = targetKeepPartial;

  
  if (!equalToCurrent) {
    
    
    nsCOMPtr<nsIFile> actualTargetToNotify;
    rv = mActualTarget->Clone(getter_AddRefs(actualTargetToNotify));
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<NotifyTargetChangeRunnable> event =
      new NotifyTargetChangeRunnable(this, actualTargetToNotify);
    NS_ENSURE_TRUE(event, NS_ERROR_FAILURE);

    rv = mControlThread->Dispatch(event, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (CheckCompletion()) {
    return NS_OK;
  }

  
  
  
  
  
  
  if (creationIoFlags == PR_APPEND) {
    uint64_t available;
    rv = mPipeInputStream->Available(&available);
    if (NS_FAILED(rv)) {
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIOutputStream> outputStream;
  rv = NS_NewLocalFileOutputStream(getter_AddRefs(outputStream),
                                   mActualTarget,
                                   PR_WRONLY | creationIoFlags, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  outputStream = NS_BufferOutputStream(outputStream, BUFFERED_OUTPUT_SIZE);
  if (!outputStream) {
    return NS_ERROR_FAILURE;
  }

  
  
  bool isShutDown = false;
  if (sha256Enabled && !mDigestContext) {
    nsNSSShutDownPreventionLock lock;
    if (!(isShutDown = isAlreadyShutDown())) {
      mDigestContext =
        PK11_CreateDigestContext(static_cast<SECOidTag>(SEC_OID_SHA256));
      NS_ENSURE_TRUE(mDigestContext, NS_ERROR_OUT_OF_MEMORY);
    }
  }
  MOZ_ASSERT(!sha256Enabled || mDigestContext || isShutDown,
             "Hashing enabled but creating digest context didn't work");
  if (mDigestContext) {
    
    
    
    
    
    
    
    outputStream = new DigestOutputStream(outputStream, mDigestContext);
  }

  
  
  {
    MutexAutoLock lock(mLock);

    rv = NS_AsyncCopy(mPipeInputStream, outputStream, mWorkerThread,
                      NS_ASYNCCOPY_VIA_READSEGMENTS, 4096, AsyncCopyCallback,
                      this, false, true, getter_AddRefs(mAsyncCopyContext),
                      GetProgressCallback());
    if (NS_FAILED(rv)) {
      NS_WARNING("NS_AsyncCopy failed.");
      mAsyncCopyContext = nullptr;
      return rv;
    }
  }

  
  
  
  
  
  
  
  NS_ADDREF_THIS();

  return NS_OK;
}


bool
BackgroundFileSaver::CheckCompletion()
{
  nsresult rv;

  MOZ_ASSERT(!mAsyncCopyContext,
             "Should not be copying when checking completion conditions.");

  bool failed = true;
  {
    MutexAutoLock lock(mLock);

    if (mComplete) {
      return true;
    }

    
    
    if (NS_SUCCEEDED(mStatus)) {
      failed = false;

      
      
      if ((mAssignedTarget && mAssignedTarget != mActualTarget) ||
          !mFinishRequested) {
        return false;
      }

      
      
      
      uint64_t available;
      rv = mPipeInputStream->Available(&available);
      if (NS_SUCCEEDED(rv) && available != 0) {
        return false;
      }
    }

    mComplete = true;
  }

  
  
  if (failed && mActualTarget && !mActualTargetKeepPartial) {
    (void)mActualTarget->Remove(false);
  }

  
  if (!failed && mDigestContext) {
    nsNSSShutDownPreventionLock lock;
    if (!isAlreadyShutDown()) {
      Digest d;
      rv = d.End(SEC_OID_SHA256, mDigestContext);
      if (NS_SUCCEEDED(rv)) {
        MutexAutoLock lock(mLock);
        mSha256 = nsDependentCSubstring(char_ptr_cast(d.get().data),
                                        d.get().len);
      }
    }
  }

  
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &BackgroundFileSaver::NotifySaveComplete);
  if (!event ||
      NS_FAILED(mControlThread->Dispatch(event, NS_DISPATCH_NORMAL))) {
    NS_WARNING("Unable to post completion event to the control thread.");
  }

  return true;
}


nsresult
BackgroundFileSaver::NotifyTargetChange(nsIFile *aTarget)
{
  if (mObserver) {
    (void)mObserver->OnTargetChange(this, aTarget);
  }

  return NS_OK;
}


nsresult
BackgroundFileSaver::NotifySaveComplete()
{
  MOZ_ASSERT(NS_IsMainThread(), "This should be called on the main thread");

  nsresult status;
  {
    MutexAutoLock lock(mLock);
    status = mStatus;
  }

  if (mObserver) {
    (void)mObserver->OnSaveComplete(this, status);
  }

  
  
  
  
  
  
  
  mWorkerThread->Shutdown();

  sThreadCount--;

  
  
  
  
  if (sThreadCount == 0) {
    Telemetry::Accumulate(Telemetry::BACKGROUNDFILESAVER_THREAD_COUNT,
                          sTelemetryMaxThreadCount);
    sTelemetryMaxThreadCount = 0;
  }

  return NS_OK;
}




NS_IMPL_THREADSAFE_ISUPPORTS4(BackgroundFileSaverOutputStream,
                              nsIBackgroundFileSaver,
                              nsIOutputStream,
                              nsIAsyncOutputStream,
                              nsIOutputStreamCallback)

BackgroundFileSaverOutputStream::BackgroundFileSaverOutputStream()
: BackgroundFileSaver()
, mAsyncWaitCallback(nullptr)
{
}

BackgroundFileSaverOutputStream::~BackgroundFileSaverOutputStream()
{
}

bool
BackgroundFileSaverOutputStream::HasInfiniteBuffer()
{
  return false;
}

nsAsyncCopyProgressFun
BackgroundFileSaverOutputStream::GetProgressCallback()
{
  return nullptr;
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::Close()
{
  return mPipeOutputStream->Close();
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::Flush()
{
  return mPipeOutputStream->Flush();
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::Write(const char *aBuf, uint32_t aCount,
                                       uint32_t *_retval)
{
  return mPipeOutputStream->Write(aBuf, aCount, _retval);
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::WriteFrom(nsIInputStream *aFromStream,
                                           uint32_t aCount, uint32_t *_retval)
{
  return mPipeOutputStream->WriteFrom(aFromStream, aCount, _retval);
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::WriteSegments(nsReadSegmentFun aReader,
                                               void *aClosure, uint32_t aCount,
                                               uint32_t *_retval)
{
  return mPipeOutputStream->WriteSegments(aReader, aClosure, aCount, _retval);
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::IsNonBlocking(bool *_retval)
{
  return mPipeOutputStream->IsNonBlocking(_retval);
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::CloseWithStatus(nsresult reason)
{
  return mPipeOutputStream->CloseWithStatus(reason);
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::AsyncWait(nsIOutputStreamCallback *aCallback,
                                           uint32_t aFlags,
                                           uint32_t aRequestedCount,
                                           nsIEventTarget *aEventTarget)
{
  NS_ENSURE_STATE(!mAsyncWaitCallback);

  mAsyncWaitCallback = aCallback;

  return mPipeOutputStream->AsyncWait(this, aFlags, aRequestedCount,
                                      aEventTarget);
}

NS_IMETHODIMP
BackgroundFileSaverOutputStream::OnOutputStreamReady(
                                 nsIAsyncOutputStream *aStream)
{
  NS_ENSURE_STATE(mAsyncWaitCallback);

  nsCOMPtr<nsIOutputStreamCallback> asyncWaitCallback = nullptr;
  asyncWaitCallback.swap(mAsyncWaitCallback);

  return asyncWaitCallback->OnOutputStreamReady(this);
}




NS_IMPL_THREADSAFE_ISUPPORTS3(BackgroundFileSaverStreamListener,
                              nsIBackgroundFileSaver,
                              nsIRequestObserver,
                              nsIStreamListener)

BackgroundFileSaverStreamListener::BackgroundFileSaverStreamListener()
: BackgroundFileSaver()
, mSuspensionLock("BackgroundFileSaverStreamListener.mSuspensionLock")
, mReceivedTooMuchData(false)
, mRequest(nullptr)
, mRequestSuspended(false)
{
}

BackgroundFileSaverStreamListener::~BackgroundFileSaverStreamListener()
{
}

bool
BackgroundFileSaverStreamListener::HasInfiniteBuffer()
{
  return true;
}

nsAsyncCopyProgressFun
BackgroundFileSaverStreamListener::GetProgressCallback()
{
  return AsyncCopyProgressCallback;
}

NS_IMETHODIMP
BackgroundFileSaverStreamListener::OnStartRequest(nsIRequest *aRequest,
                                                  nsISupports *aContext)
{
  NS_ENSURE_ARG(aRequest);

  return NS_OK;
}

NS_IMETHODIMP
BackgroundFileSaverStreamListener::OnStopRequest(nsIRequest *aRequest,
                                                 nsISupports *aContext,
                                                 nsresult aStatusCode)
{
  
  
  if (NS_FAILED(aStatusCode)) {
    Finish(aStatusCode);
  }

  return NS_OK;
}

NS_IMETHODIMP
BackgroundFileSaverStreamListener::OnDataAvailable(nsIRequest *aRequest,
                                                   nsISupports *aContext,
                                                   nsIInputStream *aInputStream,
                                                   uint64_t aOffset,
                                                   uint32_t aCount)
{
  nsresult rv;

  NS_ENSURE_ARG(aRequest);

  
  
  uint32_t writeCount;
  rv = mPipeOutputStream->WriteFrom(aInputStream, aCount, &writeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  if (writeCount < aCount) {
    NS_WARNING("Reading from the input stream should not have failed.");
    return NS_ERROR_UNEXPECTED;
  }

  bool stateChanged = false;
  {
    MutexAutoLock lock(mSuspensionLock);

    if (!mReceivedTooMuchData) {
      uint64_t available;
      nsresult rv = mPipeInputStream->Available(&available);
      if (NS_SUCCEEDED(rv) && available > REQUEST_SUSPEND_AT) {
        mReceivedTooMuchData = true;
        mRequest = aRequest;
        stateChanged = true;
      }
    }
  }

  if (stateChanged) {
    NotifySuspendOrResume();
  }

  return NS_OK;
}



void
BackgroundFileSaverStreamListener::AsyncCopyProgressCallback(void *aClosure,
                                                             uint32_t aCount)
{
  BackgroundFileSaverStreamListener *self =
    (BackgroundFileSaverStreamListener *)aClosure;

  
  MutexAutoLock lock(self->mSuspensionLock);

  
  
  
  if (self->mReceivedTooMuchData) {
    uint64_t available;
    nsresult rv = self->mPipeInputStream->Available(&available);
    if (NS_FAILED(rv) || available < REQUEST_RESUME_AT) {
      self->mReceivedTooMuchData = false;

      
      nsCOMPtr<nsIRunnable> event = NS_NewRunnableMethod(self,
        &BackgroundFileSaverStreamListener::NotifySuspendOrResume);
      if (!event || NS_FAILED(self->mControlThread->Dispatch(event,
                                                    NS_DISPATCH_NORMAL))) {
        NS_WARNING("Unable to post resume event to the control thread.");
      }
    }
  }
}


nsresult
BackgroundFileSaverStreamListener::NotifySuspendOrResume()
{
  
  MutexAutoLock lock(mSuspensionLock);

  if (mReceivedTooMuchData) {
    if (!mRequestSuspended) {
      
      if (NS_SUCCEEDED(mRequest->Suspend())) {
        mRequestSuspended = true;
      } else {
        NS_WARNING("Unable to suspend the request.");
      }
    }
  } else {
    if (mRequestSuspended) {
      
      if (NS_SUCCEEDED(mRequest->Resume())) {
        mRequestSuspended = false;
      } else {
        NS_WARNING("Unable to resume the request.");
      }
    }
  }

  return NS_OK;
}



NS_IMPL_THREADSAFE_ISUPPORTS1(DigestOutputStream,
                              nsIOutputStream)

DigestOutputStream::DigestOutputStream(nsIOutputStream* aStream,
                                       PK11Context* aContext) :
  mOutputStream(aStream)
  , mDigestContext(aContext)
{
  MOZ_ASSERT(mDigestContext, "Can't have null digest context");
  MOZ_ASSERT(mOutputStream, "Can't have null output stream");
}

DigestOutputStream::~DigestOutputStream()
{
  shutdown(calledFromObject);
}

NS_IMETHODIMP
DigestOutputStream::Close()
{
  return mOutputStream->Close();
}

NS_IMETHODIMP
DigestOutputStream::Flush()
{
  return mOutputStream->Flush();
}

NS_IMETHODIMP
DigestOutputStream::Write(const char* aBuf, uint32_t aCount, uint32_t* retval)
{
  nsNSSShutDownPreventionLock lock;
  if (isAlreadyShutDown()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = MapSECStatus(PK11_DigestOp(mDigestContext,
                                           uint8_t_ptr_cast(aBuf), aCount));
  NS_ENSURE_SUCCESS(rv, rv);

  return mOutputStream->Write(aBuf, aCount, retval);
}

NS_IMETHODIMP
DigestOutputStream::WriteFrom(nsIInputStream* aFromStream,
                              uint32_t aCount, uint32_t* retval)
{
  
  
  
  MOZ_NOT_REACHED("DigestOutputStream::WriteFrom not implemented");
}

NS_IMETHODIMP
DigestOutputStream::WriteSegments(nsReadSegmentFun aReader,
                                  void *aClosure, uint32_t aCount,
                                  uint32_t *retval)
{
  MOZ_NOT_REACHED("DigestOutputStream::WriteSegments not implemented");
}

NS_IMETHODIMP
DigestOutputStream::IsNonBlocking(bool *retval)
{
  return mOutputStream->IsNonBlocking(retval);
}


} 
} 
