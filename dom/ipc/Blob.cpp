



#include "BlobChild.h"
#include "BlobParent.h"

#include "BackgroundParent.h"
#include "ContentChild.h"
#include "ContentParent.h"
#include "FileDescriptorSetChild.h"
#include "jsapi.h"
#include "mozilla/Assertions.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Monitor.h"
#include "mozilla/Mutex.h"
#include "mozilla/unused.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/nsIContentParent.h"
#include "mozilla/dom/nsIContentChild.h"
#include "mozilla/dom/PBlobStreamChild.h"
#include "mozilla/dom/PBlobStreamParent.h"
#include "mozilla/dom/indexedDB/FileSnapshot.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "mozilla/ipc/PBackgroundParent.h"
#include "mozilla/ipc/PFileDescriptorSetParent.h"
#include "MultipartFileImpl.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"
#include "nsID.h"
#include "nsIInputStream.h"
#include "nsIIPCSerializableInputStream.h"
#include "nsIMultiplexInputStream.h"
#include "nsIRemoteBlob.h"
#include "nsISeekableStream.h"
#include "nsIUUIDGenerator.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsStringStream.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"
#include "WorkerPrivate.h"

#ifdef DEBUG
#include "BackgroundChild.h" 
#endif

#ifdef OS_POSIX
#include "chrome/common/file_descriptor_set_posix.h"
#endif

#define DISABLE_ASSERTS_FOR_FUZZING 0

#if DISABLE_ASSERTS_FOR_FUZZING
#define ASSERT_UNLESS_FUZZING(...) do { } while (0)
#else
#define ASSERT_UNLESS_FUZZING(...) MOZ_ASSERT(false, __VA_ARGS__)
#endif

#define PRIVATE_REMOTE_INPUT_STREAM_IID \
  {0x30c7699f, 0x51d2, 0x48c8, {0xad, 0x56, 0xc0, 0x16, 0xd7, 0x6f, 0x71, 0x27}}

namespace mozilla {
namespace dom {

using namespace mozilla::ipc;
using namespace mozilla::dom::indexedDB;
using namespace mozilla::dom::workers;

namespace {

const char kUUIDGeneratorContractId[] = "@mozilla.org/uuid-generator;1";

const uint32_t kMaxFileDescriptorsPerMessage = 250;

#ifdef OS_POSIX

static_assert(FileDescriptorSet::MAX_DESCRIPTORS_PER_MESSAGE == 250,
              "MAX_DESCRIPTORS_PER_MESSAGE mismatch!");
#endif

StaticRefPtr<nsIUUIDGenerator> gUUIDGenerator;

GeckoProcessType gProcessType = GeckoProcessType_Invalid;

void
CommonStartup()
{
  MOZ_ASSERT(NS_IsMainThread());

  gProcessType = XRE_GetProcessType();
  MOZ_ASSERT(gProcessType != GeckoProcessType_Invalid);

  nsCOMPtr<nsIUUIDGenerator> uuidGen = do_GetService(kUUIDGeneratorContractId);
  MOZ_RELEASE_ASSERT(uuidGen);

  gUUIDGenerator = uuidGen;
  ClearOnShutdown(&gUUIDGenerator);
}

template <class ManagerType>
struct ConcreteManagerTypeTraits;

template <>
struct ConcreteManagerTypeTraits<nsIContentChild>
{
  typedef ContentChild Type;
};

template <>
struct ConcreteManagerTypeTraits<PBackgroundChild>
{
  typedef PBackgroundChild Type;
};

template <>
struct ConcreteManagerTypeTraits<nsIContentParent>
{
  typedef ContentParent Type;
};

template <>
struct ConcreteManagerTypeTraits<PBackgroundParent>
{
  typedef PBackgroundParent Type;
};

void
AssertCorrectThreadForManager(nsIContentChild* aManager)
{
  MOZ_ASSERT(NS_IsMainThread());
}

void
AssertCorrectThreadForManager(nsIContentParent* aManager)
{
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);
  MOZ_ASSERT(NS_IsMainThread());
}

void
AssertCorrectThreadForManager(PBackgroundChild* aManager)
{
#ifdef DEBUG
  if (aManager) {
    PBackgroundChild* backgroundChild = BackgroundChild::GetForCurrentThread();
    MOZ_ASSERT(backgroundChild);
    MOZ_ASSERT(backgroundChild == aManager);
  }
#endif
}

void
AssertCorrectThreadForManager(PBackgroundParent* aManager)
{
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);
  AssertIsOnBackgroundThread();
}

intptr_t
ActorManagerProcessID(nsIContentParent* aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return reinterpret_cast<intptr_t>(aManager);
}

intptr_t
ActorManagerProcessID(PBackgroundParent* aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return BackgroundParent::GetRawContentParentForComparison(aManager);
}

bool
ActorManagerIsSameProcess(nsIContentParent* aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return false;
}

bool
ActorManagerIsSameProcess(PBackgroundParent* aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return !BackgroundParent::IsOtherProcessActor(aManager);
}

bool
EventTargetIsOnCurrentThread(nsIEventTarget* aEventTarget)
{
  if (!aEventTarget) {
    return NS_IsMainThread();
  }

  bool current;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aEventTarget->IsOnCurrentThread(&current)));

  return current;
}

class CancelableRunnableWrapper MOZ_FINAL
  : public nsCancelableRunnable
{
  nsCOMPtr<nsIRunnable> mRunnable;
#ifdef DEBUG
  nsCOMPtr<nsIEventTarget> mDEBUGEventTarget;
#endif

public:
  CancelableRunnableWrapper(nsIRunnable* aRunnable,
                            nsIEventTarget* aEventTarget)
    : mRunnable(aRunnable)
#ifdef DEBUG
    , mDEBUGEventTarget(aEventTarget)
#endif
  {
    MOZ_ASSERT(aRunnable);
    MOZ_ASSERT(aEventTarget);
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~CancelableRunnableWrapper()
  { }

  NS_DECL_NSIRUNNABLE
  NS_DECL_NSICANCELABLERUNNABLE
};

NS_IMPL_ISUPPORTS_INHERITED0(CancelableRunnableWrapper, nsCancelableRunnable)

NS_IMETHODIMP
CancelableRunnableWrapper::Run()
{
  DebugOnly<bool> onTarget;
  MOZ_ASSERT(mDEBUGEventTarget);
  MOZ_ASSERT(NS_SUCCEEDED(mDEBUGEventTarget->IsOnCurrentThread(&onTarget)));
  MOZ_ASSERT(onTarget);

  nsCOMPtr<nsIRunnable> runnable;
  mRunnable.swap(runnable);

  if (runnable) {
    return runnable->Run();
  }

  return NS_OK;
}

NS_IMETHODIMP
CancelableRunnableWrapper::Cancel()
{
  DebugOnly<bool> onTarget;
  MOZ_ASSERT(mDEBUGEventTarget);
  MOZ_ASSERT(NS_SUCCEEDED(mDEBUGEventTarget->IsOnCurrentThread(&onTarget)));
  MOZ_ASSERT(onTarget);

  if (NS_WARN_IF(!mRunnable)) {
    return NS_ERROR_UNEXPECTED;
  }

  unused << Run();
  MOZ_ASSERT(!mRunnable);

  return NS_OK;
}


template <template <class> class SmartPtr, class T>
void
ReleaseOnTarget(SmartPtr<T>& aDoomed, nsIEventTarget* aTarget)
{
  MOZ_ASSERT(aDoomed);
  MOZ_ASSERT(!EventTargetIsOnCurrentThread(aTarget));

  T* doomedRaw;
  aDoomed.forget(&doomedRaw);

  auto* doomedSupports = static_cast<nsISupports*>(doomedRaw);

  nsCOMPtr<nsIRunnable> releaseRunnable =
    NS_NewNonOwningRunnableMethod(doomedSupports, &nsISupports::Release);
  MOZ_ASSERT(releaseRunnable);

  if (aTarget) {
    
    
    releaseRunnable = new CancelableRunnableWrapper(releaseRunnable, aTarget);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aTarget->Dispatch(releaseRunnable,
                                                   NS_DISPATCH_NORMAL)));
  } else {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(releaseRunnable)));
  }
}

template <class ManagerType>
void
ConstructFileDescriptorSet(ManagerType* aManager,
                           nsTArray<FileDescriptor>& aFDs,
                           OptionalFileDescriptorSet& aOptionalFDSet)
{
  typedef typename ConcreteManagerTypeTraits<ManagerType>::Type
          ConcreteManagerType;

  MOZ_ASSERT(aManager);

  if (aFDs.IsEmpty()) {
    aOptionalFDSet = void_t();
    return;
  }

  if (aFDs.Length() <= kMaxFileDescriptorsPerMessage) {
    aOptionalFDSet = nsTArray<FileDescriptor>();
    aOptionalFDSet.get_ArrayOfFileDescriptor().SwapElements(aFDs);
    return;
  }

  auto* concreteManager = static_cast<ConcreteManagerType*>(aManager);

  PFileDescriptorSetParent* fdSet =
    concreteManager->SendPFileDescriptorSetConstructor(aFDs[0]);
  if (!fdSet) {
    aOptionalFDSet = void_t();
    return;
  }

  for (uint32_t index = 1; index < aFDs.Length(); index++) {
    if (!fdSet->SendAddFileDescriptor(aFDs[index])) {
      aOptionalFDSet = void_t();
      return;
    }
  }

  aOptionalFDSet = fdSet;
}

void
OptionalFileDescriptorSetToFDs(OptionalFileDescriptorSet& aOptionalSet,
                               nsTArray<FileDescriptor>& aFDs)
{
  MOZ_ASSERT(aFDs.IsEmpty());

  switch (aOptionalSet.type()) {
    case OptionalFileDescriptorSet::Tvoid_t:
      return;

    case OptionalFileDescriptorSet::TArrayOfFileDescriptor:
      aOptionalSet.get_ArrayOfFileDescriptor().SwapElements(aFDs);
      return;

    case OptionalFileDescriptorSet::TPFileDescriptorSetChild: {
      FileDescriptorSetChild* fdSetActor =
        static_cast<FileDescriptorSetChild*>(
          aOptionalSet.get_PFileDescriptorSetChild());
      MOZ_ASSERT(fdSetActor);

      fdSetActor->ForgetFileDescriptors(aFDs);
      MOZ_ASSERT(!aFDs.IsEmpty());

      PFileDescriptorSetChild::Send__delete__(fdSetActor);
      return;
    }

    default:
      MOZ_CRASH("Unknown type!");
  }

  MOZ_CRASH("Should never get here!");
}

class NS_NO_VTABLE IPrivateRemoteInputStream
  : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(PRIVATE_REMOTE_INPUT_STREAM_IID)

  
  virtual nsIInputStream*
  BlockAndGetInternalStream() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(IPrivateRemoteInputStream,
                              PRIVATE_REMOTE_INPUT_STREAM_IID)



class BlobInputStreamTether MOZ_FINAL
  : public nsIMultiplexInputStream
  , public nsISeekableStream
  , public nsIIPCSerializableInputStream
{
  nsCOMPtr<nsIInputStream> mStream;
  nsRefPtr<FileImpl> mBlobImpl;

  nsIMultiplexInputStream* mWeakMultiplexStream;
  nsISeekableStream* mWeakSeekableStream;
  nsIIPCSerializableInputStream* mWeakSerializableStream;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_FORWARD_NSIINPUTSTREAM(mStream->)
  NS_FORWARD_SAFE_NSIMULTIPLEXINPUTSTREAM(mWeakMultiplexStream)
  NS_FORWARD_SAFE_NSISEEKABLESTREAM(mWeakSeekableStream)
  NS_FORWARD_SAFE_NSIIPCSERIALIZABLEINPUTSTREAM(mWeakSerializableStream)

  BlobInputStreamTether(nsIInputStream* aStream, FileImpl* aBlobImpl)
    : mStream(aStream)
    , mBlobImpl(aBlobImpl)
    , mWeakMultiplexStream(nullptr)
    , mWeakSeekableStream(nullptr)
    , mWeakSerializableStream(nullptr)
  {
    MOZ_ASSERT(aStream);
    MOZ_ASSERT(aBlobImpl);

    nsCOMPtr<nsIMultiplexInputStream> multiplexStream =
      do_QueryInterface(aStream);
    if (multiplexStream) {
      MOZ_ASSERT(SameCOMIdentity(aStream, multiplexStream));
      mWeakMultiplexStream = multiplexStream;
    }

    nsCOMPtr<nsISeekableStream> seekableStream = do_QueryInterface(aStream);
    if (seekableStream) {
      MOZ_ASSERT(SameCOMIdentity(aStream, seekableStream));
      mWeakSeekableStream = seekableStream;
    }

    nsCOMPtr<nsIIPCSerializableInputStream> serializableStream =
      do_QueryInterface(aStream);
    if (serializableStream) {
      MOZ_ASSERT(SameCOMIdentity(aStream, serializableStream));
      mWeakSerializableStream = serializableStream;
    }
  }

private:
  ~BlobInputStreamTether()
  { }
};

NS_IMPL_ADDREF(BlobInputStreamTether)
NS_IMPL_RELEASE(BlobInputStreamTether)

NS_INTERFACE_MAP_BEGIN(BlobInputStreamTether)
  NS_INTERFACE_MAP_ENTRY(nsIInputStream)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIMultiplexInputStream,
                                     mWeakMultiplexStream)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsISeekableStream, mWeakSeekableStream)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIIPCSerializableInputStream,
                                     mWeakSerializableStream)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIInputStream)
NS_INTERFACE_MAP_END

class RemoteInputStream MOZ_FINAL
  : public nsIInputStream
  , public nsISeekableStream
  , public nsIIPCSerializableInputStream
  , public IPrivateRemoteInputStream
{
  Monitor mMonitor;
  BlobChild* mActor;
  nsCOMPtr<nsIInputStream> mStream;
  nsRefPtr<FileImpl> mBlobImpl;
  nsCOMPtr<nsIEventTarget> mEventTarget;
  nsISeekableStream* mWeakSeekableStream;
  uint64_t mStart;
  uint64_t mLength;

public:
  explicit
  RemoteInputStream(FileImpl* aBlobImpl);

  RemoteInputStream(BlobChild* aActor,
                    FileImpl* aBlobImpl,
                    uint64_t aStart,
                    uint64_t aLength);

  bool
  IsOnOwningThread() const
  {
    return EventTargetIsOnCurrentThread(mEventTarget);
  }

  void
  AssertIsOnOwningThread() const
  {
    MOZ_ASSERT(IsOnOwningThread());
  }

  bool
  IsWorkerStream() const
  {
    return !!mActor;
  }

  void
  SetStream(nsIInputStream* aStream);

  NS_DECL_THREADSAFE_ISUPPORTS

private:
  ~RemoteInputStream();

  nsresult
  BlockAndWaitForStream();

  void
  ReallyBlockAndWaitForStream();

  bool
  IsSeekableStream();

  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSISEEKABLESTREAM
  NS_DECL_NSIIPCSERIALIZABLEINPUTSTREAM

  virtual nsIInputStream*
  BlockAndGetInternalStream() MOZ_OVERRIDE;
};

class InputStreamChild MOZ_FINAL
  : public PBlobStreamChild
{
  nsRefPtr<RemoteInputStream> mRemoteStream;

public:
  explicit
  InputStreamChild(RemoteInputStream* aRemoteStream)
    : mRemoteStream(aRemoteStream)
  {
    MOZ_ASSERT(aRemoteStream);
    aRemoteStream->AssertIsOnOwningThread();
  }

  InputStreamChild()
  { }

  ~InputStreamChild()
  { }

private:
  
  virtual bool
  Recv__delete__(const InputStreamParams& aParams,
                 const OptionalFileDescriptorSet& aFDs) MOZ_OVERRIDE;
};

class InputStreamParent MOZ_FINAL
  : public PBlobStreamParent
{
  typedef mozilla::ipc::InputStreamParams InputStreamParams;
  typedef mozilla::ipc::OptionalFileDescriptorSet OptionalFileDescriptorSet;

  bool* mSyncLoopGuard;
  InputStreamParams* mParams;
  OptionalFileDescriptorSet* mFDs;

#ifdef DEBUG
  PRThread* mOwningThread;
#endif

public:
  InputStreamParent()
    : mSyncLoopGuard(nullptr)
    , mParams(nullptr)
    , mFDs(nullptr)
  {
#ifdef DEBUG
    mOwningThread = PR_GetCurrentThread();
#endif

    AssertIsOnOwningThread();

    MOZ_COUNT_CTOR(InputStreamParent);
  }

  InputStreamParent(bool* aSyncLoopGuard,
                    InputStreamParams* aParams,
                    OptionalFileDescriptorSet* aFDs)
    : mSyncLoopGuard(aSyncLoopGuard)
    , mParams(aParams)
    , mFDs(aFDs)
  {
#ifdef DEBUG
    mOwningThread = PR_GetCurrentThread();
#endif

    AssertIsOnOwningThread();
    MOZ_ASSERT(aSyncLoopGuard);
    MOZ_ASSERT(!*aSyncLoopGuard);
    MOZ_ASSERT(aParams);
    MOZ_ASSERT(aFDs);

    MOZ_COUNT_CTOR(InputStreamParent);
  }

  ~InputStreamParent()
  {
    AssertIsOnOwningThread();

    MOZ_COUNT_DTOR(InputStreamParent);
  }

  void
  AssertIsOnOwningThread() const
  {
#ifdef DEBUG
    MOZ_ASSERT(PR_GetCurrentThread() == mOwningThread);
#endif
  }

  bool
  Destroy(const InputStreamParams& aParams,
          const OptionalFileDescriptorSet& aFDs)
  {
    AssertIsOnOwningThread();

    if (mSyncLoopGuard) {
      MOZ_ASSERT(!*mSyncLoopGuard);

      *mSyncLoopGuard = true;
      *mParams = aParams;
      *mFDs = aFDs;

      
      delete this;
      return true;
    }

    
    return PBlobStreamParent::Send__delete__(this, aParams, aFDs);
  }

private:
  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE
  {
    
  }
};

class EmptyBlobImpl MOZ_FINAL
  : public FileImplBase
{
public:
  explicit EmptyBlobImpl(const nsAString& aContentType)
    : FileImplBase(aContentType, 0)
  {
    mImmutable = true;
  }

  EmptyBlobImpl(const nsAString& aName,
                const nsAString& aContentType,
                uint64_t aLastModifiedDate)
    : FileImplBase(aName, aContentType, 0, aLastModifiedDate)
  {
    mImmutable = true;
  }

private:
  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t ,
              uint64_t aLength,
              const nsAString& aContentType,
              ErrorResult& ) MOZ_OVERRIDE
  {
    MOZ_ASSERT(!aLength);

    nsRefPtr<FileImpl> sliceImpl = new EmptyBlobImpl(aContentType);

    DebugOnly<bool> isMutable;
    MOZ_ASSERT(NS_SUCCEEDED(sliceImpl->GetMutable(&isMutable)));
    MOZ_ASSERT(!isMutable);

    return sliceImpl.forget();
  }

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE
  {
    NS_ENSURE_ARG_POINTER(aStream);

    nsString emptyString;
    nsresult rv = NS_NewStringInputStream(aStream, emptyString);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    return NS_OK;
  }
};


class SameProcessInputStreamBlobImpl MOZ_FINAL
  : public FileImplBase
{
  nsCOMPtr<nsIInputStream> mInputStream;

public:
  SameProcessInputStreamBlobImpl(const nsAString& aContentType,
                                 uint64_t aLength,
                                 nsIInputStream* aInputStream)
    : FileImplBase(aContentType, aLength)
    , mInputStream(aInputStream)
  {
    MOZ_ASSERT(aLength != UINT64_MAX);
    MOZ_ASSERT(aInputStream);

    mImmutable = true;
  }

  SameProcessInputStreamBlobImpl(const nsAString& aName,
                                 const nsAString& aContentType,
                                 uint64_t aLength,
                                 uint64_t aLastModifiedDate,
                                 nsIInputStream* aInputStream)
    : FileImplBase(aName, aContentType, aLength, aLastModifiedDate)
    , mInputStream(aInputStream)
  {
    MOZ_ASSERT(aLength != UINT64_MAX);
    MOZ_ASSERT(aLastModifiedDate != UINT64_MAX);
    MOZ_ASSERT(aInputStream);

    mImmutable = true;
  }

private:
  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t ,
              uint64_t ,
              const nsAString& ,
              ErrorResult& ) MOZ_OVERRIDE
  {
    MOZ_CRASH("Not implemented");
  }

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE
  {
    NS_ENSURE_ARG_POINTER(aStream);

    nsCOMPtr<nsIInputStream> inputStream = mInputStream;
    inputStream.forget(aStream);
    return NS_OK;
  }
};

struct MOZ_STACK_CLASS CreateBlobImplMetadata MOZ_FINAL
{
  nsString mContentType;
  nsString mName;
  uint64_t mLength;
  uint64_t mLastModifiedDate;
  bool mHasRecursed;
  const bool mIsSameProcessActor;

  explicit CreateBlobImplMetadata(bool aIsSameProcessActor)
    : mLength(0)
    , mLastModifiedDate(0)
    , mHasRecursed(false)
    , mIsSameProcessActor(aIsSameProcessActor)
  {
    MOZ_COUNT_CTOR(CreateBlobImplMetadata);

    mName.SetIsVoid(true);
  }

  ~CreateBlobImplMetadata()
  {
    MOZ_COUNT_DTOR(CreateBlobImplMetadata);
  }

  bool
  IsFile() const
  {
    return !mName.IsVoid();
  }
};

already_AddRefed<FileImpl>
CreateBlobImpl(const nsID& aKnownBlobIDData,
               const CreateBlobImplMetadata& aMetadata)
{
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);
  MOZ_ASSERT(aMetadata.mHasRecursed);

  nsRefPtr<FileImpl> blobImpl = BlobParent::GetBlobImplForID(aKnownBlobIDData);
  if (NS_WARN_IF(!blobImpl)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(blobImpl->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

  return blobImpl.forget();
}

already_AddRefed<FileImpl>
CreateBlobImpl(const nsTArray<uint8_t>& aMemoryData,
               const CreateBlobImplMetadata& aMetadata)
{
  static_assert(sizeof(aMemoryData.Length()) <= sizeof(size_t),
                "String length won't fit in size_t!");
  static_assert(sizeof(size_t) <= sizeof(uint64_t),
                "size_t won't fit in uint64_t!");

  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

  nsRefPtr<FileImpl> blobImpl;

  if (auto length = static_cast<size_t>(aMemoryData.Length())) {
    static MOZ_CONSTEXPR_VAR size_t elementSizeMultiplier =
      sizeof(aMemoryData[0]) / sizeof(char);

    if (!aMetadata.mHasRecursed &&
        NS_WARN_IF(aMetadata.mLength != uint64_t(length))) {
      ASSERT_UNLESS_FUZZING();
      return nullptr;
    }

    void* buffer = moz_malloc(length * elementSizeMultiplier);
    if (NS_WARN_IF(!buffer)) {
      return nullptr;
    }

    memcpy(buffer, aMemoryData.Elements(), length * elementSizeMultiplier);

    if (!aMetadata.mHasRecursed && aMetadata.IsFile()) {
      blobImpl =
        new FileImplMemory(buffer,
                           uint64_t(length),
                           aMetadata.mName,
                           aMetadata.mContentType,
                           aMetadata.mLastModifiedDate);
    } else {
      blobImpl =
        new FileImplMemory(buffer, uint64_t(length), aMetadata.mContentType);
    }
  } else if (!aMetadata.mHasRecursed && aMetadata.IsFile()) {
    blobImpl =
      new EmptyBlobImpl(aMetadata.mName,
                        aMetadata.mContentType,
                        aMetadata.mLastModifiedDate);
  } else {
    blobImpl = new EmptyBlobImpl(aMetadata.mContentType);
  }

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(blobImpl->SetMutable(false)));

  return blobImpl.forget();
}

already_AddRefed<FileImpl>
CreateBlobImpl(intptr_t aAddRefedInputStream,
               const CreateBlobImplMetadata& aMetadata)
{
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);
  MOZ_ASSERT(aMetadata.mIsSameProcessActor);
  MOZ_ASSERT(aAddRefedInputStream);

  nsCOMPtr<nsIInputStream> inputStream =
    dont_AddRef(
      reinterpret_cast<nsIInputStream*>(aAddRefedInputStream));

  nsRefPtr<FileImpl> blobImpl;
  if (!aMetadata.mHasRecursed && aMetadata.IsFile()) {
    blobImpl =
      new SameProcessInputStreamBlobImpl(aMetadata.mName,
                                         aMetadata.mContentType,
                                         aMetadata.mLength,
                                         aMetadata.mLastModifiedDate,
                                         inputStream);
  } else {
    blobImpl =
      new SameProcessInputStreamBlobImpl(aMetadata.mContentType,
                                         aMetadata.mLength,
                                         inputStream);
  }

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(blobImpl->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

  return blobImpl.forget();
}

already_AddRefed<FileImpl>
CreateBlobImpl(const nsTArray<BlobData>& aBlobData,
               CreateBlobImplMetadata& aMetadata);

already_AddRefed<FileImpl>
CreateBlobImplFromBlobData(const BlobData& aBlobData,
                           CreateBlobImplMetadata& aMetadata)
{
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

  nsRefPtr<FileImpl> blobImpl;

  switch (aBlobData.type()) {
    case BlobData::TnsID: {
      blobImpl = CreateBlobImpl(aBlobData.get_nsID(), aMetadata);
      break;
    }

    case BlobData::TArrayOfuint8_t: {
      blobImpl = CreateBlobImpl(aBlobData.get_ArrayOfuint8_t(), aMetadata);
      break;
    }

    case BlobData::Tintptr_t: {
      blobImpl = CreateBlobImpl(aBlobData.get_intptr_t(), aMetadata);
      break;
    }

    case BlobData::TArrayOfBlobData: {
      blobImpl = CreateBlobImpl(aBlobData.get_ArrayOfBlobData(), aMetadata);
      break;
    }

    default:
      MOZ_CRASH("Unknown params!");
  }

  return blobImpl.forget();
}

already_AddRefed<FileImpl>
CreateBlobImpl(const nsTArray<BlobData>& aBlobDatas,
               CreateBlobImplMetadata& aMetadata)
{
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

  
  if (aBlobDatas.Length() == 1) {
    const BlobData& blobData = aBlobDatas[0];

    nsRefPtr<FileImpl> blobImpl =
      CreateBlobImplFromBlobData(blobData, aMetadata);
    if (NS_WARN_IF(!blobImpl)) {
      return nullptr;
    }

    DebugOnly<bool> isMutable;
    MOZ_ASSERT(NS_SUCCEEDED(blobImpl->GetMutable(&isMutable)));
    MOZ_ASSERT(!isMutable);

    return blobImpl.forget();
  }

  FallibleTArray<nsRefPtr<FileImpl>> fallibleBlobImpls;
  if (NS_WARN_IF(!fallibleBlobImpls.SetLength(aBlobDatas.Length()))) {
    return nullptr;
  }

  nsTArray<nsRefPtr<FileImpl>> blobImpls;
  fallibleBlobImpls.SwapElements(blobImpls);

  const bool hasRecursed = aMetadata.mHasRecursed;
  aMetadata.mHasRecursed = true;

  for (uint32_t count = aBlobDatas.Length(), index = 0;
       index < count;
       index++) {
    const BlobData& blobData = aBlobDatas[index];
    nsRefPtr<FileImpl>& blobImpl = blobImpls[index];

    blobImpl = CreateBlobImplFromBlobData(blobData, aMetadata);
    if (NS_WARN_IF(!blobImpl)) {
      return nullptr;
    }

    DebugOnly<bool> isMutable;
    MOZ_ASSERT(NS_SUCCEEDED(blobImpl->GetMutable(&isMutable)));
    MOZ_ASSERT(!isMutable);
  }

  nsRefPtr<FileImpl> blobImpl;
  if (!hasRecursed && aMetadata.IsFile()) {
    blobImpl =
      new MultipartFileImpl(blobImpls, aMetadata.mName, aMetadata.mContentType);
  } else {
    blobImpl =
      new MultipartFileImpl(blobImpls, aMetadata.mContentType);
  }

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(blobImpl->SetMutable(false)));

  return blobImpl.forget();
}

already_AddRefed<FileImpl>
CreateBlobImpl(const ParentBlobConstructorParams& aParams,
               const BlobData& aBlobData,
               bool aIsSameProcessActor)
{
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);
  MOZ_ASSERT(aParams.blobParams().type() ==
               AnyBlobConstructorParams::TNormalBlobConstructorParams ||
             aParams.blobParams().type() ==
               AnyBlobConstructorParams::TFileBlobConstructorParams);

  CreateBlobImplMetadata metadata(aIsSameProcessActor);

  if (aParams.blobParams().type() ==
        AnyBlobConstructorParams::TNormalBlobConstructorParams) {
    const NormalBlobConstructorParams& params =
      aParams.blobParams().get_NormalBlobConstructorParams();

    if (NS_WARN_IF(params.length() == UINT64_MAX)) {
      ASSERT_UNLESS_FUZZING();
      return nullptr;
    }

    metadata.mContentType = params.contentType();
    metadata.mLength = params.length();
  } else {
    const FileBlobConstructorParams& params =
      aParams.blobParams().get_FileBlobConstructorParams();

    if (NS_WARN_IF(params.length() == UINT64_MAX)) {
      ASSERT_UNLESS_FUZZING();
      return nullptr;
    }

    if (NS_WARN_IF(params.modDate() == UINT64_MAX)) {
      ASSERT_UNLESS_FUZZING();
      return nullptr;
    }

    metadata.mContentType = params.contentType();
    metadata.mName = params.name();
    metadata.mLength = params.length();
    metadata.mLastModifiedDate = params.modDate();
  }

  nsRefPtr<FileImpl> blobImpl =
    CreateBlobImplFromBlobData(aBlobData, metadata);
  return blobImpl.forget();
}

void
BlobDataFromBlobImpl(FileImpl* aBlobImpl, BlobData& aBlobData)
{
  MOZ_ASSERT(gProcessType != GeckoProcessType_Default);
  MOZ_ASSERT(aBlobImpl);

  const nsTArray<nsRefPtr<FileImpl>>* subBlobs = aBlobImpl->GetSubBlobImpls();

  if (subBlobs) {
    aBlobData = nsTArray<BlobData>();

    nsTArray<BlobData>& subBlobDatas = aBlobData.get_ArrayOfBlobData();
    subBlobDatas.SetLength(subBlobs->Length());

    for (uint32_t count = subBlobs->Length(), index = 0;
         index < count;
         index++) {
      BlobDataFromBlobImpl(subBlobs->ElementAt(index), subBlobDatas[index]);
    }

    return;
  }

  nsCOMPtr<nsIRemoteBlob> remoteBlob = do_QueryInterface(aBlobImpl);
  if (remoteBlob) {
    BlobChild* actor = remoteBlob->GetBlobChild();
    MOZ_ASSERT(actor);

    aBlobData = actor->ParentID();
    return;
  }

  MOZ_ASSERT(aBlobImpl->IsMemoryFile());

  nsCOMPtr<nsIInputStream> inputStream;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aBlobImpl->GetInternalStream(getter_AddRefs(inputStream))));

  DebugOnly<bool> isNonBlocking;
  MOZ_ASSERT(NS_SUCCEEDED(inputStream->IsNonBlocking(&isNonBlocking)));
  MOZ_ASSERT(isNonBlocking);

  uint64_t available;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(inputStream->Available(&available)));

  MOZ_ASSERT(available <= uint64_t(UINT32_MAX));

  aBlobData = nsTArray<uint8_t>();

  nsTArray<uint8_t>& blobData = aBlobData.get_ArrayOfuint8_t();

  blobData.SetLength(size_t(available));

  uint32_t readCount;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    inputStream->Read(reinterpret_cast<char*>(blobData.Elements()),
                      uint32_t(available),
                      &readCount)));
}

RemoteInputStream::RemoteInputStream(FileImpl* aBlobImpl)
  : mMonitor("RemoteInputStream.mMonitor")
  , mActor(nullptr)
  , mBlobImpl(aBlobImpl)
  , mWeakSeekableStream(nullptr)
  , mStart(0)
  , mLength(0)
{
  MOZ_ASSERT(aBlobImpl);

  if (!NS_IsMainThread()) {
    mEventTarget = do_GetCurrentThread();
    MOZ_ASSERT(mEventTarget);
  }

  MOZ_ASSERT(IsOnOwningThread());
}

RemoteInputStream::RemoteInputStream(BlobChild* aActor,
                                     FileImpl* aBlobImpl,
                                     uint64_t aStart,
                                     uint64_t aLength)
  : mMonitor("RemoteInputStream.mMonitor")
  , mActor(aActor)
  , mBlobImpl(aBlobImpl)
  , mEventTarget(NS_GetCurrentThread())
  , mWeakSeekableStream(nullptr)
  , mStart(aStart)
  , mLength(aLength)
{
  MOZ_ASSERT(!NS_IsMainThread());
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(aBlobImpl);

  MOZ_ASSERT(IsOnOwningThread());
}

RemoteInputStream::~RemoteInputStream()
{
  if (!IsOnOwningThread()) {
    mStream = nullptr;
    mWeakSeekableStream = nullptr;

    if (mBlobImpl) {
      ReleaseOnTarget(mBlobImpl, mEventTarget);
    }
  }
}

void
RemoteInputStream::SetStream(nsIInputStream* aStream)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aStream);

  nsCOMPtr<nsIInputStream> stream = aStream;
  nsCOMPtr<nsISeekableStream> seekableStream = do_QueryInterface(aStream);

  MOZ_ASSERT_IF(seekableStream, SameCOMIdentity(aStream, seekableStream));

  {
    MonitorAutoLock lock(mMonitor);

    MOZ_ASSERT_IF(mStream, IsWorkerStream());

    if (!mStream) {
      MOZ_ASSERT(!mWeakSeekableStream);

      mStream.swap(stream);
      mWeakSeekableStream = seekableStream;

      mMonitor.Notify();
    }
  }
}

nsresult
RemoteInputStream::BlockAndWaitForStream()
{
  if (IsOnOwningThread()) {
    if (NS_IsMainThread()) {
      NS_WARNING("Blocking the main thread is not supported!");
      return NS_ERROR_FAILURE;
    }

    MOZ_ASSERT(IsWorkerStream());

    InputStreamParams params;
    OptionalFileDescriptorSet optionalFDs;

    mActor->SendBlobStreamSync(mStart, mLength, &params, &optionalFDs);

    nsTArray<FileDescriptor> fds;
    OptionalFileDescriptorSetToFDs(optionalFDs, fds);

    nsCOMPtr<nsIInputStream> stream = DeserializeInputStream(params, fds);
    MOZ_ASSERT(stream);

    SetStream(stream);
    return NS_OK;
  }

  ReallyBlockAndWaitForStream();

  return NS_OK;
}

void
RemoteInputStream::ReallyBlockAndWaitForStream()
{
  MOZ_ASSERT(!IsOnOwningThread());

  DebugOnly<bool> waited;

  {
    MonitorAutoLock lock(mMonitor);

    waited = !mStream;

    while (!mStream) {
      mMonitor.Wait();
    }
  }

  MOZ_ASSERT(mStream);

#ifdef DEBUG
  if (waited && mWeakSeekableStream) {
    int64_t position;
    MOZ_ASSERT(NS_SUCCEEDED(mWeakSeekableStream->Tell(&position)),
                "Failed to determine initial stream position!");
    MOZ_ASSERT(!position, "Stream not starting at 0!");
  }
#endif
}

bool
RemoteInputStream::IsSeekableStream()
{
  if (IsOnOwningThread()) {
    if (!mStream) {
      NS_WARNING("Don't know if this stream is seekable yet!");
      return true;
    }
  } else {
    ReallyBlockAndWaitForStream();
  }

  return !!mWeakSeekableStream;
}

NS_IMPL_ADDREF(RemoteInputStream)
NS_IMPL_RELEASE(RemoteInputStream)

NS_INTERFACE_MAP_BEGIN(RemoteInputStream)
  NS_INTERFACE_MAP_ENTRY(nsIInputStream)
  NS_INTERFACE_MAP_ENTRY(nsIIPCSerializableInputStream)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsISeekableStream, IsSeekableStream())
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIInputStream)
  NS_INTERFACE_MAP_ENTRY(IPrivateRemoteInputStream)
NS_INTERFACE_MAP_END

NS_IMETHODIMP
RemoteInputStream::Close()
{
  nsresult rv = BlockAndWaitForStream();
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<FileImpl> blobImpl;
  mBlobImpl.swap(blobImpl);

  rv = mStream->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
RemoteInputStream::Available(uint64_t* aAvailable)
{
  if (!IsOnOwningThread()) {
    nsresult rv = BlockAndWaitForStream();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStream->Available(aAvailable);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#ifdef DEBUG
  if (NS_IsMainThread()) {
    NS_WARNING("Someone is trying to do main-thread I/O...");
  }
#endif

  nsresult rv;

  
  nsCOMPtr<nsIInputStream> inputStream;
  {
    MonitorAutoLock lock(mMonitor);

    inputStream = mStream;
  }

  
  if (inputStream) {
    rv = inputStream->Available(aAvailable);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  
  if (!mBlobImpl) {
    return NS_BASE_STREAM_CLOSED;
  }

  
  NS_WARNING("Available() called before real stream has been delivered, "
              "guessing the amount of data available!");

  ErrorResult error;
  *aAvailable = mBlobImpl->GetSize(error);
  if (NS_WARN_IF(error.Failed())) {
    return error.ErrorCode();
  }

  return NS_OK;
}

NS_IMETHODIMP
RemoteInputStream::Read(char* aBuffer, uint32_t aCount, uint32_t* aResult)
{
  nsresult rv = BlockAndWaitForStream();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mStream->Read(aBuffer, aCount, aResult);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
RemoteInputStream::ReadSegments(nsWriteSegmentFun aWriter,
                                void* aClosure,
                                uint32_t aCount,
                                uint32_t* aResult)
{
  nsresult rv = BlockAndWaitForStream();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mStream->ReadSegments(aWriter, aClosure, aCount, aResult);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
RemoteInputStream::IsNonBlocking(bool* aNonBlocking)
{
  NS_ENSURE_ARG_POINTER(aNonBlocking);

  *aNonBlocking = false;
  return NS_OK;
}

NS_IMETHODIMP
RemoteInputStream::Seek(int32_t aWhence, int64_t aOffset)
{
  nsresult rv = BlockAndWaitForStream();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mWeakSeekableStream) {
    NS_WARNING("Underlying blob stream is not seekable!");
    return NS_ERROR_NO_INTERFACE;
  }

  rv = mWeakSeekableStream->Seek(aWhence, aOffset);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
RemoteInputStream::Tell(int64_t* aResult)
{
  
  
  
  if (IsOnOwningThread() && !mStream) {
    *aResult = 0;
    return NS_OK;
  }

  nsresult rv = BlockAndWaitForStream();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mWeakSeekableStream) {
    NS_WARNING("Underlying blob stream is not seekable!");
    return NS_ERROR_NO_INTERFACE;
  }

  rv = mWeakSeekableStream->Tell(aResult);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
RemoteInputStream::SetEOF()
{
  nsresult rv = BlockAndWaitForStream();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mWeakSeekableStream) {
    NS_WARNING("Underlying blob stream is not seekable!");
    return NS_ERROR_NO_INTERFACE;
  }

  rv = mWeakSeekableStream->SetEOF();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
RemoteInputStream::Serialize(InputStreamParams& aParams,
                             FileDescriptorArray& )
{
  MOZ_RELEASE_ASSERT(mBlobImpl);

  nsCOMPtr<nsIRemoteBlob> remote = do_QueryInterface(mBlobImpl);
  MOZ_ASSERT(remote);

  BlobChild* actor = remote->GetBlobChild();
  MOZ_ASSERT(actor);

  aParams = RemoteInputStreamParams(actor->ParentID());
}

bool
RemoteInputStream::Deserialize(const InputStreamParams& ,
                               const FileDescriptorArray& )
{
  
  
  MOZ_CRASH("RemoteInputStream should never be deserialized");
}

nsIInputStream*
RemoteInputStream::BlockAndGetInternalStream()
{
  MOZ_ASSERT(!IsOnOwningThread());

  nsresult rv = BlockAndWaitForStream();
  NS_ENSURE_SUCCESS(rv, nullptr);

  return mStream;
}

} 

StaticAutoPtr<BlobParent::IDTable> BlobParent::sIDTable;
StaticAutoPtr<Mutex> BlobParent::sIDTableMutex;





class BlobParent::IDTableEntry MOZ_FINAL
{
  const nsID mID;
  const intptr_t mProcessID;
  const nsRefPtr<FileImpl> mBlobImpl;

public:
  static already_AddRefed<IDTableEntry>
  Create(const nsID& aID, intptr_t aProcessID, FileImpl* aBlobImpl)
  {
    MOZ_ASSERT(aBlobImpl);

    DebugOnly<bool> isMutable;
    MOZ_ASSERT(NS_SUCCEEDED(aBlobImpl->GetMutable(&isMutable)));
    MOZ_ASSERT(!isMutable);

    return GetOrCreateInternal(aID,
                               aProcessID,
                               aBlobImpl,
                                true,
                                false,
                                false);
  }

  static already_AddRefed<IDTableEntry>
  Get(const nsID& aID, intptr_t aProcessID)
  {
    return GetOrCreateInternal(aID,
                               aProcessID,
                               nullptr,
                                false,
                                true,
                                false);
  }

  static already_AddRefed<IDTableEntry>
  Get(const nsID& aID)
  {
    return GetOrCreateInternal(aID,
                               0,
                               nullptr,
                                false,
                                true,
                                true);
  }

  static already_AddRefed<IDTableEntry>
  GetOrCreate(const nsID& aID, intptr_t aProcessID, FileImpl* aBlobImpl)
  {
    MOZ_ASSERT(aBlobImpl);

    DebugOnly<bool> isMutable;
    MOZ_ASSERT(NS_SUCCEEDED(aBlobImpl->GetMutable(&isMutable)));
    MOZ_ASSERT(!isMutable);

    return GetOrCreateInternal(aID,
                               aProcessID,
                               aBlobImpl,
                                true,
                                true,
                                false);
  }

  const nsID&
  ID() const
  {
    return mID;
  }

  intptr_t
  ProcessID() const
  {
    return mProcessID;
  }

  FileImpl*
  BlobImpl() const
  {
    return mBlobImpl;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(IDTableEntry)

private:
  IDTableEntry(const nsID& aID, intptr_t aProcessID, FileImpl* aBlobImpl);
  ~IDTableEntry();

  static already_AddRefed<IDTableEntry>
  GetOrCreateInternal(const nsID& aID,
                      intptr_t aProcessID,
                      FileImpl* aBlobImpl,
                      bool aMayCreate,
                      bool aMayGet,
                      bool aIgnoreProcessID);
};











class BlobParent::OpenStreamRunnable MOZ_FINAL
  : public nsRunnable
{
  friend class nsRevocableEventPtr<OpenStreamRunnable>;

  
  BlobParent* mBlobActor;
  InputStreamParent* mStreamActor;

  nsCOMPtr<nsIInputStream> mStream;
  nsCOMPtr<nsIIPCSerializableInputStream> mSerializable;
  nsCOMPtr<nsIEventTarget> mActorTarget;
  nsCOMPtr<nsIThread> mIOTarget;

  bool mRevoked;
  bool mClosing;

public:
  OpenStreamRunnable(BlobParent* aBlobActor,
                     InputStreamParent* aStreamActor,
                     nsIInputStream* aStream,
                     nsIIPCSerializableInputStream* aSerializable,
                     nsIThread* aIOTarget)
    : mBlobActor(aBlobActor)
    , mStreamActor(aStreamActor)
    , mStream(aStream)
    , mSerializable(aSerializable)
    , mIOTarget(aIOTarget)
    , mRevoked(false)
    , mClosing(false)
  {
    MOZ_ASSERT(aBlobActor);
    aBlobActor->AssertIsOnOwningThread();
    MOZ_ASSERT(aStreamActor);
    MOZ_ASSERT(aStream);
    
    MOZ_ASSERT(aIOTarget);

    if (!NS_IsMainThread()) {
      AssertIsOnBackgroundThread();

      mActorTarget = do_GetCurrentThread();
      MOZ_ASSERT(mActorTarget);
    }

    AssertIsOnOwningThread();
  }

  nsresult
  Dispatch()
  {
    AssertIsOnOwningThread();
    MOZ_ASSERT(mIOTarget);

    nsresult rv = mIOTarget->Dispatch(this, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_DECL_ISUPPORTS_INHERITED

private:
  ~OpenStreamRunnable()
  { }

  bool
  IsOnOwningThread() const
  {
    return EventTargetIsOnCurrentThread(mActorTarget);
  }

  void
  AssertIsOnOwningThread() const
  {
    MOZ_ASSERT(IsOnOwningThread());
  }

  void
  Revoke()
  {
    AssertIsOnOwningThread();
#ifdef DEBUG
    mBlobActor = nullptr;
    mStreamActor = nullptr;
#endif
    mRevoked = true;
  }

  nsresult
  OpenStream()
  {
    MOZ_ASSERT(!IsOnOwningThread());
    MOZ_ASSERT(mStream);

    if (!mSerializable) {
      nsCOMPtr<IPrivateRemoteInputStream> remoteStream =
        do_QueryInterface(mStream);
      MOZ_ASSERT(remoteStream, "Must QI to IPrivateRemoteInputStream here!");

      nsCOMPtr<nsIInputStream> realStream =
        remoteStream->BlockAndGetInternalStream();
      NS_ENSURE_TRUE(realStream, NS_ERROR_FAILURE);

      mSerializable = do_QueryInterface(realStream);
      if (!mSerializable) {
        MOZ_ASSERT(false, "Must be serializable!");
        return NS_ERROR_FAILURE;
      }

      mStream.swap(realStream);
    }

    
    
    uint64_t available;
    if (NS_FAILED(mStream->Available(&available))) {
      NS_WARNING("Available failed on this stream!");
    }

    if (mActorTarget) {
      nsresult rv = mActorTarget->Dispatch(this, NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(this)));
    }

    return NS_OK;
  }

  nsresult
  CloseStream()
  {
    MOZ_ASSERT(!IsOnOwningThread());
    MOZ_ASSERT(mStream);

    
    nsCOMPtr<nsIInputStream> stream;
    mStream.swap(stream);

    nsCOMPtr<nsIThread> ioTarget;
    mIOTarget.swap(ioTarget);

    NS_WARN_IF_FALSE(NS_SUCCEEDED(stream->Close()), "Failed to close stream!");

    nsCOMPtr<nsIRunnable> shutdownRunnable =
      NS_NewRunnableMethod(ioTarget, &nsIThread::Shutdown);
    MOZ_ASSERT(shutdownRunnable);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(shutdownRunnable)));

    return NS_OK;
  }

  nsresult
  SendResponse()
  {
    AssertIsOnOwningThread();
    MOZ_ASSERT(mStream);
    MOZ_ASSERT(mSerializable);
    MOZ_ASSERT(mIOTarget);
    MOZ_ASSERT(!mClosing);

    nsCOMPtr<nsIIPCSerializableInputStream> serializable;
    mSerializable.swap(serializable);

    if (mRevoked) {
      MOZ_ASSERT(!mBlobActor);
      MOZ_ASSERT(!mStreamActor);
    }
    else {
      MOZ_ASSERT(mBlobActor);
      MOZ_ASSERT(mBlobActor->HasManager());
      MOZ_ASSERT(mStreamActor);

      InputStreamParams params;
      nsTArray<FileDescriptor> fds;
      serializable->Serialize(params, fds);

      MOZ_ASSERT(params.type() != InputStreamParams::T__None);

      OptionalFileDescriptorSet optionalFDSet;
      if (nsIContentParent* contentManager = mBlobActor->GetContentManager()) {
        ConstructFileDescriptorSet(contentManager, fds, optionalFDSet);
      } else {
        ConstructFileDescriptorSet(mBlobActor->GetBackgroundManager(),
                                   fds,
                                   optionalFDSet);
      }

      mStreamActor->Destroy(params, optionalFDSet);

      mBlobActor->NoteRunnableCompleted(this);

#ifdef DEBUG
      mBlobActor = nullptr;
      mStreamActor = nullptr;
#endif
    }

    
    
    
    nsCOMPtr<nsIThread> kungFuDeathGrip = mIOTarget;

    mClosing = true;

    nsresult rv = mIOTarget->Dispatch(this, NS_DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mIOTarget);

    if (IsOnOwningThread()) {
      return SendResponse();
    }

    if (!mClosing) {
      return OpenStream();
    }

    return CloseStream();
  }
};

NS_IMPL_ISUPPORTS_INHERITED0(BlobParent::OpenStreamRunnable, nsRunnable)





class BlobChild::RemoteBlobImpl
  : public FileImplBase
  , public nsIRemoteBlob
{
protected:
  class CreateStreamHelper;

  BlobChild* mActor;
  nsCOMPtr<nsIEventTarget> mActorTarget;

  nsRefPtr<FileImpl> mSameProcessFileImpl;

  const bool mIsSlice;

public:
  
  RemoteBlobImpl(BlobChild* aActor,
                 const nsAString& aName,
                 const nsAString& aContentType,
                 uint64_t aLength,
                 uint64_t aModDate);

  
  RemoteBlobImpl(BlobChild* aActor,
                 const nsAString& aContentType,
                 uint64_t aLength);

  
  RemoteBlobImpl(BlobChild* aActor,
                 FileImpl* aSameProcessBlobImpl,
                 const nsAString& aName,
                 const nsAString& aContentType,
                 uint64_t aLength,
                 uint64_t aModDate);

  
  RemoteBlobImpl(BlobChild* aActor,
                 FileImpl* aSameProcessBlobImpl,
                 const nsAString& aContentType,
                 uint64_t aLength);

  
  explicit
  RemoteBlobImpl(BlobChild* aActor);

  void
  NoteDyingActor();

  BlobChild*
  GetActor() const
  {
    MOZ_ASSERT(ActorEventTargetIsOnCurrentThread());

    return mActor;
  }

  nsIEventTarget*
  GetActorEventTarget() const
  {
    return mActorTarget;
  }

  bool
  ActorEventTargetIsOnCurrentThread() const
  {
    return EventTargetIsOnCurrentThread(BaseRemoteBlobImpl()->mActorTarget);
  }

  bool
  IsSlice() const
  {
    return mIsSlice;
  }

  RemoteBlobSliceImpl*
  AsSlice() const;

  RemoteBlobImpl*
  BaseRemoteBlobImpl() const;

  NS_DECL_ISUPPORTS_INHERITED

  virtual void
  GetMozFullPathInternal(nsAString& aFileName, ErrorResult& aRv) MOZ_OVERRIDE;

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart,
              uint64_t aLength,
              const nsAString& aContentType,
              ErrorResult& aRv) MOZ_OVERRIDE;

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual int64_t
  GetFileId() MOZ_OVERRIDE;

  virtual int64_t GetLastModified(ErrorResult& aRv) MOZ_OVERRIDE;

  virtual BlobChild*
  GetBlobChild() MOZ_OVERRIDE;

  virtual BlobParent*
  GetBlobParent() MOZ_OVERRIDE;

protected:
  
  RemoteBlobImpl(const nsAString& aContentType, uint64_t aLength);

  ~RemoteBlobImpl()
  {
    MOZ_ASSERT_IF(mActorTarget,
                  EventTargetIsOnCurrentThread(mActorTarget));
  }

  void
  CommonInit(BlobChild* aActor);

  void
  Destroy();
};

class BlobChild::RemoteBlobImpl::CreateStreamHelper MOZ_FINAL
  : public nsRunnable
{
  Monitor mMonitor;
  nsRefPtr<RemoteBlobImpl> mRemoteBlobImpl;
  nsRefPtr<RemoteInputStream> mInputStream;
  const uint64_t mStart;
  const uint64_t mLength;
  bool mDone;

public:
  explicit CreateStreamHelper(RemoteBlobImpl* aRemoteBlobImpl);

  nsresult
  GetStream(nsIInputStream** aInputStream);

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIRUNNABLE

private:
  ~CreateStreamHelper()
  {
    MOZ_ASSERT(!mRemoteBlobImpl);
    MOZ_ASSERT(!mInputStream);
    MOZ_ASSERT(mDone);
  }

  void
  RunInternal(RemoteBlobImpl* aBaseRemoteBlobImpl, bool aNotify);
};

class BlobChild::RemoteBlobSliceImpl MOZ_FINAL
  : public RemoteBlobImpl
{
  nsRefPtr<RemoteBlobImpl> mParent;
  bool mActorWasCreated;

public:
  RemoteBlobSliceImpl(RemoteBlobImpl* aParent,
                      uint64_t aStart,
                      uint64_t aLength,
                      const nsAString& aContentType);

  RemoteBlobImpl*
  Parent() const
  {
    MOZ_ASSERT(mParent);

    return const_cast<RemoteBlobImpl*>(mParent.get());
  }

  uint64_t
  Start() const
  {
    return mStart;
  }

  NS_DECL_ISUPPORTS_INHERITED

  virtual BlobChild*
  GetBlobChild() MOZ_OVERRIDE;

private:
  ~RemoteBlobSliceImpl()
  { }
};





class BlobParent::RemoteBlobImpl MOZ_FINAL
  : public FileImpl
  , public nsIRemoteBlob
{
  BlobParent* mActor;
  nsCOMPtr<nsIEventTarget> mActorTarget;
  nsRefPtr<FileImpl> mBlobImpl;

public:
  RemoteBlobImpl(BlobParent* aActor, FileImpl* aBlobImpl);

  void
  NoteDyingActor();

  NS_DECL_ISUPPORTS_INHERITED

  virtual void
  GetName(nsAString& aName) MOZ_OVERRIDE;

  virtual nsresult
  GetPath(nsAString& aPath) MOZ_OVERRIDE;

  virtual int64_t
  GetLastModified(ErrorResult& aRv) MOZ_OVERRIDE;

  virtual void
  GetMozFullPath(nsAString& aName, ErrorResult& aRv) MOZ_OVERRIDE;

  virtual void
  GetMozFullPathInternal(nsAString& aFileName, ErrorResult& aRv) MOZ_OVERRIDE;

  virtual uint64_t
  GetSize(ErrorResult& aRv) MOZ_OVERRIDE;

  virtual void
  GetType(nsAString& aType) MOZ_OVERRIDE;

  virtual already_AddRefed<FileImpl>
  CreateSlice(uint64_t aStart,
              uint64_t aLength,
              const nsAString& aContentType,
              ErrorResult& aRv) MOZ_OVERRIDE;

  virtual const nsTArray<nsRefPtr<FileImpl>>*
  GetSubBlobImpls() const MOZ_OVERRIDE;

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual int64_t
  GetFileId() MOZ_OVERRIDE;

  virtual void
  AddFileInfo(FileInfo* aFileInfo) MOZ_OVERRIDE;

  virtual FileInfo*
  GetFileInfo(FileManager* aFileManager) MOZ_OVERRIDE;

  virtual nsresult
  GetSendInfo(nsIInputStream** aBody,
              uint64_t* aContentLength,
              nsACString& aContentType,
              nsACString& aCharset) MOZ_OVERRIDE;

  virtual nsresult
  GetMutable(bool* aMutable) const MOZ_OVERRIDE;

  virtual nsresult
  SetMutable(bool aMutable) MOZ_OVERRIDE;

  virtual void
  SetLazyData(const nsAString& aName,
              const nsAString& aContentType,
              uint64_t aLength,
              uint64_t aLastModifiedDate) MOZ_OVERRIDE;

  virtual bool
  IsMemoryFile() const MOZ_OVERRIDE;

  virtual bool
  IsSizeUnknown() const MOZ_OVERRIDE;

  virtual bool
  IsDateUnknown() const MOZ_OVERRIDE;

  virtual bool
  IsFile() const MOZ_OVERRIDE;

  virtual void
  Unlink() MOZ_OVERRIDE;

  virtual void
  Traverse(nsCycleCollectionTraversalCallback& aCallback) MOZ_OVERRIDE;

  virtual BlobChild*
  GetBlobChild() MOZ_OVERRIDE;

  virtual BlobParent*
  GetBlobParent() MOZ_OVERRIDE;

private:
  ~RemoteBlobImpl()
  {
    MOZ_ASSERT_IF(mActorTarget,
                  EventTargetIsOnCurrentThread(mActorTarget));
  }

  void
  Destroy();
};





BlobChild::
RemoteBlobImpl::RemoteBlobImpl(BlobChild* aActor,
                               const nsAString& aName,
                               const nsAString& aContentType,
                               uint64_t aLength,
                               uint64_t aModDate)
  : FileImplBase(aName, aContentType, aLength, aModDate)
  , mIsSlice(false)
{
  CommonInit(aActor);
}

BlobChild::
RemoteBlobImpl::RemoteBlobImpl(BlobChild* aActor,
                               const nsAString& aContentType,
                               uint64_t aLength)
  : FileImplBase(aContentType, aLength)
  , mIsSlice(false)
{
  CommonInit(aActor);
}

BlobChild::
RemoteBlobImpl::RemoteBlobImpl(BlobChild* aActor,
                               FileImpl* aSameProcessBlobImpl,
                               const nsAString& aName,
                               const nsAString& aContentType,
                               uint64_t aLength,
                               uint64_t aModDate)
  : FileImplBase(aName, aContentType, aLength, aModDate)
  , mSameProcessFileImpl(aSameProcessBlobImpl)
  , mIsSlice(false)
{
  MOZ_ASSERT(aSameProcessBlobImpl);
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

  CommonInit(aActor);
}

BlobChild::
RemoteBlobImpl::RemoteBlobImpl(BlobChild* aActor,
                               FileImpl* aSameProcessBlobImpl,
                               const nsAString& aContentType,
                               uint64_t aLength)
  : FileImplBase(aContentType, aLength)
  , mSameProcessFileImpl(aSameProcessBlobImpl)
  , mIsSlice(false)
{
  MOZ_ASSERT(aSameProcessBlobImpl);
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

  CommonInit(aActor);
}

BlobChild::
RemoteBlobImpl::RemoteBlobImpl(BlobChild* aActor)
  : FileImplBase(EmptyString(), EmptyString(), UINT64_MAX, UINT64_MAX)
  , mIsSlice(false)
{
  CommonInit(aActor);
}

BlobChild::
RemoteBlobImpl::RemoteBlobImpl(const nsAString& aContentType, uint64_t aLength)
  : FileImplBase(aContentType, aLength)
  , mActor(nullptr)
  , mIsSlice(true)
{
  mImmutable = true;
}

void
BlobChild::
RemoteBlobImpl::CommonInit(BlobChild* aActor)
{
  MOZ_ASSERT(aActor);
  aActor->AssertIsOnOwningThread();
  MOZ_ASSERT(!mIsSlice);

  mActor = aActor;
  mActorTarget = aActor->EventTarget();

  mImmutable = true;
}

void
BlobChild::
RemoteBlobImpl::NoteDyingActor()
{
  MOZ_ASSERT(mActor);
  mActor->AssertIsOnOwningThread();

  mActor = nullptr;
}

BlobChild::RemoteBlobSliceImpl*
BlobChild::
RemoteBlobImpl::AsSlice() const
{
  MOZ_ASSERT(IsSlice());

  return static_cast<RemoteBlobSliceImpl*>(const_cast<RemoteBlobImpl*>(this));
}

BlobChild::RemoteBlobImpl*
BlobChild::
RemoteBlobImpl::BaseRemoteBlobImpl() const
{
  if (IsSlice()) {
    return AsSlice()->Parent()->BaseRemoteBlobImpl();
  }

  return const_cast<RemoteBlobImpl*>(this);
}

void
BlobChild::
RemoteBlobImpl::Destroy()
{
  if (EventTargetIsOnCurrentThread(mActorTarget)) {
    if (mActor) {
      mActor->AssertIsOnOwningThread();
      mActor->NoteDyingRemoteBlobImpl();
    }

    delete this;
    return;
  }

  nsCOMPtr<nsIRunnable> destroyRunnable =
    NS_NewNonOwningRunnableMethod(this, &RemoteBlobImpl::Destroy);

  if (mActorTarget) {
    destroyRunnable =
      new CancelableRunnableWrapper(destroyRunnable, mActorTarget);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mActorTarget->Dispatch(destroyRunnable,
                                                        NS_DISPATCH_NORMAL)));
  } else {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(destroyRunnable)));
  }
}

NS_IMPL_ADDREF(BlobChild::RemoteBlobImpl)
NS_IMPL_RELEASE_WITH_DESTROY(BlobChild::RemoteBlobImpl, Destroy())
NS_IMPL_QUERY_INTERFACE_INHERITED(BlobChild::RemoteBlobImpl,
                                  FileImpl,
                                  nsIRemoteBlob)

void
BlobChild::
RemoteBlobImpl::GetMozFullPathInternal(nsAString& aFilePath,
                                       ErrorResult& aRv)
{
  if (!EventTargetIsOnCurrentThread(mActorTarget)) {
    MOZ_CRASH("Not implemented!");
  }

  if (mSameProcessFileImpl) {
    MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

    mSameProcessFileImpl->GetMozFullPathInternal(aFilePath, aRv);
    return;
  }

  if (!mActor) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }

  nsString filePath;
  if (!mActor->SendGetFilePath(&filePath)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  aFilePath = filePath;
}

already_AddRefed<FileImpl>
BlobChild::
RemoteBlobImpl::CreateSlice(uint64_t aStart,
                            uint64_t aLength,
                            const nsAString& aContentType,
                            ErrorResult& aRv)
{
  
  if (mSameProcessFileImpl) {
    MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

    return mSameProcessFileImpl->CreateSlice(aStart,
                                             aLength,
                                             aContentType,
                                             aRv);
  }

  nsRefPtr<RemoteBlobSliceImpl> slice =
    new RemoteBlobSliceImpl(this, aStart, aLength, aContentType);
  return slice.forget();
}

nsresult
BlobChild::
RemoteBlobImpl::GetInternalStream(nsIInputStream** aStream)
{
  
  if (mSameProcessFileImpl) {
    MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

    nsCOMPtr<nsIInputStream> realStream;
    nsresult rv =
      mSameProcessFileImpl->GetInternalStream(getter_AddRefs(realStream));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsRefPtr<BlobInputStreamTether> tether =
      new BlobInputStreamTether(realStream, mSameProcessFileImpl);
    tether.forget(aStream);
    return NS_OK;
  }

  nsRefPtr<CreateStreamHelper> helper = new CreateStreamHelper(this);
  return helper->GetStream(aStream);
}

int64_t
BlobChild::
RemoteBlobImpl::GetFileId()
{
  if (!EventTargetIsOnCurrentThread(mActorTarget)) {
    MOZ_CRASH("Not implemented!");
  }

  if (mSameProcessFileImpl) {
    MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

    return mSameProcessFileImpl->GetFileId();
  }

  int64_t fileId;
  if (mActor && mActor->SendGetFileId(&fileId)) {
    return fileId;
  }

  return -1;
}

int64_t
BlobChild::
RemoteBlobImpl::GetLastModified(ErrorResult& aRv)
{
  if (IsDateUnknown()) {
    return 0;
  }

  return mLastModificationDate;
}

BlobChild*
BlobChild::
RemoteBlobImpl::GetBlobChild()
{
  return mActor;
}

BlobParent*
BlobChild::
RemoteBlobImpl::GetBlobParent()
{
  return nullptr;
}





BlobChild::RemoteBlobImpl::
CreateStreamHelper::CreateStreamHelper(RemoteBlobImpl* aRemoteBlobImpl)
  : mMonitor("BlobChild::RemoteBlobImpl::CreateStreamHelper::mMonitor")
  , mRemoteBlobImpl(aRemoteBlobImpl)
  , mStart(aRemoteBlobImpl->IsSlice() ? aRemoteBlobImpl->AsSlice()->Start() : 0)
  , mLength(0)
  , mDone(false)
{
  
  MOZ_ASSERT(aRemoteBlobImpl);

  ErrorResult rv;
  const_cast<uint64_t&>(mLength) = aRemoteBlobImpl->GetSize(rv);
  MOZ_ASSERT(!rv.Failed());
}

nsresult
BlobChild::RemoteBlobImpl::
CreateStreamHelper::GetStream(nsIInputStream** aInputStream)
{
  
  MOZ_ASSERT(aInputStream);
  MOZ_ASSERT(mRemoteBlobImpl);
  MOZ_ASSERT(!mInputStream);
  MOZ_ASSERT(!mDone);

  nsRefPtr<RemoteBlobImpl> baseRemoteBlobImpl =
    mRemoteBlobImpl->BaseRemoteBlobImpl();
  MOZ_ASSERT(baseRemoteBlobImpl);

  if (EventTargetIsOnCurrentThread(baseRemoteBlobImpl->GetActorEventTarget())) {
    RunInternal(baseRemoteBlobImpl, false);
  } else {
    nsCOMPtr<nsIEventTarget> target = baseRemoteBlobImpl->GetActorEventTarget();
    if (!target) {
      target = do_GetMainThread();
    }

    MOZ_ASSERT(target);

    nsresult rv = target->Dispatch(this, NS_DISPATCH_NORMAL);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    DebugOnly<bool> warned = false;

    {
      MonitorAutoLock lock(mMonitor);

      while (!mDone) {
#ifdef DEBUG
        if (!warned) {
          NS_WARNING("RemoteBlobImpl::GetInternalStream() called on thread "
                     "that can't send messages, blocking here to wait for the "
                     "actor's thread to send the message!");
        }
#endif
        lock.Wait();
      }
    }
  }

  MOZ_ASSERT(!mRemoteBlobImpl);
  MOZ_ASSERT(mDone);

  if (!mInputStream) {
    return NS_ERROR_UNEXPECTED;
  }

  mInputStream.forget(aInputStream);
  return NS_OK;
}

void
BlobChild::RemoteBlobImpl::
CreateStreamHelper::RunInternal(RemoteBlobImpl* aBaseRemoteBlobImpl,
                                bool aNotify)
{
  MOZ_ASSERT(aBaseRemoteBlobImpl);
  MOZ_ASSERT(aBaseRemoteBlobImpl->ActorEventTargetIsOnCurrentThread());
  MOZ_ASSERT(!mInputStream);
  MOZ_ASSERT(!mDone);

  if (BlobChild* actor = aBaseRemoteBlobImpl->GetActor()) {
    nsRefPtr<RemoteInputStream> stream;

    if (!NS_IsMainThread() && GetCurrentThreadWorkerPrivate()) {
      stream =
        new RemoteInputStream(actor, aBaseRemoteBlobImpl, mStart, mLength);
    } else {
      stream = new RemoteInputStream(aBaseRemoteBlobImpl);
    }

    InputStreamChild* streamActor = new InputStreamChild(stream);
    if (actor->SendPBlobStreamConstructor(streamActor, mStart, mLength)) {
      stream.swap(mInputStream);
    }
  }

  mRemoteBlobImpl = nullptr;

  if (aNotify) {
    MonitorAutoLock lock(mMonitor);
    mDone = true;
    lock.Notify();
  } else {
    mDone = true;
  }
}

NS_IMPL_ISUPPORTS_INHERITED0(BlobChild::RemoteBlobImpl::CreateStreamHelper,
                             nsRunnable)

NS_IMETHODIMP
BlobChild::RemoteBlobImpl::
CreateStreamHelper::Run()
{
  MOZ_ASSERT(mRemoteBlobImpl);
  MOZ_ASSERT(mRemoteBlobImpl->ActorEventTargetIsOnCurrentThread());

  nsRefPtr<RemoteBlobImpl> baseRemoteBlobImpl =
    mRemoteBlobImpl->BaseRemoteBlobImpl();
  MOZ_ASSERT(baseRemoteBlobImpl);

  RunInternal(baseRemoteBlobImpl, true);
  return NS_OK;
}





BlobChild::
RemoteBlobSliceImpl::RemoteBlobSliceImpl(RemoteBlobImpl* aParent,
                                         uint64_t aStart,
                                         uint64_t aLength,
                                         const nsAString& aContentType)
  : RemoteBlobImpl(aContentType, aLength)
  , mParent(aParent->BaseRemoteBlobImpl())
  , mActorWasCreated(false)
{
  MOZ_ASSERT(mParent);
  MOZ_ASSERT(mParent->BaseRemoteBlobImpl() == mParent);

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(aParent->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

#ifdef DEBUG
  {
    ErrorResult rv;
    uint64_t parentSize = aParent->GetSize(rv);
    MOZ_ASSERT(!rv.Failed());
    MOZ_ASSERT(parentSize >= aStart + aLength);
  }
#endif

  
  mStart = aParent->IsSlice() ? aParent->AsSlice()->mStart + aStart : aStart;
}

NS_IMPL_ISUPPORTS_INHERITED0(BlobChild::RemoteBlobSliceImpl,
                             BlobChild::RemoteBlobImpl)

BlobChild*
BlobChild::
RemoteBlobSliceImpl::GetBlobChild()
{
  MOZ_ASSERT_IF(!ActorEventTargetIsOnCurrentThread(),
                mActorWasCreated);

  if (mActorWasCreated) {
    return RemoteBlobImpl::GetBlobChild();
  }

  MOZ_ASSERT(ActorEventTargetIsOnCurrentThread());

  mActorWasCreated = true;

  BlobChild* baseActor = mParent->GetActor();
  MOZ_ASSERT(baseActor);
  MOZ_ASSERT(baseActor->HasManager());

  nsID id;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(gUUIDGenerator->GenerateUUIDInPlace(&id)));

  ParentBlobConstructorParams params(
    SlicedBlobConstructorParams(nullptr ,
                                baseActor ,
                                id ,
                                mStart ,
                                mStart + mLength ,
                                mContentType ));

  if (nsIContentChild* contentManager = baseActor->GetContentManager()) {
    mActor = SendSliceConstructor(contentManager, this, params);
  } else {
    mActor =
      SendSliceConstructor(baseActor->GetBackgroundManager(), this, params);
  }

  return mActor;
}





BlobParent::
RemoteBlobImpl::RemoteBlobImpl(BlobParent* aActor, FileImpl* aBlobImpl)
  : mActor(aActor)
  , mActorTarget(aActor->EventTarget())
  , mBlobImpl(aBlobImpl)
{
  MOZ_ASSERT(aActor);
  aActor->AssertIsOnOwningThread();
  MOZ_ASSERT(aBlobImpl);

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(aBlobImpl->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);
}

void
BlobParent::
RemoteBlobImpl::NoteDyingActor()
{
  MOZ_ASSERT(mActor);
  mActor->AssertIsOnOwningThread();

  mActor = nullptr;
}

void
BlobParent::
RemoteBlobImpl::Destroy()
{
  if (EventTargetIsOnCurrentThread(mActorTarget)) {
    if (mActor) {
      mActor->AssertIsOnOwningThread();
      mActor->NoteDyingRemoteBlobImpl();
    }

    delete this;
    return;
  }

  nsCOMPtr<nsIRunnable> destroyRunnable =
    NS_NewNonOwningRunnableMethod(this, &RemoteBlobImpl::Destroy);

  if (mActorTarget) {
    destroyRunnable =
      new CancelableRunnableWrapper(destroyRunnable, mActorTarget);

    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mActorTarget->Dispatch(destroyRunnable,
                                                        NS_DISPATCH_NORMAL)));
  } else {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(destroyRunnable)));
  }
}

NS_IMPL_ADDREF(BlobParent::RemoteBlobImpl)
NS_IMPL_RELEASE_WITH_DESTROY(BlobParent::RemoteBlobImpl, Destroy())
NS_IMPL_QUERY_INTERFACE_INHERITED(BlobParent::RemoteBlobImpl,
                                  FileImpl,
                                  nsIRemoteBlob)

void
BlobParent::
RemoteBlobImpl::GetName(nsAString& aName)
{
  mBlobImpl->GetName(aName);
}

nsresult
BlobParent::
RemoteBlobImpl::GetPath(nsAString& aPath)
{
  return mBlobImpl->GetPath(aPath);
}

int64_t
BlobParent::
RemoteBlobImpl::GetLastModified(ErrorResult& aRv)
{
  return mBlobImpl->GetLastModified(aRv);
}

void
BlobParent::
RemoteBlobImpl::GetMozFullPath(nsAString& aName, ErrorResult& aRv)
{
  mBlobImpl->GetMozFullPath(aName, aRv);
}

void
BlobParent::
RemoteBlobImpl::GetMozFullPathInternal(nsAString& aFileName, ErrorResult& aRv)
{
  mBlobImpl->GetMozFullPathInternal(aFileName, aRv);
}

uint64_t
BlobParent::
RemoteBlobImpl::GetSize(ErrorResult& aRv)
{
  return mBlobImpl->GetSize(aRv);
}

void
BlobParent::
RemoteBlobImpl::GetType(nsAString& aType)
{
  mBlobImpl->GetType(aType);
}

already_AddRefed<FileImpl>
BlobParent::
RemoteBlobImpl::CreateSlice(uint64_t aStart,
                            uint64_t aLength,
                            const nsAString& aContentType,
                            ErrorResult& aRv)
{
  return mBlobImpl->CreateSlice(aStart, aLength, aContentType, aRv);
}

const nsTArray<nsRefPtr<FileImpl>>*
BlobParent::
RemoteBlobImpl::GetSubBlobImpls() const
{
  return mBlobImpl->GetSubBlobImpls();
}

nsresult
BlobParent::
RemoteBlobImpl::GetInternalStream(nsIInputStream** aStream)
{
  return mBlobImpl->GetInternalStream(aStream);
}

int64_t
BlobParent::
RemoteBlobImpl::GetFileId()
{
  return mBlobImpl->GetFileId();
}

void
BlobParent::
RemoteBlobImpl::AddFileInfo(FileInfo* aFileInfo)
{
  return mBlobImpl->AddFileInfo(aFileInfo);
}

FileInfo*
BlobParent::
RemoteBlobImpl::GetFileInfo(FileManager* aFileManager)
{
  return mBlobImpl->GetFileInfo(aFileManager);
}

nsresult
BlobParent::
RemoteBlobImpl::GetSendInfo(nsIInputStream** aBody,
                            uint64_t* aContentLength,
                            nsACString& aContentType,
                            nsACString& aCharset)
{
  return mBlobImpl->GetSendInfo(aBody,
                                aContentLength,
                                aContentType,
                                aCharset);
}

nsresult
BlobParent::
RemoteBlobImpl::GetMutable(bool* aMutable) const
{
  return mBlobImpl->GetMutable(aMutable);
}

nsresult
BlobParent::
RemoteBlobImpl::SetMutable(bool aMutable)
{
  return mBlobImpl->SetMutable(aMutable);
}

void
BlobParent::
RemoteBlobImpl::SetLazyData(const nsAString& aName,
                            const nsAString& aContentType,
                            uint64_t aLength,
                            uint64_t aLastModifiedDate)
{
  MOZ_CRASH("This should never be called!");
}

bool
BlobParent::
RemoteBlobImpl::IsMemoryFile() const
{
  return mBlobImpl->IsMemoryFile();
}

bool
BlobParent::
RemoteBlobImpl::IsSizeUnknown() const
{
  return mBlobImpl->IsSizeUnknown();
}

bool
BlobParent::
RemoteBlobImpl::IsDateUnknown() const
{
  return mBlobImpl->IsDateUnknown();
}

bool
BlobParent::
RemoteBlobImpl::IsFile() const
{
  return mBlobImpl->IsFile();
}

void
BlobParent::
RemoteBlobImpl::Unlink()
{
  return mBlobImpl->Unlink();
}

void
BlobParent::
RemoteBlobImpl::Traverse(nsCycleCollectionTraversalCallback& aCallback)
{
  return mBlobImpl->Traverse(aCallback);
}

BlobChild*
BlobParent::
RemoteBlobImpl::GetBlobChild()
{
  return nullptr;
}

BlobParent*
BlobParent::
RemoteBlobImpl::GetBlobParent()
{
  return mActor;
}





BlobChild::BlobChild(nsIContentChild* aManager, FileImpl* aBlobImpl)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aBlobImpl);
}

BlobChild::BlobChild(PBackgroundChild* aManager, FileImpl* aBlobImpl)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  if (!NS_IsMainThread()) {
    mEventTarget = do_GetCurrentThread();
    MOZ_ASSERT(mEventTarget);
  }

  CommonInit(aBlobImpl);
}

BlobChild::BlobChild(nsIContentChild* aManager, BlobChild* aOther)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aOther,  nullptr);
}

BlobChild::BlobChild(PBackgroundChild* aManager,
                     BlobChild* aOther,
                     FileImpl* aBlobImpl)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(aBlobImpl);

  if (!NS_IsMainThread()) {
    mEventTarget = do_GetCurrentThread();
    MOZ_ASSERT(mEventTarget);
  }

  CommonInit(aOther, aBlobImpl);
}

BlobChild::BlobChild(nsIContentChild* aManager,
                     const ChildBlobConstructorParams& aParams)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aParams);
}

BlobChild::BlobChild(PBackgroundChild* aManager,
                     const ChildBlobConstructorParams& aParams)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  if (!NS_IsMainThread()) {
    mEventTarget = do_GetCurrentThread();
    MOZ_ASSERT(mEventTarget);
  }

  CommonInit(aParams);
}

BlobChild::BlobChild(nsIContentChild* aManager,
                     const nsID& aParentID,
                     RemoteBlobSliceImpl* aRemoteBlobSliceImpl)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aParentID, aRemoteBlobSliceImpl);
}

BlobChild::BlobChild(PBackgroundChild* aManager,
                     const nsID& aParentID,
                     RemoteBlobSliceImpl* aRemoteBlobSliceImpl)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  if (!NS_IsMainThread()) {
    mEventTarget = do_GetCurrentThread();
    MOZ_ASSERT(mEventTarget);
  }

  CommonInit(aParentID, aRemoteBlobSliceImpl);
}

BlobChild::~BlobChild()
{
  AssertIsOnOwningThread();

  MOZ_COUNT_DTOR(BlobChild);
}

void
BlobChild::CommonInit(FileImpl* aBlobImpl)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aBlobImpl);

  MOZ_COUNT_CTOR(BlobChild);

  mBlobImpl = aBlobImpl;
  mRemoteBlobImpl = nullptr;

  mBlobImpl->AddRef();
  mOwnsBlobImpl = true;

  memset(&mParentID, 0, sizeof(mParentID));
}

void
BlobChild::CommonInit(BlobChild* aOther, FileImpl* aBlobImpl)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aOther);
  MOZ_ASSERT_IF(mContentManager, aOther->GetBackgroundManager());
  MOZ_ASSERT_IF(mContentManager, !aBlobImpl);
  MOZ_ASSERT_IF(mBackgroundManager, aBlobImpl);

  nsRefPtr<FileImpl> otherImpl;
  if (mBackgroundManager && aOther->GetBackgroundManager()) {
    otherImpl = aBlobImpl;
  } else {
    otherImpl = aOther->GetBlobImpl();
  }
  MOZ_ASSERT(otherImpl);

  nsString contentType;
  otherImpl->GetType(contentType);

  ErrorResult rv;
  uint64_t length = otherImpl->GetSize(rv);
  MOZ_ASSERT(!rv.Failed());

  nsRefPtr<RemoteBlobImpl> remoteBlob;
  if (otherImpl->IsFile()) {
    nsString name;
    otherImpl->GetName(name);

    uint64_t modDate = otherImpl->GetLastModified(rv);
    MOZ_ASSERT(!rv.Failed());

    remoteBlob = new RemoteBlobImpl(this, name, contentType, length, modDate);
  } else {
    remoteBlob = new RemoteBlobImpl(this, contentType, length);
  }

  CommonInit(aOther->ParentID(), remoteBlob);
}

void
BlobChild::CommonInit(const ChildBlobConstructorParams& aParams)
{
  AssertIsOnOwningThread();

  MOZ_COUNT_CTOR(BlobChild);

  const AnyBlobConstructorParams& blobParams = aParams.blobParams();

  AnyBlobConstructorParams::Type paramsType = blobParams.type();
  MOZ_ASSERT(paramsType != AnyBlobConstructorParams::T__None &&
             paramsType !=
               AnyBlobConstructorParams::TSlicedBlobConstructorParams &&
             paramsType !=
               AnyBlobConstructorParams::TKnownBlobConstructorParams);

  nsRefPtr<RemoteBlobImpl> remoteBlob;

  switch (paramsType) {
    case AnyBlobConstructorParams::TNormalBlobConstructorParams: {
      const NormalBlobConstructorParams& params =
        blobParams.get_NormalBlobConstructorParams();
      remoteBlob =
        new RemoteBlobImpl(this, params.contentType(), params.length());
      break;
    }

    case AnyBlobConstructorParams::TFileBlobConstructorParams: {
      const FileBlobConstructorParams& params =
        blobParams.get_FileBlobConstructorParams();
      remoteBlob = new RemoteBlobImpl(this,
                                      params.name(),
                                      params.contentType(),
                                      params.length(),
                                      params.modDate());
      break;
    }

    case AnyBlobConstructorParams::TSameProcessBlobConstructorParams: {
      MOZ_ASSERT(gProcessType == GeckoProcessType_Default);

      const SameProcessBlobConstructorParams& params =
        blobParams.get_SameProcessBlobConstructorParams();
      MOZ_ASSERT(params.addRefedFileImpl());

      nsRefPtr<FileImpl> blobImpl =
        dont_AddRef(reinterpret_cast<FileImpl*>(params.addRefedFileImpl()));

      ErrorResult rv;
      uint64_t size = blobImpl->GetSize(rv);
      MOZ_ASSERT(!rv.Failed());

      nsString contentType;
      blobImpl->GetType(contentType);

      if (blobImpl->IsFile()) {
        nsString name;
        blobImpl->GetName(name);

        uint64_t lastModifiedDate = blobImpl->GetLastModified(rv);
        MOZ_ASSERT(!rv.Failed());

        remoteBlob =
          new RemoteBlobImpl(this,
                             blobImpl,
                             name,
                             contentType,
                             size,
                             lastModifiedDate);
      } else {
        remoteBlob = new RemoteBlobImpl(this, blobImpl, contentType, size);
      }

      break;
    }

    case AnyBlobConstructorParams::TMysteryBlobConstructorParams: {
      remoteBlob = new RemoteBlobImpl(this);
      break;
    }

    default:
      MOZ_CRASH("Unknown params!");
  }

  MOZ_ASSERT(remoteBlob);

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(remoteBlob->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

  mRemoteBlobImpl = remoteBlob;

  remoteBlob.forget(&mBlobImpl);
  mOwnsBlobImpl = true;

  mParentID = aParams.id();
}

void
BlobChild::CommonInit(const nsID& aParentID, RemoteBlobImpl* aRemoteBlobImpl)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aRemoteBlobImpl);

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(aRemoteBlobImpl->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

  MOZ_COUNT_CTOR(BlobChild);

  nsRefPtr<RemoteBlobImpl> remoteBlob = aRemoteBlobImpl;

  mRemoteBlobImpl = remoteBlob;

  remoteBlob.forget(&mBlobImpl);
  mOwnsBlobImpl = true;

  mParentID = aParentID;
}

#ifdef DEBUG

void
BlobChild::AssertIsOnOwningThread() const
{
  MOZ_ASSERT(IsOnOwningThread());
}

#endif 


void
BlobChild::Startup(const FriendKey& )
{
  MOZ_ASSERT(XRE_GetProcessType() != GeckoProcessType_Default);

  CommonStartup();
}


BlobChild*
BlobChild::GetOrCreate(nsIContentChild* aManager, FileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return GetOrCreateFromImpl(aManager, aBlobImpl);
}


BlobChild*
BlobChild::GetOrCreate(PBackgroundChild* aManager, FileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return GetOrCreateFromImpl(aManager, aBlobImpl);
}


BlobChild*
BlobChild::Create(nsIContentChild* aManager,
                  const ChildBlobConstructorParams& aParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return CreateFromParams(aManager, aParams);
}


BlobChild*
BlobChild::Create(PBackgroundChild* aManager,
                  const ChildBlobConstructorParams& aParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return CreateFromParams(aManager, aParams);
}


template <class ChildManagerType>
BlobChild*
BlobChild::GetOrCreateFromImpl(ChildManagerType* aManager,
                               FileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(aBlobImpl);

  
  
  if (nsCOMPtr<nsIRemoteBlob> remoteBlob = do_QueryInterface(aBlobImpl)) {
    BlobChild* actor =
      MaybeGetActorFromRemoteBlob(remoteBlob, aManager, aBlobImpl);
    if (actor) {
      return actor;
    }
  }

  
  if (NS_WARN_IF(NS_FAILED(aBlobImpl->SetMutable(false)))) {
    return nullptr;
  }

  MOZ_ASSERT(!aBlobImpl->IsSizeUnknown());
  MOZ_ASSERT(!aBlobImpl->IsDateUnknown());

  AnyBlobConstructorParams blobParams;

  nsCOMPtr<nsIInputStream> snapshotInputStream;

  if (gProcessType == GeckoProcessType_Default) {
    nsCOMPtr<PIFileImplSnapshot> snapshot = do_QueryInterface(aBlobImpl);
    if (snapshot) {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        aBlobImpl->GetInternalStream(getter_AddRefs(snapshotInputStream))));
    }
  }

  if (gProcessType == GeckoProcessType_Default && !snapshotInputStream) {
    nsRefPtr<FileImpl> sameProcessImpl = aBlobImpl;
    auto addRefedFileImpl =
      reinterpret_cast<intptr_t>(sameProcessImpl.forget().take());

    blobParams = SameProcessBlobConstructorParams(addRefedFileImpl);
  } else {
    BlobData blobData;
    if (snapshotInputStream) {
      blobData =
        reinterpret_cast<intptr_t>(snapshotInputStream.forget().take());
    } else {
      BlobDataFromBlobImpl(aBlobImpl, blobData);
    }

    nsString contentType;
    aBlobImpl->GetType(contentType);

    ErrorResult rv;
    uint64_t length = aBlobImpl->GetSize(rv);
    MOZ_ASSERT(!rv.Failed());

    if (aBlobImpl->IsFile()) {
      nsString name;
      aBlobImpl->GetName(name);

      uint64_t modDate = aBlobImpl->GetLastModified(rv);
      MOZ_ASSERT(!rv.Failed());

      blobParams =
        FileBlobConstructorParams(name, contentType, length, modDate, blobData);
    } else {
      blobParams = NormalBlobConstructorParams(contentType, length, blobData);
    }
  }

  BlobChild* actor = new BlobChild(aManager, aBlobImpl);

  ParentBlobConstructorParams params(blobParams);

  if (NS_WARN_IF(!aManager->SendPBlobConstructor(actor, params))) {
    BlobChild::Destroy(actor);
    return nullptr;
  }

  return actor;
}


template <class ChildManagerType>
BlobChild*
BlobChild::CreateFromParams(ChildManagerType* aManager,
                            const ChildBlobConstructorParams& aParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  const AnyBlobConstructorParams& blobParams = aParams.blobParams();

  switch (blobParams.type()) {
    case AnyBlobConstructorParams::TNormalBlobConstructorParams:
    case AnyBlobConstructorParams::TFileBlobConstructorParams:
    case AnyBlobConstructorParams::TSameProcessBlobConstructorParams:
    case AnyBlobConstructorParams::TMysteryBlobConstructorParams: {
      return new BlobChild(aManager, aParams);
    }

    case AnyBlobConstructorParams::TSlicedBlobConstructorParams: {
      MOZ_CRASH("Parent should never send SlicedBlobConstructorParams!");
    }

    case AnyBlobConstructorParams::TKnownBlobConstructorParams: {
      MOZ_CRASH("Parent should never send KnownBlobConstructorParams!");
    }

    default:
      MOZ_CRASH("Unknown params!");
  }

  MOZ_CRASH("Should never get here!");
}


template <class ChildManagerType>
BlobChild*
BlobChild::SendSliceConstructor(ChildManagerType* aManager,
                                RemoteBlobSliceImpl* aRemoteBlobSliceImpl,
                                const ParentBlobConstructorParams& aParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(aRemoteBlobSliceImpl);
  MOZ_ASSERT(aParams.blobParams().type() ==
               AnyBlobConstructorParams::TSlicedBlobConstructorParams);

  const nsID& id = aParams.blobParams().get_SlicedBlobConstructorParams().id();

  BlobChild* newActor = new BlobChild(aManager, id, aRemoteBlobSliceImpl);

  if (aManager->SendPBlobConstructor(newActor, aParams)) {
    if (gProcessType != GeckoProcessType_Default || !NS_IsMainThread()) {
      newActor->SendWaitForSliceCreation();
    }
    return newActor;
  }

  BlobChild::Destroy(newActor);
  return nullptr;
}


BlobChild*
BlobChild::MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                                       nsIContentChild* aManager,
                                       FileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aRemoteBlob);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(aBlobImpl);

  if (BlobChild* actor = aRemoteBlob->GetBlobChild()) {
    if (actor->GetContentManager() == aManager) {
      return actor;
    }

    MOZ_ASSERT(actor->GetBackgroundManager());

    actor = new BlobChild(aManager, actor);

    ParentBlobConstructorParams params(
      KnownBlobConstructorParams(actor->ParentID()));

    aManager->SendPBlobConstructor(actor, params);

    return actor;
  }

  return nullptr;
}


BlobChild*
BlobChild::MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                                       PBackgroundChild* aManager,
                                       FileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aRemoteBlob);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(aBlobImpl);

  if (BlobChild* actor = aRemoteBlob->GetBlobChild()) {
    if (actor->GetBackgroundManager() == aManager) {
      return actor;
    }

    actor = new BlobChild(aManager, actor, aBlobImpl);

    ParentBlobConstructorParams params(
      KnownBlobConstructorParams(actor->ParentID()));

    aManager->SendPBlobConstructor(actor, params);

    return actor;
  }

  return nullptr;
}

const nsID&
BlobChild::ParentID() const
{
  MOZ_ASSERT(mRemoteBlobImpl);

  return mParentID;
}

already_AddRefed<FileImpl>
BlobChild::GetBlobImpl()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);

  nsRefPtr<FileImpl> blobImpl;

  
  
  
  if (mRemoteBlobImpl && mOwnsBlobImpl) {
    blobImpl = dont_AddRef(mBlobImpl);
    mOwnsBlobImpl = false;
  } else {
    blobImpl = mBlobImpl;
  }

  MOZ_ASSERT(blobImpl);

  return blobImpl.forget();
}

bool
BlobChild::SetMysteryBlobInfo(const nsString& aName,
                              const nsString& aContentType,
                              uint64_t aLength,
                              uint64_t aLastModifiedDate)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(mRemoteBlobImpl);
  MOZ_ASSERT(aLastModifiedDate != UINT64_MAX);

  mBlobImpl->SetLazyData(aName, aContentType, aLength, aLastModifiedDate);

  FileBlobConstructorParams params(aName,
                                   aContentType,
                                   aLength,
                                   aLastModifiedDate,
                                   void_t() );
  return SendResolveMystery(params);
}

bool
BlobChild::SetMysteryBlobInfo(const nsString& aContentType, uint64_t aLength)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(mRemoteBlobImpl);

  nsString voidString;
  voidString.SetIsVoid(true);

  mBlobImpl->SetLazyData(voidString, aContentType, aLength, UINT64_MAX);

  NormalBlobConstructorParams params(aContentType,
                                     aLength,
                                     void_t() );
  return SendResolveMystery(params);
}

void
BlobChild::NoteDyingRemoteBlobImpl()
{
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(mRemoteBlobImpl);
  MOZ_ASSERT(!mOwnsBlobImpl);

  
  
  
  if (!IsOnOwningThread()) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewNonOwningRunnableMethod(this, &BlobChild::NoteDyingRemoteBlobImpl);

    if (mEventTarget) {
      runnable = new CancelableRunnableWrapper(runnable, mEventTarget);

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mEventTarget->Dispatch(runnable,
                                                          NS_DISPATCH_NORMAL)));
    } else {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
    }

    return;
  }

  
  
  mBlobImpl = nullptr;
  mRemoteBlobImpl = nullptr;

  PBlobChild::Send__delete__(this);
}

bool
BlobChild::IsOnOwningThread() const
{
  return EventTargetIsOnCurrentThread(mEventTarget);
}

void
BlobChild::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnOwningThread();

  if (mRemoteBlobImpl) {
    mRemoteBlobImpl->NoteDyingActor();
  }

  if (mBlobImpl && mOwnsBlobImpl) {
    mBlobImpl->Release();
  }

#ifdef DEBUG
  mBlobImpl = nullptr;
  mRemoteBlobImpl = nullptr;
  mBackgroundManager = nullptr;
  mContentManager = nullptr;
  mOwnsBlobImpl = false;
#endif
}

PBlobStreamChild*
BlobChild::AllocPBlobStreamChild(const uint64_t& aStart,
                                 const uint64_t& aLength)
{
  AssertIsOnOwningThread();

  return new InputStreamChild();
}

bool
BlobChild::DeallocPBlobStreamChild(PBlobStreamChild* aActor)
{
  AssertIsOnOwningThread();

  delete static_cast<InputStreamChild*>(aActor);
  return true;
}





BlobParent::BlobParent(nsIContentParent* aManager, IDTableEntry* aIDTableEntry)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aIDTableEntry);
}

BlobParent::BlobParent(PBackgroundParent* aManager, IDTableEntry* aIDTableEntry)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
  , mEventTarget(do_GetCurrentThread())
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(mEventTarget);

  CommonInit(aIDTableEntry);
}

BlobParent::BlobParent(nsIContentParent* aManager,
                       FileImpl* aBlobImpl,
                       IDTableEntry* aIDTableEntry)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aBlobImpl, aIDTableEntry);
}

BlobParent::BlobParent(PBackgroundParent* aManager,
                       FileImpl* aBlobImpl,
                       IDTableEntry* aIDTableEntry)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
  , mEventTarget(do_GetCurrentThread())
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(mEventTarget);

  CommonInit(aBlobImpl, aIDTableEntry);
}

BlobParent::~BlobParent()
{
  AssertIsOnOwningThread();

  MOZ_COUNT_DTOR(BlobParent);
}

void
BlobParent::CommonInit(IDTableEntry* aIDTableEntry)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aIDTableEntry);
  MOZ_ASSERT(aIDTableEntry->BlobImpl());

  MOZ_COUNT_CTOR(BlobParent);

  mBlobImpl = aIDTableEntry->BlobImpl();
  mRemoteBlobImpl = nullptr;

  mBlobImpl->AddRef();
  mOwnsBlobImpl = true;

  mIDTableEntry = aIDTableEntry;
}

void
BlobParent::CommonInit(FileImpl* aBlobImpl, IDTableEntry* aIDTableEntry)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aBlobImpl);
  MOZ_ASSERT(aIDTableEntry);

  MOZ_COUNT_CTOR(BlobParent);

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(aBlobImpl->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

  nsRefPtr<RemoteBlobImpl> remoteBlobImpl = new RemoteBlobImpl(this, aBlobImpl);

  MOZ_ASSERT(NS_SUCCEEDED(remoteBlobImpl->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

  mRemoteBlobImpl = remoteBlobImpl;

  remoteBlobImpl.forget(&mBlobImpl);
  mOwnsBlobImpl = true;

  mIDTableEntry = aIDTableEntry;
}

#ifdef DEBUG

void
BlobParent::AssertIsOnOwningThread() const
{
  MOZ_ASSERT(IsOnOwningThread());
}

#endif 


void
BlobParent::Startup(const FriendKey& )
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);

  CommonStartup();

  ClearOnShutdown(&sIDTable);

  sIDTableMutex = new Mutex("BlobParent::sIDTableMutex");
  ClearOnShutdown(&sIDTableMutex);
}


BlobParent*
BlobParent::GetOrCreate(nsIContentParent* aManager, FileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return GetOrCreateFromImpl(aManager, aBlobImpl);
}


BlobParent*
BlobParent::GetOrCreate(PBackgroundParent* aManager, FileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return GetOrCreateFromImpl(aManager, aBlobImpl);
}


BlobParent*
BlobParent::Create(nsIContentParent* aManager,
                   const ParentBlobConstructorParams& aParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return CreateFromParams(aManager, aParams);
}


BlobParent*
BlobParent::Create(PBackgroundParent* aManager,
                   const ParentBlobConstructorParams& aParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return CreateFromParams(aManager, aParams);
}


already_AddRefed<FileImpl>
BlobParent::GetBlobImplForID(const nsID& aID)
{
  if (NS_WARN_IF(gProcessType != GeckoProcessType_Default)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  nsRefPtr<IDTableEntry> idTableEntry = IDTableEntry::Get(aID);
  if (NS_WARN_IF(!idTableEntry)) {
    return nullptr;
  }

  nsRefPtr<FileImpl> blobImpl = idTableEntry->BlobImpl();
  MOZ_ASSERT(blobImpl);

  return blobImpl.forget();
}


template <class ParentManagerType>
BlobParent*
BlobParent::GetOrCreateFromImpl(ParentManagerType* aManager,
                                FileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(aBlobImpl);

  
  
  if (nsCOMPtr<nsIRemoteBlob> remoteBlob = do_QueryInterface(aBlobImpl)) {
    BlobParent* actor = MaybeGetActorFromRemoteBlob(remoteBlob, aManager);
    if (actor) {
      return actor;
    }
  }

  
  if (NS_WARN_IF(NS_FAILED(aBlobImpl->SetMutable(false)))) {
    return nullptr;
  }

  const bool isSameProcessActor = ActorManagerIsSameProcess(aManager);

  AnyBlobConstructorParams blobParams;

  bool isSnapshot;

  if (isSameProcessActor) {
    nsCOMPtr<PIFileImplSnapshot> snapshot = do_QueryInterface(aBlobImpl);
    isSnapshot = !!snapshot;
  } else {
    isSnapshot = false;
  }

  if (isSameProcessActor && !isSnapshot) {
    nsRefPtr<FileImpl> sameProcessImpl = aBlobImpl;
    auto addRefedFileImpl =
      reinterpret_cast<intptr_t>(sameProcessImpl.forget().take());

    blobParams = SameProcessBlobConstructorParams(addRefedFileImpl);
  } else {
    if (aBlobImpl->IsSizeUnknown() || aBlobImpl->IsDateUnknown()) {
      
      
      
      blobParams = MysteryBlobConstructorParams();
    } else {
      nsString contentType;
      aBlobImpl->GetType(contentType);

      ErrorResult rv;
      uint64_t length = aBlobImpl->GetSize(rv);
      MOZ_ASSERT(!rv.Failed());

      if (aBlobImpl->IsFile()) {
        nsString name;
        aBlobImpl->GetName(name);

        uint64_t modDate = aBlobImpl->GetLastModified(rv);
        MOZ_ASSERT(!rv.Failed());

        blobParams =
          FileBlobConstructorParams(name, contentType, length, modDate, void_t());
      } else {
        blobParams = NormalBlobConstructorParams(contentType, length, void_t());
      }
    }
  }

  nsID id;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(gUUIDGenerator->GenerateUUIDInPlace(&id)));

  nsRefPtr<IDTableEntry> idTableEntry =
    IDTableEntry::GetOrCreate(id, ActorManagerProcessID(aManager), aBlobImpl);
  MOZ_ASSERT(idTableEntry);

  BlobParent* actor = new BlobParent(aManager, idTableEntry);

  ChildBlobConstructorParams params(id, blobParams);
  if (NS_WARN_IF(!aManager->SendPBlobConstructor(actor, params))) {
    BlobParent::Destroy(actor);
    return nullptr;
  }

  return actor;
}


template <class ParentManagerType>
BlobParent*
BlobParent::CreateFromParams(ParentManagerType* aManager,
                             const ParentBlobConstructorParams& aParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  const AnyBlobConstructorParams& blobParams = aParams.blobParams();

  switch (blobParams.type()) {
    case AnyBlobConstructorParams::TMysteryBlobConstructorParams: {
      ASSERT_UNLESS_FUZZING();
      return nullptr;
    }

    case AnyBlobConstructorParams::TNormalBlobConstructorParams:
    case AnyBlobConstructorParams::TFileBlobConstructorParams: {
      const OptionalBlobData& optionalBlobData =
        blobParams.type() ==
          AnyBlobConstructorParams::TNormalBlobConstructorParams ?
        blobParams.get_NormalBlobConstructorParams().optionalBlobData() :
        blobParams.get_FileBlobConstructorParams().optionalBlobData();

      if (NS_WARN_IF(optionalBlobData.type() != OptionalBlobData::TBlobData)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      nsRefPtr<FileImpl> blobImpl =
        CreateBlobImpl(aParams,
                       optionalBlobData.get_BlobData(),
                       ActorManagerIsSameProcess(aManager));
      if (NS_WARN_IF(!blobImpl)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      nsID id;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(gUUIDGenerator->GenerateUUIDInPlace(&id)));

      nsRefPtr<IDTableEntry> idTableEntry =
        IDTableEntry::Create(id, ActorManagerProcessID(aManager), blobImpl);
      if (NS_WARN_IF(!idTableEntry)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      return new BlobParent(aManager, blobImpl, idTableEntry);
    }

    case AnyBlobConstructorParams::TSlicedBlobConstructorParams: {
      const SlicedBlobConstructorParams& params =
        blobParams.get_SlicedBlobConstructorParams();

      if (NS_WARN_IF(params.end() < params.begin())) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      auto* actor =
        const_cast<BlobParent*>(
          static_cast<const BlobParent*>(params.sourceParent()));
      MOZ_ASSERT(actor);

      nsRefPtr<FileImpl> source = actor->GetBlobImpl();
      MOZ_ASSERT(source);

      ErrorResult rv;
      nsRefPtr<FileImpl> slice =
        source->CreateSlice(params.begin(),
                            params.end() - params.begin(),
                            params.contentType(),
                            rv);
      if (NS_WARN_IF(rv.Failed())) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(slice->SetMutable(false)));

      nsRefPtr<IDTableEntry> idTableEntry =
        IDTableEntry::Create(params.id(),
                             ActorManagerProcessID(aManager),
                             slice);
      if (NS_WARN_IF(!idTableEntry)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      return new BlobParent(aManager, slice, idTableEntry);
    }

    case AnyBlobConstructorParams::TKnownBlobConstructorParams: {
      const KnownBlobConstructorParams& params =
        blobParams.get_KnownBlobConstructorParams();

      nsRefPtr<IDTableEntry> idTableEntry =
        IDTableEntry::Get(params.id(), ActorManagerProcessID(aManager));
      if (NS_WARN_IF(!idTableEntry)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      return new BlobParent(aManager, idTableEntry);
    }

    case AnyBlobConstructorParams::TSameProcessBlobConstructorParams: {
      if (NS_WARN_IF(!ActorManagerIsSameProcess(aManager))) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      const SameProcessBlobConstructorParams& params =
        blobParams.get_SameProcessBlobConstructorParams();

      nsRefPtr<FileImpl> blobImpl =
        dont_AddRef(reinterpret_cast<FileImpl*>(params.addRefedFileImpl()));
      MOZ_ASSERT(blobImpl);

      nsID id;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(gUUIDGenerator->GenerateUUIDInPlace(&id)));

      nsRefPtr<IDTableEntry> idTableEntry =
        IDTableEntry::Create(id, ActorManagerProcessID(aManager), blobImpl);
      MOZ_ASSERT(idTableEntry);

      return new BlobParent(aManager, blobImpl, idTableEntry);
    }

    default:
      MOZ_CRASH("Unknown params!");
  }

  MOZ_CRASH("Should never get here!");
}


template <class ParentManagerType>
BlobParent*
BlobParent::SendSliceConstructor(
                             ParentManagerType* aManager,
                             const ParentBlobConstructorParams& aParams,
                             const ChildBlobConstructorParams& aOtherSideParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  BlobParent* newActor = BlobParent::Create(aManager, aParams);
  MOZ_ASSERT(newActor);

  if (aManager->SendPBlobConstructor(newActor, aOtherSideParams)) {
    return newActor;
  }

  BlobParent::Destroy(newActor);
  return nullptr;
}


BlobParent*
BlobParent::MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                                        nsIContentParent* aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aRemoteBlob);
  MOZ_ASSERT(aManager);

  BlobParent* actor = aRemoteBlob->GetBlobParent();
  if (actor && actor->GetContentManager() == aManager) {
    return actor;
  }

  return nullptr;
}


BlobParent*
BlobParent::MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                                        PBackgroundParent* aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aRemoteBlob);
  MOZ_ASSERT(aManager);

  BlobParent* actor = aRemoteBlob->GetBlobParent();
  if (actor && actor->GetBackgroundManager() == aManager) {
    return actor;
  }

  return nullptr;
}

already_AddRefed<FileImpl>
BlobParent::GetBlobImpl()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);

  nsRefPtr<FileImpl> blobImpl;

  
  
  
  if (mRemoteBlobImpl && mOwnsBlobImpl) {
    blobImpl = dont_AddRef(mBlobImpl);
    mOwnsBlobImpl = false;
  } else {
    blobImpl = mBlobImpl;
  }

  MOZ_ASSERT(blobImpl);

  return blobImpl.forget();
}

void
BlobParent::NoteDyingRemoteBlobImpl()
{
  MOZ_ASSERT(mRemoteBlobImpl);
  MOZ_ASSERT(!mOwnsBlobImpl);

  
  
  
  if (!IsOnOwningThread()) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewNonOwningRunnableMethod(this, &BlobParent::NoteDyingRemoteBlobImpl);

    if (mEventTarget) {
      runnable = new CancelableRunnableWrapper(runnable, mEventTarget);

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mEventTarget->Dispatch(runnable,
                                                          NS_DISPATCH_NORMAL)));
    } else {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(runnable)));
    }

    return;
  }

  
  
  mBlobImpl = nullptr;
  mRemoteBlobImpl = nullptr;

  unused << PBlobParent::Send__delete__(this);
}

void
BlobParent::NoteRunnableCompleted(OpenStreamRunnable* aRunnable)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aRunnable);

  for (uint32_t count = mOpenStreamRunnables.Length(), index = 0;
       index < count;
       index++) {
    nsRevocableEventPtr<OpenStreamRunnable>& runnable =
      mOpenStreamRunnables[index];

    if (runnable.get() == aRunnable) {
      runnable.Forget();
      mOpenStreamRunnables.RemoveElementAt(index);
      return;
    }
  }

  MOZ_CRASH("Runnable not in our array!");
}

bool
BlobParent::IsOnOwningThread() const
{
  return EventTargetIsOnCurrentThread(mEventTarget);
}

void
BlobParent::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnOwningThread();

  if (mRemoteBlobImpl) {
    mRemoteBlobImpl->NoteDyingActor();
  }

  if (mBlobImpl && mOwnsBlobImpl) {
    mBlobImpl->Release();
  }

#ifdef DEBUG
  mBlobImpl = nullptr;
  mRemoteBlobImpl = nullptr;
  mBackgroundManager = nullptr;
  mContentManager = nullptr;
  mOwnsBlobImpl = false;
#endif
}

PBlobStreamParent*
BlobParent::AllocPBlobStreamParent(const uint64_t& aStart,
                                   const uint64_t& aLength)
{
  AssertIsOnOwningThread();

  if (NS_WARN_IF(mRemoteBlobImpl)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  return new InputStreamParent();
}

bool
BlobParent::RecvPBlobStreamConstructor(PBlobStreamParent* aActor,
                                       const uint64_t& aStart,
                                       const uint64_t& aLength)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(!mRemoteBlobImpl);
  MOZ_ASSERT(mOwnsBlobImpl);

  auto* actor = static_cast<InputStreamParent*>(aActor);

  
  if (NS_WARN_IF(UINT64_MAX - aLength < aStart)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  ErrorResult errorResult;
  uint64_t blobLength = mBlobImpl->GetSize(errorResult);
  MOZ_ASSERT(!errorResult.Failed());

  if (NS_WARN_IF(aStart + aLength > blobLength)) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  nsRefPtr<FileImpl> blobImpl;

  if (!aStart && aLength == blobLength) {
    blobImpl = mBlobImpl;
  } else {
    nsString type;
    mBlobImpl->GetType(type);

    blobImpl = mBlobImpl->CreateSlice(aStart, aLength, type, errorResult);
    if (NS_WARN_IF(errorResult.Failed())) {
      return false;
    }
  }

  nsCOMPtr<nsIInputStream> stream;
  errorResult = blobImpl->GetInternalStream(getter_AddRefs(stream));
  if (NS_WARN_IF(errorResult.Failed())) {
    return false;
  }

  
  
  if (mBlobImpl->IsMemoryFile()) {
    InputStreamParams params;
    nsTArray<FileDescriptor> fds;
    SerializeInputStream(stream, params, fds);

    MOZ_ASSERT(params.type() != InputStreamParams::T__None);
    MOZ_ASSERT(fds.IsEmpty());

    return actor->Destroy(params, void_t());
  }

  nsCOMPtr<nsIRemoteBlob> remoteBlob = do_QueryInterface(mBlobImpl);
  nsCOMPtr<IPrivateRemoteInputStream> remoteStream;
  if (remoteBlob) {
    remoteStream = do_QueryInterface(stream);
  }

  
  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIIPCSerializableInputStream> serializableStream;
  if (!remoteBlob ||
      remoteBlob->GetBlobParent() == this ||
      !remoteStream) {
    serializableStream = do_QueryInterface(stream);
    if (!serializableStream) {
      MOZ_ASSERT(false, "Must be serializable!");
      return false;
    }
  }

  nsCOMPtr<nsIThread> target;
  errorResult = NS_NewNamedThread("Blob Opener", getter_AddRefs(target));
  if (NS_WARN_IF(errorResult.Failed())) {
    return false;
  }

  nsRefPtr<OpenStreamRunnable> runnable =
    new OpenStreamRunnable(this, actor, stream, serializableStream, target);

  errorResult = runnable->Dispatch();
  if (NS_WARN_IF(errorResult.Failed())) {
    return false;
  }

  
  *mOpenStreamRunnables.AppendElement() = runnable;
  return true;
}

bool
BlobParent::DeallocPBlobStreamParent(PBlobStreamParent* aActor)
{
  AssertIsOnOwningThread();

  delete static_cast<InputStreamParent*>(aActor);
  return true;
}

bool
BlobParent::RecvResolveMystery(const ResolveMysteryParams& aParams)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aParams.type() != ResolveMysteryParams::T__None);
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(!mRemoteBlobImpl);
  MOZ_ASSERT(mOwnsBlobImpl);

  switch (aParams.type()) {
    case ResolveMysteryParams::TNormalBlobConstructorParams: {
      const NormalBlobConstructorParams& params =
        aParams.get_NormalBlobConstructorParams();

      if (NS_WARN_IF(params.length() == UINT64_MAX)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }

      nsString voidString;
      voidString.SetIsVoid(true);

      mBlobImpl->SetLazyData(voidString,
                             params.contentType(),
                             params.length(),
                             UINT64_MAX);
      return true;
    }

    case ResolveMysteryParams::TFileBlobConstructorParams: {
      const FileBlobConstructorParams& params =
        aParams.get_FileBlobConstructorParams();
      if (NS_WARN_IF(params.name().IsVoid())) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }

      if (NS_WARN_IF(params.length() == UINT64_MAX)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }

      if (NS_WARN_IF(params.modDate() == UINT64_MAX)) {
        ASSERT_UNLESS_FUZZING();
        return false;
      }

      mBlobImpl->SetLazyData(params.name(),
                             params.contentType(),
                             params.length(),
                             params.modDate());
      return true;
    }

    default:
      MOZ_CRASH("Unknown params!");
  }

  MOZ_CRASH("Should never get here!");
}

bool
BlobParent::RecvBlobStreamSync(const uint64_t& aStart,
                               const uint64_t& aLength,
                               InputStreamParams* aParams,
                               OptionalFileDescriptorSet* aFDs)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(!mRemoteBlobImpl);
  MOZ_ASSERT(mOwnsBlobImpl);

  bool finished = false;

  {
    
    
    auto* streamActor = new InputStreamParent(&finished, aParams, aFDs);

    if (NS_WARN_IF(!RecvPBlobStreamConstructor(streamActor, aStart, aLength))) {
      
      
      delete streamActor;
      return false;
    }
  }

  if (finished) {
    
    return true;
  }

  
  
  nsIThread* currentThread = NS_GetCurrentThread();
  MOZ_ASSERT(currentThread);

  while (!finished) {
    MOZ_ALWAYS_TRUE(NS_ProcessNextEvent(currentThread));
  }

  return true;
}

bool
BlobParent::RecvWaitForSliceCreation()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(mOwnsBlobImpl);

  
  
  MOZ_ASSERT(mIDTableEntry);

#ifdef DEBUG
  {
    MOZ_ASSERT(sIDTableMutex);
    MutexAutoLock lock(*sIDTableMutex);

    MOZ_ASSERT(sIDTable);
    MOZ_ASSERT(sIDTable->Contains(mIDTableEntry->ID()));
  }
#endif

  return true;
}

bool
BlobParent::RecvGetFileId(int64_t* aFileId)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(!mRemoteBlobImpl);
  MOZ_ASSERT(mOwnsBlobImpl);

  if (NS_WARN_IF(!IndexedDatabaseManager::InTestingMode())) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }

  *aFileId = mBlobImpl->GetFileId();
  return true;
}

bool
BlobParent::RecvGetFilePath(nsString* aFilePath)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(!mRemoteBlobImpl);
  MOZ_ASSERT(mOwnsBlobImpl);

  
#ifdef MOZ_CHILD_PERMISSIONS
  if (NS_WARN_IF(!IndexedDatabaseManager::InTestingMode())) {
    ASSERT_UNLESS_FUZZING();
    return false;
  }
#endif

  nsString filePath;
  ErrorResult rv;
  mBlobImpl->GetMozFullPathInternal(filePath, rv);
  if (NS_WARN_IF(rv.Failed())) {
    return false;
  }

  *aFilePath = filePath;
  return true;
}





BlobParent::
IDTableEntry::IDTableEntry(const nsID& aID,
                           intptr_t aProcessID,
                           FileImpl* aBlobImpl)
  : mID(aID)
  , mProcessID(aProcessID)
  , mBlobImpl(aBlobImpl)
{
  MOZ_ASSERT(aBlobImpl);
}

BlobParent::
IDTableEntry::~IDTableEntry()
{
  MOZ_ASSERT(sIDTableMutex);
  sIDTableMutex->AssertNotCurrentThreadOwns();
  MOZ_ASSERT(sIDTable);

  {
    MutexAutoLock lock(*sIDTableMutex);
    MOZ_ASSERT(sIDTable->Get(mID) == this);

    sIDTable->Remove(mID);

    if (!sIDTable->Count()) {
      sIDTable = nullptr;
    }
  }
}


already_AddRefed<BlobParent::IDTableEntry>
BlobParent::
IDTableEntry::GetOrCreateInternal(const nsID& aID,
                                  intptr_t aProcessID,
                                  FileImpl* aBlobImpl,
                                  bool aMayCreate,
                                  bool aMayGet,
                                  bool aIgnoreProcessID)
{
  MOZ_ASSERT(gProcessType == GeckoProcessType_Default);
  MOZ_ASSERT(sIDTableMutex);
  sIDTableMutex->AssertNotCurrentThreadOwns();

  nsRefPtr<IDTableEntry> entry;

  {
    MutexAutoLock lock(*sIDTableMutex);

    if (!sIDTable) {
      if (NS_WARN_IF(!aMayCreate)) {
        return nullptr;
      }

      sIDTable = new IDTable();
    }

    entry = sIDTable->Get(aID);

    if (entry) {
      MOZ_ASSERT_IF(aBlobImpl, entry->BlobImpl() == aBlobImpl);

      if (NS_WARN_IF(!aMayGet)) {
        return nullptr;
      }

      if (!aIgnoreProcessID && NS_WARN_IF(entry->mProcessID != aProcessID)) {
        return nullptr;
      }
    } else {
      if (NS_WARN_IF(!aMayCreate)) {
        return nullptr;
      }

      MOZ_ASSERT(aBlobImpl);

      entry = new IDTableEntry(aID, aProcessID, aBlobImpl);

      sIDTable->Put(aID, entry);
    }
  }

  MOZ_ASSERT(entry);

  return entry.forget();
}





bool
InputStreamChild::Recv__delete__(const InputStreamParams& aParams,
                                 const OptionalFileDescriptorSet& aOptionalSet)
{
  MOZ_ASSERT(mRemoteStream);
  mRemoteStream->AssertIsOnOwningThread();

  nsTArray<FileDescriptor> fds;
  OptionalFileDescriptorSetToFDs(
    
    const_cast<OptionalFileDescriptorSet&>(aOptionalSet),
    fds);

  nsCOMPtr<nsIInputStream> stream = DeserializeInputStream(aParams, fds);
  MOZ_ASSERT(stream);

  mRemoteStream->SetStream(stream);
  return true;
}

} 
} 
