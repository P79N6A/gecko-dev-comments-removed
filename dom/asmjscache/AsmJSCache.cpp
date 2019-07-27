





#include "AsmJSCache.h"

#include <stdio.h>

#include "js/RootingAPI.h"
#include "jsfriendapi.h"
#include "mozilla/Assertions.h"
#include "mozilla/CondVar.h"
#include "mozilla/dom/asmjscache/PAsmJSCacheEntryChild.h"
#include "mozilla/dom/asmjscache/PAsmJSCacheEntryParent.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/PermissionMessageUtils.h"
#include "mozilla/dom/quota/Client.h"
#include "mozilla/dom/quota/OriginOrPatternString.h"
#include "mozilla/dom/quota/QuotaManager.h"
#include "mozilla/dom/quota/QuotaObject.h"
#include "mozilla/dom/quota/UsageInfo.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/unused.h"
#include "nsIAtom.h"
#include "nsIFile.h"
#include "nsIPermissionManager.h"
#include "nsIPrincipal.h"
#include "nsIRunnable.h"
#include "nsISimpleEnumerator.h"
#include "nsIThread.h"
#include "nsIXULAppInfo.h"
#include "nsJSPrincipals.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "prio.h"
#include "private/pprio.h"
#include "mozilla/Services.h"

#define ASMJSCACHE_METADATA_FILE_NAME "metadata"
#define ASMJSCACHE_ENTRY_FILE_NAME_BASE "module"

using mozilla::dom::quota::AssertIsOnIOThread;
using mozilla::dom::quota::OriginOrPatternString;
using mozilla::dom::quota::PersistenceType;
using mozilla::dom::quota::QuotaManager;
using mozilla::dom::quota::QuotaObject;
using mozilla::dom::quota::UsageInfo;
using mozilla::unused;
using mozilla::HashString;

namespace mozilla {

MOZ_TYPE_SPECIFIC_SCOPED_POINTER_TEMPLATE(ScopedPRFileDesc, PRFileDesc, PR_Close);

namespace dom {
namespace asmjscache {

namespace {

bool
IsMainProcess()
{
  return XRE_GetProcessType() == GeckoProcessType_Default;
}



static const size_t sMinCachedModuleLength = 10000;


static const unsigned sNumFastHashChars = 4096;

nsresult
WriteMetadataFile(nsIFile* aMetadataFile, const Metadata& aMetadata)
{
  int32_t openFlags = PR_WRONLY | PR_TRUNCATE | PR_CREATE_FILE;

  JS::BuildIdCharVector buildId;
  bool ok = GetBuildId(&buildId);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  ScopedPRFileDesc fd;
  nsresult rv = aMetadataFile->OpenNSPRFileDesc(openFlags, 0644, &fd.rwget());
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t length = buildId.length();
  int32_t bytesWritten = PR_Write(fd, &length, sizeof(length));
  NS_ENSURE_TRUE(bytesWritten == sizeof(length), NS_ERROR_UNEXPECTED);

  bytesWritten = PR_Write(fd, buildId.begin(), length);
  NS_ENSURE_TRUE(bytesWritten == int32_t(length), NS_ERROR_UNEXPECTED);

  bytesWritten = PR_Write(fd, &aMetadata, sizeof(aMetadata));
  NS_ENSURE_TRUE(bytesWritten == sizeof(aMetadata), NS_ERROR_UNEXPECTED);

  return NS_OK;
}

nsresult
ReadMetadataFile(nsIFile* aMetadataFile, Metadata& aMetadata)
{
  int32_t openFlags = PR_RDONLY;

  ScopedPRFileDesc fd;
  nsresult rv = aMetadataFile->OpenNSPRFileDesc(openFlags, 0644, &fd.rwget());
  NS_ENSURE_SUCCESS(rv, rv);

  

  JS::BuildIdCharVector currentBuildId;
  bool ok = GetBuildId(&currentBuildId);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  uint32_t length;
  int32_t bytesRead = PR_Read(fd, &length, sizeof(length));
  NS_ENSURE_TRUE(bytesRead == sizeof(length), NS_ERROR_UNEXPECTED);

  NS_ENSURE_TRUE(currentBuildId.length() == length, NS_ERROR_UNEXPECTED);

  JS::BuildIdCharVector fileBuildId;
  ok = fileBuildId.resize(length);
  NS_ENSURE_TRUE(ok, NS_ERROR_OUT_OF_MEMORY);

  bytesRead = PR_Read(fd, fileBuildId.begin(), length);
  NS_ENSURE_TRUE(bytesRead == int32_t(length), NS_ERROR_UNEXPECTED);

  for (uint32_t i = 0; i < length; i++) {
    if (currentBuildId[i] != fileBuildId[i]) {
      return NS_ERROR_FAILURE;
    }
  }

  

  bytesRead = PR_Read(fd, &aMetadata, sizeof(aMetadata));
  NS_ENSURE_TRUE(bytesRead == sizeof(aMetadata), NS_ERROR_UNEXPECTED);

  return NS_OK;
}

nsresult
GetCacheFile(nsIFile* aDirectory, unsigned aModuleIndex, nsIFile** aCacheFile)
{
  nsCOMPtr<nsIFile> cacheFile;
  nsresult rv = aDirectory->Clone(getter_AddRefs(cacheFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString cacheFileName = NS_LITERAL_STRING(ASMJSCACHE_ENTRY_FILE_NAME_BASE);
  cacheFileName.AppendInt(aModuleIndex);
  rv = cacheFile->Append(cacheFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  cacheFile.forget(aCacheFile);
  return NS_OK;
}

class AutoDecreaseUsageForOrigin
{
  const nsACString& mGroup;
  const nsACString& mOrigin;

public:
  uint64_t mFreed;

  AutoDecreaseUsageForOrigin(const nsACString& aGroup,
                             const nsACString& aOrigin)

  : mGroup(aGroup),
    mOrigin(aOrigin),
    mFreed(0)
  { }

  ~AutoDecreaseUsageForOrigin()
  {
    AssertIsOnIOThread();

    if (!mFreed) {
      return;
    }

    QuotaManager* qm = QuotaManager::Get();
    MOZ_ASSERT(qm, "We are on the QuotaManager's IO thread");

    qm->DecreaseUsageForOrigin(quota::PERSISTENCE_TYPE_TEMPORARY,
                               mGroup, mOrigin, mFreed);
  }
};

static void
EvictEntries(nsIFile* aDirectory, const nsACString& aGroup,
             const nsACString& aOrigin, uint64_t aNumBytes,
             Metadata& aMetadata)
{
  AssertIsOnIOThread();

  AutoDecreaseUsageForOrigin usage(aGroup, aOrigin);

  for (int i = Metadata::kLastEntry; i >= 0 && usage.mFreed < aNumBytes; i--) {
    Metadata::Entry& entry = aMetadata.mEntries[i];
    unsigned moduleIndex = entry.mModuleIndex;

    nsCOMPtr<nsIFile> file;
    nsresult rv = GetCacheFile(aDirectory, moduleIndex, getter_AddRefs(file));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    bool exists;
    rv = file->Exists(&exists);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return;
    }

    if (exists) {
      int64_t fileSize;
      rv = file->GetFileSize(&fileSize);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return;
      }

      rv = file->Remove(false);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return;
      }

      usage.mFreed += fileSize;
    }

    entry.clear();
  }
}








class FileDescriptorHolder : public nsRunnable
{
public:
  FileDescriptorHolder()
  : mQuotaObject(nullptr),
    mFileSize(INT64_MIN),
    mFileDesc(nullptr),
    mFileMap(nullptr),
    mMappedMemory(nullptr)
  { }

  ~FileDescriptorHolder()
  {
    
    MOZ_ASSERT(!mQuotaObject);
    MOZ_ASSERT(!mMappedMemory);
    MOZ_ASSERT(!mFileMap);
    MOZ_ASSERT(!mFileDesc);
  }

  size_t
  FileSize() const
  {
    MOZ_ASSERT(mFileSize >= 0, "Accessing FileSize of unopened file");
    return mFileSize;
  }

  PRFileDesc*
  FileDesc() const
  {
    MOZ_ASSERT(mFileDesc, "Accessing FileDesc of unopened file");
    return mFileDesc;
  }

  bool
  MapMemory(OpenMode aOpenMode)
  {
    MOZ_ASSERT(!mFileMap, "Cannot call MapMemory twice");

    PRFileMapProtect mapFlags = aOpenMode == eOpenForRead ? PR_PROT_READONLY
                                                          : PR_PROT_READWRITE;

    mFileMap = PR_CreateFileMap(mFileDesc, mFileSize, mapFlags);
    NS_ENSURE_TRUE(mFileMap, false);

    mMappedMemory = PR_MemMap(mFileMap, 0, mFileSize);
    NS_ENSURE_TRUE(mMappedMemory, false);

    return true;
  }

  void*
  MappedMemory() const
  {
    MOZ_ASSERT(mMappedMemory, "Accessing MappedMemory of un-mapped file");
    return mMappedMemory;
  }

protected:
  
  
  
  void
  Finish()
  {
    if (mMappedMemory) {
      PR_MemUnmap(mMappedMemory, mFileSize);
      mMappedMemory = nullptr;
    }
    if (mFileMap) {
      PR_CloseFileMap(mFileMap);
      mFileMap = nullptr;
    }
    if (mFileDesc) {
      PR_Close(mFileDesc);
      mFileDesc = nullptr;
    }

    
    
    
    mQuotaObject = nullptr;
  }

  nsRefPtr<QuotaObject> mQuotaObject;
  int64_t mFileSize;
  PRFileDesc* mFileDesc;
  PRFileMap* mFileMap;
  void* mMappedMemory;
};





class File : public virtual FileDescriptorHolder
{
public:
  class AutoClose
  {
    File* mFile;

  public:
    explicit AutoClose(File* aFile = nullptr)
    : mFile(aFile)
    { }

    void
    Init(File* aFile)
    {
      MOZ_ASSERT(!mFile);
      mFile = aFile;
    }

    File*
    operator->() const
    {
      MOZ_ASSERT(mFile);
      return mFile;
    }

    void
    Forget(File** aFile)
    {
      *aFile = mFile;
      mFile = nullptr;
    }

    ~AutoClose()
    {
      if (mFile) {
        mFile->Close();
      }
    }
  };

  bool
  BlockUntilOpen(AutoClose* aCloser)
  {
    MOZ_ASSERT(!mWaiting, "Can only call BlockUntilOpen once");
    MOZ_ASSERT(!mOpened, "Can only call BlockUntilOpen once");

    mWaiting = true;

    nsresult rv = NS_DispatchToMainThread(this);
    NS_ENSURE_SUCCESS(rv, false);

    {
      MutexAutoLock lock(mMutex);
      while (mWaiting) {
        mCondVar.Wait();
      }
    }

    if (!mOpened) {
      return false;
    }

    
    
    
    aCloser->Init(this);
    AddRef();
    return true;
  }

  
  
  
  virtual void
  Close() = 0;

protected:
  File()
  : mMutex("File::mMutex"),
    mCondVar(mMutex, "File::mCondVar"),
    mWaiting(false),
    mOpened(false)
  { }

  ~File()
  {
    MOZ_ASSERT(!mWaiting, "Shouldn't be destroyed while thread is waiting");
    MOZ_ASSERT(!mOpened, "OnClose() should have been called");
  }

  void
  OnOpen()
  {
    Notify(true);
  }

  void
  OnFailure()
  {
    FileDescriptorHolder::Finish();

    Notify(false);
  }

  void
  OnClose()
  {
    FileDescriptorHolder::Finish();

    MOZ_ASSERT(mOpened);
    mOpened = false;

    
    
    
    Release();
  }

private:
  void
  Notify(bool aSuccess)
  {
    MOZ_ASSERT(NS_IsMainThread());

    MutexAutoLock lock(mMutex);
    MOZ_ASSERT(mWaiting);

    mWaiting = false;
    mOpened = aSuccess;
    mCondVar.Notify();
  }

  Mutex mMutex;
  CondVar mCondVar;
  bool mWaiting;
  bool mOpened;
};




class MainProcessRunnable : public virtual FileDescriptorHolder
{
public:
  NS_DECL_NSIRUNNABLE

  
  
  
  MainProcessRunnable(nsIPrincipal* aPrincipal,
                      OpenMode aOpenMode,
                      WriteParams aWriteParams)
  : mPrincipal(aPrincipal),
    mOpenMode(aOpenMode),
    mWriteParams(aWriteParams),
    mNeedAllowNextSynchronizedOp(false),
    mPersistence(quota::PERSISTENCE_TYPE_INVALID),
    mState(eInitial)
  {
    MOZ_ASSERT(IsMainProcess());
  }

  virtual ~MainProcessRunnable()
  {
    MOZ_ASSERT(mState == eFinished);
    MOZ_ASSERT(!mNeedAllowNextSynchronizedOp);
  }

protected:
  
  
  void
  OpenForRead(unsigned aModuleIndex)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mState == eWaitingToOpenCacheFileForRead);
    MOZ_ASSERT(mOpenMode == eOpenForRead);

    mModuleIndex = aModuleIndex;
    mState = eReadyToOpenCacheFileForRead;
    DispatchToIOThread();
  }

  
  
  
  
  void
  CacheMiss()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mState == eFailedToReadMetadata ||
               mState == eWaitingToOpenCacheFileForRead);
    MOZ_ASSERT(mOpenMode == eOpenForRead);

    if (mPersistence == quota::PERSISTENCE_TYPE_TEMPORARY) {
      Fail();
      return;
    }

    
    
    MOZ_ASSERT(mPersistence == quota::PERSISTENCE_TYPE_PERSISTENT);
    FinishOnMainThread();
    mState = eInitial;
    NS_DispatchToMainThread(this);
  }

  
  
  
  void
  Close()
  {
    MOZ_ASSERT(mState == eOpened);
    mState = eClosing;
    NS_DispatchToMainThread(this);
  }

  
  
  void
  Fail()
  {
    MOZ_ASSERT(mState != eOpened &&
               mState != eClosing &&
               mState != eFailing &&
               mState != eFinished);

    mState = eFailing;
    NS_DispatchToMainThread(this);
  }

  
  virtual void
  OnOpenMetadataForRead(const Metadata& aMetadata) = 0;

  
  virtual void
  OnOpenCacheFile() = 0;

  
  
  virtual void
  OnFailure()
  {
    FinishOnMainThread();
  }

  
  
  virtual void
  OnClose()
  {
    FinishOnMainThread();
  }

private:
  nsresult
  InitOnMainThread();

  nsresult
  ReadMetadata();

  nsresult
  OpenCacheFileForWrite();

  nsresult
  OpenCacheFileForRead();

  void
  FinishOnMainThread();

  void
  DispatchToIOThread()
  {
    
    QuotaManager* qm = QuotaManager::Get();
    if (!qm) {
      Fail();
      return;
    }

    nsresult rv = qm->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      Fail();
      return;
    }
  }

  nsIPrincipal* const mPrincipal;
  const OpenMode mOpenMode;
  const WriteParams mWriteParams;

  
  bool mNeedAllowNextSynchronizedOp;
  quota::PersistenceType mPersistence;
  nsCString mGroup;
  nsCString mOrigin;
  nsCString mStorageId;

  
  nsCOMPtr<nsIFile> mDirectory;
  nsCOMPtr<nsIFile> mMetadataFile;
  Metadata mMetadata;

  
  unsigned mModuleIndex;

  enum State {
    eInitial, 
    eWaitingToOpenMetadata, 
    eReadyToReadMetadata, 
    eFailedToReadMetadata, 
    eSendingMetadataForRead, 
    eWaitingToOpenCacheFileForRead, 
    eReadyToOpenCacheFileForRead, 
    eSendingCacheFile, 
    eOpened, 
    eClosing, 
    eFailing, 
    eFinished, 
  };
  State mState;
};

nsresult
MainProcessRunnable::InitOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mState == eInitial);

  QuotaManager* qm = QuotaManager::GetOrCreate();
  NS_ENSURE_STATE(qm);

  nsresult rv = QuotaManager::GetInfoFromPrincipal(mPrincipal, &mGroup,
                                                   &mOrigin, nullptr, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  bool isApp = mPrincipal->GetAppStatus() !=
               nsIPrincipal::APP_STATUS_NOT_INSTALLED;

  if (mOpenMode == eOpenForWrite) {
    MOZ_ASSERT(mPersistence == quota::PERSISTENCE_TYPE_INVALID);
    if (mWriteParams.mInstalled) {
      
      
      
      
      
      
      MOZ_ASSERT(isApp);

      nsCOMPtr<nsIPermissionManager> pm =
        services::GetPermissionManager();
      NS_ENSURE_TRUE(pm, NS_ERROR_UNEXPECTED);

      uint32_t permission;
      rv = pm->TestPermissionFromPrincipal(mPrincipal,
                                           PERMISSION_STORAGE_UNLIMITED,
                                           &permission);
      NS_ENSURE_SUCCESS(rv, NS_ERROR_UNEXPECTED);

      
      
      mPersistence = permission == nsIPermissionManager::ALLOW_ACTION
                     ? quota::PERSISTENCE_TYPE_PERSISTENT
                     : quota::PERSISTENCE_TYPE_TEMPORARY;
    } else {
      mPersistence = quota::PERSISTENCE_TYPE_TEMPORARY;
    }
  } else {
    
    
    
    
    
    if (mPersistence == quota::PERSISTENCE_TYPE_INVALID) {
      mPersistence = isApp ? quota::PERSISTENCE_TYPE_PERSISTENT
                           : quota::PERSISTENCE_TYPE_TEMPORARY;
    } else {
      MOZ_ASSERT(isApp);
      MOZ_ASSERT(mPersistence == quota::PERSISTENCE_TYPE_PERSISTENT);
      mPersistence = quota::PERSISTENCE_TYPE_TEMPORARY;
    }
  }

  QuotaManager::GetStorageId(mPersistence, mOrigin, quota::Client::ASMJS,
                             NS_LITERAL_STRING("asmjs"), mStorageId);

  return NS_OK;
}

nsresult
MainProcessRunnable::ReadMetadata()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mState == eReadyToReadMetadata);

  QuotaManager* qm = QuotaManager::Get();
  MOZ_ASSERT(qm, "We are on the QuotaManager's IO thread");

  
  
  bool trackQuota = mPersistence == quota::PERSISTENCE_TYPE_TEMPORARY;

  nsresult rv = qm->EnsureOriginIsInitialized(mPersistence, mGroup, mOrigin,
                                              trackQuota,
                                              getter_AddRefs(mDirectory));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDirectory->Append(NS_LITERAL_STRING(ASMJSCACHE_DIRECTORY_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = mDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    rv = mDirectory->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    DebugOnly<bool> isDirectory;
    MOZ_ASSERT(NS_SUCCEEDED(mDirectory->IsDirectory(&isDirectory)));
    MOZ_ASSERT(isDirectory, "Should have caught this earlier!");
  }

  rv = mDirectory->Clone(getter_AddRefs(mMetadataFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMetadataFile->Append(NS_LITERAL_STRING(ASMJSCACHE_METADATA_FILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMetadataFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists && NS_FAILED(ReadMetadataFile(mMetadataFile, mMetadata))) {
    exists = false;
  }

  if (!exists) {
    
    if (mOpenMode == eOpenForRead) {
      return NS_ERROR_FILE_NOT_FOUND;
    }

    
    for (unsigned i = 0; i < Metadata::kNumEntries; i++) {
      Metadata::Entry& entry = mMetadata.mEntries[i];
      entry.mModuleIndex = i;
      entry.clear();
    }
  }

  return NS_OK;
}

nsresult
MainProcessRunnable::OpenCacheFileForWrite()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mState == eReadyToReadMetadata);
  MOZ_ASSERT(mOpenMode == eOpenForWrite);

  mFileSize = mWriteParams.mSize;

  
  mModuleIndex = mMetadata.mEntries[Metadata::kLastEntry].mModuleIndex;

  nsCOMPtr<nsIFile> file;
  nsresult rv = GetCacheFile(mDirectory, mModuleIndex, getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  QuotaManager* qm = QuotaManager::Get();
  MOZ_ASSERT(qm, "We are on the QuotaManager's IO thread");

  
  
  
  
  if (mPersistence == quota::PERSISTENCE_TYPE_TEMPORARY) {
    
    
    
    mQuotaObject = qm->GetQuotaObject(mPersistence, mGroup, mOrigin, file);
    NS_ENSURE_STATE(mQuotaObject);

    if (!mQuotaObject->MaybeAllocateMoreSpace(0, mWriteParams.mSize)) {
      
      
      
      
      EvictEntries(mDirectory, mGroup, mOrigin, mWriteParams.mSize, mMetadata);
      if (!mQuotaObject->MaybeAllocateMoreSpace(0, mWriteParams.mSize)) {
        return NS_ERROR_FAILURE;
      }
    }
  }

  int32_t openFlags = PR_RDWR | PR_TRUNCATE | PR_CREATE_FILE;
  rv = file->OpenNSPRFileDesc(openFlags, 0644, &mFileDesc);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PodMove(mMetadata.mEntries + 1, mMetadata.mEntries, Metadata::kLastEntry);
  Metadata::Entry& entry = mMetadata.mEntries[0];
  entry.mFastHash = mWriteParams.mFastHash;
  entry.mNumChars = mWriteParams.mNumChars;
  entry.mFullHash = mWriteParams.mFullHash;
  entry.mModuleIndex = mModuleIndex;

  rv = WriteMetadataFile(mMetadataFile, mMetadata);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
MainProcessRunnable::OpenCacheFileForRead()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mState == eReadyToOpenCacheFileForRead);
  MOZ_ASSERT(mOpenMode == eOpenForRead);

  nsCOMPtr<nsIFile> file;
  nsresult rv = GetCacheFile(mDirectory, mModuleIndex, getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  QuotaManager* qm = QuotaManager::Get();
  MOZ_ASSERT(qm, "We are on the QuotaManager's IO thread");

  if (mPersistence == quota::PERSISTENCE_TYPE_TEMPORARY) {
    
    
    
    mQuotaObject = qm->GetQuotaObject(mPersistence, mGroup, mOrigin, file);
    NS_ENSURE_STATE(mQuotaObject);
  }

  rv = file->GetFileSize(&mFileSize);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t openFlags = PR_RDONLY | nsIFile::OS_READAHEAD;
  rv = file->OpenNSPRFileDesc(openFlags, 0644, &mFileDesc);
  NS_ENSURE_SUCCESS(rv, rv);

  
  unsigned lruIndex = 0;
  while (mMetadata.mEntries[lruIndex].mModuleIndex != mModuleIndex) {
    if (++lruIndex == Metadata::kNumEntries) {
      return NS_ERROR_UNEXPECTED;
    }
  }
  Metadata::Entry entry = mMetadata.mEntries[lruIndex];
  PodMove(mMetadata.mEntries + 1, mMetadata.mEntries, lruIndex);
  mMetadata.mEntries[0] = entry;

  rv = WriteMetadataFile(mMetadataFile, mMetadata);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
MainProcessRunnable::FinishOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  FileDescriptorHolder::Finish();

  if (mNeedAllowNextSynchronizedOp) {
    mNeedAllowNextSynchronizedOp = false;
    QuotaManager* qm = QuotaManager::Get();
    if (qm) {
      qm->AllowNextSynchronizedOp(OriginOrPatternString::FromOrigin(mOrigin),
                                  Nullable<PersistenceType>(mPersistence),
                                  mStorageId);
    }
  }
}

NS_IMETHODIMP
MainProcessRunnable::Run()
{
  nsresult rv;

  
  
  switch (mState) {
    case eInitial: {
      MOZ_ASSERT(NS_IsMainThread());

      rv = InitOnMainThread();
      if (NS_FAILED(rv)) {
        Fail();
        return NS_OK;
      }

      mState = eWaitingToOpenMetadata;
      rv = QuotaManager::Get()->WaitForOpenAllowed(
                                     OriginOrPatternString::FromOrigin(mOrigin),
                                     Nullable<PersistenceType>(mPersistence),
                                     mStorageId, this);
      if (NS_FAILED(rv)) {
        Fail();
        return NS_OK;
      }

      mNeedAllowNextSynchronizedOp = true;
      return NS_OK;
    }

    case eWaitingToOpenMetadata: {
      MOZ_ASSERT(NS_IsMainThread());

      mState = eReadyToReadMetadata;
      DispatchToIOThread();
      return NS_OK;
    }

    case eReadyToReadMetadata: {
      AssertIsOnIOThread();

      rv = ReadMetadata();
      if (NS_FAILED(rv)) {
        mState = eFailedToReadMetadata;
        NS_DispatchToMainThread(this);
        return NS_OK;
      }

      if (mOpenMode == eOpenForRead) {
        mState = eSendingMetadataForRead;
        NS_DispatchToMainThread(this);
        return NS_OK;
      }

      rv = OpenCacheFileForWrite();
      if (NS_FAILED(rv)) {
        Fail();
        return NS_OK;
      }

      mState = eSendingCacheFile;
      NS_DispatchToMainThread(this);
      return NS_OK;
    }

    case eFailedToReadMetadata: {
      MOZ_ASSERT(NS_IsMainThread());

      CacheMiss();
      return NS_OK;
    }

    case eSendingMetadataForRead: {
      MOZ_ASSERT(NS_IsMainThread());
      MOZ_ASSERT(mOpenMode == eOpenForRead);

      mState = eWaitingToOpenCacheFileForRead;
      OnOpenMetadataForRead(mMetadata);
      return NS_OK;
    }

    case eReadyToOpenCacheFileForRead: {
      AssertIsOnIOThread();
      MOZ_ASSERT(mOpenMode == eOpenForRead);

      rv = OpenCacheFileForRead();
      if (NS_FAILED(rv)) {
        Fail();
        return NS_OK;
      }

      mState = eSendingCacheFile;
      NS_DispatchToMainThread(this);
      return NS_OK;
    }

    case eSendingCacheFile: {
      MOZ_ASSERT(NS_IsMainThread());

      mState = eOpened;
      OnOpenCacheFile();
      return NS_OK;
    }

    case eFailing: {
      MOZ_ASSERT(NS_IsMainThread());

      mState = eFinished;
      OnFailure();
      return NS_OK;
    }

    case eClosing: {
      MOZ_ASSERT(NS_IsMainThread());

      mState = eFinished;
      OnClose();
      return NS_OK;
    }

    case eWaitingToOpenCacheFileForRead:
    case eOpened:
    case eFinished: {
      MOZ_MAKE_COMPILER_ASSUME_IS_UNREACHABLE("Shouldn't Run() in this state");
    }
  }

  MOZ_MAKE_COMPILER_ASSUME_IS_UNREACHABLE("Corrupt state");
  return NS_OK;
}

bool
FindHashMatch(const Metadata& aMetadata, const ReadParams& aReadParams,
              unsigned* aModuleIndex)
{
  
  
  
  
  
  
  
  
  
  uint32_t numChars = aReadParams.mLimit - aReadParams.mBegin;
  MOZ_ASSERT(numChars > sNumFastHashChars);
  uint32_t fastHash = HashString(aReadParams.mBegin, sNumFastHashChars);

  for (unsigned i = 0; i < Metadata::kNumEntries ; i++) {
    
    
    Metadata::Entry entry = aMetadata.mEntries[i];
    if (entry.mFastHash != fastHash) {
      continue;
    }

    
    
    
    
    if (numChars < entry.mNumChars) {
      continue;
    }
    uint32_t fullHash = HashString(aReadParams.mBegin, entry.mNumChars);
    if (entry.mFullHash != fullHash) {
      continue;
    }

    *aModuleIndex = entry.mModuleIndex;
    return true;
  }

  return false;
}


class SingleProcessRunnable MOZ_FINAL : public File,
                                        private MainProcessRunnable
{
public:
  
  
  
  
  
  SingleProcessRunnable(nsIPrincipal* aPrincipal,
                        OpenMode aOpenMode,
                        WriteParams aWriteParams,
                        ReadParams aReadParams)
  : MainProcessRunnable(aPrincipal, aOpenMode, aWriteParams),
    mReadParams(aReadParams)
  {
    MOZ_ASSERT(IsMainProcess());
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_COUNT_CTOR(SingleProcessRunnable);
  }

protected:
  ~SingleProcessRunnable()
  {
    MOZ_COUNT_DTOR(SingleProcessRunnable);
  }

private:
  void
  OnOpenMetadataForRead(const Metadata& aMetadata) MOZ_OVERRIDE
  {
    uint32_t moduleIndex;
    if (FindHashMatch(aMetadata, mReadParams, &moduleIndex)) {
      MainProcessRunnable::OpenForRead(moduleIndex);
    } else {
      MainProcessRunnable::CacheMiss();
    }
  }

  void
  OnOpenCacheFile() MOZ_OVERRIDE
  {
    File::OnOpen();
  }

  void
  Close() MOZ_OVERRIDE MOZ_FINAL
  {
    MainProcessRunnable::Close();
  }

  void
  OnFailure() MOZ_OVERRIDE
  {
    MainProcessRunnable::OnFailure();
    File::OnFailure();
  }

  void
  OnClose() MOZ_OVERRIDE MOZ_FINAL
  {
    MainProcessRunnable::OnClose();
    File::OnClose();
  }

  
  NS_IMETHODIMP
  Run() MOZ_OVERRIDE
  {
    return MainProcessRunnable::Run();
  }

  ReadParams mReadParams;
};




class ParentProcessRunnable MOZ_FINAL : public PAsmJSCacheEntryParent,
                                        public MainProcessRunnable
{
public:
  
  
  
  ParentProcessRunnable(nsIPrincipal* aPrincipal,
                        OpenMode aOpenMode,
                        WriteParams aWriteParams)
  : MainProcessRunnable(aPrincipal, aOpenMode, aWriteParams),
    mPrincipalHolder(aPrincipal),
    mActorDestroyed(false),
    mOpened(false),
    mFinished(false)
  {
    MOZ_ASSERT(IsMainProcess());
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_COUNT_CTOR(ParentProcessRunnable);
  }

private:
  ~ParentProcessRunnable()
  {
    MOZ_ASSERT(!mPrincipalHolder, "Should have already been released");
    MOZ_ASSERT(mActorDestroyed);
    MOZ_ASSERT(mFinished);
    MOZ_COUNT_DTOR(ParentProcessRunnable);
  }

  bool
  Recv__delete__() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!mFinished);
    mFinished = true;

    if (mOpened) {
      MainProcessRunnable::Close();
    } else {
      MainProcessRunnable::Fail();
    }

    return true;
  }

  void
  ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE
  {
    MOZ_ASSERT(!mActorDestroyed);
    mActorDestroyed = true;

    
    

    if (mFinished) {
      return;
    }

    mFinished = true;

    if (mOpened) {
      MainProcessRunnable::Close();
    } else {
      MainProcessRunnable::Fail();
    }
  }

  void
  OnOpenMetadataForRead(const Metadata& aMetadata) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (!SendOnOpenMetadataForRead(aMetadata)) {
      unused << Send__delete__(this);
    }
  }

  bool
  RecvSelectCacheFileToRead(const uint32_t& aModuleIndex) MOZ_OVERRIDE
  {
    MainProcessRunnable::OpenForRead(aModuleIndex);
    return true;
  }

  bool
  RecvCacheMiss() MOZ_OVERRIDE
  {
    MainProcessRunnable::CacheMiss();
    return true;
  }

  void
  OnOpenCacheFile() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    MOZ_ASSERT(!mOpened);
    mOpened = true;

    FileDescriptor::PlatformHandleType handle =
      FileDescriptor::PlatformHandleType(PR_FileDesc2NativeHandle(mFileDesc));
    if (!SendOnOpenCacheFile(mFileSize, FileDescriptor(handle))) {
      unused << Send__delete__(this);
    }
  }

  void
  OnClose() MOZ_OVERRIDE MOZ_FINAL
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mOpened);

    mFinished = true;

    MainProcessRunnable::OnClose();

    MOZ_ASSERT(mActorDestroyed);

    mPrincipalHolder = nullptr;
  }

  void
  OnFailure() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!mOpened);

    mFinished = true;

    MainProcessRunnable::OnFailure();

    if (!mActorDestroyed) {
      unused << Send__delete__(this);
    }

    mPrincipalHolder = nullptr;
  }

  nsCOMPtr<nsIPrincipal> mPrincipalHolder;
  bool mActorDestroyed;
  bool mOpened;
  bool mFinished;
};

} 

PAsmJSCacheEntryParent*
AllocEntryParent(OpenMode aOpenMode,
                 WriteParams aWriteParams,
                 nsIPrincipal* aPrincipal)
{
  nsRefPtr<ParentProcessRunnable> runnable =
    new ParentProcessRunnable(aPrincipal, aOpenMode, aWriteParams);

  nsresult rv = NS_DispatchToMainThread(runnable);
  NS_ENSURE_SUCCESS(rv, nullptr);

  
  return runnable.forget().take();
}

void
DeallocEntryParent(PAsmJSCacheEntryParent* aActor)
{
  
  nsRefPtr<ParentProcessRunnable> op =
    dont_AddRef(static_cast<ParentProcessRunnable*>(aActor));
}

namespace {

class ChildProcessRunnable MOZ_FINAL : public File,
                                       public PAsmJSCacheEntryChild
{
public:
  NS_DECL_NSIRUNNABLE

  
  
  
  
  
  ChildProcessRunnable(nsIPrincipal* aPrincipal,
                       OpenMode aOpenMode,
                       WriteParams aWriteParams,
                       ReadParams aReadParams)
  : mPrincipal(aPrincipal),
    mOpenMode(aOpenMode),
    mWriteParams(aWriteParams),
    mReadParams(aReadParams),
    mActorDestroyed(false),
    mState(eInitial)
  {
    MOZ_ASSERT(!IsMainProcess());
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_COUNT_CTOR(ChildProcessRunnable);
  }

protected:
  ~ChildProcessRunnable()
  {
    MOZ_ASSERT(mState == eFinished);
    MOZ_ASSERT(mActorDestroyed);
    MOZ_COUNT_DTOR(ChildProcessRunnable);
  }

private:
  bool
  RecvOnOpenMetadataForRead(const Metadata& aMetadata) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mState == eOpening);

    uint32_t moduleIndex;
    if (FindHashMatch(aMetadata, mReadParams, &moduleIndex)) {
      return SendSelectCacheFileToRead(moduleIndex);
    }

    return SendCacheMiss();
  }

  bool
  RecvOnOpenCacheFile(const int64_t& aFileSize,
                      const FileDescriptor& aFileDesc) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mState == eOpening);

    mFileSize = aFileSize;

    mFileDesc = PR_ImportFile(PROsfd(aFileDesc.PlatformHandle()));
    if (!mFileDesc) {
      return false;
    }

    mState = eOpened;
    File::OnOpen();
    return true;
  }

  bool
  Recv__delete__() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mState == eOpening);

    Fail();
    return true;
  }

  void
  ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());
    mActorDestroyed = true;
  }

  void
  Close() MOZ_OVERRIDE MOZ_FINAL
  {
    MOZ_ASSERT(mState == eOpened);

    mState = eClosing;
    NS_DispatchToMainThread(this);
  }

private:
  void
  Fail()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mState == eInitial || mState == eOpening);

    mState = eFinished;
    File::OnFailure();
  }

  nsIPrincipal* const mPrincipal;
  const OpenMode mOpenMode;
  WriteParams mWriteParams;
  ReadParams mReadParams;
  bool mActorDestroyed;

  enum State {
    eInitial, 
    eOpening, 
    eOpened, 
    eClosing, 
    eFinished 
  };
  State mState;
};

NS_IMETHODIMP
ChildProcessRunnable::Run()
{
  switch (mState) {
    case eInitial: {
      MOZ_ASSERT(NS_IsMainThread());

      
      
      AddRef();

      if (!ContentChild::GetSingleton()->SendPAsmJSCacheEntryConstructor(
        this, mOpenMode, mWriteParams, IPC::Principal(mPrincipal)))
      {
        
        
        
        
        Release();

        Fail();
        return NS_OK;
      }

      mState = eOpening;
      return NS_OK;
    }

    case eClosing: {
      MOZ_ASSERT(NS_IsMainThread());

      
      
      
      File::OnClose();

      if (!mActorDestroyed) {
        unused << Send__delete__(this);
      }

      mState = eFinished;
      return NS_OK;
    }

    case eOpening:
    case eOpened:
    case eFinished: {
      MOZ_MAKE_COMPILER_ASSUME_IS_UNREACHABLE("Shouldn't Run() in this state");
    }
  }

  MOZ_MAKE_COMPILER_ASSUME_IS_UNREACHABLE("Corrupt state");
  return NS_OK;
}

} 

void
DeallocEntryChild(PAsmJSCacheEntryChild* aActor)
{
  
  static_cast<ChildProcessRunnable*>(aActor)->Release();
}

namespace {

bool
OpenFile(nsIPrincipal* aPrincipal,
         OpenMode aOpenMode,
         WriteParams aWriteParams,
         ReadParams aReadParams,
         File::AutoClose* aFile)
{
  MOZ_ASSERT_IF(aOpenMode == eOpenForRead, aWriteParams.mSize == 0);
  MOZ_ASSERT_IF(aOpenMode == eOpenForWrite, aReadParams.mBegin == nullptr);

  
  
  
  
  
  
  
  
  
  
  
  
  if (NS_IsMainThread()) {
    return false;
  }

  
  
  
  nsRefPtr<File> file;
  if (IsMainProcess()) {
    file = new SingleProcessRunnable(aPrincipal, aOpenMode, aWriteParams,
                                     aReadParams);
  } else {
    file = new ChildProcessRunnable(aPrincipal, aOpenMode, aWriteParams,
                                    aReadParams);
  }

  if (!file->BlockUntilOpen(aFile)) {
    return false;
  }

  return file->MapMemory(aOpenMode);
}

} 

typedef uint32_t AsmJSCookieType;
static const uint32_t sAsmJSCookie = 0x600d600d;

bool
OpenEntryForRead(nsIPrincipal* aPrincipal,
                 const char16_t* aBegin,
                 const char16_t* aLimit,
                 size_t* aSize,
                 const uint8_t** aMemory,
                 intptr_t* aFile)
{
  if (size_t(aLimit - aBegin) < sMinCachedModuleLength) {
    return false;
  }

  ReadParams readParams;
  readParams.mBegin = aBegin;
  readParams.mLimit = aLimit;

  File::AutoClose file;
  WriteParams notAWrite;
  if (!OpenFile(aPrincipal, eOpenForRead, notAWrite, readParams, &file)) {
    return false;
  }

  
  
  
  
  
  
  
  
  
  
  
  if (file->FileSize() < sizeof(AsmJSCookieType) ||
      *(AsmJSCookieType*)file->MappedMemory() != sAsmJSCookie) {
    return false;
  }

  *aSize = file->FileSize() - sizeof(AsmJSCookieType);
  *aMemory = (uint8_t*) file->MappedMemory() + sizeof(AsmJSCookieType);

  
  
  file.Forget(reinterpret_cast<File**>(aFile));
  return true;
}

void
CloseEntryForRead(size_t aSize,
                  const uint8_t* aMemory,
                  intptr_t aFile)
{
  File::AutoClose file(reinterpret_cast<File*>(aFile));

  MOZ_ASSERT(aSize + sizeof(AsmJSCookieType) == file->FileSize());
  MOZ_ASSERT(aMemory - sizeof(AsmJSCookieType) == file->MappedMemory());
}

bool
OpenEntryForWrite(nsIPrincipal* aPrincipal,
                  bool aInstalled,
                  const char16_t* aBegin,
                  const char16_t* aEnd,
                  size_t aSize,
                  uint8_t** aMemory,
                  intptr_t* aFile)
{
  if (size_t(aEnd - aBegin) < sMinCachedModuleLength) {
    return false;
  }

  
  aSize += sizeof(AsmJSCookieType);

  static_assert(sNumFastHashChars < sMinCachedModuleLength, "HashString safe");

  WriteParams writeParams;
  writeParams.mInstalled = aInstalled;
  writeParams.mSize = aSize;
  writeParams.mFastHash = HashString(aBegin, sNumFastHashChars);
  writeParams.mNumChars = aEnd - aBegin;
  writeParams.mFullHash = HashString(aBegin, writeParams.mNumChars);

  File::AutoClose file;
  ReadParams notARead;
  if (!OpenFile(aPrincipal, eOpenForWrite, writeParams, notARead, &file)) {
    return false;
  }

  
  
  
  *aMemory = (uint8_t*) file->MappedMemory() + sizeof(AsmJSCookieType);

  
  
  file.Forget(reinterpret_cast<File**>(aFile));
  return true;
}

void
CloseEntryForWrite(size_t aSize,
                   uint8_t* aMemory,
                   intptr_t aFile)
{
  File::AutoClose file(reinterpret_cast<File*>(aFile));

  MOZ_ASSERT(aSize + sizeof(AsmJSCookieType) == file->FileSize());
  MOZ_ASSERT(aMemory - sizeof(AsmJSCookieType) == file->MappedMemory());

  
  if (PR_SyncMemMap(file->FileDesc(),
                    file->MappedMemory(),
                    file->FileSize()) == PR_SUCCESS) {
    *(AsmJSCookieType*)file->MappedMemory() = sAsmJSCookie;
  }
}

bool
GetBuildId(JS::BuildIdCharVector* aBuildID)
{
  nsCOMPtr<nsIXULAppInfo> info = do_GetService("@mozilla.org/xre/app-info;1");
  if (!info) {
    return false;
  }

  nsCString buildID;
  nsresult rv = info->GetPlatformBuildID(buildID);
  NS_ENSURE_SUCCESS(rv, false);

  if (!aBuildID->resize(buildID.Length())) {
    return false;
  }

  for (size_t i = 0; i < buildID.Length(); i++) {
    (*aBuildID)[i] = buildID[i];
  }

  return true;
}

class Client : public quota::Client
{
  ~Client() {}

public:
  NS_IMETHOD_(MozExternalRefCountType)
  AddRef() MOZ_OVERRIDE;

  NS_IMETHOD_(MozExternalRefCountType)
  Release() MOZ_OVERRIDE;

  virtual Type
  GetType() MOZ_OVERRIDE
  {
    return ASMJS;
  }

  virtual nsresult
  InitOrigin(PersistenceType aPersistenceType,
             const nsACString& aGroup,
             const nsACString& aOrigin,
             UsageInfo* aUsageInfo) MOZ_OVERRIDE
  {
    if (!aUsageInfo) {
      return NS_OK;
    }
    return GetUsageForOrigin(aPersistenceType, aGroup, aOrigin, aUsageInfo);
  }

  virtual nsresult
  GetUsageForOrigin(PersistenceType aPersistenceType,
                    const nsACString& aGroup,
                    const nsACString& aOrigin,
                    UsageInfo* aUsageInfo) MOZ_OVERRIDE
  {
    QuotaManager* qm = QuotaManager::Get();
    MOZ_ASSERT(qm, "We were being called by the QuotaManager");

    nsCOMPtr<nsIFile> directory;
    nsresult rv = qm->GetDirectoryForOrigin(aPersistenceType, aOrigin,
                                            getter_AddRefs(directory));
    NS_ENSURE_SUCCESS(rv, rv);
    MOZ_ASSERT(directory, "We're here because the origin directory exists");

    rv = directory->Append(NS_LITERAL_STRING(ASMJSCACHE_DIRECTORY_NAME));
    NS_ENSURE_SUCCESS(rv, rv);

    DebugOnly<bool> exists;
    MOZ_ASSERT(NS_SUCCEEDED(directory->Exists(&exists)) && exists);

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) &&
           hasMore && !aUsageInfo->Canceled()) {
      nsCOMPtr<nsISupports> entry;
      rv = entries->GetNext(getter_AddRefs(entry));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
      NS_ENSURE_TRUE(file, NS_NOINTERFACE);

      int64_t fileSize;
      rv = file->GetFileSize(&fileSize);
      NS_ENSURE_SUCCESS(rv, rv);

      MOZ_ASSERT(fileSize >= 0, "Negative size?!");

      
      
      aUsageInfo->AppendToDatabaseUsage(uint64_t(fileSize));
    }
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  virtual void
  OnOriginClearCompleted(PersistenceType aPersistenceType,
                         const OriginOrPatternString& aOriginOrPattern)
                         MOZ_OVERRIDE
  { }

  virtual void
  ReleaseIOThreadObjects() MOZ_OVERRIDE
  { }

  virtual bool
  IsFileServiceUtilized() MOZ_OVERRIDE
  {
    return false;
  }

  virtual bool
  IsTransactionServiceActivated() MOZ_OVERRIDE
  {
    return false;
  }

  virtual void
  WaitForStoragesToComplete(nsTArray<nsIOfflineStorage*>& aStorages,
                            nsIRunnable* aCallback) MOZ_OVERRIDE
  {
    MOZ_ASSERT_UNREACHABLE("There are no storages");
  }

  virtual void
  AbortTransactionsForStorage(nsIOfflineStorage* aStorage) MOZ_OVERRIDE
  {
    MOZ_ASSERT_UNREACHABLE("There are no storages");
  }

  virtual bool
  HasTransactionsForStorage(nsIOfflineStorage* aStorage) MOZ_OVERRIDE
  {
    return false;
  }

  virtual void
  ShutdownTransactionService() MOZ_OVERRIDE
  { }

private:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
};

NS_IMPL_ADDREF(asmjscache::Client)
NS_IMPL_RELEASE(asmjscache::Client)

quota::Client*
CreateClient()
{
  return new Client();
}

} 
} 
} 

namespace IPC {

using mozilla::dom::asmjscache::Metadata;
using mozilla::dom::asmjscache::WriteParams;

void
ParamTraits<Metadata>::Write(Message* aMsg, const paramType& aParam)
{
  for (unsigned i = 0; i < Metadata::kNumEntries; i++) {
    const Metadata::Entry& entry = aParam.mEntries[i];
    WriteParam(aMsg, entry.mFastHash);
    WriteParam(aMsg, entry.mNumChars);
    WriteParam(aMsg, entry.mFullHash);
    WriteParam(aMsg, entry.mModuleIndex);
  }
}

bool
ParamTraits<Metadata>::Read(const Message* aMsg, void** aIter,
                            paramType* aResult)
{
  for (unsigned i = 0; i < Metadata::kNumEntries; i++) {
    Metadata::Entry& entry = aResult->mEntries[i];
    if (!ReadParam(aMsg, aIter, &entry.mFastHash) ||
        !ReadParam(aMsg, aIter, &entry.mNumChars) ||
        !ReadParam(aMsg, aIter, &entry.mFullHash) ||
        !ReadParam(aMsg, aIter, &entry.mModuleIndex))
    {
      return false;
    }
  }
  return true;
}

void
ParamTraits<Metadata>::Log(const paramType& aParam, std::wstring* aLog)
{
  for (unsigned i = 0; i < Metadata::kNumEntries; i++) {
    const Metadata::Entry& entry = aParam.mEntries[i];
    LogParam(entry.mFastHash, aLog);
    LogParam(entry.mNumChars, aLog);
    LogParam(entry.mFullHash, aLog);
    LogParam(entry.mModuleIndex, aLog);
  }
}

void
ParamTraits<WriteParams>::Write(Message* aMsg, const paramType& aParam)
{
  WriteParam(aMsg, aParam.mSize);
  WriteParam(aMsg, aParam.mFastHash);
  WriteParam(aMsg, aParam.mNumChars);
  WriteParam(aMsg, aParam.mFullHash);
  WriteParam(aMsg, aParam.mInstalled);
}

bool
ParamTraits<WriteParams>::Read(const Message* aMsg, void** aIter,
                               paramType* aResult)
{
  return ReadParam(aMsg, aIter, &aResult->mSize) &&
         ReadParam(aMsg, aIter, &aResult->mFastHash) &&
         ReadParam(aMsg, aIter, &aResult->mNumChars) &&
         ReadParam(aMsg, aIter, &aResult->mFullHash) &&
         ReadParam(aMsg, aIter, &aResult->mInstalled);
}

void
ParamTraits<WriteParams>::Log(const paramType& aParam, std::wstring* aLog)
{
  LogParam(aParam.mSize, aLog);
  LogParam(aParam.mFastHash, aLog);
  LogParam(aParam.mNumChars, aLog);
  LogParam(aParam.mFullHash, aLog);
  LogParam(aParam.mInstalled, aLog);
}

} 
