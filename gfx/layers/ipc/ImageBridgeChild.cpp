




#include "ImageBridgeChild.h"
#include <vector>                       
#include "CompositorParent.h"           
#include "ImageBridgeParent.h"          
#include "ImageContainer.h"             
#include "Layers.h"                     
#include "ShadowLayers.h"               
#include "base/message_loop.h"          
#include "base/platform_thread.h"       
#include "base/process.h"               
#include "base/process_util.h"          
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
 
using namespace base;
using namespace mozilla::ipc;
using namespace mozilla::gfx;

namespace mozilla {
namespace ipc {
class Shmem;
}

namespace layers {

class PGrallocBufferChild;
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
    NS_ABORT_IF_FALSE(!Finished(), "forgot BeginTransaction?");
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
  AutoEndTransaction(CompositableTransaction* aTxn) : mTxn(aTxn) {}
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
  mTxn->AddNoSwapEdit(OpUseTexture(nullptr, aCompositable->GetIPDLActor(),
                                   nullptr, aTexture->GetIPDLActor()));
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

void
ImageBridgeChild::UpdatedTexture(CompositableClient* aCompositable,
                                 TextureClient* aTexture,
                                 nsIntRegion* aRegion)
{
  MOZ_ASSERT(aCompositable);
  MOZ_ASSERT(aTexture);
  MOZ_ASSERT(aCompositable->GetIPDLActor());
  MOZ_ASSERT(aTexture->GetIPDLActor());
  MaybeRegion region = aRegion ? MaybeRegion(*aRegion)
                               : MaybeRegion(null_t());
  mTxn->AddNoSwapEdit(OpUpdateTexture(nullptr, aCompositable->GetIPDLActor(),
                                      nullptr, aTexture->GetIPDLActor(),
                                      region));
}

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


static void StopImageBridgeSync(ReentrantMonitor *aBarrier, bool *aDone)
{
  ReentrantMonitorAutoEnter autoMon(*aBarrier);

  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");
  if (sImageBridgeChildSingleton) {

    sImageBridgeChildSingleton->SendStop();
  }
  *aDone = true;
  aBarrier->NotifyAll();
}


static void DeleteImageBridgeSync(ReentrantMonitor *aBarrier, bool *aDone)
{
  ReentrantMonitorAutoEnter autoMon(*aBarrier);

  NS_ABORT_IF_FALSE(InImageBridgeChildThread(),
                    "Should be in ImageBridgeChild thread.");
  sImageBridgeChildSingleton = nullptr;
  sImageBridgeParentSingleton = nullptr;
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


struct GrallocParam {
  IntSize size;
  uint32_t format;
  uint32_t usage;
  MaybeMagicGrallocBufferHandle* handle;
  PGrallocBufferChild** child;

  GrallocParam(const IntSize& aSize,
               const uint32_t& aFormat,
               const uint32_t& aUsage,
               MaybeMagicGrallocBufferHandle* aHandle,
               PGrallocBufferChild** aChild)
    : size(aSize)
    , format(aFormat)
    , usage(aUsage)
    , handle(aHandle)
    , child(aChild)
  {}
};


static void AllocGrallocBufferSync(const GrallocParam& aParam,
                                   Monitor* aBarrier,
                                   bool* aDone)
{
  MonitorAutoLock autoMon(*aBarrier);

  sImageBridgeChildSingleton->AllocGrallocBufferNow(aParam.size,
                                                    aParam.format,
                                                    aParam.usage,
                                                    aParam.handle,
                                                    aParam.child);
  *aDone = true;
  aBarrier->NotifyAll();
}


static void ConnectImageBridge(ImageBridgeChild * child, ImageBridgeParent * parent)
{
  MessageLoop *parentMsgLoop = parent->GetMessageLoop();
  ipc::MessageChannel *parentChannel = parent->GetIPCChannel();
  child->Open(parentChannel, parentMsgLoop, mozilla::ipc::ChildSide);
}

ImageBridgeChild::ImageBridgeChild()
{
  mTxn = new CompositableTransaction();
}
ImageBridgeChild::~ImageBridgeChild()
{
  delete mTxn;
}

void
ImageBridgeChild::Connect(CompositableClient* aCompositable)
{
  MOZ_ASSERT(aCompositable);
  uint64_t id = 0;
  PCompositableChild* child =
    SendPCompositableConstructor(aCompositable->GetTextureInfo(), &id);
  MOZ_ASSERT(child);
  aCompositable->InitIPDLActor(child, id);
}

PCompositableChild*
ImageBridgeChild::AllocPCompositableChild(const TextureInfo& aInfo, uint64_t* aID)
{
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
                                 ProcessHandle aOtherProcess)
{
  
  sImageBridgeChildSingleton->Open(aTransport, aOtherProcess,
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

static void FlushAllImagesSync(ImageClient* aClient, ImageContainer* aContainer, bool aExceptFront, ReentrantMonitor* aBarrier, bool* aDone)
{
  ImageBridgeChild::FlushAllImagesNow(aClient, aContainer, aExceptFront);

  ReentrantMonitorAutoEnter autoMon(*aBarrier);
  *aDone = true;
  aBarrier->NotifyAll();
}


void ImageBridgeChild::FlushAllImages(ImageClient* aClient, ImageContainer* aContainer, bool aExceptFront)
{
  if (InImageBridgeChildThread()) {
    FlushAllImagesNow(aClient, aContainer, aExceptFront);
    return;
  }

  ReentrantMonitor barrier("CreateImageClient Lock");
  ReentrantMonitorAutoEnter autoMon(barrier);
  bool done = false;

  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(&FlushAllImagesSync, aClient, aContainer, aExceptFront, &barrier, &done));

  
  
  while (!done) {
    barrier.Wait();
  }
}


void ImageBridgeChild::FlushAllImagesNow(ImageClient* aClient, ImageContainer* aContainer, bool aExceptFront)
{
  MOZ_ASSERT(aClient);
  sImageBridgeChildSingleton->BeginTransaction();
  if (aContainer && !aExceptFront) {
    aContainer->ClearCurrentImage();
  }
  aClient->FlushAllImages(aExceptFront);
  aClient->OnTransaction();
  sImageBridgeChildSingleton->EndTransaction();
}

void
ImageBridgeChild::BeginTransaction()
{
  MOZ_ASSERT(mTxn->Finished(), "uncommitted txn?");
  mTxn->Begin();
}

class MOZ_STACK_CLASS AutoRemoveTextures
{
public:
  AutoRemoveTextures(ImageBridgeChild* aImageBridge)
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
  ShadowLayerForwarder::PlatformSyncBeforeUpdate();

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
    case EditReply::TOpTextureSwap: {
      const OpTextureSwap& ots = reply.get_OpTextureSwap();

      CompositableClient* compositable =
        CompositableClient::FromIPDLActor(ots.compositableChild());

      MOZ_ASSERT(compositable);

      compositable->SetDescriptorFromReply(ots.textureId(), ots.image());
      break;
    }
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
}


PImageBridgeChild*
ImageBridgeChild::StartUpInChildProcess(Transport* aTransport,
                                        ProcessId aOtherProcess)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main Thread!");

  ProcessHandle processHandle;
  if (!base::OpenProcessHandle(aOtherProcess, &processHandle)) {
    return nullptr;
  }

  sImageBridgeChildThread = new Thread("ImageBridgeChild");
  if (!sImageBridgeChildThread->Start()) {
    return nullptr;
  }

  sImageBridgeChildSingleton = new ImageBridgeChild();
  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(ConnectImageBridgeInChildProcess,
                        aTransport, processHandle));

  return sImageBridgeChildSingleton;
}

void ImageBridgeChild::ShutDown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on the main Thread!");
  if (ImageBridgeChild::IsCreated()) {
    ImageBridgeChild::DestroyBridge();
    delete sImageBridgeChildThread;
    sImageBridgeChildThread = nullptr;
  }
}

bool ImageBridgeChild::StartUpOnThread(Thread* aThread)
{
  NS_ABORT_IF_FALSE(aThread, "ImageBridge needs a thread.");
  if (sImageBridgeChildSingleton == nullptr) {
    sImageBridgeChildThread = aThread;
    if (!aThread->IsRunning()) {
      aThread->Start();
    }
    sImageBridgeChildSingleton = new ImageBridgeChild();
    sImageBridgeParentSingleton = new ImageBridgeParent(
      CompositorParent::CompositorLoop(), nullptr);
    sImageBridgeChildSingleton->ConnectAsync(sImageBridgeParentSingleton);
    return true;
  } else {
    return false;
  }
}

void ImageBridgeChild::DestroyBridge()
{
  NS_ABORT_IF_FALSE(!InImageBridgeChildThread(),
                    "This method must not be called in this thread.");
  
  

  if (!IsCreated()) {
    return;
  }

  ReentrantMonitor barrier("ImageBridgeDestroyTask lock");
  ReentrantMonitorAutoEnter autoMon(barrier);

  bool done = false;
  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(FROM_HERE,
                  NewRunnableFunction(&StopImageBridgeSync, &barrier, &done));
  while (!done) {
    barrier.Wait();
  }

  done = false;
  sImageBridgeChildSingleton->GetMessageLoop()->PostTask(FROM_HERE,
                  NewRunnableFunction(&DeleteImageBridgeSync, &barrier, &done));
  while (!done) {
    barrier.Wait();
  }

}

bool InImageBridgeChildThread()
{
  return sImageBridgeChildThread->thread_id() == PlatformThread::CurrentId();
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
  RefPtr<ImageClient> client
    = ImageClient::CreateImageClient(aType, this, 0);
  MOZ_ASSERT(client, "failed to create ImageClient");
  if (client) {
    client->Connect();
  }
  return client.forget();
}

PGrallocBufferChild*
ImageBridgeChild::AllocPGrallocBufferChild(const IntSize&, const uint32_t&, const uint32_t&,
                                           MaybeMagicGrallocBufferHandle*)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  return GrallocBufferActor::Create();
#else
  NS_RUNTIMEABORT("No gralloc buffers for you");
  return nullptr;
#endif
}

bool
ImageBridgeChild::DeallocPGrallocBufferChild(PGrallocBufferChild* actor)
{
#ifdef MOZ_HAVE_SURFACEDESCRIPTORGRALLOC
  delete actor;
  return true;
#else
  NS_RUNTIMEABORT("Um, how did we get here?");
  return false;
#endif
}

bool
ImageBridgeChild::AllocUnsafeShmem(size_t aSize,
                                   ipc::SharedMemory::SharedMemoryType aType,
                                   ipc::Shmem* aShmem)
{
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

PGrallocBufferChild*
ImageBridgeChild::AllocGrallocBuffer(const IntSize& aSize,
                                     uint32_t aFormat,
                                     uint32_t aUsage,
                                     MaybeMagicGrallocBufferHandle* aHandle)
{
  if (InImageBridgeChildThread()) {
    PGrallocBufferChild* child = nullptr;
    ImageBridgeChild::AllocGrallocBufferNow(aSize, aFormat, aUsage, aHandle, &child);
    return child;
  }

  Monitor barrier("AllocGrallocBuffer Lock");
  MonitorAutoLock autoMon(barrier);
  bool done = false;
  PGrallocBufferChild* child = nullptr;

  GetMessageLoop()->PostTask(
    FROM_HERE,
    NewRunnableFunction(&AllocGrallocBufferSync,
                        GrallocParam(aSize, aFormat, aUsage, aHandle, &child), &barrier, &done));

  while (!done) {
    barrier.Wait();
  }

  return child;
}

void
ImageBridgeChild::AllocGrallocBufferNow(const gfx::IntSize& aSize,
                                        uint32_t aFormat, uint32_t aUsage,
                                        MaybeMagicGrallocBufferHandle* aHandle,
                                        PGrallocBufferChild** aChild)
{
#ifdef MOZ_WIDGET_GONK
  *aChild = SendPGrallocBufferConstructor(aSize,
                                          aFormat,
                                          aUsage,
                                          aHandle);
#else
  NS_RUNTIMEABORT("not implemented");
  aChild = nullptr;
#endif
}

static void ProxyDeallocGrallocBufferNow(ISurfaceAllocator* aAllocator,
                                         PGrallocBufferChild* aChild,
                                         ReentrantMonitor* aBarrier,
                                         bool* aDone)
{
  MOZ_ASSERT(aChild);
  MOZ_ASSERT(aDone);
  MOZ_ASSERT(aBarrier);

#ifdef MOZ_WIDGET_GONK
  PGrallocBufferChild::Send__delete__(aChild);
#else
  NS_RUNTIMEABORT("not implemented");
#endif

  ReentrantMonitorAutoEnter autoMon(*aBarrier);
  *aDone = true;
  aBarrier->NotifyAll();
}

void
ImageBridgeChild::DeallocGrallocBuffer(PGrallocBufferChild* aChild)
{
  MOZ_ASSERT(aChild);
  if (InImageBridgeChildThread()) {
#ifdef MOZ_WIDGET_GONK
    PGrallocBufferChild::Send__delete__(aChild);
#else
    NS_RUNTIMEABORT("not implemented");
#endif
  } else {
    ReentrantMonitor barrier("AllocatorProxy Dealloc");
    ReentrantMonitorAutoEnter autoMon(barrier);

    bool done = false;
    GetMessageLoop()->PostTask(FROM_HERE,
                               NewRunnableFunction(&ProxyDeallocGrallocBufferNow,
                                                   this,
                                                   aChild,
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
  return TextureClient::CreateIPDLActor();
}

bool
ImageBridgeChild::DeallocPTextureChild(PTextureChild* actor)
{
  return TextureClient::DestroyIPDLActor(actor);
}

PTextureChild*
ImageBridgeChild::CreateTexture(const SurfaceDescriptor& aSharedData,
                                TextureFlags aFlags)
{
  return SendPTextureConstructor(aSharedData, aFlags);
}

void
ImageBridgeChild::RemoveTextureFromCompositable(CompositableClient* aCompositable,
                                                TextureClient* aTexture)
{
  if (aTexture->GetFlags() & TEXTURE_DEALLOCATE_CLIENT) {
    mTxn->AddEdit(OpRemoveTexture(nullptr, aCompositable->GetIPDLActor(),
                                  nullptr, aTexture->GetIPDLActor()));
  } else {
    mTxn->AddNoSwapEdit(OpRemoveTexture(nullptr, aCompositable->GetIPDLActor(),
                                        nullptr, aTexture->GetIPDLActor()));
  }
  
  HoldUntilTransaction(aTexture);
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
  return OtherProcess() == ipc::kInvalidProcessHandle;
}

} 
} 
