



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
#include "mozilla/dom/nsIContentParent.h"
#include "mozilla/dom/nsIContentChild.h"
#include "mozilla/dom/PBlobStreamChild.h"
#include "mozilla/dom/PBlobStreamParent.h"
#include "mozilla/dom/indexedDB/IndexedDatabaseManager.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "mozilla/ipc/PBackgroundParent.h"
#include "mozilla/ipc/PFileDescriptorSetParent.h"
#include "nsDataHashtable.h"
#include "nsDOMFile.h"
#include "nsHashKeys.h"
#include "nsID.h"
#include "nsIDOMFile.h"
#include "nsIInputStream.h"
#include "nsIIPCSerializableInputStream.h"
#include "nsIMultiplexInputStream.h"
#include "nsIRemoteBlob.h"
#include "nsISeekableStream.h"
#include "nsIUUIDGenerator.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsXULAppAPI.h"

#ifdef DEBUG
#include "BackgroundChild.h" 
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

namespace {

const char kUUIDGeneratorContractId[] = "@mozilla.org/uuid-generator;1";

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
EventTargetIsOnCurrentThread(nsIEventTarget* aEventTarget)
{
  if (!aEventTarget) {
    return NS_IsMainThread();
  }

  bool current;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aEventTarget->IsOnCurrentThread(&current)));

  return current;
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
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aTarget->Dispatch(releaseRunnable,
                                                   NS_DISPATCH_NORMAL)));
  } else {
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(releaseRunnable)));
  }
}

template <class ManagerType>
PFileDescriptorSetParent*
ConstructFileDescriptorSet(ManagerType* aManager,
                           const nsTArray<FileDescriptor>& aFDs)
{
  typedef typename ConcreteManagerTypeTraits<ManagerType>::Type
          ConcreteManagerType;

  MOZ_ASSERT(aManager);

  if (aFDs.IsEmpty()) {
    return nullptr;
  }

  auto* concreteManager = static_cast<ConcreteManagerType*>(aManager);

  PFileDescriptorSetParent* fdSet =
    concreteManager->SendPFileDescriptorSetConstructor(aFDs[0]);
  if (!fdSet) {
    return nullptr;
  }

  for (uint32_t index = 1; index < aFDs.Length(); index++) {
    if (!fdSet->SendAddFileDescriptor(aFDs[index])) {
      return nullptr;
    }
  }

  return fdSet;
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
  nsRefPtr<DOMFileImpl> mBlobImpl;

  nsIMultiplexInputStream* mWeakMultiplexStream;
  nsISeekableStream* mWeakSeekableStream;
  nsIIPCSerializableInputStream* mWeakSerializableStream;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_FORWARD_NSIINPUTSTREAM(mStream->)
  NS_FORWARD_SAFE_NSIMULTIPLEXINPUTSTREAM(mWeakMultiplexStream)
  NS_FORWARD_SAFE_NSISEEKABLESTREAM(mWeakSeekableStream)
  NS_FORWARD_SAFE_NSIIPCSERIALIZABLEINPUTSTREAM(mWeakSerializableStream)

  BlobInputStreamTether(nsIInputStream* aStream, DOMFileImpl* aBlobImpl)
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
  nsCOMPtr<nsIInputStream> mStream;
  nsRefPtr<DOMFileImpl> mBlobImpl;
  nsCOMPtr<nsIEventTarget> mEventTarget;
  nsISeekableStream* mWeakSeekableStream;

public:
  NS_DECL_THREADSAFE_ISUPPORTS

  RemoteInputStream(DOMFileImpl* aBlobImpl)
    : mMonitor("RemoteInputStream.mMonitor")
    , mBlobImpl(aBlobImpl)
    , mWeakSeekableStream(nullptr)
  {
    MOZ_ASSERT(IsOnOwningThread());
    MOZ_ASSERT(aBlobImpl);

    if (!NS_IsMainThread()) {
      mEventTarget = do_GetCurrentThread();
      MOZ_ASSERT(mEventTarget);
    }
  }

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

  void
  Serialize(InputStreamParams& aParams,
            FileDescriptorArray& )
  {
    nsCOMPtr<nsIRemoteBlob> remote = do_QueryInterface(mBlobImpl);
    MOZ_ASSERT(remote);
    MOZ_ASSERT(remote->GetBlobChild());

    aParams = RemoteInputStreamParams(
      nullptr ,
      remote->GetBlobChild() );
  }

  bool
  Deserialize(const InputStreamParams& aParams,
              const FileDescriptorArray& )
  {
    
    
    MOZ_CRASH("RemoteInputStream should never be deserialized");
  }

  void
  SetStream(nsIInputStream* aStream)
  {
    AssertIsOnOwningThread();
    MOZ_ASSERT(aStream);

    nsCOMPtr<nsIInputStream> stream = aStream;
    nsCOMPtr<nsISeekableStream> seekableStream = do_QueryInterface(aStream);

    MOZ_ASSERT_IF(seekableStream, SameCOMIdentity(aStream, seekableStream));

    {
      MonitorAutoLock lock(mMonitor);

      MOZ_ASSERT(!mStream);
      MOZ_ASSERT(!mWeakSeekableStream);

      mStream.swap(stream);
      mWeakSeekableStream = seekableStream;

      mMonitor.Notify();
    }
  }

  NS_IMETHOD
  Close() MOZ_OVERRIDE
  {
    nsresult rv = BlockAndWaitForStream();
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<DOMFileImpl> blobImpl;
    mBlobImpl.swap(blobImpl);

    rv = mStream->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD
  Available(uint64_t* aAvailable) MOZ_OVERRIDE
  {
    
    if (IsOnOwningThread()) {
      return NS_BASE_STREAM_CLOSED;
    }

    nsresult rv = BlockAndWaitForStream();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStream->Available(aAvailable);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD
  Read(char* aBuffer, uint32_t aCount, uint32_t* aResult) MOZ_OVERRIDE
  {
    nsresult rv = BlockAndWaitForStream();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStream->Read(aBuffer, aCount, aResult);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD
  ReadSegments(nsWriteSegmentFun aWriter, void* aClosure, uint32_t aCount,
               uint32_t* aResult) MOZ_OVERRIDE
  {
    nsresult rv = BlockAndWaitForStream();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStream->ReadSegments(aWriter, aClosure, aCount, aResult);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD
  IsNonBlocking(bool* aNonBlocking) MOZ_OVERRIDE
  {
    NS_ENSURE_ARG_POINTER(aNonBlocking);

    *aNonBlocking = false;
    return NS_OK;
  }

  NS_IMETHOD
  Seek(int32_t aWhence, int64_t aOffset) MOZ_OVERRIDE
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

  NS_IMETHOD
  Tell(int64_t* aResult) MOZ_OVERRIDE
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

  NS_IMETHOD
  SetEOF() MOZ_OVERRIDE
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

  virtual nsIInputStream*
  BlockAndGetInternalStream() MOZ_OVERRIDE
  {
    MOZ_ASSERT(!IsOnOwningThread());

    nsresult rv = BlockAndWaitForStream();
    NS_ENSURE_SUCCESS(rv, nullptr);

    return mStream;
  }

private:
  ~RemoteInputStream()
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
  ReallyBlockAndWaitForStream()
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

  nsresult
  BlockAndWaitForStream()
  {
    if (IsOnOwningThread()) {
      NS_WARNING("Blocking the owning thread is not supported!");
      return NS_ERROR_FAILURE;
    }

    ReallyBlockAndWaitForStream();

    return NS_OK;
  }

  bool
  IsSeekableStream()
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
};

NS_IMPL_ADDREF(RemoteInputStream)
NS_IMPL_RELEASE(RemoteInputStream)

NS_INTERFACE_MAP_BEGIN(RemoteInputStream)
  NS_INTERFACE_MAP_ENTRY(nsIInputStream)
  NS_INTERFACE_MAP_ENTRY(nsIIPCSerializableInputStream)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsISeekableStream, IsSeekableStream())
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIInputStream)
  NS_INTERFACE_MAP_ENTRY(IPrivateRemoteInputStream)
NS_INTERFACE_MAP_END

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
public:
  InputStreamParent()
  { }

  ~InputStreamParent()
  { }

private:
  
  virtual void
  ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE
  {
    
  }
};

} 

StaticAutoPtr<BlobParent::IDTable> BlobParent::sIDTable;
StaticAutoPtr<Mutex> BlobParent::sIDTableMutex;

class BlobParent::IDTableEntry MOZ_FINAL
{
  const nsID mID;
  const intptr_t mProcessID;
  const nsRefPtr<DOMFileImpl> mBlobImpl;

public:
  static already_AddRefed<IDTableEntry>
  Create(const nsID& aID, intptr_t aProcessID, DOMFileImpl* aBlobImpl)
  {
    MOZ_ASSERT(aBlobImpl);

    DebugOnly<bool> isMutable;
    MOZ_ASSERT(NS_SUCCEEDED(aBlobImpl->GetMutable(&isMutable)));
    MOZ_ASSERT(!isMutable);

    return GetOrCreateInternal(aID,
                               aProcessID,
                               aBlobImpl,
                                true,
                                false);
  }

  static already_AddRefed<IDTableEntry>
  Get(const nsID& aID, intptr_t aProcessID)
  {
    return GetOrCreateInternal(aID,
                               aProcessID,
                               nullptr,
                                false,
                                true);
  }

  static already_AddRefed<IDTableEntry>
  GetOrCreate(const nsID& aID, intptr_t aProcessID, DOMFileImpl* aBlobImpl)
  {
    MOZ_ASSERT(aBlobImpl);

    DebugOnly<bool> isMutable;
    MOZ_ASSERT(NS_SUCCEEDED(aBlobImpl->GetMutable(&isMutable)));
    MOZ_ASSERT(!isMutable);

    return GetOrCreateInternal(aID,
                               aProcessID,
                               aBlobImpl,
                                true,
                                true);
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

  DOMFileImpl*
  BlobImpl() const
  {
    return mBlobImpl;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(IDTableEntry)

private:
  IDTableEntry(const nsID& aID, intptr_t aProcessID, DOMFileImpl* aBlobImpl);
  ~IDTableEntry();

  static already_AddRefed<IDTableEntry>
  GetOrCreateInternal(const nsID& aID,
                      intptr_t aProcessID,
                      DOMFileImpl* aBlobImpl,
                      bool aMayCreate,
                      bool aMayGet);
};







class BlobParent::OpenStreamRunnable MOZ_FINAL
  : public nsRunnable
{
  friend class nsRevocableEventPtr<OpenStreamRunnable>;

  
  BlobParent* mBlobActor;
  PBlobStreamParent* mStreamActor;

  nsCOMPtr<nsIInputStream> mStream;
  nsCOMPtr<nsIIPCSerializableInputStream> mSerializable;
  nsCOMPtr<nsIEventTarget> mActorTarget;
  nsCOMPtr<nsIThread> mIOTarget;

  bool mRevoked;
  bool mClosing;

public:
  OpenStreamRunnable(BlobParent* aBlobActor,
                     PBlobStreamParent* aStreamActor,
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
      nsAutoTArray<FileDescriptor, 10> fds;
      serializable->Serialize(params, fds);

      MOZ_ASSERT(params.type() != InputStreamParams::T__None);

      PFileDescriptorSetParent* fdSet;
      if (nsIContentParent* contentManager = mBlobActor->GetContentManager()) {
        fdSet = ConstructFileDescriptorSet(contentManager, fds);
      } else {
        fdSet = ConstructFileDescriptorSet(mBlobActor->GetBackgroundManager(),
                                           fds);
      }

      OptionalFileDescriptorSet optionalFDs;
      if (fdSet) {
        optionalFDs = fdSet;
      } else {
        optionalFDs = void_t();
      }

      unused <<
        PBlobStreamParent::Send__delete__(mStreamActor, params, optionalFDs);

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





class BlobChild::RemoteBlobImpl MOZ_FINAL
  : public DOMFileImplBase
  , public nsIRemoteBlob
{
  class StreamHelper;
  class SliceHelper;

  BlobChild* mActor;
  nsCOMPtr<nsIEventTarget> mActorTarget;

public:
  RemoteBlobImpl(BlobChild* aActor,
                 const nsAString& aName,
                 const nsAString& aContentType,
                 uint64_t aLength,
                 uint64_t aModDate)
    : DOMFileImplBase(aName, aContentType, aLength, aModDate)
  {
    CommonInit(aActor);
  }

  RemoteBlobImpl(BlobChild* aActor,
                 const nsAString& aContentType,
                 uint64_t aLength)
    : DOMFileImplBase(aContentType, aLength)
  {
    CommonInit(aActor);
  }

  explicit
  RemoteBlobImpl(BlobChild* aActor)
    : DOMFileImplBase(EmptyString(), EmptyString(), UINT64_MAX, UINT64_MAX)
  {
    CommonInit(aActor);
  }

  void
  NoteDyingActor()
  {
    MOZ_ASSERT(mActor);
    mActor->AssertIsOnOwningThread();

    mActor = nullptr;
  }

  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult
  GetMozFullPathInternal(nsAString &aFilePath) MOZ_OVERRIDE;

  virtual already_AddRefed<DOMFileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength, const nsAString& aContentType)
              MOZ_OVERRIDE;

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual int64_t
  GetFileId() MOZ_OVERRIDE;

  virtual nsresult
  GetLastModifiedDate(JSContext* cx,
                      JS::MutableHandle<JS::Value> aLastModifiedDate)
                      MOZ_OVERRIDE;

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
  CommonInit(BlobChild* aActor)
  {
    MOZ_ASSERT(aActor);
    aActor->AssertIsOnOwningThread();

    mActor = aActor;
    mActorTarget = aActor->EventTarget();

    mImmutable = true;
  }

  void
  Destroy()
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
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mActorTarget->Dispatch(destroyRunnable,
                                                          NS_DISPATCH_NORMAL)));
    } else {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(destroyRunnable)));
    }
  }
};

class BlobChild::RemoteBlobImpl::StreamHelper MOZ_FINAL
  : public nsRunnable
{
  Monitor mMonitor;
  BlobChild* mActor;
  nsRefPtr<DOMFileImpl> mBlobImpl;
  nsRefPtr<RemoteInputStream> mInputStream;
  bool mDone;

public:
  StreamHelper(BlobChild* aActor, DOMFileImpl* aBlobImpl)
    : mMonitor("BlobChild::RemoteBlobImpl::StreamHelper::mMonitor")
    , mActor(aActor)
    , mBlobImpl(aBlobImpl)
    , mDone(false)
  {
    
    MOZ_ASSERT(aActor);
    MOZ_ASSERT(aBlobImpl);
  }

  nsresult
  GetStream(nsIInputStream** aInputStream)
  {
    
    MOZ_ASSERT(aInputStream);
    MOZ_ASSERT(mActor);
    MOZ_ASSERT(!mInputStream);
    MOZ_ASSERT(!mDone);

    if (mActor->IsOnOwningThread()) {
      RunInternal(false);
    } else {
      nsCOMPtr<nsIEventTarget> target = mActor->EventTarget();
      if (!target) {
        target = do_GetMainThread();
      }

      MOZ_ASSERT(target);

      nsresult rv = target->Dispatch(this, NS_DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, rv);

      {
        MonitorAutoLock lock(mMonitor);
        while (!mDone) {
          lock.Wait();
        }
      }
    }

    MOZ_ASSERT(!mActor);
    MOZ_ASSERT(mDone);

    if (!mInputStream) {
      return NS_ERROR_UNEXPECTED;
    }

    mInputStream.forget(aInputStream);
    return NS_OK;
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mActor);
    mActor->AssertIsOnOwningThread();

    RunInternal(true);
    return NS_OK;
  }

private:
  void
  RunInternal(bool aNotify)
  {
    MOZ_ASSERT(mActor);
    mActor->AssertIsOnOwningThread();
    MOZ_ASSERT(!mInputStream);
    MOZ_ASSERT(!mDone);

    nsRefPtr<RemoteInputStream> stream = new RemoteInputStream(mBlobImpl);

    InputStreamChild* streamActor = new InputStreamChild(stream);
    if (mActor->SendPBlobStreamConstructor(streamActor)) {
      stream.swap(mInputStream);
    }

    mActor = nullptr;

    if (aNotify) {
      MonitorAutoLock lock(mMonitor);
      mDone = true;
      lock.Notify();
    }
    else {
      mDone = true;
    }
  }
};

class BlobChild::RemoteBlobImpl::SliceHelper MOZ_FINAL
  : public nsRunnable
{
  Monitor mMonitor;
  BlobChild* mActor;
  nsRefPtr<DOMFileImpl> mSlice;
  uint64_t mStart;
  uint64_t mLength;
  nsString mContentType;
  bool mDone;

public:
  explicit
  SliceHelper(BlobChild* aActor)
    : mMonitor("BlobChild::RemoteBlobImpl::SliceHelper::mMonitor")
    , mActor(aActor)
    , mStart(0)
    , mLength(0)
    , mDone(false)
  {
    
    MOZ_ASSERT(aActor);
  }

  DOMFileImpl*
  GetSlice(uint64_t aStart,
           uint64_t aLength,
           const nsAString& aContentType)
  {
    
    MOZ_ASSERT(mActor);
    MOZ_ASSERT(!mSlice);
    MOZ_ASSERT(!mDone);

    mStart = aStart;
    mLength = aLength;
    mContentType = aContentType;

    if (mActor->IsOnOwningThread()) {
      RunInternal(false);
    } else {
      nsCOMPtr<nsIEventTarget> target = mActor->EventTarget();
      if (!target) {
        target = do_GetMainThread();
      }

      MOZ_ASSERT(target);

      nsresult rv = target->Dispatch(this, NS_DISPATCH_NORMAL);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return nullptr;
      }

      {
        MonitorAutoLock lock(mMonitor);
        while (!mDone) {
          lock.Wait();
        }
      }
    }

    MOZ_ASSERT(!mActor);
    MOZ_ASSERT(mDone);

    if (NS_WARN_IF(!mSlice)) {
      return nullptr;
    }

    return mSlice;
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mActor);
    mActor->AssertIsOnOwningThread();

    RunInternal(true);
    return NS_OK;
  }

private:
  void
  RunInternal(bool aNotify)
  {
    MOZ_ASSERT(mActor);
    mActor->AssertIsOnOwningThread();
    MOZ_ASSERT(!mSlice);
    MOZ_ASSERT(!mDone);

    NS_ENSURE_TRUE_VOID(mActor->HasManager());

    nsID id;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(gUUIDGenerator->GenerateUUIDInPlace(&id)));

    ChildBlobConstructorParams params(
      id,
      NormalBlobConstructorParams(mContentType ,
                                  mLength ));

    ParentBlobConstructorParams otherSideParams(
      SlicedBlobConstructorParams(nullptr ,
                                  mActor ,
                                  id ,
                                  mStart ,
                                  mStart + mLength ,
                                  mContentType ),
      void_t() );

    BlobChild* newActor;
    if (nsIContentChild* contentManager = mActor->GetContentManager()) {
      newActor = SendSliceConstructor(contentManager, params, otherSideParams);
    } else {
      newActor = SendSliceConstructor(mActor->GetBackgroundManager(),
                                      params,
                                      otherSideParams);
    }

    if (newActor) {
      mSlice = newActor->GetBlobImpl();
    }

    mActor = nullptr;

    if (aNotify) {
      MonitorAutoLock lock(mMonitor);
      mDone = true;
      lock.Notify();
    }
    else {
      mDone = true;
    }
  }
};





NS_IMPL_ADDREF(BlobChild::RemoteBlobImpl)
NS_IMPL_RELEASE_WITH_DESTROY(BlobChild::RemoteBlobImpl, Destroy())
NS_IMPL_QUERY_INTERFACE_INHERITED(BlobChild::RemoteBlobImpl,
                                  DOMFileImpl,
                                  nsIRemoteBlob)

nsresult
BlobChild::
RemoteBlobImpl::GetMozFullPathInternal(nsAString &aFilePath)
{
  if (!mActor) {
    return NS_ERROR_UNEXPECTED;
  }

  nsString filePath;
  if (!mActor->SendGetFilePath(&filePath)) {
    return NS_ERROR_FAILURE;
  }

  aFilePath = filePath;
  return NS_OK;
}

already_AddRefed<DOMFileImpl>
BlobChild::
RemoteBlobImpl::CreateSlice(uint64_t aStart,
                            uint64_t aLength,
                            const nsAString& aContentType)
{
  if (!mActor) {
    return nullptr;
  }

  nsRefPtr<SliceHelper> helper = new SliceHelper(mActor);

  nsRefPtr<DOMFileImpl> impl = helper->GetSlice(aStart, aLength, aContentType);
  if (NS_WARN_IF(!impl)) {
    return nullptr;
  }

  return impl.forget();
}

nsresult
BlobChild::
RemoteBlobImpl::GetInternalStream(nsIInputStream** aStream)
{
  if (!mActor) {
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<StreamHelper> helper = new StreamHelper(mActor, this);
  return helper->GetStream(aStream);
}

int64_t
BlobChild::
RemoteBlobImpl::GetFileId()
{
  int64_t fileId;
  if (mActor && mActor->SendGetFileId(&fileId)) {
    return fileId;
  }

  return -1;
}

nsresult
BlobChild::
RemoteBlobImpl::GetLastModifiedDate(
                                 JSContext* cx,
                                 JS::MutableHandle<JS::Value> aLastModifiedDate)
{
  if (IsDateUnknown()) {
    aLastModifiedDate.setNull();
  } else {
    JSObject* date = JS_NewDateObjectMsec(cx, mLastModificationDate);
    if (!date) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aLastModifiedDate.setObject(*date);
  }
  return NS_OK;
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





BlobChild::BlobChild(nsIContentChild* aManager, DOMFileImpl* aBlobImpl)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aBlobImpl);
}

BlobChild::BlobChild(PBackgroundChild* aManager, DOMFileImpl* aBlobImpl)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aBlobImpl);

  if (!NS_IsMainThread()) {
    mEventTarget = do_GetCurrentThread();
    MOZ_ASSERT(mEventTarget);
  }
}

BlobChild::BlobChild(nsIContentChild* aManager, BlobChild* aOther)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aOther);
}

BlobChild::BlobChild(PBackgroundChild* aManager, BlobChild* aOther)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aOther);

  if (!NS_IsMainThread()) {
    mEventTarget = do_GetCurrentThread();
    MOZ_ASSERT(mEventTarget);
  }
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

  CommonInit(aParams);

  if (!NS_IsMainThread()) {
    mEventTarget = do_GetCurrentThread();
    MOZ_ASSERT(mEventTarget);
  }
}

BlobChild::~BlobChild()
{
  AssertIsOnOwningThread();

  MOZ_COUNT_DTOR(BlobChild);
}

void
BlobChild::CommonInit(DOMFileImpl* aBlobImpl)
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
BlobChild::CommonInit(BlobChild* aOther)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aOther);
  MOZ_ASSERT_IF(mContentManager, aOther->GetBackgroundManager());
  MOZ_ASSERT_IF(mBackgroundManager, aOther->GetContentManager());

  MOZ_COUNT_CTOR(BlobChild);

  nsRefPtr<DOMFileImpl> otherImpl = aOther->GetBlobImpl();
  MOZ_ASSERT(otherImpl);

  nsString contentType;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(otherImpl->GetType(contentType)));

  uint64_t length;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(otherImpl->GetSize(&length)));

  nsRefPtr<RemoteBlobImpl> remoteBlob;
  if (otherImpl->IsFile()) {
    nsString name;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(otherImpl->GetName(name)));

    uint64_t modDate;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(otherImpl->GetMozLastModifiedDate(&modDate)));

    remoteBlob = new RemoteBlobImpl(this, name, contentType, length, modDate);
  } else {
    remoteBlob = new RemoteBlobImpl(this, contentType, length);
  }

  MOZ_ASSERT(remoteBlob);

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(remoteBlob->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

  mRemoteBlobImpl = remoteBlob;

  remoteBlob.forget(&mBlobImpl);
  mOwnsBlobImpl = true;

  mParentID = aOther->ParentID();
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
BlobChild::GetOrCreate(nsIContentChild* aManager, DOMFileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return GetOrCreateFromImpl(aManager, aBlobImpl);
}


BlobChild*
BlobChild::GetOrCreate(PBackgroundChild* aManager, DOMFileImpl* aBlobImpl)
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
                               DOMFileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(aBlobImpl);

  
  
  if (nsCOMPtr<nsIRemoteBlob> remoteBlob = do_QueryInterface(aBlobImpl)) {
    BlobChild* actor = MaybeGetActorFromRemoteBlob(remoteBlob, aManager);
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

  nsString contentType;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aBlobImpl->GetType(contentType)));

  uint64_t length;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aBlobImpl->GetSize(&length)));

  nsCOMPtr<nsIInputStream> stream;
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
    aBlobImpl->GetInternalStream(getter_AddRefs(stream))));

  if (aBlobImpl->IsFile()) {
    nsString name;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aBlobImpl->GetName(name)));

    uint64_t modDate;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aBlobImpl->GetMozLastModifiedDate(&modDate)));

    blobParams = FileBlobConstructorParams(name, contentType, length, modDate);
  } else {
    blobParams = NormalBlobConstructorParams(contentType, length);
  }

  InputStreamParams inputStreamParams;

  nsTArray<FileDescriptor> fds;
  SerializeInputStream(stream, inputStreamParams, fds);

  MOZ_ASSERT(inputStreamParams.type() != InputStreamParams::T__None);
  MOZ_ASSERT(fds.IsEmpty());

  BlobChild* actor = new BlobChild(aManager, aBlobImpl);

  ParentBlobConstructorParams params(blobParams, inputStreamParams);

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
    case AnyBlobConstructorParams::TMysteryBlobConstructorParams: {
      return new BlobChild(aManager, aParams);
    }

    case AnyBlobConstructorParams::TSlicedBlobConstructorParams: {
      const SlicedBlobConstructorParams& params =
        blobParams.get_SlicedBlobConstructorParams();
      MOZ_ASSERT(params.optionalID().type() == OptionalID::Tvoid_t);

      auto* actor =
        const_cast<BlobChild*>(
          static_cast<const BlobChild*>(params.sourceChild()));
      MOZ_ASSERT(actor);

      nsRefPtr<DOMFileImpl> source = actor->GetBlobImpl();
      MOZ_ASSERT(source);

      nsRefPtr<DOMFileImpl> slice;
      if (NS_WARN_IF(NS_FAILED(source->Slice(params.begin(),
                                             params.end(),
                                             params.contentType(),
                                             3,
                                             getter_AddRefs(slice))))) {
        return nullptr;
      }

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(slice->SetMutable(false)));

      actor = new BlobChild(aManager, slice);

      actor->mParentID = aParams.id();

      return actor;
    }

    case AnyBlobConstructorParams::TKnownBlobConstructorParams: {
      MOZ_CRASH("Parent should never send this type!");
    }

    default:
      MOZ_CRASH("Unknown params!");
  }

  MOZ_CRASH("Should never get here!");
}


template <class ChildManagerType>
BlobChild*
BlobChild::SendSliceConstructor(
                            ChildManagerType* aManager,
                            const ChildBlobConstructorParams& aParams,
                            const ParentBlobConstructorParams& aOtherSideParams)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  BlobChild* newActor = BlobChild::Create(aManager, aParams);
  MOZ_ASSERT(newActor);

  if (aManager->SendPBlobConstructor(newActor, aOtherSideParams)) {
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
                                       nsIContentChild* aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aRemoteBlob);
  MOZ_ASSERT(aManager);

  if (BlobChild* actor = aRemoteBlob->GetBlobChild()) {
    if (actor->GetContentManager() == aManager) {
      return actor;
    }

    MOZ_ASSERT(actor->GetBackgroundManager());

    actor = new BlobChild(aManager, actor);

    ParentBlobConstructorParams params(
      KnownBlobConstructorParams(actor->ParentID()) ,
      void_t() );

    aManager->SendPBlobConstructor(actor, params);

    return actor;
  }

  return nullptr;
}


BlobChild*
BlobChild::MaybeGetActorFromRemoteBlob(nsIRemoteBlob* aRemoteBlob,
                                       PBackgroundChild* aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aRemoteBlob);
  MOZ_ASSERT(aManager);

  if (BlobChild* actor = aRemoteBlob->GetBlobChild()) {
    if (actor->GetBackgroundManager() == aManager) {
      return actor;
    }

    MOZ_ASSERT(actor->GetContentManager());

    actor = new BlobChild(aManager, actor);

    ParentBlobConstructorParams params(
      KnownBlobConstructorParams(actor->ParentID()) ,
      void_t() );

    aManager->SendPBlobConstructor(actor, params);

    return actor;
  }

  return nullptr;
}

const nsID&
BlobChild::ParentID() const
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mRemoteBlobImpl);

  return mParentID;
}

already_AddRefed<DOMFileImpl>
BlobChild::GetBlobImpl()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);

  nsRefPtr<DOMFileImpl> blobImpl;

  
  
  
  if (mRemoteBlobImpl && mOwnsBlobImpl) {
    blobImpl = dont_AddRef(mBlobImpl);
    mOwnsBlobImpl = false;
  } else {
    blobImpl = mBlobImpl;
  }

  MOZ_ASSERT(blobImpl);

  return blobImpl.forget();
}

already_AddRefed<nsIDOMBlob>
BlobChild::GetBlob()
{
  AssertIsOnOwningThread();

  nsRefPtr<DOMFileImpl> blobImpl = GetBlobImpl();
  MOZ_ASSERT(blobImpl);

  nsCOMPtr<nsIDOMBlob> blob = new DOMFile(blobImpl);
  return blob.forget();
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

  FileBlobConstructorParams params(aName, aContentType, aLength,
                                   aLastModifiedDate);
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

  NormalBlobConstructorParams params(aContentType, aLength);
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
BlobChild::AllocPBlobStreamChild()
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





class BlobParent::RemoteBlobImplBase
  : public nsISupports
{
  friend class BlobParent;

  BlobParent* mActor;
  nsCOMPtr<nsIEventTarget> mActorTarget;
  bool mForwards;

protected:
  RemoteBlobImplBase(BlobParent* aActor, bool aForwards)
    : mActor(aActor)
    , mActorTarget(aActor->EventTarget())
    , mForwards(aForwards)
  {
    MOZ_ASSERT(aActor);
    aActor->AssertIsOnOwningThread();

    MOZ_COUNT_CTOR(BlobParent::RemoteBlobImplBase);
  }

  virtual
  ~RemoteBlobImplBase()
  {
    MOZ_ASSERT_IF(mActorTarget,
                  EventTargetIsOnCurrentThread(mActorTarget));

    MOZ_COUNT_DTOR(BlobParent::RemoteBlobImplBase);
  }

  void
  Destroy()
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
      NS_NewNonOwningRunnableMethod(this, &RemoteBlobImplBase::Destroy);

    if (mActorTarget) {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(mActorTarget->Dispatch(destroyRunnable,
                                                          NS_DISPATCH_NORMAL)));
    } else {
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(destroyRunnable)));
    }
  }

private:
  void
  NoteDyingActor()
  {
    MOZ_ASSERT(mActor);
    mActor->AssertIsOnOwningThread();

    mActor = nullptr;
  }
};

class BlobParent::RemoteBlobImpl MOZ_FINAL
  : public RemoteBlobImplBase
  , public DOMFileImplBase
  , public nsIRemoteBlob
{
  friend class mozilla::dom::BlobParent;

  class SliceHelper;

  InputStreamParams mInputStreamParams;

public:
  NS_DECL_ISUPPORTS_INHERITED

  virtual already_AddRefed<DOMFileImpl>
  CreateSlice(uint64_t aStart, uint64_t aLength, const nsAString& aContentType)
              MOZ_OVERRIDE;

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE;

  virtual nsresult
  GetLastModifiedDate(JSContext* cx,
                      JS::MutableHandle<JS::Value> aLastModifiedDate)
                      MOZ_OVERRIDE;

  virtual BlobChild*
  GetBlobChild() MOZ_OVERRIDE;

  virtual BlobParent*
  GetBlobParent() MOZ_OVERRIDE;

private:
  RemoteBlobImpl(BlobParent* aActor,
                 const InputStreamParams& aInputStreamParams,
                 const nsAString& aName,
                 const nsAString& aContentType,
                 uint64_t aLength,
                 uint64_t aModDate)
    : RemoteBlobImplBase(aActor,  false)
    , DOMFileImplBase(aName, aContentType, aLength, aModDate)
  {
    CommonInit(aInputStreamParams);
  }

  RemoteBlobImpl(BlobParent* aActor,
                 const InputStreamParams& aInputStreamParams,
                 const nsAString& aContentType,
                 uint64_t aLength)
    : RemoteBlobImplBase(aActor,  false)
    , DOMFileImplBase(aContentType, aLength)
  {
    CommonInit(aInputStreamParams);
  }

  ~RemoteBlobImpl()
  { }

  void
  CommonInit(const InputStreamParams& aInputStreamParams)
  {
    MOZ_ASSERT(aInputStreamParams.type() != InputStreamParams::T__None);

    mInputStreamParams = aInputStreamParams;

    mImmutable = true;
  }
};

class BlobParent::RemoteBlobImpl::SliceHelper MOZ_FINAL
  : public nsRunnable
{
  Monitor mMonitor;
  BlobParent* mActor;
  nsRefPtr<DOMFileImpl> mSlice;
  uint64_t mStart;
  uint64_t mLength;
  nsString mContentType;
  bool mDone;

public:
  explicit
  SliceHelper(BlobParent* aActor)
    : mMonitor("BlobParent::RemoteBlobImpl::SliceHelper::mMonitor")
    , mActor(aActor)
    , mStart(0)
    , mLength(0)
    , mDone(false)
  {
    
    MOZ_ASSERT(aActor);
  }

  DOMFileImpl*
  GetSlice(uint64_t aStart,
           uint64_t aLength,
           const nsAString& aContentType)
  {
    
    MOZ_ASSERT(mActor);
    MOZ_ASSERT(!mSlice);
    MOZ_ASSERT(!mDone);

    mStart = aStart;
    mLength = aLength;
    mContentType = aContentType;

    if (mActor->IsOnOwningThread()) {
      RunInternal(false);
    } else {
      nsCOMPtr<nsIEventTarget> target = mActor->EventTarget();
      if (!target) {
        target = do_GetMainThread();
      }

      MOZ_ASSERT(target);

      nsresult rv = target->Dispatch(this, NS_DISPATCH_NORMAL);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return nullptr;
      }

      {
        MonitorAutoLock lock(mMonitor);
        while (!mDone) {
          lock.Wait();
        }
      }
    }

    MOZ_ASSERT(!mActor);
    MOZ_ASSERT(mDone);

    if (NS_WARN_IF(!mSlice)) {
      return nullptr;
    }

    return mSlice;
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mActor);
    mActor->AssertIsOnOwningThread();

    RunInternal(true);
    return NS_OK;
  }

private:
  void
  RunInternal(bool aNotify)
  {
    MOZ_ASSERT(mActor);
    mActor->AssertIsOnOwningThread();
    MOZ_ASSERT(!mSlice);
    MOZ_ASSERT(!mDone);

    NS_ENSURE_TRUE_VOID(mActor->HasManager());

    nsID id;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(gUUIDGenerator->GenerateUUIDInPlace(&id)));

    ParentBlobConstructorParams params(
      NormalBlobConstructorParams(mContentType ,
                                  mLength ),
      void_t() );

    ChildBlobConstructorParams otherSideParams(
      id,
      SlicedBlobConstructorParams(mActor ,
                                  nullptr ,
                                  void_t() ,
                                  mStart ,
                                  mStart + mLength ,
                                  mContentType ));

    BlobParent* newActor;
    if (nsIContentParent* contentManager = mActor->GetContentManager()) {
      newActor = SendSliceConstructor(contentManager, params, otherSideParams);
    } else {
      newActor = SendSliceConstructor(mActor->GetBackgroundManager(),
                                      params,
                                      otherSideParams);
    }

    if (newActor) {
      mSlice = newActor->GetBlobImpl();
    }

    mActor = nullptr;

    if (aNotify) {
      MonitorAutoLock lock(mMonitor);
      mDone = true;
      lock.Notify();
    }
    else {
      mDone = true;
    }
  }
};

class BlobParent::ForwardingRemoteBlobImpl MOZ_FINAL
  : public RemoteBlobImplBase
  , public DOMFileImpl
  , public nsIRemoteBlob
{
  friend class mozilla::dom::BlobParent;

  typedef mozilla::dom::indexedDB::FileInfo FileInfo;
  typedef mozilla::dom::indexedDB::FileManager FileManager;

  nsRefPtr<DOMFileImpl> mBlobImpl;
  nsCOMPtr<nsIRemoteBlob> mRemoteBlob;

public:
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult
  GetName(nsAString& aName) MOZ_OVERRIDE
  {
    return mBlobImpl->GetName(aName);
  }

  virtual nsresult
  GetPath(nsAString& aPath) MOZ_OVERRIDE
  {
    return mBlobImpl->GetPath(aPath);
  }

  virtual nsresult
  GetLastModifiedDate(JSContext* aCx,
                      JS::MutableHandle<JS::Value> aDate) MOZ_OVERRIDE
  {
    return mBlobImpl->GetLastModifiedDate(aCx, aDate);
  }

  virtual nsresult
  GetMozFullPath(nsAString& aName) MOZ_OVERRIDE
  {
    return mBlobImpl->GetMozFullPath(aName);
  }

  virtual nsresult
  GetMozFullPathInternal(nsAString& aFileName) MOZ_OVERRIDE
  {
    return mBlobImpl->GetMozFullPathInternal(aFileName);
  }

  virtual nsresult
  GetSize(uint64_t* aSize) MOZ_OVERRIDE
  {
    return mBlobImpl->GetSize(aSize);
  }

  virtual nsresult
  GetType(nsAString& aType) MOZ_OVERRIDE
  {
    return mBlobImpl->GetType(aType);
  }

  virtual nsresult
  GetMozLastModifiedDate(uint64_t* aDate) MOZ_OVERRIDE
  {
    return mBlobImpl->GetMozLastModifiedDate(aDate);
  }

  virtual already_AddRefed<DOMFileImpl>
  CreateSlice(uint64_t aStart,
              uint64_t aLength,
              const nsAString& aContentType) MOZ_OVERRIDE
  {
    return mBlobImpl->CreateSlice(aStart, aLength, aContentType);
  }

  virtual const nsTArray<nsRefPtr<DOMFileImpl>>*
  GetSubBlobImpls() const MOZ_OVERRIDE
  {
    return mBlobImpl->GetSubBlobImpls();
  }

  virtual nsresult
  GetInternalStream(nsIInputStream** aStream) MOZ_OVERRIDE
  {
    return mBlobImpl->GetInternalStream(aStream);
  }

  virtual int64_t
  GetFileId() MOZ_OVERRIDE
  {
    return mBlobImpl->GetFileId();
  }

  virtual void
  AddFileInfo(FileInfo* aFileInfo) MOZ_OVERRIDE
  {
    return mBlobImpl->AddFileInfo(aFileInfo);
  }

  virtual FileInfo*
  GetFileInfo(FileManager* aFileManager) MOZ_OVERRIDE
  {
    return mBlobImpl->GetFileInfo(aFileManager);
  }

  virtual nsresult
  GetSendInfo(nsIInputStream** aBody,
              uint64_t* aContentLength,
              nsACString& aContentType,
              nsACString& aCharset) MOZ_OVERRIDE
  {
    return mBlobImpl->GetSendInfo(aBody,
                                  aContentLength,
                                  aContentType,
                                  aCharset);
  }

  virtual nsresult
  GetMutable(bool* aMutable) const MOZ_OVERRIDE
  {
    return mBlobImpl->GetMutable(aMutable);
  }

  virtual nsresult
  SetMutable(bool aMutable) MOZ_OVERRIDE
  {
    return mBlobImpl->SetMutable(aMutable);
  }

  virtual void
  SetLazyData(const nsAString& aName,
              const nsAString& aContentType,
              uint64_t aLength,
              uint64_t aLastModifiedDate) MOZ_OVERRIDE
  {
    MOZ_CRASH("This should never be called!");
  }

  virtual bool
  IsMemoryFile() const MOZ_OVERRIDE
  {
    return mBlobImpl->IsMemoryFile();
  }

  virtual bool
  IsSizeUnknown() const MOZ_OVERRIDE
  {
    return mBlobImpl->IsSizeUnknown();
  }

  virtual bool
  IsDateUnknown() const MOZ_OVERRIDE
  {
    return mBlobImpl->IsDateUnknown();
  }

  virtual bool
  IsFile() const MOZ_OVERRIDE
  {
    return mBlobImpl->IsFile();
  }

  virtual nsresult
  Initialize(nsISupports* aOwner,
             JSContext* aCx,
             JSObject* aObj,
             const JS::CallArgs& aArgs) MOZ_OVERRIDE
  {
    MOZ_CRASH("This should never be called!");
  }

  virtual void
  Unlink() MOZ_OVERRIDE
  {
    return mBlobImpl->Unlink();
  }

  virtual void
  Traverse(nsCycleCollectionTraversalCallback& aCallback) MOZ_OVERRIDE
  {
    return mBlobImpl->Traverse(aCallback);
  }

  virtual BlobChild*
  GetBlobChild() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mRemoteBlob);

    return mRemoteBlob->GetBlobChild();
  }

  virtual BlobParent*
  GetBlobParent() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mRemoteBlob);

    return mRemoteBlob->GetBlobParent();
  }

private:
  ForwardingRemoteBlobImpl(BlobParent* aActor,
                           DOMFileImpl* aBlobImpl)
    : RemoteBlobImplBase(aActor,  true)
    , mBlobImpl(aBlobImpl)
    , mRemoteBlob(do_QueryObject(aBlobImpl))
  {
    MOZ_ASSERT(aBlobImpl);

    DebugOnly<bool> isMutable;
    MOZ_ASSERT(NS_SUCCEEDED(aBlobImpl->GetMutable(&isMutable)));
    MOZ_ASSERT(!isMutable);
  }

  ~ForwardingRemoteBlobImpl()
  { }
};





NS_IMPL_ADDREF(BlobParent::RemoteBlobImpl)
NS_IMPL_RELEASE_WITH_DESTROY(BlobParent::RemoteBlobImpl, Destroy())
NS_IMPL_QUERY_INTERFACE_INHERITED(BlobParent::RemoteBlobImpl,
                                  DOMFileImplBase,
                                  nsIRemoteBlob)

already_AddRefed<DOMFileImpl>
BlobParent::
RemoteBlobImpl::CreateSlice(uint64_t aStart,
                            uint64_t aLength,
                            const nsAString& aContentType)
{
  if (!mActor) {
    return nullptr;
  }

  nsRefPtr<SliceHelper> helper = new SliceHelper(mActor);

  nsRefPtr<DOMFileImpl> impl = helper->GetSlice(aStart, aLength, aContentType);
  if (NS_WARN_IF(!impl)) {
    return nullptr;
  }

  return impl.forget();
}

nsresult
BlobParent::
RemoteBlobImpl::GetInternalStream(nsIInputStream** aStream)
{
  MOZ_ASSERT(mInputStreamParams.type() != InputStreamParams::T__None);

  nsTArray<FileDescriptor> fds;
  nsCOMPtr<nsIInputStream> realStream =
    DeserializeInputStream(mInputStreamParams, fds);
  if (!realStream) {
    NS_WARNING("Failed to deserialize stream!");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIInputStream> stream =
    new BlobInputStreamTether(realStream, this);
  stream.forget(aStream);
  return NS_OK;
}

nsresult
BlobParent::
RemoteBlobImpl::GetLastModifiedDate(
                                 JSContext* cx,
                                 JS::MutableHandle<JS::Value> aLastModifiedDate)
{
  if (IsDateUnknown()) {
    aLastModifiedDate.setNull();
  } else {
    JSObject* date = JS_NewDateObjectMsec(cx, mLastModificationDate);
    if (!date) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    aLastModifiedDate.setObject(*date);
  }
  return NS_OK;
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

NS_IMPL_ADDREF(BlobParent::ForwardingRemoteBlobImpl)
NS_IMPL_RELEASE_WITH_DESTROY(BlobParent::ForwardingRemoteBlobImpl, Destroy())
NS_INTERFACE_MAP_BEGIN(BlobParent::ForwardingRemoteBlobImpl)
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(nsIRemoteBlob, mRemoteBlob)
NS_INTERFACE_MAP_END_INHERITING(DOMFileImpl)





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
                       const ParentBlobConstructorParams& aParams,
                       IDTableEntry* aIDTableEntry)
  : mBackgroundManager(nullptr)
  , mContentManager(aManager)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  CommonInit(aParams, aIDTableEntry);
}

BlobParent::BlobParent(PBackgroundParent* aManager,
                       const ParentBlobConstructorParams& aParams,
                       IDTableEntry* aIDTableEntry)
  : mBackgroundManager(aManager)
  , mContentManager(nullptr)
  , mEventTarget(do_GetCurrentThread())
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);
  MOZ_ASSERT(mEventTarget);

  CommonInit(aParams, aIDTableEntry);
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
BlobParent::CommonInit(const ParentBlobConstructorParams& aParams,
                       IDTableEntry* aIDTableEntry)
{
  AssertIsOnOwningThread();

  MOZ_COUNT_CTOR(BlobParent);

  const AnyBlobConstructorParams& blobParams = aParams.blobParams();

  const AnyBlobConstructorParams::Type paramsType = blobParams.type();
  MOZ_ASSERT(paramsType ==
               AnyBlobConstructorParams::TNormalBlobConstructorParams ||
             paramsType ==
               AnyBlobConstructorParams::TFileBlobConstructorParams ||
             paramsType ==
               AnyBlobConstructorParams::TKnownBlobConstructorParams);
  MOZ_ASSERT_IF(paramsType ==
                  AnyBlobConstructorParams::TKnownBlobConstructorParams,
                aIDTableEntry);
  MOZ_ASSERT_IF(paramsType !=
                  AnyBlobConstructorParams::TKnownBlobConstructorParams,
                !aIDTableEntry);
  MOZ_ASSERT_IF(paramsType ==
                  AnyBlobConstructorParams::TKnownBlobConstructorParams,
                aParams.optionalInputStreamParams().type() ==
                   OptionalInputStreamParams::Tvoid_t);
  MOZ_ASSERT_IF(paramsType !=
                  AnyBlobConstructorParams::TKnownBlobConstructorParams,
                aParams.optionalInputStreamParams().type() ==
                   OptionalInputStreamParams::TInputStreamParams);

  nsRefPtr<DOMFileImpl> remoteBlobImpl;
  RemoteBlobImplBase* remoteBlobBase = nullptr;

  switch (paramsType) {
    case AnyBlobConstructorParams::TNormalBlobConstructorParams: {
      const InputStreamParams& inputStreamParams =
        aParams.optionalInputStreamParams().get_InputStreamParams();

      const NormalBlobConstructorParams& params =
        blobParams.get_NormalBlobConstructorParams();

      nsRefPtr<RemoteBlobImpl> impl =
        new RemoteBlobImpl(this,
                           inputStreamParams,
                           params.contentType(),
                           params.length());

      remoteBlobBase = impl;
      remoteBlobImpl = impl.forget();
      break;
    }

    case AnyBlobConstructorParams::TFileBlobConstructorParams: {
      const InputStreamParams& inputStreamParams =
        aParams.optionalInputStreamParams().get_InputStreamParams();

      const FileBlobConstructorParams& params =
        blobParams.get_FileBlobConstructorParams();

      nsRefPtr<RemoteBlobImpl> impl =
        new RemoteBlobImpl(this,
                           inputStreamParams,
                           params.name(),
                           params.contentType(),
                           params.length(),
                           params.modDate());

      remoteBlobBase = impl;
      remoteBlobImpl = impl.forget();
      break;
    }

    case AnyBlobConstructorParams::TKnownBlobConstructorParams: {
      nsRefPtr<ForwardingRemoteBlobImpl> impl =
        new ForwardingRemoteBlobImpl(this, aIDTableEntry->BlobImpl());

      remoteBlobBase = impl;
      remoteBlobImpl = impl.forget();
      break;
    }

    default:
      MOZ_CRASH("Unknown params!");
  }

  MOZ_ASSERT(remoteBlobImpl);
  MOZ_ASSERT(remoteBlobBase);

  DebugOnly<bool> isMutable;
  MOZ_ASSERT(NS_SUCCEEDED(remoteBlobImpl->GetMutable(&isMutable)));
  MOZ_ASSERT(!isMutable);

  mRemoteBlobImpl = remoteBlobBase;

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
BlobParent::GetOrCreate(nsIContentParent* aManager, DOMFileImpl* aBlobImpl)
{
  AssertCorrectThreadForManager(aManager);
  MOZ_ASSERT(aManager);

  return GetOrCreateFromImpl(aManager, aBlobImpl);
}


BlobParent*
BlobParent::GetOrCreate(PBackgroundParent* aManager, DOMFileImpl* aBlobImpl)
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


template <class ParentManagerType>
BlobParent*
BlobParent::GetOrCreateFromImpl(ParentManagerType* aManager,
                                DOMFileImpl* aBlobImpl)
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

  AnyBlobConstructorParams blobParams;

  if (aBlobImpl->IsSizeUnknown() || aBlobImpl->IsDateUnknown()) {
    
    
    
    blobParams = MysteryBlobConstructorParams();
  } else {
    nsString contentType;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aBlobImpl->GetType(contentType)));

    uint64_t length;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aBlobImpl->GetSize(&length)));

    if (aBlobImpl->IsFile()) {
      nsString name;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(aBlobImpl->GetName(name)));

      uint64_t modDate;
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        aBlobImpl->GetMozLastModifiedDate(&modDate)));

      blobParams =
        FileBlobConstructorParams(name, contentType, length, modDate);
    } else {
      blobParams = NormalBlobConstructorParams(contentType, length);
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
      if (aParams.optionalInputStreamParams().type() !=
            OptionalInputStreamParams::TInputStreamParams) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      return new BlobParent(aManager, aParams,  nullptr);
    }

    case AnyBlobConstructorParams::TSlicedBlobConstructorParams: {
      if (aParams.optionalInputStreamParams().type() !=
            OptionalInputStreamParams::Tvoid_t) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      const SlicedBlobConstructorParams& params =
        blobParams.get_SlicedBlobConstructorParams();
      if (NS_WARN_IF(params.optionalID().type() != OptionalID::TnsID)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      auto* actor =
        const_cast<BlobParent*>(
          static_cast<const BlobParent*>(params.sourceParent()));
      MOZ_ASSERT(actor);

      nsRefPtr<DOMFileImpl> source = actor->GetBlobImpl();
      MOZ_ASSERT(source);

      nsRefPtr<DOMFileImpl> slice;
      if (NS_WARN_IF(NS_FAILED(source->Slice(params.begin(),
                                             params.end(),
                                             params.contentType(),
                                             3,
                                             getter_AddRefs(slice))))) {
        return nullptr;
      }

      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(slice->SetMutable(false)));

      nsRefPtr<IDTableEntry> idTableEntry =
        IDTableEntry::Create(params.optionalID(),
                             ActorManagerProcessID(aManager),
                             slice);
      if (NS_WARN_IF(!idTableEntry)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      return new BlobParent(aManager, idTableEntry);
    }

    case AnyBlobConstructorParams::TKnownBlobConstructorParams: {
      if (aParams.optionalInputStreamParams().type() !=
            OptionalInputStreamParams::Tvoid_t) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      const KnownBlobConstructorParams& params =
        blobParams.get_KnownBlobConstructorParams();

      nsRefPtr<IDTableEntry> idTableEntry =
        IDTableEntry::Get(params.id(), ActorManagerProcessID(aManager));
      if (NS_WARN_IF(!idTableEntry)) {
        ASSERT_UNLESS_FUZZING();
        return nullptr;
      }

      return new BlobParent(aManager, aParams, idTableEntry);
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

already_AddRefed<DOMFileImpl>
BlobParent::GetBlobImpl()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);

  nsRefPtr<DOMFileImpl> blobImpl;

  
  
  
  if (mRemoteBlobImpl && mOwnsBlobImpl) {
    blobImpl = dont_AddRef(mBlobImpl);
    mOwnsBlobImpl = false;
  } else {
    blobImpl = mBlobImpl;
  }

  MOZ_ASSERT(blobImpl);

  return blobImpl.forget();
}

already_AddRefed<nsIDOMBlob>
BlobParent::GetBlob()
{
  MOZ_ASSERT(!mBackgroundManager,
             "Don't call this method on a non-DOM thread! Use GetBlobImpl()!");
  AssertIsOnOwningThread();

  nsRefPtr<DOMFileImpl> blobImpl = GetBlobImpl();
  MOZ_ASSERT(blobImpl);

  nsCOMPtr<nsIDOMBlob> blob = new DOMFile(blobImpl);
  return blob.forget();
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
BlobParent::AllocPBlobStreamParent()
{
  AssertIsOnOwningThread();

  if (NS_WARN_IF(mRemoteBlobImpl && !mRemoteBlobImpl->mForwards)) {
    ASSERT_UNLESS_FUZZING();
    return nullptr;
  }

  return new InputStreamParent();
}

bool
BlobParent::RecvPBlobStreamConstructor(PBlobStreamParent* aActor)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aActor);
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT_IF(mRemoteBlobImpl, mRemoteBlobImpl->mForwards);
  MOZ_ASSERT(mOwnsBlobImpl);

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = mBlobImpl->GetInternalStream(getter_AddRefs(stream));
  NS_ENSURE_SUCCESS(rv, false);

  
  
  if (mBlobImpl->IsMemoryFile()) {
    InputStreamParams params;
    nsTArray<FileDescriptor> fds;
    SerializeInputStream(stream, params, fds);

    MOZ_ASSERT(params.type() != InputStreamParams::T__None);
    MOZ_ASSERT(fds.IsEmpty());

    return PBlobStreamParent::Send__delete__(aActor, params, void_t());
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
  rv = NS_NewNamedThread("Blob Opener", getter_AddRefs(target));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  nsRefPtr<OpenStreamRunnable> runnable =
    new OpenStreamRunnable(this, aActor, stream, serializableStream, target);

  rv = runnable->Dispatch();
  NS_ENSURE_SUCCESS(rv, false);

  
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
BlobParent::RecvWaitForSliceCreation()
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mBlobImpl);
  MOZ_ASSERT(!mRemoteBlobImpl);
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
  using namespace mozilla::dom::indexedDB;

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
  using namespace mozilla::dom::indexedDB;

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
  nsresult rv = mBlobImpl->GetMozFullPathInternal(filePath);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  *aFilePath = filePath;
  return true;
}

bool
InputStreamChild::Recv__delete__(const InputStreamParams& aParams,
                                 const OptionalFileDescriptorSet& aFDs)
{
  MOZ_ASSERT(mRemoteStream);
  mRemoteStream->AssertIsOnOwningThread();

  nsTArray<FileDescriptor> fds;
  if (aFDs.type() == OptionalFileDescriptorSet::TPFileDescriptorSetChild) {
    FileDescriptorSetChild* fdSetActor =
      static_cast<FileDescriptorSetChild*>(aFDs.get_PFileDescriptorSetChild());
    MOZ_ASSERT(fdSetActor);

    fdSetActor->ForgetFileDescriptors(fds);
    MOZ_ASSERT(!fds.IsEmpty());

    fdSetActor->Send__delete__(fdSetActor);
  }

  nsCOMPtr<nsIInputStream> stream = DeserializeInputStream(aParams, fds);
  MOZ_ASSERT(stream);

  mRemoteStream->SetStream(stream);
  return true;
}

BlobParent::
IDTableEntry::IDTableEntry(const nsID& aID,
                           intptr_t aProcessID,
                           DOMFileImpl* aBlobImpl)
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
                                  DOMFileImpl* aBlobImpl,
                                  bool aMayCreate,
                                  bool aMayGet)
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

      if (NS_WARN_IF(entry->mProcessID != aProcessID)) {
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

} 
} 
