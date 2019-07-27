



#include "CacheLog.h"
#include "CacheEntry.h"
#include "CacheStorageService.h"
#include "CacheObserver.h"
#include "CacheFileUtils.h"
#include "CacheIndex.h"

#include "nsIInputStream.h"
#include "nsIOutputStream.h"
#include "nsISeekableStream.h"
#include "nsIURI.h"
#include "nsICacheEntryOpenCallback.h"
#include "nsICacheStorage.h"
#include "nsISerializable.h"
#include "nsIStreamTransportService.h"
#include "nsISizeOf.h"

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsString.h"
#include "nsProxyRelease.h"
#include "nsSerializationHelper.h"
#include "nsThreadUtils.h"
#include "mozilla/Telemetry.h"
#include <math.h>
#include <algorithm>

namespace mozilla {
namespace net {

static uint32_t const ENTRY_WANTED =
  nsICacheEntryOpenCallback::ENTRY_WANTED;
static uint32_t const RECHECK_AFTER_WRITE_FINISHED =
  nsICacheEntryOpenCallback::RECHECK_AFTER_WRITE_FINISHED;
static uint32_t const ENTRY_NEEDS_REVALIDATION =
  nsICacheEntryOpenCallback::ENTRY_NEEDS_REVALIDATION;
static uint32_t const ENTRY_NOT_WANTED =
  nsICacheEntryOpenCallback::ENTRY_NOT_WANTED;

NS_IMPL_ISUPPORTS(CacheEntryHandle, nsICacheEntry)



CacheEntryHandle::CacheEntryHandle(CacheEntry* aEntry)
: mEntry(aEntry)
{
  MOZ_COUNT_CTOR(CacheEntryHandle);

#ifdef DEBUG
  if (!mEntry->HandlesCount()) {
    
    
    
    CacheStorageService::Self()->Lock().AssertCurrentThreadOwns();
  }
#endif

  mEntry->AddHandleRef();

  LOG(("New CacheEntryHandle %p for entry %p", this, aEntry));
}

CacheEntryHandle::~CacheEntryHandle()
{
  mEntry->ReleaseHandleRef();
  mEntry->OnHandleClosed(this);

  MOZ_COUNT_DTOR(CacheEntryHandle);
}



CacheEntry::Callback::Callback(CacheEntry* aEntry,
                               nsICacheEntryOpenCallback *aCallback,
                               bool aReadOnly, bool aCheckOnAnyThread)
: mEntry(aEntry)
, mCallback(aCallback)
, mTargetThread(do_GetCurrentThread())
, mReadOnly(aReadOnly)
, mCheckOnAnyThread(aCheckOnAnyThread)
, mRecheckAfterWrite(false)
, mNotWanted(false)
{
  MOZ_COUNT_CTOR(CacheEntry::Callback);

  
  
  MOZ_ASSERT(mEntry->HandlesCount());
  mEntry->AddHandleRef();
}

CacheEntry::Callback::Callback(CacheEntry::Callback const &aThat)
: mEntry(aThat.mEntry)
, mCallback(aThat.mCallback)
, mTargetThread(aThat.mTargetThread)
, mReadOnly(aThat.mReadOnly)
, mCheckOnAnyThread(aThat.mCheckOnAnyThread)
, mRecheckAfterWrite(aThat.mRecheckAfterWrite)
, mNotWanted(aThat.mNotWanted)
{
  MOZ_COUNT_CTOR(CacheEntry::Callback);

  
  
  MOZ_ASSERT(mEntry->HandlesCount());
  mEntry->AddHandleRef();
}

CacheEntry::Callback::~Callback()
{
  ProxyRelease(mCallback, mTargetThread);

  mEntry->ReleaseHandleRef();
  MOZ_COUNT_DTOR(CacheEntry::Callback);
}

void CacheEntry::Callback::ExchangeEntry(CacheEntry* aEntry)
{
  if (mEntry == aEntry)
    return;

  
  
  MOZ_ASSERT(aEntry->HandlesCount());
  aEntry->AddHandleRef();
  mEntry->ReleaseHandleRef();
  mEntry = aEntry;
}

nsresult CacheEntry::Callback::OnCheckThread(bool *aOnCheckThread) const
{
  if (!mCheckOnAnyThread) {
    
    return mTargetThread->IsOnCurrentThread(aOnCheckThread);
  }

  
  *aOnCheckThread = true;
  return NS_OK;
}

nsresult CacheEntry::Callback::OnAvailThread(bool *aOnAvailThread) const
{
  return mTargetThread->IsOnCurrentThread(aOnAvailThread);
}



NS_IMPL_ISUPPORTS(CacheEntry,
                  nsICacheEntry,
                  nsIRunnable,
                  CacheFileListener)

CacheEntry::CacheEntry(const nsACString& aStorageID,
                       nsIURI* aURI,
                       const nsACString& aEnhanceID,
                       bool aUseDisk)
: mFrecency(0)
, mSortingExpirationTime(uint32_t(-1))
, mLock("CacheEntry")
, mFileStatus(NS_ERROR_NOT_INITIALIZED)
, mURI(aURI)
, mEnhanceID(aEnhanceID)
, mStorageID(aStorageID)
, mUseDisk(aUseDisk)
, mIsDoomed(false)
, mSecurityInfoLoaded(false)
, mPreventCallbacks(false)
, mHasData(false)
, mState(NOTLOADED)
, mRegistration(NEVERREGISTERED)
, mWriter(nullptr)
, mPredictedDataSize(0)
, mUseCount(0)
, mReleaseThread(NS_GetCurrentThread())
{
  MOZ_COUNT_CTOR(CacheEntry);

  mService = CacheStorageService::Self();

  CacheStorageService::Self()->RecordMemoryOnlyEntry(
    this, !aUseDisk, true );
}

CacheEntry::~CacheEntry()
{
  ProxyRelease(mURI, mReleaseThread);

  LOG(("CacheEntry::~CacheEntry [this=%p]", this));
  MOZ_COUNT_DTOR(CacheEntry);
}

#ifdef PR_LOG

char const * CacheEntry::StateString(uint32_t aState)
{
  switch (aState) {
  case NOTLOADED:     return "NOTLOADED";
  case LOADING:       return "LOADING";
  case EMPTY:         return "EMPTY";
  case WRITING:       return "WRITING";
  case READY:         return "READY";
  case REVALIDATING:  return "REVALIDATING";
  }

  return "?";
}

#endif

nsresult CacheEntry::HashingKeyWithStorage(nsACString &aResult) const
{
  return HashingKey(mStorageID, mEnhanceID, mURI, aResult);
}

nsresult CacheEntry::HashingKey(nsACString &aResult) const
{
  return HashingKey(EmptyCString(), mEnhanceID, mURI, aResult);
}


nsresult CacheEntry::HashingKey(nsCSubstring const& aStorageID,
                                nsCSubstring const& aEnhanceID,
                                nsIURI* aURI,
                                nsACString &aResult)
{
  nsAutoCString spec;
  nsresult rv = aURI->GetAsciiSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  return HashingKey(aStorageID, aEnhanceID, spec, aResult);
}


nsresult CacheEntry::HashingKey(nsCSubstring const& aStorageID,
                                nsCSubstring const& aEnhanceID,
                                nsCSubstring const& aURISpec,
                                nsACString &aResult)
{
  




  aResult.Append(aStorageID);

  if (!aEnhanceID.IsEmpty()) {
    CacheFileUtils::AppendTagWithValue(aResult, '~', aEnhanceID);
  }

  
  aResult.Append(':');
  aResult.Append(aURISpec);

  return NS_OK;
}

void CacheEntry::AsyncOpen(nsICacheEntryOpenCallback* aCallback, uint32_t aFlags)
{
  LOG(("CacheEntry::AsyncOpen [this=%p, state=%s, flags=%d, callback=%p]",
    this, StateString(mState), aFlags, aCallback));

  bool readonly = aFlags & nsICacheStorage::OPEN_READONLY;
  bool bypassIfBusy = aFlags & nsICacheStorage::OPEN_BYPASS_IF_BUSY;
  bool truncate = aFlags & nsICacheStorage::OPEN_TRUNCATE;
  bool priority = aFlags & nsICacheStorage::OPEN_PRIORITY;
  bool multithread = aFlags & nsICacheStorage::CHECK_MULTITHREADED;

  MOZ_ASSERT(!readonly || !truncate, "Bad flags combination");
  MOZ_ASSERT(!(truncate && mState > LOADING), "Must not call truncate on already loaded entry");

  Callback callback(this, aCallback, readonly, multithread);

  if (!Open(callback, truncate, priority, bypassIfBusy)) {
    
    LOG(("  writing or revalidating, callback wants to bypass cache"));
    callback.mNotWanted = true;
    InvokeAvailableCallback(callback);
  }
}

bool CacheEntry::Open(Callback & aCallback, bool aTruncate,
                      bool aPriority, bool aBypassIfBusy)
{
  mozilla::MutexAutoLock lock(mLock);

  
  if (aBypassIfBusy && (mState == WRITING || mState == REVALIDATING)) {
    return false;
  }

  RememberCallback(aCallback);

  
  if (Load(aTruncate, aPriority)) {
    
    return true;
  }

  InvokeCallbacks();

  return true;
}

bool CacheEntry::Load(bool aTruncate, bool aPriority)
{
  LOG(("CacheEntry::Load [this=%p, trunc=%d]", this, aTruncate));

  mLock.AssertCurrentThreadOwns();

  if (mState > LOADING) {
    LOG(("  already loaded"));
    return false;
  }

  if (mState == LOADING) {
    LOG(("  already loading"));
    return true;
  }

  mState = LOADING;

  MOZ_ASSERT(!mFile);

  nsresult rv;

  nsAutoCString fileKey;
  rv = HashingKeyWithStorage(fileKey);

  
  
  
  
  
  
  if ((!aTruncate || !mUseDisk) && NS_SUCCEEDED(rv)) {
    
    
    CacheIndex::EntryStatus status;
    if (NS_SUCCEEDED(CacheIndex::HasEntry(fileKey, &status))) {
      switch (status) {
      case CacheIndex::DOES_NOT_EXIST:
        LOG(("  entry doesn't exist according information from the index, truncating"));
        aTruncate = true;
        break;
      case CacheIndex::EXISTS:
      case CacheIndex::DO_NOT_KNOW:
        if (!mUseDisk) {
          LOG(("  entry open as memory-only, but there is (status=%d) a file, dooming it", status));
          CacheFileIOManager::DoomFileByKey(fileKey, nullptr);
        }
        break;
      }
    }
  }

  mFile = new CacheFile();

  BackgroundOp(Ops::REGISTER);

  bool directLoad = aTruncate || !mUseDisk;
  if (directLoad) {
    mFileStatus = NS_OK;
    
    
    mLoadStart = TimeStamp::NowLoRes();
  } else {
    mLoadStart = TimeStamp::Now();
  }

  {
    mozilla::MutexAutoUnlock unlock(mLock);

    LOG(("  performing load, file=%p", mFile.get()));
    if (NS_SUCCEEDED(rv)) {
      rv = mFile->Init(fileKey,
                       aTruncate,
                       !mUseDisk,
                       aPriority,
                       directLoad ? nullptr : this);
    }

    if (NS_FAILED(rv)) {
      mFileStatus = rv;
      AsyncDoom(nullptr);
      return false;
    }
  }

  if (directLoad) {
    
    mState = EMPTY;
  }

  return mState == LOADING;
}

NS_IMETHODIMP CacheEntry::OnFileReady(nsresult aResult, bool aIsNew)
{
  LOG(("CacheEntry::OnFileReady [this=%p, rv=0x%08x, new=%d]",
      this, aResult, aIsNew));

  MOZ_ASSERT(!mLoadStart.IsNull());

  if (NS_SUCCEEDED(aResult)) {
    if (aIsNew) {
      mozilla::Telemetry::AccumulateTimeDelta(
        mozilla::Telemetry::NETWORK_CACHE_V2_MISS_TIME_MS,
        mLoadStart);
    }
    else {
      mozilla::Telemetry::AccumulateTimeDelta(
        mozilla::Telemetry::NETWORK_CACHE_V2_HIT_TIME_MS,
        mLoadStart);
    }
  }

  
  
  
  
  mozilla::MutexAutoLock lock(mLock);

  MOZ_ASSERT(mState == LOADING);

  mState = (aIsNew || NS_FAILED(aResult))
    ? EMPTY
    : READY;

  mFileStatus = aResult;

  if (mState == READY) {
    mHasData = true;

    uint32_t frecency;
    mFile->GetFrecency(&frecency);
    
    
    mFrecency = INT2FRECENCY(frecency);
  }

  InvokeCallbacks();
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::OnFileDoomed(nsresult aResult)
{
  if (mDoomCallback) {
    nsRefPtr<DoomCallbackRunnable> event =
      new DoomCallbackRunnable(this, aResult);
    NS_DispatchToMainThread(event);
  }

  return NS_OK;
}

already_AddRefed<CacheEntryHandle> CacheEntry::ReopenTruncated(bool aMemoryOnly,
                                                               nsICacheEntryOpenCallback* aCallback)
{
  LOG(("CacheEntry::ReopenTruncated [this=%p]", this));

  mLock.AssertCurrentThreadOwns();

  
  mPreventCallbacks = true;

  nsRefPtr<CacheEntryHandle> handle;
  nsRefPtr<CacheEntry> newEntry;
  {
    mozilla::MutexAutoUnlock unlock(mLock);

    
    nsresult rv = CacheStorageService::Self()->AddStorageEntry(
      GetStorageID(), GetURI(), GetEnhanceID(),
      mUseDisk && !aMemoryOnly,
      true, 
      true, 
      getter_AddRefs(handle));

    if (NS_SUCCEEDED(rv)) {
      newEntry = handle->Entry();
      LOG(("  exchanged entry %p by entry %p, rv=0x%08x", this, newEntry.get(), rv));
      newEntry->AsyncOpen(aCallback, nsICacheStorage::OPEN_TRUNCATE);
    } else {
      LOG(("  exchanged of entry %p failed, rv=0x%08x", this, rv));
      AsyncDoom(nullptr);
    }
  }

  mPreventCallbacks = false;

  if (!newEntry)
    return nullptr;

  newEntry->TransferCallbacks(*this);
  mCallbacks.Clear();

  
  
  
  
  
  nsRefPtr<CacheEntryHandle> writeHandle =
    newEntry->NewWriteHandle();
  return writeHandle.forget();
}

void CacheEntry::TransferCallbacks(CacheEntry & aFromEntry)
{
  mozilla::MutexAutoLock lock(mLock);

  LOG(("CacheEntry::TransferCallbacks [entry=%p, from=%p]",
    this, &aFromEntry));

  if (!mCallbacks.Length())
    mCallbacks.SwapElements(aFromEntry.mCallbacks);
  else
    mCallbacks.AppendElements(aFromEntry.mCallbacks);

  uint32_t callbacksLength = mCallbacks.Length();
  if (callbacksLength) {
    
    for (uint32_t i = 0; i < callbacksLength; ++i)
      mCallbacks[i].ExchangeEntry(this);

    BackgroundOp(Ops::CALLBACKS, true);
  }
}

void CacheEntry::RememberCallback(Callback & aCallback)
{
  mLock.AssertCurrentThreadOwns();

  LOG(("CacheEntry::RememberCallback [this=%p, cb=%p, state=%s]",
    this, aCallback.mCallback.get(), StateString(mState)));

  mCallbacks.AppendElement(aCallback);
}

void CacheEntry::InvokeCallbacksLock()
{
  mozilla::MutexAutoLock lock(mLock);
  InvokeCallbacks();
}

void CacheEntry::InvokeCallbacks()
{
  mLock.AssertCurrentThreadOwns();

  LOG(("CacheEntry::InvokeCallbacks BEGIN [this=%p]", this));

  
  if (InvokeCallbacks(false))
    InvokeCallbacks(true);

  LOG(("CacheEntry::InvokeCallbacks END [this=%p]", this));
}

bool CacheEntry::InvokeCallbacks(bool aReadOnly)
{
  mLock.AssertCurrentThreadOwns();

  uint32_t i = 0;
  while (i < mCallbacks.Length()) {
    if (mPreventCallbacks) {
      LOG(("  callbacks prevented!"));
      return false;
    }

    if (!mIsDoomed && (mState == WRITING || mState == REVALIDATING)) {
      LOG(("  entry is being written/revalidated"));
      return false;
    }

    if (mCallbacks[i].mReadOnly != aReadOnly) {
      
      ++i;
      continue;
    }

    bool onCheckThread;
    nsresult rv = mCallbacks[i].OnCheckThread(&onCheckThread);

    if (NS_SUCCEEDED(rv) && !onCheckThread) {
      
      nsRefPtr<nsRunnableMethod<CacheEntry> > event =
        NS_NewRunnableMethod(this, &CacheEntry::InvokeCallbacksLock);

      rv = mCallbacks[i].mTargetThread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
      if (NS_SUCCEEDED(rv)) {
        LOG(("  re-dispatching to target thread"));
        return false;
      }
    }

    Callback callback = mCallbacks[i];
    mCallbacks.RemoveElementAt(i);

    if (NS_SUCCEEDED(rv) && !InvokeCallback(callback)) {
      
      
      
      
      
      mCallbacks.InsertElementAt(i, callback);
      ++i;
    }
  }

  return true;
}

bool CacheEntry::InvokeCallback(Callback & aCallback)
{
  LOG(("CacheEntry::InvokeCallback [this=%p, state=%s, cb=%p]",
    this, StateString(mState), aCallback.mCallback.get()));

  mLock.AssertCurrentThreadOwns();

  
  if (!mIsDoomed) {
    
    MOZ_ASSERT(mState > LOADING);

    if (mState == WRITING || mState == REVALIDATING) {
      
      
      
      
      LOG(("  entry is being written/revalidated, callback bypassed"));
      return false;
    }

    
    
    
    if (!aCallback.mRecheckAfterWrite) {

      if (!aCallback.mReadOnly) {
        if (mState == EMPTY) {
          
          
          
          mState = WRITING;
          LOG(("  advancing to WRITING state"));
        }

        if (!aCallback.mCallback) {
          
          
          
          return true;
        }
      }

      if (mState == READY) {
        
        uint32_t checkResult;
        {
          
          mozilla::MutexAutoUnlock unlock(mLock);

          nsresult rv = aCallback.mCallback->OnCacheEntryCheck(
            this, nullptr, &checkResult);
          LOG(("  OnCacheEntryCheck: rv=0x%08x, result=%d", rv, checkResult));

          if (NS_FAILED(rv))
            checkResult = ENTRY_NOT_WANTED;
        }

        switch (checkResult) {
        case ENTRY_WANTED:
          
          
          
          break;

        case RECHECK_AFTER_WRITE_FINISHED:
          LOG(("  consumer will check on the entry again after write is done"));
          
          aCallback.mRecheckAfterWrite = true;
          break;

        case ENTRY_NEEDS_REVALIDATION:
          LOG(("  will be holding callbacks until entry is revalidated"));
          
          
          
          mState = REVALIDATING;
          break;

        case ENTRY_NOT_WANTED:
          LOG(("  consumer not interested in the entry"));
          
          aCallback.mNotWanted = true;
          break;
        }
      }
    }
  }

  if (aCallback.mCallback) {
    if (!mIsDoomed && aCallback.mRecheckAfterWrite) {
      
      
      bool bypass = !mHasData;
      if (!bypass && NS_SUCCEEDED(mFileStatus)) {
        int64_t _unused;
        bypass = !mFile->DataSize(&_unused);
      }

      if (bypass) {
        LOG(("  bypassing, entry data still being written"));
        return false;
      }

      
      aCallback.mRecheckAfterWrite = false;
      return InvokeCallback(aCallback);
    }

    mozilla::MutexAutoUnlock unlock(mLock);
    InvokeAvailableCallback(aCallback);
  }

  return true;
}

void CacheEntry::InvokeAvailableCallback(Callback const & aCallback)
{
  LOG(("CacheEntry::InvokeAvailableCallback [this=%p, state=%s, cb=%p, r/o=%d, n/w=%d]",
    this, StateString(mState), aCallback.mCallback.get(), aCallback.mReadOnly, aCallback.mNotWanted));

  nsresult rv;

  uint32_t const state = mState;

  
  MOZ_ASSERT(state > LOADING || mIsDoomed);

  bool onAvailThread;
  rv = aCallback.OnAvailThread(&onAvailThread);
  if (NS_FAILED(rv)) {
    LOG(("  target thread dead?"));
    return;
  }

  if (!onAvailThread) {
    
    nsRefPtr<AvailableCallbackRunnable> event =
      new AvailableCallbackRunnable(this, aCallback);

    rv = aCallback.mTargetThread->Dispatch(event, nsIEventTarget::DISPATCH_NORMAL);
    LOG(("  redispatched, (rv = 0x%08x)", rv));
    return;
  }

  if (NS_SUCCEEDED(mFileStatus)) {
    
    mFile->OnFetched();
  }

  if (mIsDoomed || aCallback.mNotWanted) {
    LOG(("  doomed or not wanted, notifying OCEA with NS_ERROR_CACHE_KEY_NOT_FOUND"));
    aCallback.mCallback->OnCacheEntryAvailable(
      nullptr, false, nullptr, NS_ERROR_CACHE_KEY_NOT_FOUND);
    return;
  }

  if (state == READY) {
    LOG(("  ready/has-meta, notifying OCEA with entry and NS_OK"));
    {
      mozilla::MutexAutoLock lock(mLock);
      BackgroundOp(Ops::FRECENCYUPDATE);
    }

    nsRefPtr<CacheEntryHandle> handle = NewHandle();
    aCallback.mCallback->OnCacheEntryAvailable(
      handle, false, nullptr, NS_OK);
    return;
  }

  if (aCallback.mReadOnly) {
    LOG(("  r/o and not ready, notifying OCEA with NS_ERROR_CACHE_KEY_NOT_FOUND"));
    aCallback.mCallback->OnCacheEntryAvailable(
      nullptr, false, nullptr, NS_ERROR_CACHE_KEY_NOT_FOUND);
    return;
  }

  
  
  
  

  

  nsRefPtr<CacheEntryHandle> handle = NewWriteHandle();
  rv = aCallback.mCallback->OnCacheEntryAvailable(
    handle, state == WRITING, nullptr, NS_OK);

  if (NS_FAILED(rv)) {
    LOG(("  writing/revalidating failed (0x%08x)", rv));

    
    OnHandleClosed(handle);
    return;
  }

  LOG(("  writing/revalidating"));
}

CacheEntryHandle* CacheEntry::NewHandle()
{
  return new CacheEntryHandle(this);
}

CacheEntryHandle* CacheEntry::NewWriteHandle()
{
  mozilla::MutexAutoLock lock(mLock);

  BackgroundOp(Ops::FRECENCYUPDATE);
  return (mWriter = NewHandle());
}

void CacheEntry::OnHandleClosed(CacheEntryHandle const* aHandle)
{
  LOG(("CacheEntry::OnHandleClosed [this=%p, state=%s, handle=%p]", this, StateString(mState), aHandle));

  nsCOMPtr<nsIOutputStream> outputStream;

  {
    mozilla::MutexAutoLock lock(mLock);

    if (mWriter != aHandle) {
      LOG(("  not the writer"));
      return;
    }

    if (mOutputStream) {
      
      
      
      mHasData = false;
    }

    outputStream.swap(mOutputStream);
    mWriter = nullptr;

    if (mState == WRITING) {
      LOG(("  reverting to state EMPTY - write failed"));
      mState = EMPTY;
    }
    else if (mState == REVALIDATING) {
      LOG(("  reverting to state READY - reval failed"));
      mState = READY;
    }

    if (mState == READY && !mHasData) {
      
      
      
      
      
      
      
      
      
      LOG(("  we are in READY state, pretend we have data regardless it"
            " has actully been never touched"));
      mHasData = true;
    }

    InvokeCallbacks();
  }

  if (outputStream) {
    LOG(("  abandoning phantom output stream"));
    outputStream->Close();
  }
}

void CacheEntry::OnOutputClosed()
{
  
  

  mozilla::MutexAutoLock lock(mLock);
  InvokeCallbacks();
}

bool CacheEntry::IsReferenced() const
{
  CacheStorageService::Self()->Lock().AssertCurrentThreadOwns();

  
  
  return mHandlesCount > 0;
}

bool CacheEntry::IsFileDoomed()
{
  if (NS_SUCCEEDED(mFileStatus)) {
    return mFile->IsDoomed();
  }

  return false;
}

uint32_t CacheEntry::GetMetadataMemoryConsumption()
{
  NS_ENSURE_SUCCESS(mFileStatus, 0);

  uint32_t size;
  if (NS_FAILED(mFile->ElementsSize(&size)))
    return 0;

  return size;
}



NS_IMETHODIMP CacheEntry::GetPersistent(bool *aPersistToDisk)
{
  
  
  
  *aPersistToDisk = mUseDisk;
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::GetKey(nsACString & aKey)
{
  return mURI->GetAsciiSpec(aKey);
}

NS_IMETHODIMP CacheEntry::GetFetchCount(int32_t *aFetchCount)
{
  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  return mFile->GetFetchCount(reinterpret_cast<uint32_t*>(aFetchCount));
}

NS_IMETHODIMP CacheEntry::GetLastFetched(uint32_t *aLastFetched)
{
  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  return mFile->GetLastFetched(aLastFetched);
}

NS_IMETHODIMP CacheEntry::GetLastModified(uint32_t *aLastModified)
{
  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  return mFile->GetLastModified(aLastModified);
}

NS_IMETHODIMP CacheEntry::GetExpirationTime(uint32_t *aExpirationTime)
{
  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  return mFile->GetExpirationTime(aExpirationTime);
}

NS_IMETHODIMP CacheEntry::GetIsForcedValid(bool *aIsForcedValid)
{
  NS_ENSURE_ARG(aIsForcedValid);

  nsAutoCString key;

  nsresult rv = HashingKeyWithStorage(key);
  if (NS_FAILED(rv)) {
    return rv;
  }

  *aIsForcedValid = CacheStorageService::Self()->IsForcedValidEntry(key);
  LOG(("CacheEntry::GetIsForcedValid [this=%p, IsForcedValid=%d]", this, *aIsForcedValid));

  return NS_OK;
}

NS_IMETHODIMP CacheEntry::ForceValidFor(uint32_t aSecondsToTheFuture)
{
  LOG(("CacheEntry::ForceValidFor [this=%p, aSecondsToTheFuture=%d]", this, aSecondsToTheFuture));

  nsAutoCString key;
  nsresult rv = HashingKeyWithStorage(key);
  if (NS_FAILED(rv)) {
    return rv;
  }

  CacheStorageService::Self()->ForceEntryValidFor(key, aSecondsToTheFuture);

  return NS_OK;
}

NS_IMETHODIMP CacheEntry::SetExpirationTime(uint32_t aExpirationTime)
{
  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  nsresult rv = mFile->SetExpirationTime(aExpirationTime);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mSortingExpirationTime = aExpirationTime;
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::OpenInputStream(int64_t offset, nsIInputStream * *_retval)
{
  LOG(("CacheEntry::OpenInputStream [this=%p]", this));

  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  nsresult rv;

  nsCOMPtr<nsIInputStream> stream;
  rv = mFile->OpenInputStream(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISeekableStream> seekable =
    do_QueryInterface(stream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MutexAutoLock lock(mLock);

  if (!mHasData) {
    
    LOG(("  creating phantom output stream"));
    rv = OpenOutputStreamInternal(0, getter_AddRefs(mOutputStream));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  stream.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::OpenOutputStream(int64_t offset, nsIOutputStream * *_retval)
{
  LOG(("CacheEntry::OpenOutputStream [this=%p]", this));

  nsresult rv;

  mozilla::MutexAutoLock lock(mLock);

  MOZ_ASSERT(mState > EMPTY);

  if (mOutputStream && !mIsDoomed) {
    LOG(("  giving phantom output stream"));
    mOutputStream.forget(_retval);
  }
  else {
    rv = OpenOutputStreamInternal(offset, _retval);
    if (NS_FAILED(rv)) return rv;
  }

  
  if (mState < READY)
    mState = READY;

  
  InvokeCallbacks();

  return NS_OK;
}

nsresult CacheEntry::OpenOutputStreamInternal(int64_t offset, nsIOutputStream * *_retval)
{
  LOG(("CacheEntry::OpenOutputStreamInternal [this=%p]", this));

  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  mLock.AssertCurrentThreadOwns();

  if (mIsDoomed) {
    LOG(("  doomed..."));
    return NS_ERROR_NOT_AVAILABLE;
  }

  MOZ_ASSERT(mState > LOADING);

  nsresult rv;

  
  
  if (!mUseDisk) {
    rv = mFile->SetMemoryOnly();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsRefPtr<CacheOutputCloseListener> listener =
    new CacheOutputCloseListener(this);

  nsCOMPtr<nsIOutputStream> stream;
  rv = mFile->OpenOutputStream(listener, getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISeekableStream> seekable =
    do_QueryInterface(stream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = seekable->Seek(nsISeekableStream::NS_SEEK_SET, offset);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mHasData = true;

  stream.swap(*_retval);
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::GetPredictedDataSize(int64_t *aPredictedDataSize)
{
  *aPredictedDataSize = mPredictedDataSize;
  return NS_OK;
}
NS_IMETHODIMP CacheEntry::SetPredictedDataSize(int64_t aPredictedDataSize)
{
  mPredictedDataSize = aPredictedDataSize;

  if (CacheObserver::EntryIsTooBig(mPredictedDataSize, mUseDisk)) {
    LOG(("CacheEntry::SetPredictedDataSize [this=%p] too big, dooming", this));
    AsyncDoom(nullptr);

    return NS_ERROR_FILE_TOO_BIG;
  }

  return NS_OK;
}

NS_IMETHODIMP CacheEntry::GetSecurityInfo(nsISupports * *aSecurityInfo)
{
  {
    mozilla::MutexAutoLock lock(mLock);
    if (mSecurityInfoLoaded) {
      NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
      return NS_OK;
    }
  }

  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  nsXPIDLCString info;
  nsCOMPtr<nsISupports> secInfo;
  nsresult rv;

  rv = mFile->GetElement("security-info", getter_Copies(info));
  NS_ENSURE_SUCCESS(rv, rv);

  if (info) {
    rv = NS_DeserializeObject(info, getter_AddRefs(secInfo));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  {
    mozilla::MutexAutoLock lock(mLock);

    mSecurityInfo.swap(secInfo);
    mSecurityInfoLoaded = true;

    NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
  }

  return NS_OK;
}
NS_IMETHODIMP CacheEntry::SetSecurityInfo(nsISupports *aSecurityInfo)
{
  nsresult rv;

  NS_ENSURE_SUCCESS(mFileStatus, mFileStatus);

  {
    mozilla::MutexAutoLock lock(mLock);

    mSecurityInfo = aSecurityInfo;
    mSecurityInfoLoaded = true;
  }

  nsCOMPtr<nsISerializable> serializable =
    do_QueryInterface(aSecurityInfo);
  if (aSecurityInfo && !serializable)
    return NS_ERROR_UNEXPECTED;

  nsCString info;
  if (serializable) {
    rv = NS_SerializeToString(serializable, info);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mFile->SetElement("security-info", info.Length() ? info.get() : nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP CacheEntry::GetStorageDataSize(uint32_t *aStorageDataSize)
{
  NS_ENSURE_ARG(aStorageDataSize);

  int64_t dataSize;
  nsresult rv = GetDataSize(&dataSize);
  if (NS_FAILED(rv))
    return rv;

  *aStorageDataSize = (uint32_t)std::min(int64_t(uint32_t(-1)), dataSize);

  return NS_OK;
}

NS_IMETHODIMP CacheEntry::AsyncDoom(nsICacheEntryDoomCallback *aCallback)
{
  LOG(("CacheEntry::AsyncDoom [this=%p]", this));

  {
    mozilla::MutexAutoLock lock(mLock);

    if (mIsDoomed || mDoomCallback)
      return NS_ERROR_IN_PROGRESS; 

    mIsDoomed = true;
    mDoomCallback = aCallback;
  }

  
  
  
  
  
  PurgeAndDoom();

  return NS_OK;
}

NS_IMETHODIMP CacheEntry::GetMetaDataElement(const char * aKey, char * *aRetval)
{
  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  return mFile->GetElement(aKey, aRetval);
}

NS_IMETHODIMP CacheEntry::SetMetaDataElement(const char * aKey, const char * aValue)
{
  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  return mFile->SetElement(aKey, aValue);
}

NS_IMETHODIMP CacheEntry::VisitMetaData(nsICacheEntryMetaDataVisitor *aVisitor)
{
  NS_ENSURE_SUCCESS(mFileStatus, NS_ERROR_NOT_AVAILABLE);

  return mFile->VisitMetaData(aVisitor);
}

NS_IMETHODIMP CacheEntry::MetaDataReady()
{
  mozilla::MutexAutoLock lock(mLock);

  LOG(("CacheEntry::MetaDataReady [this=%p, state=%s]", this, StateString(mState)));

  MOZ_ASSERT(mState > EMPTY);

  if (mState == WRITING)
    mState = READY;

  InvokeCallbacks();

  return NS_OK;
}

NS_IMETHODIMP CacheEntry::SetValid()
{
  LOG(("CacheEntry::SetValid [this=%p, state=%s]", this, StateString(mState)));

  nsCOMPtr<nsIOutputStream> outputStream;

  {
    mozilla::MutexAutoLock lock(mLock);

    MOZ_ASSERT(mState > EMPTY);

    mState = READY;
    mHasData = true;

    InvokeCallbacks();

    outputStream.swap(mOutputStream);
  }

  if (outputStream) {
    LOG(("  abandoning phantom output stream"));
    outputStream->Close();
  }

  return NS_OK;
}

NS_IMETHODIMP CacheEntry::Recreate(bool aMemoryOnly,
                                   nsICacheEntry **_retval)
{
  LOG(("CacheEntry::Recreate [this=%p, state=%s]", this, StateString(mState)));

  mozilla::MutexAutoLock lock(mLock);

  nsRefPtr<CacheEntryHandle> handle = ReopenTruncated(aMemoryOnly, nullptr);
  if (handle) {
    handle.forget(_retval);
    return NS_OK;
  }

  BackgroundOp(Ops::CALLBACKS, true);
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::GetDataSize(int64_t *aDataSize)
{
  LOG(("CacheEntry::GetDataSize [this=%p]", this));
  *aDataSize = 0;

  {
    mozilla::MutexAutoLock lock(mLock);

    if (!mHasData) {
      LOG(("  write in progress (no data)"));
      return NS_ERROR_IN_PROGRESS;
    }
  }

  NS_ENSURE_SUCCESS(mFileStatus, mFileStatus);

  
  if (!mFile->DataSize(aDataSize)) {
    LOG(("  write in progress (stream active)"));
    return NS_ERROR_IN_PROGRESS;
  }

  LOG(("  size=%lld", *aDataSize));
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::MarkValid()
{
  
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::MaybeMarkValid()
{
  
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::HasWriteAccess(bool aWriteAllowed, bool *aWriteAccess)
{
  *aWriteAccess = aWriteAllowed;
  return NS_OK;
}

NS_IMETHODIMP CacheEntry::Close()
{
  
  return NS_OK;
}



NS_IMETHODIMP CacheEntry::Run()
{
  MOZ_ASSERT(CacheStorageService::IsOnManagementThread());

  mozilla::MutexAutoLock lock(mLock);

  BackgroundOp(mBackgroundOperations.Grab());
  return NS_OK;
}



double CacheEntry::GetFrecency() const
{
  MOZ_ASSERT(CacheStorageService::IsOnManagementThread());
  return mFrecency;
}

uint32_t CacheEntry::GetExpirationTime() const
{
  MOZ_ASSERT(CacheStorageService::IsOnManagementThread());
  return mSortingExpirationTime;
}

bool CacheEntry::IsRegistered() const
{
  MOZ_ASSERT(CacheStorageService::IsOnManagementThread());
  return mRegistration == REGISTERED;
}

bool CacheEntry::CanRegister() const
{
  MOZ_ASSERT(CacheStorageService::IsOnManagementThread());
  return mRegistration == NEVERREGISTERED;
}

void CacheEntry::SetRegistered(bool aRegistered)
{
  MOZ_ASSERT(CacheStorageService::IsOnManagementThread());

  if (aRegistered) {
    MOZ_ASSERT(mRegistration == NEVERREGISTERED);
    mRegistration = REGISTERED;
  }
  else {
    MOZ_ASSERT(mRegistration == REGISTERED);
    mRegistration = DEREGISTERED;
  }
}

bool CacheEntry::Purge(uint32_t aWhat)
{
  LOG(("CacheEntry::Purge [this=%p, what=%d]", this, aWhat));

  MOZ_ASSERT(CacheStorageService::IsOnManagementThread());

  switch (aWhat) {
  case PURGE_DATA_ONLY_DISK_BACKED:
  case PURGE_WHOLE_ONLY_DISK_BACKED:
    
    if (!mUseDisk) {
      LOG(("  not using disk"));
      return false;
    }
  }

  if (mState == WRITING || mState == LOADING || mFrecency == 0) {
    
    
    
    
    
    LOG(("  state=%s, frecency=%1.10f", StateString(mState), mFrecency));
    return false;
  }

  if (NS_SUCCEEDED(mFileStatus) && mFile->IsWriteInProgress()) {
    
    
    
    
    LOG(("  file still under use"));
    return false;
  }

  switch (aWhat) {
  case PURGE_WHOLE_ONLY_DISK_BACKED:
  case PURGE_WHOLE:
    {
      if (!CacheStorageService::Self()->RemoveEntry(this, true)) {
        LOG(("  not purging, still referenced"));
        return false;
      }

      CacheStorageService::Self()->UnregisterEntry(this);

      
      return true;
    }

  case PURGE_DATA_ONLY_DISK_BACKED:
    {
      NS_ENSURE_SUCCESS(mFileStatus, false);

      mFile->ThrowMemoryCachedData();

      
      return false;
    }
  }

  LOG(("  ?"));
  return false;
}

void CacheEntry::PurgeAndDoom()
{
  LOG(("CacheEntry::PurgeAndDoom [this=%p]", this));

  CacheStorageService::Self()->RemoveEntry(this);
  DoomAlreadyRemoved();
}

void CacheEntry::DoomAlreadyRemoved()
{
  LOG(("CacheEntry::DoomAlreadyRemoved [this=%p]", this));

  mozilla::MutexAutoLock lock(mLock);

  mIsDoomed = true;

  
  
  
  DoomFile();

  
  
  BackgroundOp(Ops::CALLBACKS, true);
  
  BackgroundOp(Ops::UNREGISTER);
}

void CacheEntry::DoomFile()
{
  nsresult rv = NS_ERROR_NOT_AVAILABLE;

  if (NS_SUCCEEDED(mFileStatus)) {
    
    rv = mFile->Doom(mDoomCallback ? this : nullptr);
    if (NS_SUCCEEDED(rv)) {
      LOG(("  file doomed"));
      return;
    }
    
    if (NS_ERROR_FILE_NOT_FOUND == rv) {
      
      
      
      
      rv = NS_OK;
    }
  }

  
  OnFileDoomed(rv);
}

void CacheEntry::BackgroundOp(uint32_t aOperations, bool aForceAsync)
{
  mLock.AssertCurrentThreadOwns();

  if (!CacheStorageService::IsOnManagementThread() || aForceAsync) {
    if (mBackgroundOperations.Set(aOperations))
      CacheStorageService::Self()->Dispatch(this);

    LOG(("CacheEntry::BackgroundOp this=%p dipatch of %x", this, aOperations));
    return;
  }

  {
    mozilla::MutexAutoUnlock unlock(mLock);

    MOZ_ASSERT(CacheStorageService::IsOnManagementThread());

    if (aOperations & Ops::FRECENCYUPDATE) {
      ++mUseCount;

      #ifndef M_LN2
      #define M_LN2 0.69314718055994530942
      #endif

      
      static double half_life = CacheObserver::HalfLifeSeconds();
      
      static double const decay = (M_LN2 / half_life) / static_cast<double>(PR_USEC_PER_SEC);

      double now_decay = static_cast<double>(PR_Now()) * decay;

      if (mFrecency == 0) {
        mFrecency = now_decay;
      }
      else {
        
        
        mFrecency = log(exp(mFrecency - now_decay) + 1) + now_decay;
      }
      LOG(("CacheEntry FRECENCYUPDATE [this=%p, frecency=%1.10f]", this, mFrecency));

      
      
      nsRefPtr<nsRunnableMethod<CacheEntry> > event =
        NS_NewRunnableMethod(this, &CacheEntry::StoreFrecency);
      NS_DispatchToMainThread(event);
    }

    if (aOperations & Ops::REGISTER) {
      LOG(("CacheEntry REGISTER [this=%p]", this));

      CacheStorageService::Self()->RegisterEntry(this);
    }

    if (aOperations & Ops::UNREGISTER) {
      LOG(("CacheEntry UNREGISTER [this=%p]", this));

      CacheStorageService::Self()->UnregisterEntry(this);
    }
  } 

  if (aOperations & Ops::CALLBACKS) {
    LOG(("CacheEntry CALLBACKS (invoke) [this=%p]", this));

    InvokeCallbacks();
  }
}

void CacheEntry::StoreFrecency()
{
  
  
  MOZ_ASSERT(NS_IsMainThread());

  if (NS_SUCCEEDED(mFileStatus)) {
    mFile->SetFrecency(FRECENCY2INT(mFrecency));
  }
}



CacheOutputCloseListener::CacheOutputCloseListener(CacheEntry* aEntry)
: mEntry(aEntry)
{
  MOZ_COUNT_CTOR(CacheOutputCloseListener);
}

CacheOutputCloseListener::~CacheOutputCloseListener()
{
  MOZ_COUNT_DTOR(CacheOutputCloseListener);
}

void CacheOutputCloseListener::OnOutputClosed()
{
  
  
  
  NS_DispatchToCurrentThread(this);
}

NS_IMETHODIMP CacheOutputCloseListener::Run()
{
  mEntry->OnOutputClosed();
  return NS_OK;
}



size_t CacheEntry::SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  size_t n = 0;
  nsCOMPtr<nsISizeOf> sizeOf;

  n += mCallbacks.SizeOfExcludingThis(mallocSizeOf);
  if (mFile) {
    n += mFile->SizeOfIncludingThis(mallocSizeOf);
  }

  sizeOf = do_QueryInterface(mURI);
  if (sizeOf) {
    n += sizeOf->SizeOfIncludingThis(mallocSizeOf);
  }

  n += mEnhanceID.SizeOfExcludingThisIfUnshared(mallocSizeOf);
  n += mStorageID.SizeOfExcludingThisIfUnshared(mallocSizeOf);

  
  
  
  
  
  

  return n;
}

size_t CacheEntry::SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  return mallocSizeOf(this) + SizeOfExcludingThis(mallocSizeOf);
}

} 
} 
