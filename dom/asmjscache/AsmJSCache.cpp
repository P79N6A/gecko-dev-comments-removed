





#include "AsmJSCache.h"

#include <stdio.h>

#include "js/RootingAPI.h"
#include "jsfriendapi.h"
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
#include "mozilla/unused.h"
#include "nsIAtom.h"
#include "nsIFile.h"
#include "nsIPrincipal.h"
#include "nsIRunnable.h"
#include "nsIThread.h"
#include "nsIXULAppInfo.h"
#include "nsJSPrincipals.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "prio.h"
#include "private/pprio.h"

#define ASMJSCACHE_FILE_NAME "module"

using mozilla::dom::quota::AssertIsOnIOThread;
using mozilla::dom::quota::OriginOrPatternString;
using mozilla::dom::quota::PersistenceType;
using mozilla::dom::quota::QuotaManager;
using mozilla::dom::quota::QuotaObject;
using mozilla::dom::quota::UsageInfo;
using mozilla::unused;

namespace mozilla {
namespace dom {
namespace asmjscache {

namespace {

bool
IsMainProcess()
{
  return XRE_GetProcessType() == GeckoProcessType_Default;
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
  BlockUntilOpen(AutoClose *aCloser)
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
                      size_t aSizeToWrite)
  : mPrincipal(aPrincipal),
    mOpenMode(aOpenMode),
    mSizeToWrite(aSizeToWrite),
    mNeedAllowNextSynchronizedOp(false),
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
  Close()
  {
    MOZ_ASSERT(mState == eOpened);
    mState = eClosing;
    NS_DispatchToMainThread(this);
  }

  
  
  void
  Fail()
  {
    MOZ_ASSERT(mState == eInitial || mState == eWaitingToOpen ||
               mState == eReadyToOpen || mState == eNotifying);

    mState = eFailing;
    NS_DispatchToMainThread(this);
  }

  
  virtual void
  OnOpen() = 0;

  
  
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
  OpenFileOnIOThread();

  void
  FinishOnMainThread();

  nsIPrincipal* const mPrincipal;
  const OpenMode mOpenMode;
  const size_t mSizeToWrite;

  
  bool mNeedAllowNextSynchronizedOp;
  nsCString mGroup;
  nsCString mOrigin;
  nsCString mStorageId;

  enum State {
    eInitial, 
    eWaitingToOpen, 
    eReadyToOpen, 
    eNotifying, 
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

  QuotaManager::GetStorageId(quota::PERSISTENCE_TYPE_TEMPORARY,
                             mOrigin, quota::Client::ASMJS,
                             NS_LITERAL_STRING(ASMJSCACHE_FILE_NAME),
                             mStorageId);

  return NS_OK;
}

nsresult
MainProcessRunnable::OpenFileOnIOThread()
{
  AssertIsOnIOThread();
  MOZ_ASSERT(mState == eReadyToOpen);

  QuotaManager* qm = QuotaManager::Get();
  MOZ_ASSERT(qm, "We are on the QuotaManager's IO thread");

  nsCOMPtr<nsIFile> path;
  nsresult rv = qm->EnsureOriginIsInitialized(quota::PERSISTENCE_TYPE_TEMPORARY,
                                              mGroup, mOrigin, true,
                                              getter_AddRefs(path));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = path->Append(NS_LITERAL_STRING(ASMJSCACHE_DIRECTORY_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = path->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    rv = path->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    DebugOnly<bool> isDirectory;
    MOZ_ASSERT(NS_SUCCEEDED(path->IsDirectory(&isDirectory)));
    MOZ_ASSERT(isDirectory, "Should have caught this earlier!");
  }

  rv = path->Append(NS_LITERAL_STRING(ASMJSCACHE_FILE_NAME));
  NS_ENSURE_SUCCESS(rv, rv);

  mQuotaObject = qm->GetQuotaObject(quota::PERSISTENCE_TYPE_TEMPORARY,
                                    mGroup, mOrigin, path);
  NS_ENSURE_STATE(mQuotaObject);

  PRIntn openFlags;
  if (mOpenMode == eOpenForRead) {
    rv = path->GetFileSize(&mFileSize);
    if (NS_FAILED(rv)) {
      return rv;
    }

    openFlags = PR_RDONLY | nsIFile::OS_READAHEAD;
  } else {
    if (!mQuotaObject->MaybeAllocateMoreSpace(0, mSizeToWrite)) {
      return NS_ERROR_FAILURE;
    }

    mFileSize = mSizeToWrite;

    MOZ_ASSERT(mOpenMode == eOpenForWrite);
    openFlags = PR_RDWR | PR_TRUNCATE | PR_CREATE_FILE;
  }

  rv = path->OpenNSPRFileDesc(openFlags, 0644, &mFileDesc);
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
                                  Nullable<PersistenceType>(), mStorageId);
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

      mState = eWaitingToOpen;
      rv = QuotaManager::Get()->WaitForOpenAllowed(
                                     OriginOrPatternString::FromOrigin(mOrigin),
                                     Nullable<PersistenceType>(), mStorageId,
                                     this);
      if (NS_FAILED(rv)) {
        Fail();
        return NS_OK;
      }

      mNeedAllowNextSynchronizedOp = true;
      return NS_OK;
    }

    case eWaitingToOpen: {
      MOZ_ASSERT(NS_IsMainThread());

      mState = eReadyToOpen;

      QuotaManager* qm = QuotaManager::Get();
      if (!qm) {
        Fail();
        return NS_OK;
      }

      rv = qm->IOThread()->Dispatch(this, NS_DISPATCH_NORMAL);
      if (NS_FAILED(rv)) {
        Fail();
        return NS_OK;
      }

      return NS_OK;
    }

    case eReadyToOpen: {
      AssertIsOnIOThread();

      rv = OpenFileOnIOThread();
      if (NS_FAILED(rv)) {
        Fail();
        return NS_OK;
      }

      mState = eNotifying;
      NS_DispatchToMainThread(this);
      return NS_OK;
    }

    case eNotifying: {
      MOZ_ASSERT(NS_IsMainThread());

      mState = eOpened;
      OnOpen();
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

    case eOpened:
    case eFinished: {
      MOZ_ASSUME_UNREACHABLE("Shouldn't Run() in this state");
    }
  }

  MOZ_ASSUME_UNREACHABLE("Corrupt state");
  return NS_OK;
}


class SingleProcessRunnable MOZ_FINAL : public File,
                                        private MainProcessRunnable
{
public:
  
  
  
  
  
  SingleProcessRunnable(nsIPrincipal* aPrincipal,
                        OpenMode aOpenMode,
                        size_t aSizeToWrite)
  : MainProcessRunnable(aPrincipal, aOpenMode, aSizeToWrite)
  {
    MOZ_ASSERT(IsMainProcess());
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_COUNT_CTOR(SingleProcessRunnable);
  }

  ~SingleProcessRunnable()
  {
    MOZ_COUNT_DTOR(SingleProcessRunnable);
  }

private:
  void
  Close() MOZ_OVERRIDE MOZ_FINAL
  {
    MainProcessRunnable::Close();
  }

  void
  OnOpen() MOZ_OVERRIDE
  {
    File::OnOpen();
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
};




class ParentProcessRunnable MOZ_FINAL : public PAsmJSCacheEntryParent,
                                        public MainProcessRunnable
{
public:
  
  
  
  ParentProcessRunnable(nsIPrincipal* aPrincipal,
                        OpenMode aOpenMode,
                        size_t aSizeToWrite)
  : MainProcessRunnable(aPrincipal, aOpenMode, aSizeToWrite),
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
    MOZ_ASSERT(mOpened);

    MOZ_ASSERT(!mFinished);
    mFinished = true;

    MainProcessRunnable::Close();
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
  OnOpen() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    MOZ_ASSERT(!mOpened);
    mOpened = true;

    FileDescriptor::PlatformHandleType handle =
      FileDescriptor::PlatformHandleType(PR_FileDesc2NativeHandle(mFileDesc));
    if (!SendOnOpen(mFileSize, handle)) {
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
                 uint32_t aSizeToWrite,
                 nsIPrincipal* aPrincipal)
{
  ParentProcessRunnable* runnable =
    new ParentProcessRunnable(aPrincipal, aOpenMode, aSizeToWrite);

  
  runnable->AddRef();

  nsresult rv = NS_DispatchToMainThread(runnable);
  NS_ENSURE_SUCCESS(rv, nullptr);

  return runnable;
}

void
DeallocEntryParent(PAsmJSCacheEntryParent* aActor)
{
  
  static_cast<ParentProcessRunnable*>(aActor)->Release();
}

namespace {

class ChildProcessRunnable MOZ_FINAL : public File,
                                       public PAsmJSCacheEntryChild
{
public:
  NS_DECL_NSIRUNNABLE

  
  
  
  
  
  ChildProcessRunnable(nsIPrincipal* aPrincipal,
                       OpenMode aOpenMode,
                       size_t aSizeToWrite)
  : mPrincipal(aPrincipal),
    mOpenMode(aOpenMode),
    mSizeToWrite(aSizeToWrite),
    mActorDestroyed(false),
    mState(eInitial)
  {
    MOZ_ASSERT(!IsMainProcess());
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_COUNT_CTOR(ChildProcessRunnable);
  }

  ~ChildProcessRunnable()
  {
    MOZ_ASSERT(mState == eFinished);
    MOZ_ASSERT(mActorDestroyed);
    MOZ_COUNT_DTOR(ChildProcessRunnable);
  }

private:
  bool
  RecvOnOpen(const int64_t& aFileSize, const FileDescriptor& aFileDesc)
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

    mState = eFinished;
    File::OnFailure();
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
  nsIPrincipal* const mPrincipal;
  const OpenMode mOpenMode;
  size_t mSizeToWrite;
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
        this, mOpenMode, mSizeToWrite, IPC::Principal(mPrincipal)))
      {
        
        
        
        
        Release();

        mState = eFinished;
        File::OnFailure();
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
      MOZ_ASSUME_UNREACHABLE("Shouldn't Run() in this state");
    }
  }

  MOZ_ASSUME_UNREACHABLE("Corrupt state");
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
         size_t aSizeToWrite,
         File::AutoClose* aFile)
{
  MOZ_ASSERT_IF(aOpenMode == eOpenForRead, aSizeToWrite == 0);

  
  
  
  
  
  
  
  
  
  
  
  
  if (NS_IsMainThread()) {
    return false;
  }

  
  
  
  nsRefPtr<File> file;
  if (IsMainProcess()) {
    file = new SingleProcessRunnable(aPrincipal, aOpenMode, aSizeToWrite);
  } else {
    file = new ChildProcessRunnable(aPrincipal, aOpenMode, aSizeToWrite);
  }

  if (!file->BlockUntilOpen(aFile)) {
    return false;
  }

  return file->MapMemory(aOpenMode);
}

} 

typedef uint32_t AsmJSCookieType;
static const uint32_t sAsmJSCookie = 0x600d600d;



static const size_t sMinCachedModuleLength = 10000;

bool
OpenEntryForRead(nsIPrincipal* aPrincipal,
                 const jschar* aBegin,
                 const jschar* aLimit,
                 size_t* aSize,
                 const uint8_t** aMemory,
                 intptr_t* aFile)
{
  if (size_t(aLimit - aBegin) < sMinCachedModuleLength) {
    return false;
  }

  File::AutoClose file;
  if (!OpenFile(aPrincipal, eOpenForRead, 0, &file)) {
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
CloseEntryForRead(JS::Handle<JSObject*> global,
                  size_t aSize,
                  const uint8_t* aMemory,
                  intptr_t aFile)
{
  File::AutoClose file(reinterpret_cast<File*>(aFile));

  MOZ_ASSERT(aSize + sizeof(AsmJSCookieType) == file->FileSize());
  MOZ_ASSERT(aMemory - sizeof(AsmJSCookieType) == file->MappedMemory());
}

bool
OpenEntryForWrite(nsIPrincipal* aPrincipal,
                  const jschar* aBegin,
                  const jschar* aEnd,
                  size_t aSize,
                  uint8_t** aMemory,
                  intptr_t* aFile)
{
  if (size_t(aEnd - aBegin) < sMinCachedModuleLength) {
    return false;
  }

  
  aSize += sizeof(AsmJSCookieType);

  File::AutoClose file;
  if (!OpenFile(aPrincipal, eOpenForWrite, aSize, &file)) {
    return false;
  }

  
  
  
  *aMemory = (uint8_t*) file->MappedMemory() + sizeof(AsmJSCookieType);

  
  
  file.Forget(reinterpret_cast<File**>(aFile));
  return true;
}

void
CloseEntryForWrite(JS::Handle<JSObject*> global,
                   size_t aSize,
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
GetBuildId(js::Vector<char>* aBuildID)
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
public:
  NS_IMETHOD_(nsrefcnt)
  AddRef() MOZ_OVERRIDE;

  NS_IMETHOD_(nsrefcnt)
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

    bool exists;
    MOZ_ASSERT(NS_SUCCEEDED(directory->Exists(&exists)) && exists);

    nsIFile* path = directory;
    rv = path->Append(NS_LITERAL_STRING(ASMJSCACHE_FILE_NAME));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = path->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);

    if (exists) {
      int64_t fileSize;
      rv = path->GetFileSize(&fileSize);
      NS_ENSURE_SUCCESS(rv, rv);

      MOZ_ASSERT(fileSize >= 0, "Negative size?!");

      
      
      aUsageInfo->AppendToDatabaseUsage(uint64_t(fileSize));
    }

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
    MOZ_ASSUME_UNREACHABLE("There are no storages");
  }

  virtual void
  AbortTransactionsForStorage(nsIOfflineStorage* aStorage) MOZ_OVERRIDE
  {
    MOZ_ASSUME_UNREACHABLE("There are no storages");
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
