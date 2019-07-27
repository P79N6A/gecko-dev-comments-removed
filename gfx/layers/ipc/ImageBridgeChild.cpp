




#include "ImageBridgeChild.h"
#include <vector>                       
#include "ImageBridgeParent.h"          
#include "ImageContainer.h"             
#include "Layers.h"                     
#include "ShadowLayers.h"               
#include "base/message_loop.h"          
#include "base/platform_thread.h"       
#include "base/process.h"               
#include "base/task.h"                  
#include "base/thread.h"                
#include "base/tracked.h"               
#include "mozilla/Assertions.h"         
#include "mozilla/Monitor.h"            
#include "mozilla/ReentrantMonitor.h"   
#include "mozilla/ipc/MessageChannel.h" 
#include "mozilla/ipc/Transport.h"      
#include "mozilla/gfx/Point.h"          
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/CompositorParent.h" 
#include "mozilla/layers/ISurfaceAllocator.h"  
#include "mozilla/layers/ImageClient.h"  
#include "mozilla/layers/LayersMessages.h"  
#include "mozilla/layers/PCompositableChild.h"  
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsISupportsImpl.h"            
#include "nsTArray.h"                   
#include "nsTArrayForwardDeclare.h"     
#include "nsThreadUtils.h"              
#include "nsXULAppAPI.h"                
#include "mozilla/StaticPtr.h"          
#include "mozilla/layers/TextureClient.h"

struct nsIntRect;

namespace mozilla {
namespace ipc {
class Shmem;
}

namespace layers {

using base::Thread;
using base::ProcessId;
using namespace mozilla::ipc;
using namespace mozilla::gfx;

typedef std::vector<CompositableOperation> OpVector;

struct CompositableTransaction
{
  CompositableTransaction()
  : mSwapRequired(false)
  , mFinished(true)
  {}
  ~CompositableTransaction()
  {
    End();
  }
  bool Finished() const
  {
    return mFinished;
  }
  void Begin()
  {
    MOZ_ASSERT(mFinished);
    mFinished = false;
  }
  void End()
  {
    mFinished = true;
    mSwapRequired = false;
    mOperations.clear();
  }
  bool IsEmpty() const
  {
    return mOperations.empty();
  }
  void AddNoSwapEdit(const CompositableOperation& op)
  {
    MOZ_ASSERT(!Finished(), "forgot BeginTransaction?");
    mOperations.push_back(op);
  }
  void AddEdit(const CompositableOperation& op)
  {
    AddNoSwapEdit(op);
    mSwapRequired = true;
  }

  OpVector mOperations;
  bool mSwapRequired;
  bool mFinished;
};

struct AutoEndTransaction {
  explicit AutoEndTransaction(CompositableTransaction* aTxn) : mTxn(aTxn) {}
  ~AutoEndTransaction() { mTxn->End(); }
  CompositableTransaction* mTxn;
};

void
ImageBridgeChild::UseTexture(CompositableClient* aCompositable,
                             TextureClient* aTexture)
{
  MOZ_ASSERT(aCompositable);
  MOZ_ASSERT(aTexture);
  MOZ_ASSERT(aCompositable->GetIPDLActor());
  MOZ_ASSERT(aTexture->GetIPDLActor());

  FenceHandle fence = aTexture->GetAcquireFenceHandle();
  mTxn->AddNoSwapEdit(OpUseTexture(nullptr, aCompositable->GetIPDLActor(),
                                   nullptr, aTexture->GetIPDLActor(),
                                   fence.IsValid() ? MaybeFence(fence) : MaybeFence(null_t())));
}

void
ImageBridgeChild::UseComponentAlphaTextures(CompositableClient* aCompositable,
                                            TextureClient* aTextureOnBlack,
                                            TextureClient* aTextureOnWhite)
{
  MOZ_ASSERT(aCompositable);
  MOZ_ASSERT(aTextureOnWhite);
  MOZ_ASSERT(aTextureOnBlack);
  MOZ_ASSERT(aCompositable->GetIPDLActor());
  MOZ_ASSERT(aTextureOnWhite->GetIPDLActor());
  MOZ_ASSERT(aTextureOnBlack->GetIPDLActor());
  MOZ_ASSERT(aTextureOnBlack->GetSize() == aTextureOnWhite->GetSize());
  mTxn->AddNoSwapEdit(OpUseComponentAlphaTextures(nullptr, aCompositable->GetIPDLActor(),
                                                  nullptr, aTextureOnBlack->GetIPDLActor(),
                                                  nullptr, aTextureOnWhite->GetIPDLActor()));
}

#ifdef MOZ_WIDGET_GONK
void
ImageBridgeChild::UseOverlaySource(CompositableClient* aCompositable,
                                   const OverlaySource& aOverlay)
{
  MOZ_ASSERT(aCompositable);
  mTxn->AddEdit(OpUseOverlaySource(nullptr, aCompositable->GetIPDLActor(), aOverlay));
}
#endif

void
ImageBridgeChild::UpdatePictureRect(CompositableClient* aCompositable,
                                    const nsIntRect& aRect)
{
  MOZ_ASSERT(aCompositable);
  MOZ_ASSERT(aCompositable->GetIPDLActor());
  mTxn->AddNoSwapEdit(OpUpdatePictureRect(nullptr, aCompositable->GetIPDLActor(), aRect));
}


static StaticRefPtr<ImageBridgeChild> sImageBridgeChildSingleton;
static StaticRefPtr<ImageBridgeParent> sImageBridgeParentSingleton;
static Thread *sImageBridgeChildThread = nullptr;

void ReleaseImageBridgeParentSingleton() {
  sImageBridgeParentSingleton = nullptr;
}



static void ImageBridgeShutdownStep1(ReentrantMonitor *aBarrier, bool *aDone)
{
  ReentrantMonitorAutoEnter autoMon(*aBarrier);

  MOZ_ASSERT(InImageBridgeChildThread(),
             "Should be in ImageBridgeChild thread.");
  if (sImageBridgeChildSingleton) {
    
    InfallibleTArray<PCompositableChild*> compositables;
    sImageBridgeChildSingleton->ManagedPCompositableChild(compositables);
    for (int i = compositables.Length() - 1; i >= 0; --i) {
      CompositableClient::FromIPDLActor(compositables[i])->Destroy();
    }
    InfallibleTArray<PTextureChild*> textures;
    sImageBridgeChildSingleton->ManagedPTextureChild(textures);
    for (int i = textures.Length() - 1; i >= 0; --i) {
      TextureClient::AsTextureClient(textures[i])->ForceRemove();
    }
    sImageBridgeChildSingleton->SendWillStop();
    sImageBridgeChildSingleton->MarkShutDown();
    
    
  }
  *aDone = true;
  aBarrier->NotifyAll();
}


static void ImageBridgeShutdownStep2(ReentrantMonitor *aBarrier, bool *aDone)
{
  ReentrantMonitorAutoEnter autoMon(*aBarrier);

  MOZ_ASSERT(InImageBridgeChildThread(),
             "Should be in ImageBridgeChild thread.");

  sImageBridgeChildSingleton->SendStop();

  *aDone = true;
  aBarrier->NotifyAll();
}


static void CreateImageClientSync(RefPtr<ImageClient>* result,
                                  ReentrantMonitor* barrier,
                                  CompositableType aType,
                                  bool *aDone)
{
  ReentrantMonitorAutoEnter autoMon(*barrier);
  *result = sImageBridgeChildSingleton->CreateImageClientNow(aType);
  *aDone = true;
  barrier->NotifyAll();
}

static void ConnectImageBridge(ImageBridgeChild * child, ImageBridgeParent * parent)
{
  MessageLoop *parentMsgLoop = parent->GetMessageLoop();
  ipc::MessageChannel *parentChannel = parent->GetIPCChannel();
  child->Open(parentChannel, parentMsgLoop, mozilla::ipc::ChildSide);
}

ImageBridgeChild::ImageBridgeChild()
  : mShuttingDown(false)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  SetMessageLoopToPostDestructionTo(MessageLoop::current());

  mTxn = new CompositableTransaction();
}
ImageBridgeChild::~ImageBridgeChild()
{
  MOZ_ASSERT(NS_IsMainThread());

  delete mTxn;
}

void
ImageBridgeChild::MarkShutDown()
{
  MOZ_ASSERT(!mShuttingDown);
  mShuttingDown = true;
}

void
ImageBridgeChild::Connect(CompositableClient* aCompositable)
{
  MOZ_ASSERT(aCompositable);
  MOZ_ASSERT(!mShuttingDown);
  uint64_t id = 0;
  PCompositableChild* child =
    SendPCompositableConstructor(aCompositable->GetTextureInfo(), &id);
  MOZ_ASSERT(child);
  aCompositable->InitIPDLActor(child, id);
}

PCompositableChild*
ImageBridgeChild::AllocPCompositableChild(const TextureInfo& aInfo, uint64_t* aID)
{
  MOZ_ASSERT(!mShuttingDown);
  return CompositableClient::CreateIPDLActor();
}

bool
ImageBridgeChild::DeallocPCompositableChild(PCompositableChild* aActor)
{
  return CompositableClient::DestroyIPDLActor(aActor);
}


Thread* ImageBridgeChild::GetThread() const
{
  return sImageBridgeChildThread;
}

ImageBridgeChild* ImageBridgeChild::GetSingleton()
{
  return sImageBridgeChildSingleton;
}

bool ImageBridgeChild::IsCreated()
{
  return GetSingleton() != nullptr;
}

void ImageBridgeChild::StartUp()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main Thread!");
  ImageBridgeChild::StartUpOnThread(new Thread("ImageBridgeChild"));
}

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

static void
ConnectImageBridgeInChildProcess(Transport* aTransport,
                                 ProcessId aOtherPid)
{
  
  sImageBridgeChildSingleton->Open(aTransport, aOtherPid,
                                   XRE_GetIOMessageLoop(),
                                   ipc::ChildSide);
#ifdef MOZ_NUWA_PROCESS
  if (IsNuwaProcess()) {
    sImageBridgeChildThread
      ->message_loop()->PostTask(FROM_HERE,
                                 NewRunnableFunction(NuwaMarkCurrentThread,
                                                     (void (*)(void *))nullptr,
                                                     (void *)nullptr));
  }
#endif
}

static void ReleaseImageClientNow(ImageClient* aClient)
{
  MOZ_ASSERT(InImageBridgeChildThread());
  aClient->Release();
}


void ImageBridgeChild::DispatchReleaseImageClient(ImageClient* aClient)
{
  if (!aClient) {
    return;
  }

  if (!IsCreated()) {
    
    
    
    
    
    MOZ_ASSERT(aClient->GetIPDLActor() == nullptr);
    aClient->Release();
    return;
  }

  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(&ReleaseImageClientNow, aClient));
}

static void ReleaseTextureClientNow(TextureClient* aClient)
{
  MOZ_ASSERT(InImageBridgeChildThread());
  aClient->Release();
}


void ImageBridgeChild::DispatchReleaseTextureClient(TextureClient* aClient)
{
  if (!aClient) {
    return;
  }

  if (!IsCreated()) {
    
    
    
    
    
    MOZ_ASSERT(aClient->GetIPDLActor() == nullptr);
    aClient->Release();
    return;
  }

  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(&ReleaseTextureClientNow, aClient));
}

static void UpdateImageClientNow(ImageClient* aClient, ImageContainer* aContainer)
{
  MOZ_ASSERT(aClient);
  MOZ_ASSERT(aContainer);
  sImageBridgeChildSingleton->BeginTransaction();
  aClient->UpdateImage(aContainer, Layer::CONTENT_OPAQUE);
  aClient->OnTransaction();
  sImageBridgeChildSingleton->EndTransaction();
}


void ImageBridgeChild::DispatchImageClientUpdate(ImageClient* aClient,
                                                 ImageContainer* aContainer)
{
  if (!aClient || !aContainer || !IsCreated()) {
    return;
  }

  if (InImageBridgeChildThread()) {
    UpdateImageClientNow(aClient, aContainer);
    return;
  }
  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction<
      void (*)(ImageClient*, ImageContainer*),
      ImageClient*,
      nsRefPtr<ImageContainer> >(&UpdateImageClientNow, aClient, aContainer));
}

static void FlushAllImagesSync(ImageClient* aClient, ImageContainer* aContainer, bool aExceptFront, AsyncTransactionTracker* aStatus)
{
  MOZ_ASSERT(aClient);
  sImageBridgeChildSingleton->BeginTransaction();
  if (aContainer && !aExceptFront) {
    aContainer->ClearCurrentImage();
  }
  aClient->FlushAllImages(aExceptFront, aStatus);
  aClient->OnTransaction();
  sImageBridgeChildSingleton->EndTransaction();
}


void ImageBridgeChild::FlushAllImages(ImageClient* aClient, ImageContainer* aContainer, bool aExceptFront)
{
  if (!IsCreated()) {
    return;
  }
  MOZ_ASSERT(aClient);
  MOZ_ASSERT(!sImageBridgeChildSingleton->mShuttingDown);
  MOZ_ASSERT(!InImageBridgeChildThread());
  if (InImageBridgeChildThread()) {
    NS_ERROR("ImageBridgeChild::FlushAllImages() is called on ImageBridge thread.");
     return;
   }

  RefPtr<AsyncTransactionTracker> status = aClient->PrepareFlushAllImages();

  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(&FlushAllImagesSync, aClient, aContainer, aExceptFront, status));

  status->WaitComplete();
}

void
ImageBridgeChild::BeginTransaction()
{
  MOZ_ASSERT(!mShuttingDown);
  MOZ_ASSERT(mTxn->Finished(), "uncommitted txn?");
  mTxn->Begin();
}

class MOZ_STACK_CLASS AutoRemoveTextures
{
public:
  explicit AutoRemoveTextures(ImageBridgeChild* aImageBridge)
    : mImageBridge(aImageBridge) {}

  ~AutoRemoveTextures()
  {
    mImageBridge->RemoveTexturesIfNecessary();
  }
private:
  ImageBridgeChild* mImageBridge;
};

void
ImageBridgeChild::EndTransaction()
{
  MOZ_ASSERT(!mShuttingDown);
  MOZ_ASSERT(!mTxn->Finished(), "forgot BeginTransaction?");

  AutoEndTransaction _(mTxn);
  AutoRemoveTextures autoRemoveTextures(this);

  if (mTxn->IsEmpty()) {
    return;
  }

  AutoInfallibleTArray<CompositableOperation, 10> cset;
  cset.SetCapacity(mTxn->mOperations.size());
  if (!mTxn->mOperations.empty()) {
    cset.AppendElements(&mTxn->mOperations.front(), mTxn->mOperations.size());
  }

  if (!IsSameProcess()) {
    ShadowLayerForwarder::PlatformSyncBeforeUpdate();
  }

  AutoInfallibleTArray<EditReply, 10> replies;

  if (mTxn->mSwapRequired) {
    if (!SendUpdate(cset, &replies)) {
      NS_WARNING("could not send async texture transaction");
      return;
    }
  } else {
    
    
    if (!SendUpdateNoSwap(cset)) {
      NS_WARNING("could not send async texture transaction (no swap)");
      return;
    }
  }
  for (nsTArray<EditReply>::size_type i = 0; i < replies.Length(); ++i) {
    const EditReply& reply = replies[i];
    switch (reply.type()) {
    case EditReply::TReturnReleaseFence: {
      const ReturnReleaseFence& rep = reply.get_ReturnReleaseFence();
      FenceHandle fence = rep.fence();
      PTextureChild* child = rep.textureChild();

      if (!fence.IsValid() || !child) {
        break;
      }
      RefPtr<TextureClient> texture = TextureClient::AsTextureClient(child);
      if (texture) {
        texture->SetReleaseFenceHandle(fence);
      }
      break;
    }
    default:
      NS_RUNTIMEABORT("not reached");
    }
  }
  SendPendingAsyncMessges();
}


PImageBridgeChild*
ImageBridgeChild::StartUpInChildProcess(Transport* aTransport,
                                        ProcessId aOtherPid)
{
  MOZ_ASSERT(NS_IsMainThread());

  gfxPlatform::GetPlatform();

  sImageBridgeChildThread = new Thread("ImageBridgeChild");
  if (!sImageBridgeChildThread->Start()) {
    return nullptr;
  }

  sImageBridgeChildSingleton = new ImageBridgeChild();
  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(ConnectImageBridgeInChildProcess,
                        aTransport, aOtherPid));

  return sImageBridgeChildSingleton;
}

void ImageBridgeChild::ShutDown()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (ImageBridgeChild::IsCreated()) {
    MOZ_ASSERT(!sImageBridgeChildSingleton->mShuttingDown);

    {
      ReentrantMonitor barrier("ImageBridge ShutdownStep1 lock");
      ReentrantMonitorAutoEnter autoMon(barrier);

      bool done = false;
      sImageBridgeChildSingleton->GetMessageLoop()->PostTask(FROM_HERE,
                      NewRunnableFunction(&ImageBridgeShutdownStep1, &barrier, &done));
      while (!done) {
        barrier.Wait();
      }
    }

    {
      ReentrantMonitor barrier("ImageBridge ShutdownStep2 lock");
      ReentrantMonitorAutoEnter autoMon(barrier);

      bool done = false;
      sImageBridgeChildSingleton->GetMessageLoop()->PostTask(FROM_HERE,
                      NewRunnableFunction(&ImageBridgeShutdownStep2, &barrier, &done));
      while (!done) {
        barrier.Wait();
      }
    }

    sImageBridgeChildSingleton = nullptr;

    delete sImageBridgeChildThread;
    sImageBridgeChildThread = nullptr;
  }
}

bool ImageBridgeChild::StartUpOnThread(Thread* aThread)
{
  MOZ_ASSERT(aThread, "ImageBridge needs a thread.");
  if (sImageBridgeChildSingleton == nullptr) {
    sImageBridgeChildThread = aThread;
    if (!aThread->IsRunning()) {
      aThread->Start();
    }
    sImageBridgeChildSingleton = new ImageBridgeChild();
    sImageBridgeParentSingleton = new ImageBridgeParent(
      CompositorParent::CompositorLoop(), nullptr, base::GetCurrentProcId());
    sImageBridgeChildSingleton->ConnectAsync(sImageBridgeParentSingleton);
    return true;
  } else {
    return false;
  }
}

bool InImageBridgeChildThread()
{
  return ImageBridgeChild::IsCreated() &&
    sImageBridgeChildThread->thread_id() == PlatformThread::CurrentId();
}

MessageLoop * ImageBridgeChild::GetMessageLoop() const
{
  return sImageBridgeChildThread->message_loop();
}

void ImageBridgeChild::ConnectAsync(ImageBridgeParent* aParent)
{
  GetMessageLoop()->PostTask(FROM_HERE, NewRunnableFunction(&ConnectImageBridge,
                                                            this, aParent));
}

void
ImageBridgeChild::IdentifyCompositorTextureHost(const TextureFactoryIdentifier& aIdentifier)
{
  if (sImageBridgeChildSingleton) {
    sImageBridgeChildSingleton->IdentifyTextureHost(aIdentifier);
  }
}

TemporaryRef<ImageClient>
ImageBridgeChild::CreateImageClient(CompositableType aType)
{
  if (InImageBridgeChildThread()) {
    return CreateImageClientNow(aType);
  }
  ReentrantMonitor barrier("CreateImageClient Lock");
  ReentrantMonitorAutoEnter autoMon(barrier);
  bool done = false;

  RefPtr<ImageClient> result = nullptr;
  GetMessageLoop()->PostTask(FROM_HERE, NewRunnableFunction(&CreateImageClientSync,
                                                            &result, &barrier, aType, &done));
  
  
  while (!done) {
    barrier.Wait();
  }
  return result.forget();
}

TemporaryRef<ImageClient>
ImageBridgeChild::CreateImageClientNow(CompositableType aType)
{
  MOZ_ASSERT(!sImageBridgeChildSingleton->mShuttingDown);
  RefPtr<ImageClient> client
    = ImageClient::CreateImageClient(aType, this, TextureFlags::NO_FLAGS);
  MOZ_ASSERT(client, "failed to create ImageClient");
  if (client) {
    client->Connect();
  }
  return client.forget();
}

bool
ImageBridgeChild::AllocUnsafeShmem(size_t aSize,
                                   ipc::SharedMemory::SharedMemoryType aType,
                                   ipc::Shmem* aShmem)
{
  MOZ_ASSERT(!mShuttingDown);
  if (InImageBridgeChildThread()) {
    return PImageBridgeChild::AllocUnsafeShmem(aSize, aType, aShmem);
  } else {
    return DispatchAllocShmemInternal(aSize, aType, aShmem, true); 
  }
}

bool
ImageBridgeChild::AllocShmem(size_t aSize,
                             ipc::SharedMemory::SharedMemoryType aType,
                             ipc::Shmem* aShmem)
{
  MOZ_ASSERT(!mShuttingDown);
  if (InImageBridgeChildThread()) {
    return PImageBridgeChild::AllocShmem(aSize, aType, aShmem);
  } else {
    return DispatchAllocShmemInternal(aSize, aType, aShmem, false); 
  }
}



struct AllocShmemParams {
  RefPtr<ISurfaceAllocator> mAllocator;
  size_t mSize;
  ipc::SharedMemory::SharedMemoryType mType;
  ipc::Shmem* mShmem;
  bool mUnsafe;
  bool mSuccess;
};

static void ProxyAllocShmemNow(AllocShmemParams* aParams,
                               ReentrantMonitor* aBarrier,
                               bool* aDone)
{
  MOZ_ASSERT(aParams);
  MOZ_ASSERT(aDone);
  MOZ_ASSERT(aBarrier);

  if (aParams->mUnsafe) {
    aParams->mSuccess = aParams->mAllocator->AllocUnsafeShmem(aParams->mSize,
                                                              aParams->mType,
                                                              aParams->mShmem);
  } else {
    aParams->mSuccess = aParams->mAllocator->AllocShmem(aParams->mSize,
                                                        aParams->mType,
                                                        aParams->mShmem);
  }

  ReentrantMonitorAutoEnter autoMon(*aBarrier);
  *aDone = true;
  aBarrier->NotifyAll();
}

bool
ImageBridgeChild::DispatchAllocShmemInternal(size_t aSize,
                                             SharedMemory::SharedMemoryType aType,
                                             ipc::Shmem* aShmem,
                                             bool aUnsafe)
{
  ReentrantMonitor barrier("AllocatorProxy alloc");
  ReentrantMonitorAutoEnter autoMon(barrier);

  AllocShmemParams params = {
    this, aSize, aType, aShmem, aUnsafe, true
  };
  bool done = false;

  GetMessageLoop()->PostTask(FROM_HERE,
                             NewRunnableFunction(&ProxyAllocShmemNow,
                                                 &params,
                                                 &barrier,
                                                 &done));
  while (!done) {
    barrier.Wait();
  }
  return params.mSuccess;
}

static void ProxyDeallocShmemNow(ISurfaceAllocator* aAllocator,
                                 ipc::Shmem* aShmem,
                                 ReentrantMonitor* aBarrier,
                                 bool* aDone)
{
  MOZ_ASSERT(aShmem);
  MOZ_ASSERT(aDone);
  MOZ_ASSERT(aBarrier);

  aAllocator->DeallocShmem(*aShmem);

  ReentrantMonitorAutoEnter autoMon(*aBarrier);
  *aDone = true;
  aBarrier->NotifyAll();
}

void
ImageBridgeChild::DeallocShmem(ipc::Shmem& aShmem)
{
  if (InImageBridgeChildThread()) {
    PImageBridgeChild::DeallocShmem(aShmem);
  } else {
    ReentrantMonitor barrier("AllocatorProxy Dealloc");
    ReentrantMonitorAutoEnter autoMon(barrier);

    bool done = false;
    GetMessageLoop()->PostTask(FROM_HERE,
                               NewRunnableFunction(&ProxyDeallocShmemNow,
                                                   this,
                                                   &aShmem,
                                                   &barrier,
                                                   &done));
    while (!done) {
      barrier.Wait();
    }
  }
}

PTextureChild*
ImageBridgeChild::AllocPTextureChild(const SurfaceDescriptor&,
                                     const TextureFlags&)
{
  MOZ_ASSERT(!mShuttingDown);
  return TextureClient::CreateIPDLActor();
}

bool
ImageBridgeChild::DeallocPTextureChild(PTextureChild* actor)
{
  return TextureClient::DestroyIPDLActor(actor);
}

bool
ImageBridgeChild::RecvParentAsyncMessages(InfallibleTArray<AsyncParentMessageData>&& aMessages)
{
  for (AsyncParentMessageArray::index_type i = 0; i < aMessages.Length(); ++i) {
    const AsyncParentMessageData& message = aMessages[i];

    switch (message.type()) {
      case AsyncParentMessageData::TOpDeliverFence: {
        const OpDeliverFence& op = message.get_OpDeliverFence();
        FenceHandle fence = op.fence();
        PTextureChild* child = op.textureChild();

        RefPtr<TextureClient> texture = TextureClient::AsTextureClient(child);
        if (texture) {
          texture->SetReleaseFenceHandle(fence);
        }
        break;
      }
      case AsyncParentMessageData::TOpDeliverFenceToTracker: {
        const OpDeliverFenceToTracker& op = message.get_OpDeliverFenceToTracker();
        FenceHandle fence = op.fence();

        AsyncTransactionTrackersHolder::SetReleaseFenceHandle(fence,
                                                              op.destHolderId(),
                                                              op.destTransactionId());
        break;
      }
      case AsyncParentMessageData::TOpReplyRemoveTexture: {
        const OpReplyRemoveTexture& op = message.get_OpReplyRemoveTexture();

        AsyncTransactionTrackersHolder::TransactionCompleteted(op.holderId(),
                                                               op.transactionId());
        break;
      }
      default:
        NS_ERROR("unknown AsyncParentMessageData type");
        return false;
    }
  }
  return true;
}

PTextureChild*
ImageBridgeChild::CreateTexture(const SurfaceDescriptor& aSharedData,
                                TextureFlags aFlags)
{
  MOZ_ASSERT(!mShuttingDown);
  return SendPTextureConstructor(aSharedData, aFlags);
}

void
ImageBridgeChild::RemoveTextureFromCompositable(CompositableClient* aCompositable,
                                                TextureClient* aTexture)
{
  MOZ_ASSERT(!mShuttingDown);
  if (aTexture->GetFlags() & TextureFlags::DEALLOCATE_CLIENT) {
    mTxn->AddEdit(OpRemoveTexture(nullptr, aCompositable->GetIPDLActor(),
                                  nullptr, aTexture->GetIPDLActor()));
  } else {
    mTxn->AddNoSwapEdit(OpRemoveTexture(nullptr, aCompositable->GetIPDLActor(),
                                        nullptr, aTexture->GetIPDLActor()));
  }
  
  HoldUntilTransaction(aTexture);
}

void
ImageBridgeChild::RemoveTextureFromCompositableAsync(AsyncTransactionTracker* aAsyncTransactionTracker,
                                                     CompositableClient* aCompositable,
                                                     TextureClient* aTexture)
{
  mTxn->AddNoSwapEdit(OpRemoveTextureAsync(CompositableClient::GetTrackersHolderId(aCompositable->GetIPDLActor()),
                                           aAsyncTransactionTracker->GetId(),
                                           nullptr, aCompositable->GetIPDLActor(),
                                           nullptr, aTexture->GetIPDLActor()));
  
  CompositableClient::HoldUntilComplete(aCompositable->GetIPDLActor(),
                                        aAsyncTransactionTracker);
}

static void RemoveTextureSync(TextureClient* aTexture, ReentrantMonitor* aBarrier, bool* aDone)
{
  aTexture->ForceRemove();

  ReentrantMonitorAutoEnter autoMon(*aBarrier);
  *aDone = true;
  aBarrier->NotifyAll();
}

void ImageBridgeChild::RemoveTexture(TextureClient* aTexture)
{
  if (InImageBridgeChildThread()) {
    MOZ_ASSERT(!mShuttingDown);
    aTexture->ForceRemove();
    return;
  }

  ReentrantMonitor barrier("RemoveTexture Lock");
  ReentrantMonitorAutoEnter autoMon(barrier);
  bool done = false;

  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(&RemoveTextureSync, aTexture, &barrier, &done));

  
  
  while (!done) {
    barrier.Wait();
  }
}

bool ImageBridgeChild::IsSameProcess() const
{
  return OtherPid() == base::GetCurrentProcId();
}

void ImageBridgeChild::SendPendingAsyncMessges()
{
}

} 
} 
