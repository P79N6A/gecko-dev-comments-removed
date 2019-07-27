



#include "CacheLog.h"
#include "CacheFile.h"

#include "CacheFileChunk.h"
#include "CacheFileInputStream.h"
#include "CacheFileOutputStream.h"
#include "nsThreadUtils.h"
#include "mozilla/DebugOnly.h"
#include <algorithm>
#include "nsComponentManagerUtils.h"
#include "nsProxyRelease.h"
#include "mozilla/Telemetry.h"










namespace mozilla {
namespace net {

class NotifyCacheFileListenerEvent : public nsRunnable {
public:
  NotifyCacheFileListenerEvent(CacheFileListener *aCallback,
                               nsresult aResult,
                               bool aIsNew)
    : mCallback(aCallback)
    , mRV(aResult)
    , mIsNew(aIsNew)
  {
    LOG(("NotifyCacheFileListenerEvent::NotifyCacheFileListenerEvent() "
         "[this=%p]", this));
    MOZ_COUNT_CTOR(NotifyCacheFileListenerEvent);
  }

protected:
  ~NotifyCacheFileListenerEvent()
  {
    LOG(("NotifyCacheFileListenerEvent::~NotifyCacheFileListenerEvent() "
         "[this=%p]", this));
    MOZ_COUNT_DTOR(NotifyCacheFileListenerEvent);
  }

public:
  NS_IMETHOD Run()
  {
    LOG(("NotifyCacheFileListenerEvent::Run() [this=%p]", this));

    mCallback->OnFileReady(mRV, mIsNew);
    return NS_OK;
  }

protected:
  nsCOMPtr<CacheFileListener> mCallback;
  nsresult                    mRV;
  bool                        mIsNew;
};

class NotifyChunkListenerEvent : public nsRunnable {
public:
  NotifyChunkListenerEvent(CacheFileChunkListener *aCallback,
                           nsresult aResult,
                           uint32_t aChunkIdx,
                           CacheFileChunk *aChunk)
    : mCallback(aCallback)
    , mRV(aResult)
    , mChunkIdx(aChunkIdx)
    , mChunk(aChunk)
  {
    LOG(("NotifyChunkListenerEvent::NotifyChunkListenerEvent() [this=%p]",
         this));
    MOZ_COUNT_CTOR(NotifyChunkListenerEvent);
  }

protected:
  ~NotifyChunkListenerEvent()
  {
    LOG(("NotifyChunkListenerEvent::~NotifyChunkListenerEvent() [this=%p]",
         this));
    MOZ_COUNT_DTOR(NotifyChunkListenerEvent);
  }

public:
  NS_IMETHOD Run()
  {
    LOG(("NotifyChunkListenerEvent::Run() [this=%p]", this));

    mCallback->OnChunkAvailable(mRV, mChunkIdx, mChunk);
    return NS_OK;
  }

protected:
  nsCOMPtr<CacheFileChunkListener> mCallback;
  nsresult                         mRV;
  uint32_t                         mChunkIdx;
  nsRefPtr<CacheFileChunk>         mChunk;
};


class DoomFileHelper : public CacheFileIOListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  explicit DoomFileHelper(CacheFileListener *aListener)
    : mListener(aListener)
  {
    MOZ_COUNT_CTOR(DoomFileHelper);
  }


  NS_IMETHOD OnFileOpened(CacheFileHandle *aHandle, nsresult aResult)
  {
    MOZ_CRASH("DoomFileHelper::OnFileOpened should not be called!");
    return NS_ERROR_UNEXPECTED;
  }

  NS_IMETHOD OnDataWritten(CacheFileHandle *aHandle, const char *aBuf,
                           nsresult aResult)
  {
    MOZ_CRASH("DoomFileHelper::OnDataWritten should not be called!");
    return NS_ERROR_UNEXPECTED;
  }

  NS_IMETHOD OnDataRead(CacheFileHandle *aHandle, char *aBuf, nsresult aResult)
  {
    MOZ_CRASH("DoomFileHelper::OnDataRead should not be called!");
    return NS_ERROR_UNEXPECTED;
  }

  NS_IMETHOD OnFileDoomed(CacheFileHandle *aHandle, nsresult aResult)
  {
    if (mListener)
      mListener->OnFileDoomed(aResult);
    return NS_OK;
  }

  NS_IMETHOD OnEOFSet(CacheFileHandle *aHandle, nsresult aResult)
  {
    MOZ_CRASH("DoomFileHelper::OnEOFSet should not be called!");
    return NS_ERROR_UNEXPECTED;
  }

  NS_IMETHOD OnFileRenamed(CacheFileHandle *aHandle, nsresult aResult)
  {
    MOZ_CRASH("DoomFileHelper::OnFileRenamed should not be called!");
    return NS_ERROR_UNEXPECTED;
  }

private:
  virtual ~DoomFileHelper()
  {
    MOZ_COUNT_DTOR(DoomFileHelper);
  }

  nsCOMPtr<CacheFileListener>  mListener;
};

NS_IMPL_ISUPPORTS(DoomFileHelper, CacheFileIOListener)


NS_IMPL_ADDREF(CacheFile)
NS_IMPL_RELEASE(CacheFile)
NS_INTERFACE_MAP_BEGIN(CacheFile)
  NS_INTERFACE_MAP_ENTRY(mozilla::net::CacheFileChunkListener)
  NS_INTERFACE_MAP_ENTRY(mozilla::net::CacheFileIOListener)
  NS_INTERFACE_MAP_ENTRY(mozilla::net::CacheFileMetadataListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports,
                                   mozilla::net::CacheFileChunkListener)
NS_INTERFACE_MAP_END_THREADSAFE

CacheFile::CacheFile()
  : mLock("CacheFile.mLock")
  , mOpeningFile(false)
  , mReady(false)
  , mMemoryOnly(false)
  , mOpenAsMemoryOnly(false)
  , mPriority(false)
  , mDataAccessed(false)
  , mDataIsDirty(false)
  , mWritingMetadata(false)
  , mPreloadWithoutInputStreams(true)
  , mPreloadChunkCount(0)
  , mStatus(NS_OK)
  , mDataSize(-1)
  , mOutput(nullptr)
{
  LOG(("CacheFile::CacheFile() [this=%p]", this));
}

CacheFile::~CacheFile()
{
  LOG(("CacheFile::~CacheFile() [this=%p]", this));

  MutexAutoLock lock(mLock);
  if (!mMemoryOnly && mReady) {
    
    WriteMetadataIfNeededLocked(true);
  }
}

nsresult
CacheFile::Init(const nsACString &aKey,
                bool aCreateNew,
                bool aMemoryOnly,
                bool aPriority,
                CacheFileListener *aCallback)
{
  MOZ_ASSERT(!mListener);
  MOZ_ASSERT(!mHandle);

  nsresult rv;

  mKey = aKey;
  mOpenAsMemoryOnly = mMemoryOnly = aMemoryOnly;
  mPriority = aPriority;

  
  
  
  
  
  
  
  
  mPreloadChunkCount = CacheObserver::PreloadChunkCount();

  LOG(("CacheFile::Init() [this=%p, key=%s, createNew=%d, memoryOnly=%d, "
       "priority=%d, listener=%p]", this, mKey.get(), aCreateNew, aMemoryOnly,
       aPriority, aCallback));

  if (mMemoryOnly) {
    MOZ_ASSERT(!aCallback);

    mMetadata = new CacheFileMetadata(mOpenAsMemoryOnly, mKey);
    mReady = true;
    mDataSize = mMetadata->Offset();
    return NS_OK;
  }
  else {
    uint32_t flags;
    if (aCreateNew) {
      MOZ_ASSERT(!aCallback);
      flags = CacheFileIOManager::CREATE_NEW;

      
      mMetadata = new CacheFileMetadata(mOpenAsMemoryOnly, mKey);
      mReady = true;
      mDataSize = mMetadata->Offset();
    } else {
      flags = CacheFileIOManager::CREATE;
    }

    if (mPriority) {
      flags |= CacheFileIOManager::PRIORITY;
    }

    mOpeningFile = true;
    mListener = aCallback;
    rv = CacheFileIOManager::OpenFile(mKey, flags, this);
    if (NS_FAILED(rv)) {
      mListener = nullptr;
      mOpeningFile = false;

      if (aCreateNew) {
        NS_WARNING("Forcing memory-only entry since OpenFile failed");
        LOG(("CacheFile::Init() - CacheFileIOManager::OpenFile() failed "
             "synchronously. We can continue in memory-only mode since "
             "aCreateNew == true. [this=%p]", this));

        mMemoryOnly = true;
      }
      else if (rv == NS_ERROR_NOT_INITIALIZED) {
        NS_WARNING("Forcing memory-only entry since CacheIOManager isn't "
                   "initialized.");
        LOG(("CacheFile::Init() - CacheFileIOManager isn't initialized, "
             "initializing entry as memory-only. [this=%p]", this));

        mMemoryOnly = true;
        mMetadata = new CacheFileMetadata(mOpenAsMemoryOnly, mKey);
        mReady = true;
        mDataSize = mMetadata->Offset();

        nsRefPtr<NotifyCacheFileListenerEvent> ev;
        ev = new NotifyCacheFileListenerEvent(aCallback, NS_OK, true);
        rv = NS_DispatchToCurrentThread(ev);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return NS_OK;
}

nsresult
CacheFile::OnChunkRead(nsresult aResult, CacheFileChunk *aChunk)
{
  CacheFileAutoLock lock(this);

  nsresult rv;

  uint32_t index = aChunk->Index();

  LOG(("CacheFile::OnChunkRead() [this=%p, rv=0x%08x, chunk=%p, idx=%u]",
       this, aResult, aChunk, index));

  if (NS_FAILED(aResult)) {
    SetError(aResult);
  }

  if (HaveChunkListeners(index)) {
    rv = NotifyChunkListeners(index, aResult, aChunk);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
CacheFile::OnChunkWritten(nsresult aResult, CacheFileChunk *aChunk)
{
  
  
  
  
  
  nsRefPtr<CacheFileChunk> deactivateChunkAgain;

  CacheFileAutoLock lock(this);

  nsresult rv;

  LOG(("CacheFile::OnChunkWritten() [this=%p, rv=0x%08x, chunk=%p, idx=%u]",
       this, aResult, aChunk, aChunk->Index()));

  MOZ_ASSERT(!mMemoryOnly);
  MOZ_ASSERT(!mOpeningFile);
  MOZ_ASSERT(mHandle);

  if (NS_FAILED(aResult)) {
    SetError(aResult);
  }

  if (NS_SUCCEEDED(aResult) && !aChunk->IsDirty()) {
    
    mMetadata->SetHash(aChunk->Index(), aChunk->Hash());
  }

  
  if (HaveChunkListeners(aChunk->Index())) {
    
    rv = NotifyChunkListeners(aChunk->Index(), aResult, aChunk);
    if (NS_SUCCEEDED(rv)) {
      MOZ_ASSERT(aChunk->mRefCnt != 2);
      return NS_OK;
    }
  }

  if (aChunk->mRefCnt != 2) {
    LOG(("CacheFile::OnChunkWritten() - Chunk is still used [this=%p, chunk=%p,"
         " refcnt=%d]", this, aChunk, aChunk->mRefCnt.get()));

    return NS_OK;
  }

  if (aChunk->IsDirty()) {
    LOG(("CacheFile::OnChunkWritten() - Unused chunk is dirty. We must go "
         "through deactivation again. [this=%p, chunk=%p]", this, aChunk));

    deactivateChunkAgain = aChunk;
    return NS_OK;
  }

  bool keepChunk = false;
  if (NS_SUCCEEDED(aResult)) {
    keepChunk = ShouldCacheChunk(aChunk->Index());
    LOG(("CacheFile::OnChunkWritten() - %s unused chunk [this=%p, chunk=%p]",
         keepChunk ? "Caching" : "Releasing", this, aChunk));
  } else {
    LOG(("CacheFile::OnChunkWritten() - Releasing failed chunk [this=%p, "
         "chunk=%p]", this, aChunk));
  }

  RemoveChunkInternal(aChunk, keepChunk);

  WriteMetadataIfNeededLocked();

  return NS_OK;
}

nsresult
CacheFile::OnChunkAvailable(nsresult aResult, uint32_t aChunkIdx,
                            CacheFileChunk *aChunk)
{
  MOZ_CRASH("CacheFile::OnChunkAvailable should not be called!");
  return NS_ERROR_UNEXPECTED;
}

nsresult
CacheFile::OnChunkUpdated(CacheFileChunk *aChunk)
{
  MOZ_CRASH("CacheFile::OnChunkUpdated should not be called!");
  return NS_ERROR_UNEXPECTED;
}

nsresult
CacheFile::OnFileOpened(CacheFileHandle *aHandle, nsresult aResult)
{
  nsresult rv;

  
  
  class AutoFailDoomListener
  {
  public:
    explicit AutoFailDoomListener(CacheFileHandle *aHandle)
      : mHandle(aHandle)
      , mAlreadyDoomed(false)
    {}
    ~AutoFailDoomListener()
    {
      if (!mListener)
        return;

      if (mHandle) {
        if (mAlreadyDoomed) {
          mListener->OnFileDoomed(mHandle, NS_OK);
        } else {
          CacheFileIOManager::DoomFile(mHandle, mListener);
        }
      } else {
        mListener->OnFileDoomed(nullptr, NS_ERROR_NOT_AVAILABLE);
      }
    }

    CacheFileHandle* mHandle;
    nsCOMPtr<CacheFileIOListener> mListener;
    bool mAlreadyDoomed;
  } autoDoom(aHandle);

  nsCOMPtr<CacheFileListener> listener;
  bool isNew = false;
  nsresult retval = NS_OK;

  {
    CacheFileAutoLock lock(this);

    MOZ_ASSERT(mOpeningFile);
    MOZ_ASSERT((NS_SUCCEEDED(aResult) && aHandle) ||
               (NS_FAILED(aResult) && !aHandle));
    MOZ_ASSERT((mListener && !mMetadata) || 
               (!mListener && mMetadata));  
    MOZ_ASSERT(!mMemoryOnly || mMetadata); 

    LOG(("CacheFile::OnFileOpened() [this=%p, rv=0x%08x, handle=%p]",
         this, aResult, aHandle));

    mOpeningFile = false;

    autoDoom.mListener.swap(mDoomAfterOpenListener);

    if (mMemoryOnly) {
      
      

      
      autoDoom.mAlreadyDoomed = true;
      return NS_OK;
    }
    else if (NS_FAILED(aResult)) {
      if (mMetadata) {
        
        
        NS_WARNING("Forcing memory-only entry since OpenFile failed");
        LOG(("CacheFile::OnFileOpened() - CacheFileIOManager::OpenFile() "
             "failed asynchronously. We can continue in memory-only mode since "
             "aCreateNew == true. [this=%p]", this));

        mMemoryOnly = true;
        return NS_OK;
      }
      else if (aResult == NS_ERROR_FILE_INVALID_PATH) {
        
        
        NS_WARNING("Forcing memory-only entry since CacheFileIOManager doesn't "
                   "have mCacheDirectory.");
        LOG(("CacheFile::OnFileOpened() - CacheFileIOManager doesn't have "
             "mCacheDirectory, initializing entry as memory-only. [this=%p]",
             this));

        mMemoryOnly = true;
        mMetadata = new CacheFileMetadata(mOpenAsMemoryOnly, mKey);
        mReady = true;
        mDataSize = mMetadata->Offset();

        isNew = true;
        retval = NS_OK;
      }
      else {
        
        isNew = false;
        retval = aResult;
      }

      mListener.swap(listener);
    }
    else {
      mHandle = aHandle;
      if (NS_FAILED(mStatus)) {
        CacheFileIOManager::DoomFile(mHandle, nullptr);
      }

      if (mMetadata) {
        InitIndexEntry();

        
        mMetadata->SetHandle(mHandle);

        
        mCachedChunks.Enumerate(&CacheFile::WriteAllCachedChunks, this);

        return NS_OK;
      }
    }
  }

  if (listener) {
    listener->OnFileReady(retval, isNew);
    return NS_OK;
  }

  MOZ_ASSERT(NS_SUCCEEDED(aResult));
  MOZ_ASSERT(!mMetadata);
  MOZ_ASSERT(mListener);

  mMetadata = new CacheFileMetadata(mHandle, mKey);

  rv = mMetadata->ReadMetadata(this);
  if (NS_FAILED(rv)) {
    mListener.swap(listener);
    listener->OnFileReady(rv, false);
  }

  return NS_OK;
}

nsresult
CacheFile::OnDataWritten(CacheFileHandle *aHandle, const char *aBuf,
                         nsresult aResult)
{
  MOZ_CRASH("CacheFile::OnDataWritten should not be called!");
  return NS_ERROR_UNEXPECTED;
}

nsresult
CacheFile::OnDataRead(CacheFileHandle *aHandle, char *aBuf, nsresult aResult)
{
  MOZ_CRASH("CacheFile::OnDataRead should not be called!");
  return NS_ERROR_UNEXPECTED;
}

nsresult
CacheFile::OnMetadataRead(nsresult aResult)
{
  MOZ_ASSERT(mListener);

  LOG(("CacheFile::OnMetadataRead() [this=%p, rv=0x%08x]", this, aResult));

  bool isNew = false;
  if (NS_SUCCEEDED(aResult)) {
    mReady = true;
    mDataSize = mMetadata->Offset();
    if (mDataSize == 0 && mMetadata->ElementsSize() == 0) {
      isNew = true;
      mMetadata->MarkDirty();
    } else {
      CacheFileAutoLock lock(this);
      PreloadChunks(0);
    }

    InitIndexEntry();
  }

  nsCOMPtr<CacheFileListener> listener;
  mListener.swap(listener);
  listener->OnFileReady(aResult, isNew);
  return NS_OK;
}

nsresult
CacheFile::OnMetadataWritten(nsresult aResult)
{
  CacheFileAutoLock lock(this);

  LOG(("CacheFile::OnMetadataWritten() [this=%p, rv=0x%08x]", this, aResult));

  MOZ_ASSERT(mWritingMetadata);
  mWritingMetadata = false;

  MOZ_ASSERT(!mMemoryOnly);
  MOZ_ASSERT(!mOpeningFile);

  if (NS_FAILED(aResult)) {
    
  }

  if (mOutput || mInputs.Length() || mChunks.Count())
    return NS_OK;

  if (IsDirty())
    WriteMetadataIfNeededLocked();

  if (!mWritingMetadata) {
    LOG(("CacheFile::OnMetadataWritten() - Releasing file handle [this=%p]",
         this));
    CacheFileIOManager::ReleaseNSPRHandle(mHandle);
  }

  return NS_OK;
}

nsresult
CacheFile::OnFileDoomed(CacheFileHandle *aHandle, nsresult aResult)
{
  nsCOMPtr<CacheFileListener> listener;

  {
    CacheFileAutoLock lock(this);

    MOZ_ASSERT(mListener);

    LOG(("CacheFile::OnFileDoomed() [this=%p, rv=0x%08x, handle=%p]",
         this, aResult, aHandle));

    mListener.swap(listener);
  }

  listener->OnFileDoomed(aResult);
  return NS_OK;
}

nsresult
CacheFile::OnEOFSet(CacheFileHandle *aHandle, nsresult aResult)
{
  MOZ_CRASH("CacheFile::OnEOFSet should not be called!");
  return NS_ERROR_UNEXPECTED;
}

nsresult
CacheFile::OnFileRenamed(CacheFileHandle *aHandle, nsresult aResult)
{
  MOZ_CRASH("CacheFile::OnFileRenamed should not be called!");
  return NS_ERROR_UNEXPECTED;
}

nsresult
CacheFile::OpenInputStream(nsIInputStream **_retval)
{
  CacheFileAutoLock lock(this);

  MOZ_ASSERT(mHandle || mMemoryOnly || mOpeningFile);

  if (!mReady) {
    LOG(("CacheFile::OpenInputStream() - CacheFile is not ready [this=%p]",
         this));

    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  mPreloadWithoutInputStreams = false;

  CacheFileInputStream *input = new CacheFileInputStream(this);

  LOG(("CacheFile::OpenInputStream() - Creating new input stream %p [this=%p]",
       input, this));

  mInputs.AppendElement(input);
  NS_ADDREF(input);

  mDataAccessed = true;
  NS_ADDREF(*_retval = input);
  return NS_OK;
}

nsresult
CacheFile::OpenOutputStream(CacheOutputCloseListener *aCloseListener, nsIOutputStream **_retval)
{
  CacheFileAutoLock lock(this);

  MOZ_ASSERT(mHandle || mMemoryOnly || mOpeningFile);

  if (!mReady) {
    LOG(("CacheFile::OpenOutputStream() - CacheFile is not ready [this=%p]",
         this));

    return NS_ERROR_NOT_AVAILABLE;
  }

  if (mOutput) {
    LOG(("CacheFile::OpenOutputStream() - We already have output stream %p "
         "[this=%p]", mOutput, this));

    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  
  mPreloadWithoutInputStreams = false;

  mOutput = new CacheFileOutputStream(this, aCloseListener);

  LOG(("CacheFile::OpenOutputStream() - Creating new output stream %p "
       "[this=%p]", mOutput, this));

  mDataAccessed = true;
  NS_ADDREF(*_retval = mOutput);
  return NS_OK;
}

nsresult
CacheFile::SetMemoryOnly()
{
  LOG(("CacheFile::SetMemoryOnly() mMemoryOnly=%d [this=%p]",
       mMemoryOnly, this));

  if (mMemoryOnly)
    return NS_OK;

  MOZ_ASSERT(mReady);

  if (!mReady) {
    LOG(("CacheFile::SetMemoryOnly() - CacheFile is not ready [this=%p]",
         this));

    return NS_ERROR_NOT_AVAILABLE;
  }

  if (mDataAccessed) {
    LOG(("CacheFile::SetMemoryOnly() - Data was already accessed [this=%p]", this));
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  mMemoryOnly = true;
  return NS_OK;
}

nsresult
CacheFile::Doom(CacheFileListener *aCallback)
{
  LOG(("CacheFile::Doom() [this=%p, listener=%p]", this, aCallback));

  CacheFileAutoLock lock(this);

  return DoomLocked(aCallback);
}

nsresult
CacheFile::DoomLocked(CacheFileListener *aCallback)
{
  MOZ_ASSERT(mHandle || mMemoryOnly || mOpeningFile);

  LOG(("CacheFile::DoomLocked() [this=%p, listener=%p]", this, aCallback));

  nsresult rv = NS_OK;

  if (mMemoryOnly) {
    return NS_ERROR_FILE_NOT_FOUND;
  }

  if (mHandle && mHandle->IsDoomed()) {
    return NS_ERROR_FILE_NOT_FOUND;
  }

  nsCOMPtr<CacheFileIOListener> listener;
  if (aCallback || !mHandle) {
    listener = new DoomFileHelper(aCallback);
  }
  if (mHandle) {
    rv = CacheFileIOManager::DoomFile(mHandle, listener);
  } else if (mOpeningFile) {
    mDoomAfterOpenListener = listener;
  }

  return rv;
}

nsresult
CacheFile::ThrowMemoryCachedData()
{
  CacheFileAutoLock lock(this);

  LOG(("CacheFile::ThrowMemoryCachedData() [this=%p]", this));

  if (mMemoryOnly) {
    
    
    
    LOG(("CacheFile::ThrowMemoryCachedData() - Ignoring request because the "
         "entry is memory-only. [this=%p]", this));

    return NS_ERROR_NOT_AVAILABLE;
  }

  if (mOpeningFile) {
    
    

    LOG(("CacheFile::ThrowMemoryCachedData() - Ignoring request because the "
         "entry is still opening the file [this=%p]", this));

    return NS_ERROR_ABORT;
  }

  
  
  mCachedChunks.Enumerate(&CacheFile::CleanUpCachedChunks, this);

  return NS_OK;
}

nsresult
CacheFile::GetElement(const char *aKey, char **_retval)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  const char *value;
  value = mMetadata->GetElement(aKey);
  if (!value)
    return NS_ERROR_NOT_AVAILABLE;

  *_retval = NS_strdup(value);
  return NS_OK;
}

nsresult
CacheFile::SetElement(const char *aKey, const char *aValue)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  PostWriteTimer();
  return mMetadata->SetElement(aKey, aValue);
}

nsresult
CacheFile::VisitMetaData(nsICacheEntryMetaDataVisitor *aVisitor)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  MOZ_ASSERT(mReady);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  return mMetadata->Visit(aVisitor);
}

nsresult
CacheFile::ElementsSize(uint32_t *_retval)
{
  CacheFileAutoLock lock(this);

  if (!mMetadata)
    return NS_ERROR_NOT_AVAILABLE;

  *_retval = mMetadata->ElementsSize();
  return NS_OK;
}

nsresult
CacheFile::SetExpirationTime(uint32_t aExpirationTime)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  PostWriteTimer();

  if (mHandle && !mHandle->IsDoomed())
    CacheFileIOManager::UpdateIndexEntry(mHandle, nullptr, &aExpirationTime);

  return mMetadata->SetExpirationTime(aExpirationTime);
}

nsresult
CacheFile::GetExpirationTime(uint32_t *_retval)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  return mMetadata->GetExpirationTime(_retval);
}

nsresult
CacheFile::SetFrecency(uint32_t aFrecency)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  PostWriteTimer();

  if (mHandle && !mHandle->IsDoomed())
    CacheFileIOManager::UpdateIndexEntry(mHandle, &aFrecency, nullptr);

  return mMetadata->SetFrecency(aFrecency);
}

nsresult
CacheFile::GetFrecency(uint32_t *_retval)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  return mMetadata->GetFrecency(_retval);
}

nsresult
CacheFile::GetLastModified(uint32_t *_retval)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  return mMetadata->GetLastModified(_retval);
}

nsresult
CacheFile::GetLastFetched(uint32_t *_retval)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  return mMetadata->GetLastFetched(_retval);
}

nsresult
CacheFile::GetFetchCount(uint32_t *_retval)
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  return mMetadata->GetFetchCount(_retval);
}

nsresult
CacheFile::OnFetched()
{
  CacheFileAutoLock lock(this);
  MOZ_ASSERT(mMetadata);
  NS_ENSURE_TRUE(mMetadata, NS_ERROR_UNEXPECTED);

  PostWriteTimer();

  return mMetadata->OnFetched();
}

void
CacheFile::Lock()
{
  mLock.Lock();
}

void
CacheFile::Unlock()
{
  nsTArray<nsISupports*> objs;
  objs.SwapElements(mObjsToRelease);

  mLock.Unlock();

  for (uint32_t i = 0; i < objs.Length(); i++)
    objs[i]->Release();
}

void
CacheFile::AssertOwnsLock() const
{
  mLock.AssertCurrentThreadOwns();
}

void
CacheFile::ReleaseOutsideLock(nsISupports *aObject)
{
  AssertOwnsLock();

  mObjsToRelease.AppendElement(aObject);
}

nsresult
CacheFile::GetChunk(uint32_t aIndex, ECallerType aCaller,
                    CacheFileChunkListener *aCallback, CacheFileChunk **_retval)
{
  CacheFileAutoLock lock(this);
  return GetChunkLocked(aIndex, aCaller, aCallback, _retval);
}

nsresult
CacheFile::GetChunkLocked(uint32_t aIndex, ECallerType aCaller,
                          CacheFileChunkListener *aCallback,
                          CacheFileChunk **_retval)
{
  AssertOwnsLock();

  LOG(("CacheFile::GetChunkLocked() [this=%p, idx=%u, caller=%d, listener=%p]",
       this, aIndex, aCaller, aCallback));

  MOZ_ASSERT(mReady);
  MOZ_ASSERT(mHandle || mMemoryOnly || mOpeningFile);
  MOZ_ASSERT((aCaller == READER && aCallback) ||
             (aCaller == WRITER && !aCallback) ||
             (aCaller == PRELOADER && !aCallback));

  
  
  bool preload = !mMemoryOnly && (aCaller == READER);

  nsresult rv;

  nsRefPtr<CacheFileChunk> chunk;
  if (mChunks.Get(aIndex, getter_AddRefs(chunk))) {
    LOG(("CacheFile::GetChunkLocked() - Found chunk %p in mChunks [this=%p]",
         chunk.get(), this));

    
    MOZ_ASSERT(aCaller != PRELOADER, "Unexpected!");

    
    
    rv = chunk->GetStatus();
    if (NS_FAILED(rv)) {
      SetError(rv);
      LOG(("CacheFile::GetChunkLocked() - Found failed chunk in mChunks "
           "[this=%p]", this));
      return rv;
    }

    if (chunk->IsReady() || aCaller == WRITER) {
      chunk.swap(*_retval);
    } else {
      rv = QueueChunkListener(aIndex, aCallback);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (preload) {
      PreloadChunks(aIndex + 1);
    }

    return NS_OK;
  }

  if (mCachedChunks.Get(aIndex, getter_AddRefs(chunk))) {
    LOG(("CacheFile::GetChunkLocked() - Reusing cached chunk %p [this=%p]",
         chunk.get(), this));

    
    MOZ_ASSERT(aCaller != PRELOADER, "Unexpected!");

    mChunks.Put(aIndex, chunk);
    mCachedChunks.Remove(aIndex);
    chunk->mFile = this;
    chunk->mActiveChunk = true;

    MOZ_ASSERT(chunk->IsReady());

    chunk.swap(*_retval);

    if (preload) {
      PreloadChunks(aIndex + 1);
    }

    return NS_OK;
  }

  int64_t off = aIndex * kChunkSize;

  if (off < mDataSize) {
    
    MOZ_ASSERT(!mMemoryOnly);
    if (mMemoryOnly) {
      
      
      LOG(("CacheFile::GetChunkLocked() - Unexpected state! Offset < mDataSize "
           "for memory-only entry. [this=%p, off=%lld, mDataSize=%lld]",
           this, off, mDataSize));

      return NS_ERROR_UNEXPECTED;
    }

    chunk = new CacheFileChunk(this, aIndex, aCaller == WRITER);
    mChunks.Put(aIndex, chunk);
    chunk->mActiveChunk = true;

    LOG(("CacheFile::GetChunkLocked() - Reading newly created chunk %p from "
         "the disk [this=%p]", chunk.get(), this));

    
    rv = chunk->Read(mHandle, std::min(static_cast<uint32_t>(mDataSize - off),
                     static_cast<uint32_t>(kChunkSize)),
                     mMetadata->GetHash(aIndex), this);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      RemoveChunkInternal(chunk, false);
      return rv;
    }

    if (aCaller == WRITER) {
      chunk.swap(*_retval);
    } else if (aCaller != PRELOADER) {
      rv = QueueChunkListener(aIndex, aCallback);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (preload) {
      PreloadChunks(aIndex + 1);
    }

    return NS_OK;
  } else if (off == mDataSize) {
    if (aCaller == WRITER) {
      
      chunk = new CacheFileChunk(this, aIndex, true);
      mChunks.Put(aIndex, chunk);
      chunk->mActiveChunk = true;

      LOG(("CacheFile::GetChunkLocked() - Created new empty chunk %p [this=%p]",
           chunk.get(), this));

      chunk->InitNew();
      mMetadata->SetHash(aIndex, chunk->Hash());

      if (HaveChunkListeners(aIndex)) {
        rv = NotifyChunkListeners(aIndex, NS_OK, chunk);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      chunk.swap(*_retval);
      return NS_OK;
    }
  } else {
    if (aCaller == WRITER) {
      

      
      if (mDataSize % kChunkSize) {
        rv = PadChunkWithZeroes(mDataSize / kChunkSize);
        NS_ENSURE_SUCCESS(rv, rv);

        MOZ_ASSERT(!(mDataSize % kChunkSize));
      }

      uint32_t startChunk = mDataSize / kChunkSize;

      if (mMemoryOnly) {
        
        
        for (uint32_t i = startChunk ; i < aIndex ; i++) {
          rv = PadChunkWithZeroes(i);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      } else {
        
        

        if (startChunk != aIndex) {
          
          rv = CacheFileIOManager::TruncateSeekSetEOF(mHandle,
                                                      startChunk * kChunkSize,
                                                      aIndex * kChunkSize,
                                                      nullptr);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        for (uint32_t i = startChunk ; i < aIndex ; i++) {
          if (HaveChunkListeners(i)) {
            rv = PadChunkWithZeroes(i);
            NS_ENSURE_SUCCESS(rv, rv);
          } else {
            mMetadata->SetHash(i, kEmptyChunkHash);
            mDataSize = (i + 1) * kChunkSize;
          }
        }
      }

      MOZ_ASSERT(mDataSize == off);
      rv = GetChunkLocked(aIndex, WRITER, nullptr, getter_AddRefs(chunk));
      NS_ENSURE_SUCCESS(rv, rv);

      chunk.swap(*_retval);
      return NS_OK;
    }
  }

  
  
  
  MOZ_ASSERT(aCaller == READER, "Unexpected!");

  if (mOutput) {
    
    rv = QueueChunkListener(aIndex, aCallback);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}

void
CacheFile::PreloadChunks(uint32_t aIndex)
{
  AssertOwnsLock();

  uint32_t limit = aIndex + mPreloadChunkCount;

  for (uint32_t i = aIndex; i < limit; ++i) {
    int64_t off = i * kChunkSize;

    if (off >= mDataSize) {
      
      return;
    }

    if (mChunks.GetWeak(i) || mCachedChunks.GetWeak(i)) {
      
      continue;
    }

    LOG(("CacheFile::PreloadChunks() - Preloading chunk [this=%p, idx=%u]",
         this, i));

    nsRefPtr<CacheFileChunk> chunk;
    GetChunkLocked(i, PRELOADER, nullptr, getter_AddRefs(chunk));
    
    
    MOZ_ASSERT(!chunk);
  }
}

bool
CacheFile::ShouldCacheChunk(uint32_t aIndex)
{
  AssertOwnsLock();

#ifdef CACHE_CHUNKS
  
  return true;
#else

  if (mPreloadChunkCount != 0 && mInputs.Length() == 0 &&
      mPreloadWithoutInputStreams && aIndex < mPreloadChunkCount) {
    
    
    
    
    
    
    return true;
  }

  
  return MustKeepCachedChunk(aIndex);
#endif
}

bool
CacheFile::MustKeepCachedChunk(uint32_t aIndex)
{
  AssertOwnsLock();

  
  
  if (mMemoryOnly || mOpeningFile) {
    return true;
  }

  if (mPreloadChunkCount == 0) {
    
    return false;
  }

  
  

  
  int64_t maxPos = static_cast<int64_t>(aIndex + 1) * kChunkSize - 1;

  
  
  int64_t minPos;
  if (mPreloadChunkCount >= aIndex) {
    minPos = 0;
  } else {
    minPos = static_cast<int64_t>(aIndex - mPreloadChunkCount) * kChunkSize;
  }

  for (uint32_t i = 0; i < mInputs.Length(); ++i) {
    int64_t inputPos = mInputs[i]->GetPosition();
    if (inputPos >= minPos && inputPos <= maxPos) {
      return true;
    }
  }

  return false;
}

nsresult
CacheFile::DeactivateChunk(CacheFileChunk *aChunk)
{
  nsresult rv;

  
  nsRefPtr<CacheFileChunk> chunk = aChunk;

  {
    CacheFileAutoLock lock(this);

    LOG(("CacheFile::DeactivateChunk() [this=%p, chunk=%p, idx=%u]",
         this, aChunk, aChunk->Index()));

    MOZ_ASSERT(mReady);
    MOZ_ASSERT((mHandle && !mMemoryOnly && !mOpeningFile) ||
               (!mHandle && mMemoryOnly && !mOpeningFile) ||
               (!mHandle && !mMemoryOnly && mOpeningFile));

    if (aChunk->mRefCnt != 2) {
      LOG(("CacheFile::DeactivateChunk() - Chunk is still used [this=%p, "
           "chunk=%p, refcnt=%d]", this, aChunk, aChunk->mRefCnt.get()));

      
      return NS_OK;
    }

#ifdef DEBUG
    {
      
      nsRefPtr<CacheFileChunk> chunkCheck;
      mChunks.Get(chunk->Index(), getter_AddRefs(chunkCheck));
      MOZ_ASSERT(chunkCheck == chunk);

      
      ChunkListeners *listeners;
      mChunkListeners.Get(chunk->Index(), &listeners);
      MOZ_ASSERT(!listeners);
    }
#endif

    if (NS_FAILED(chunk->GetStatus())) {
      SetError(chunk->GetStatus());
    }

    if (NS_FAILED(mStatus)) {
      
      LOG(("CacheFile::DeactivateChunk() - Releasing chunk because of status "
           "[this=%p, chunk=%p, mStatus=0x%08x]", this, chunk.get(), mStatus));

      RemoveChunkInternal(chunk, false);
      return mStatus;
    }

    if (chunk->IsDirty() && !mMemoryOnly && !mOpeningFile) {
      LOG(("CacheFile::DeactivateChunk() - Writing dirty chunk to the disk "
           "[this=%p]", this));

      mDataIsDirty = true;

      rv = chunk->Write(mHandle, this);
      if (NS_FAILED(rv)) {
        LOG(("CacheFile::DeactivateChunk() - CacheFileChunk::Write() failed "
             "synchronously. Removing it. [this=%p, chunk=%p, rv=0x%08x]",
             this, chunk.get(), rv));

        RemoveChunkInternal(chunk, false);

        SetError(rv);
        return rv;
      }

      

      
      
      chunk = nullptr;
      return NS_OK;
    }

    bool keepChunk = ShouldCacheChunk(aChunk->Index());
    LOG(("CacheFile::DeactivateChunk() - %s unused chunk [this=%p, chunk=%p]",
         keepChunk ? "Caching" : "Releasing", this, chunk.get()));

    RemoveChunkInternal(chunk, keepChunk);

    if (!mMemoryOnly)
      WriteMetadataIfNeededLocked();
  }

  return NS_OK;
}

void
CacheFile::RemoveChunkInternal(CacheFileChunk *aChunk, bool aCacheChunk)
{
  AssertOwnsLock();

  aChunk->mActiveChunk = false;
  ReleaseOutsideLock(static_cast<CacheFileChunkListener *>(
                       aChunk->mFile.forget().take()));

  if (aCacheChunk) {
    mCachedChunks.Put(aChunk->Index(), aChunk);
  }

  mChunks.Remove(aChunk->Index());
}

int64_t
CacheFile::BytesFromChunk(uint32_t aIndex)
{
  AssertOwnsLock();

  if (!mDataSize)
    return 0;

  
  uint32_t lastChunk = (mDataSize - 1) / kChunkSize;
  if (aIndex > lastChunk)
    return 0;

  
  
  
  uint32_t maxPreloadedChunk;
  if (mMemoryOnly) {
    maxPreloadedChunk = lastChunk;
  } else {
    maxPreloadedChunk = std::min(aIndex + mPreloadChunkCount, lastChunk);
  }

  uint32_t i;
  for (i = aIndex; i <= maxPreloadedChunk; ++i) {
    CacheFileChunk * chunk;

    chunk = mChunks.GetWeak(i);
    if (chunk) {
      MOZ_ASSERT(i == lastChunk || chunk->mDataSize == kChunkSize);
      if (chunk->IsReady()) {
        continue;
      }

      
      break;
    }

    chunk = mCachedChunks.GetWeak(i);
    if (chunk) {
      MOZ_ASSERT(i == lastChunk || chunk->mDataSize == kChunkSize);
      continue;
    }

    break;
  }

  
  int64_t advance = int64_t(i - aIndex) * kChunkSize;
  
  int64_t tail = mDataSize - (aIndex * kChunkSize);

  return std::min(advance, tail);
}

static uint32_t
StatusToTelemetryEnum(nsresult aStatus)
{
  if (NS_SUCCEEDED(aStatus)) {
    return 0;
  }

  switch (aStatus) {
    case NS_BASE_STREAM_CLOSED:
      return 0; 
    case NS_ERROR_OUT_OF_MEMORY:
      return 2;
    case NS_ERROR_FILE_DISK_FULL:
      return 3;
    case NS_ERROR_FILE_CORRUPTED:
      return 4;
    case NS_ERROR_FILE_NOT_FOUND:
      return 5;
    case NS_BINDING_ABORTED:
      return 6;
    default:
      return 1; 
  }

  NS_NOTREACHED("We should never get here");
}

nsresult
CacheFile::RemoveInput(CacheFileInputStream *aInput, nsresult aStatus)
{
  CacheFileAutoLock lock(this);

  LOG(("CacheFile::RemoveInput() [this=%p, input=%p, status=0x%08x]", this,
       aInput, aStatus));

  DebugOnly<bool> found;
  found = mInputs.RemoveElement(aInput);
  MOZ_ASSERT(found);

  ReleaseOutsideLock(static_cast<nsIInputStream*>(aInput));

  if (!mMemoryOnly)
    WriteMetadataIfNeededLocked();

  
  
  mCachedChunks.Enumerate(&CacheFile::CleanUpCachedChunks, this);

  Telemetry::Accumulate(Telemetry::NETWORK_CACHE_V2_INPUT_STREAM_STATUS,
                        StatusToTelemetryEnum(aStatus));

  return NS_OK;
}

nsresult
CacheFile::RemoveOutput(CacheFileOutputStream *aOutput, nsresult aStatus)
{
  AssertOwnsLock();

  LOG(("CacheFile::RemoveOutput() [this=%p, output=%p, status=0x%08x]", this,
       aOutput, aStatus));

  if (mOutput != aOutput) {
    LOG(("CacheFile::RemoveOutput() - This output was already removed, ignoring"
         " call [this=%p]", this));
    return NS_OK;
  }

  mOutput = nullptr;

  
  NotifyListenersAboutOutputRemoval();

  if (!mMemoryOnly)
    WriteMetadataIfNeededLocked();

  
  aOutput->NotifyCloseListener();

  Telemetry::Accumulate(Telemetry::NETWORK_CACHE_V2_OUTPUT_STREAM_STATUS,
                        StatusToTelemetryEnum(aStatus));

  return NS_OK;
}

nsresult
CacheFile::NotifyChunkListener(CacheFileChunkListener *aCallback,
                               nsIEventTarget *aTarget,
                               nsresult aResult,
                               uint32_t aChunkIdx,
                               CacheFileChunk *aChunk)
{
  LOG(("CacheFile::NotifyChunkListener() [this=%p, listener=%p, target=%p, "
       "rv=0x%08x, idx=%u, chunk=%p]", this, aCallback, aTarget, aResult,
       aChunkIdx, aChunk));

  nsresult rv;
  nsRefPtr<NotifyChunkListenerEvent> ev;
  ev = new NotifyChunkListenerEvent(aCallback, aResult, aChunkIdx, aChunk);
  if (aTarget)
    rv = aTarget->Dispatch(ev, NS_DISPATCH_NORMAL);
  else
    rv = NS_DispatchToCurrentThread(ev);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
CacheFile::QueueChunkListener(uint32_t aIndex,
                              CacheFileChunkListener *aCallback)
{
  LOG(("CacheFile::QueueChunkListener() [this=%p, idx=%u, listener=%p]",
       this, aIndex, aCallback));

  AssertOwnsLock();

  MOZ_ASSERT(aCallback);

  ChunkListenerItem *item = new ChunkListenerItem();
  item->mTarget = CacheFileIOManager::IOTarget();
  if (!item->mTarget) {
    LOG(("CacheFile::QueueChunkListener() - Cannot get Cache I/O thread! Using "
         "main thread for callback."));
    item->mTarget = do_GetMainThread();
  }
  item->mCallback = aCallback;

  ChunkListeners *listeners;
  if (!mChunkListeners.Get(aIndex, &listeners)) {
    listeners = new ChunkListeners();
    mChunkListeners.Put(aIndex, listeners);
  }

  listeners->mItems.AppendElement(item);
  return NS_OK;
}

nsresult
CacheFile::NotifyChunkListeners(uint32_t aIndex, nsresult aResult,
                                CacheFileChunk *aChunk)
{
  LOG(("CacheFile::NotifyChunkListeners() [this=%p, idx=%u, rv=0x%08x, "
       "chunk=%p]", this, aIndex, aResult, aChunk));

  AssertOwnsLock();

  nsresult rv, rv2;

  ChunkListeners *listeners;
  mChunkListeners.Get(aIndex, &listeners);
  MOZ_ASSERT(listeners);

  rv = NS_OK;
  for (uint32_t i = 0 ; i < listeners->mItems.Length() ; i++) {
    ChunkListenerItem *item = listeners->mItems[i];
    rv2 = NotifyChunkListener(item->mCallback, item->mTarget, aResult, aIndex,
                              aChunk);
    if (NS_FAILED(rv2) && NS_SUCCEEDED(rv))
      rv = rv2;
    delete item;
  }

  mChunkListeners.Remove(aIndex);

  return rv;
}

bool
CacheFile::HaveChunkListeners(uint32_t aIndex)
{
  ChunkListeners *listeners;
  mChunkListeners.Get(aIndex, &listeners);
  return !!listeners;
}

void
CacheFile::NotifyListenersAboutOutputRemoval()
{
  LOG(("CacheFile::NotifyListenersAboutOutputRemoval() [this=%p]", this));

  AssertOwnsLock();

  
  mChunkListeners.Enumerate(&CacheFile::FailListenersIfNonExistentChunk,
                            this);

  
  mChunks.Enumerate(&CacheFile::FailUpdateListeners, this);
}

bool
CacheFile::DataSize(int64_t* aSize)
{
  CacheFileAutoLock lock(this);

  if (mOutput)
    return false;

  *aSize = mDataSize;
  return true;
}

bool
CacheFile::IsDoomed()
{
  if (!mHandle)
    return false;

  return mHandle->IsDoomed();
}

bool
CacheFile::IsWriteInProgress()
{
  
  
  

  bool result = false;

  if (!mMemoryOnly) {
    result = mDataIsDirty ||
             (mMetadata && mMetadata->IsDirty()) ||
             mWritingMetadata;
  }

  result = result ||
           mOpeningFile ||
           mOutput ||
           mChunks.Count();

  return result;
}

bool
CacheFile::IsDirty()
{
  return mDataIsDirty || mMetadata->IsDirty();
}

void
CacheFile::WriteMetadataIfNeeded()
{
  LOG(("CacheFile::WriteMetadataIfNeeded() [this=%p]", this));

  CacheFileAutoLock lock(this);

  if (!mMemoryOnly)
    WriteMetadataIfNeededLocked();
}

void
CacheFile::WriteMetadataIfNeededLocked(bool aFireAndForget)
{
  
  

  LOG(("CacheFile::WriteMetadataIfNeededLocked() [this=%p]", this));

  nsresult rv;

  AssertOwnsLock();
  MOZ_ASSERT(!mMemoryOnly);

  if (!mMetadata) {
    MOZ_CRASH("Must have metadata here");
    return;
  }

  if (NS_FAILED(mStatus))
    return;

  if (!IsDirty() || mOutput || mInputs.Length() || mChunks.Count() ||
      mWritingMetadata || mOpeningFile)
    return;

  if (!aFireAndForget) {
    
    
    CacheFileIOManager::UnscheduleMetadataWrite(this);
  }

  LOG(("CacheFile::WriteMetadataIfNeededLocked() - Writing metadata [this=%p]",
       this));

  rv = mMetadata->WriteMetadata(mDataSize, aFireAndForget ? nullptr : this);
  if (NS_SUCCEEDED(rv)) {
    mWritingMetadata = true;
    mDataIsDirty = false;
  } else {
    LOG(("CacheFile::WriteMetadataIfNeededLocked() - Writing synchronously "
         "failed [this=%p]", this));
    
    SetError(rv);
  }
}

void
CacheFile::PostWriteTimer()
{
  if (mMemoryOnly)
    return;

  LOG(("CacheFile::PostWriteTimer() [this=%p]", this));

  CacheFileIOManager::ScheduleMetadataWrite(this);
}

PLDHashOperator
CacheFile::WriteAllCachedChunks(const uint32_t& aIdx,
                                nsRefPtr<CacheFileChunk>& aChunk,
                                void* aClosure)
{
  CacheFile *file = static_cast<CacheFile*>(aClosure);

  LOG(("CacheFile::WriteAllCachedChunks() [this=%p, idx=%u, chunk=%p]",
       file, aIdx, aChunk.get()));

  file->mChunks.Put(aIdx, aChunk);
  aChunk->mFile = file;
  aChunk->mActiveChunk = true;

  MOZ_ASSERT(aChunk->IsReady());

  NS_ADDREF(aChunk);
  file->ReleaseOutsideLock(aChunk);

  return PL_DHASH_REMOVE;
}

PLDHashOperator
CacheFile::FailListenersIfNonExistentChunk(
  const uint32_t& aIdx,
  nsAutoPtr<ChunkListeners>& aListeners,
  void* aClosure)
{
  CacheFile *file = static_cast<CacheFile*>(aClosure);

  LOG(("CacheFile::FailListenersIfNonExistentChunk() [this=%p, idx=%u]",
       file, aIdx));

  nsRefPtr<CacheFileChunk> chunk;
  file->mChunks.Get(aIdx, getter_AddRefs(chunk));
  if (chunk) {
    MOZ_ASSERT(!chunk->IsReady());
    return PL_DHASH_NEXT;
  }

  for (uint32_t i = 0 ; i < aListeners->mItems.Length() ; i++) {
    ChunkListenerItem *item = aListeners->mItems[i];
    file->NotifyChunkListener(item->mCallback, item->mTarget,
                              NS_ERROR_NOT_AVAILABLE, aIdx, nullptr);
    delete item;
  }

  return PL_DHASH_REMOVE;
}

PLDHashOperator
CacheFile::FailUpdateListeners(
  const uint32_t& aIdx,
  nsRefPtr<CacheFileChunk>& aChunk,
  void* aClosure)
{
#ifdef PR_LOGGING
  CacheFile *file = static_cast<CacheFile*>(aClosure);
#endif

  LOG(("CacheFile::FailUpdateListeners() [this=%p, idx=%u]",
       file, aIdx));

  if (aChunk->IsReady()) {
    aChunk->NotifyUpdateListeners();
  }

  return PL_DHASH_NEXT;
}

PLDHashOperator
CacheFile::CleanUpCachedChunks(const uint32_t& aIdx,
                               nsRefPtr<CacheFileChunk>& aChunk,
                               void* aClosure)
{
  CacheFile *file = static_cast<CacheFile*>(aClosure);

  LOG(("CacheFile::CleanUpCachedChunks() [this=%p, idx=%u, chunk=%p]", file,
       aIdx, aChunk.get()));

  if (file->MustKeepCachedChunk(aIdx)) {
    LOG(("CacheFile::CleanUpCachedChunks() - Keeping chunk"));
    return PL_DHASH_NEXT;
  }

  LOG(("CacheFile::CleanUpCachedChunks() - Removing chunk"));
  return PL_DHASH_REMOVE;
}

nsresult
CacheFile::PadChunkWithZeroes(uint32_t aChunkIdx)
{
  AssertOwnsLock();

  
  
  MOZ_ASSERT(mDataSize / kChunkSize == aChunkIdx);

  nsresult rv;
  nsRefPtr<CacheFileChunk> chunk;
  rv = GetChunkLocked(aChunkIdx, WRITER, nullptr, getter_AddRefs(chunk));
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("CacheFile::PadChunkWithZeroes() - Zeroing hole in chunk %d, range %d-%d"
       " [this=%p]", aChunkIdx, chunk->DataSize(), kChunkSize - 1, this));

  chunk->EnsureBufSize(kChunkSize);
  memset(chunk->BufForWriting() + chunk->DataSize(), 0, kChunkSize - chunk->DataSize());

  chunk->UpdateDataSize(chunk->DataSize(), kChunkSize - chunk->DataSize(),
                        false);

  ReleaseOutsideLock(chunk.forget().take());

  return NS_OK;
}

void
CacheFile::SetError(nsresult aStatus)
{
  AssertOwnsLock();

  if (NS_SUCCEEDED(mStatus)) {
    mStatus = aStatus;
    if (mHandle) {
      CacheFileIOManager::DoomFile(mHandle, nullptr);
    }
  }
}

nsresult
CacheFile::InitIndexEntry()
{
  MOZ_ASSERT(mHandle);

  if (mHandle->IsDoomed())
    return NS_OK;

  nsresult rv;

  rv = CacheFileIOManager::InitIndexEntry(mHandle,
                                          mMetadata->AppId(),
                                          mMetadata->IsAnonymous(),
                                          mMetadata->IsInBrowser());
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t expTime;
  mMetadata->GetExpirationTime(&expTime);

  uint32_t frecency;
  mMetadata->GetFrecency(&frecency);

  rv = CacheFileIOManager::UpdateIndexEntry(mHandle, &frecency, &expTime);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}



namespace { 

size_t
CollectChunkSize(uint32_t const & aIdx,
                 nsRefPtr<mozilla::net::CacheFileChunk> const & aChunk,
                 mozilla::MallocSizeOf mallocSizeOf, void* aClosure)
{
  return aChunk->SizeOfIncludingThis(mallocSizeOf);
}

} 

size_t
CacheFile::SizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  CacheFileAutoLock lock(const_cast<CacheFile*>(this));

  size_t n = 0;
  n += mKey.SizeOfExcludingThisIfUnshared(mallocSizeOf);
  n += mChunks.SizeOfExcludingThis(CollectChunkSize, mallocSizeOf);
  n += mCachedChunks.SizeOfExcludingThis(CollectChunkSize, mallocSizeOf);
  if (mMetadata) {
    n += mMetadata->SizeOfIncludingThis(mallocSizeOf);
  }

  
  n += mInputs.SizeOfExcludingThis(mallocSizeOf);
  for (uint32_t i = 0; i < mInputs.Length(); ++i) {
    n += mInputs[i]->SizeOfIncludingThis(mallocSizeOf);
  }

  
  if (mOutput) {
    n += mOutput->SizeOfIncludingThis(mallocSizeOf);
  }

  
  n += mChunkListeners.SizeOfExcludingThis(nullptr, mallocSizeOf);
  n += mObjsToRelease.SizeOfExcludingThis(mallocSizeOf);

  

  return n;
}

size_t
CacheFile::SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const
{
  return mallocSizeOf(this) + SizeOfExcludingThis(mallocSizeOf);
}

} 
} 
