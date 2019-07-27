




#include "mozilla/layers/CompositableClient.h"
#include <stdint.h>                     
#include "gfxPlatform.h"                
#include "mozilla/layers/CompositableForwarder.h"
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/TextureClientOGL.h"
#include "mozilla/mozalloc.h"           
#include "mozilla/layers/PCompositableChild.h"
#ifdef XP_WIN
#include "gfxWindowsPlatform.h"         
#include "mozilla/layers/TextureD3D11.h"
#include "mozilla/layers/TextureD3D9.h"
#endif
#include "gfxUtils.h"

namespace mozilla {
namespace layers {

using namespace mozilla::gfx;







class CompositableChild : public PCompositableChild
                        , public AsyncTransactionTrackersHolder
{
public:
  CompositableChild()
  : mCompositableClient(nullptr), mAsyncID(0)
  {
    MOZ_COUNT_CTOR(CompositableChild);
  }

  virtual ~CompositableChild()
  {
    MOZ_COUNT_DTOR(CompositableChild);
  }

  virtual void ActorDestroy(ActorDestroyReason) override {
    DestroyAsyncTransactionTrackersHolder();
    if (mCompositableClient) {
      mCompositableClient->mCompositableChild = nullptr;
    }
  }

  CompositableClient* mCompositableClient;

  uint64_t mAsyncID;
};

void
RemoveTextureFromCompositableTracker::ReleaseTextureClient()
{
  if (mTextureClient &&
      mTextureClient->GetAllocator() &&
      !mTextureClient->GetAllocator()->IsImageBridgeChild())
  {
    TextureClientReleaseTask* task = new TextureClientReleaseTask(mTextureClient);
    RefPtr<ISurfaceAllocator> allocator = mTextureClient->GetAllocator();
    mTextureClient = nullptr;
    allocator->GetMessageLoop()->PostTask(FROM_HERE, task);
  } else {
    mTextureClient = nullptr;
  }
}

 void
CompositableClient::TransactionCompleteted(PCompositableChild* aActor, uint64_t aTransactionId)
{
  CompositableChild* child = static_cast<CompositableChild*>(aActor);
  child->TransactionCompleteted(aTransactionId);
}

 void
CompositableClient::HoldUntilComplete(PCompositableChild* aActor, AsyncTransactionTracker* aTracker)
{
  CompositableChild* child = static_cast<CompositableChild*>(aActor);
  child->HoldUntilComplete(aTracker);
}

 uint64_t
CompositableClient::GetTrackersHolderId(PCompositableChild* aActor)
{
  CompositableChild* child = static_cast<CompositableChild*>(aActor);
  return child->GetId();
}

 PCompositableChild*
CompositableClient::CreateIPDLActor()
{
  return new CompositableChild();
}

 bool
CompositableClient::DestroyIPDLActor(PCompositableChild* actor)
{
  delete actor;
  return true;
}

void
CompositableClient::InitIPDLActor(PCompositableChild* aActor, uint64_t aAsyncID)
{
  MOZ_ASSERT(aActor);
  CompositableChild* child = static_cast<CompositableChild*>(aActor);
  mCompositableChild = child;
  child->mCompositableClient = this;
  child->mAsyncID = aAsyncID;
}

 CompositableClient*
CompositableClient::FromIPDLActor(PCompositableChild* aActor)
{
  MOZ_ASSERT(aActor);
  return static_cast<CompositableChild*>(aActor)->mCompositableClient;
}

CompositableClient::CompositableClient(CompositableForwarder* aForwarder,
                                       TextureFlags aTextureFlags)
: mCompositableChild(nullptr)
, mForwarder(aForwarder)
, mTextureFlags(aTextureFlags)
, mDestroyed(false)
{
  MOZ_COUNT_CTOR(CompositableClient);
}

CompositableClient::~CompositableClient()
{
  MOZ_COUNT_DTOR(CompositableClient);
  Destroy();
}

LayersBackend
CompositableClient::GetCompositorBackendType() const
{
  return mForwarder->GetCompositorBackendType();
}

void
CompositableClient::SetIPDLActor(CompositableChild* aChild)
{
  mCompositableChild = aChild;
}

PCompositableChild*
CompositableClient::GetIPDLActor() const
{
  return mCompositableChild;
}

bool
CompositableClient::Connect()
{
  if (!GetForwarder() || GetIPDLActor()) {
    return false;
  }
  GetForwarder()->Connect(this);
  return true;
}

void
CompositableClient::Destroy()
{
  mDestroyed = true;

  if (!mCompositableChild) {
    return;
  }
  
  
  mForwarder->SendPendingAsyncMessges();
  
  mCompositableChild->mCompositableClient = nullptr;
  PCompositableChild::Send__delete__(mCompositableChild);
  mCompositableChild = nullptr;
}

uint64_t
CompositableClient::GetAsyncID() const
{
  if (mCompositableChild) {
    return mCompositableChild->mAsyncID;
  }
  return 0; 
}

already_AddRefed<BufferTextureClient>
CompositableClient::CreateBufferTextureClient(gfx::SurfaceFormat aFormat,
                                              gfx::IntSize aSize,
                                              gfx::BackendType aMoz2DBackend,
                                              TextureFlags aTextureFlags)
{
  return TextureClient::CreateForRawBufferAccess(GetForwarder(),
                                                 aFormat, aSize, aMoz2DBackend,
                                                 aTextureFlags | mTextureFlags);
}

already_AddRefed<TextureClient>
CompositableClient::CreateTextureClientForDrawing(gfx::SurfaceFormat aFormat,
                                                  gfx::IntSize aSize,
                                                  gfx::BackendType aMoz2DBackend,
                                                  TextureFlags aTextureFlags,
                                                  TextureAllocationFlags aAllocFlags)
{
  return TextureClient::CreateForDrawing(GetForwarder(),
                                         aFormat, aSize, aMoz2DBackend,
                                         aTextureFlags | mTextureFlags,
                                         aAllocFlags);
}

bool
CompositableClient::AddTextureClient(TextureClient* aClient)
{
  if(!aClient || !aClient->IsAllocated()) {
    return false;
  }
  aClient->SetAddedToCompositableClient();
  return aClient->InitIPDLActor(mForwarder);
}

void
CompositableClient::OnTransaction()
{
}

void
CompositableClient::ClearCachedResources()
{
  if (mTextureClientRecycler) {
    mTextureClientRecycler = nullptr;
  }
}

void
CompositableClient::RemoveTexture(TextureClient* aTexture)
{
  mForwarder->RemoveTextureFromCompositable(this, aTexture);
}

TextureClientRecycleAllocator*
CompositableClient::GetTextureClientRecycler()
{
  if (mTextureClientRecycler) {
    return mTextureClientRecycler;
  }

  if (!mForwarder) {
    return nullptr;
  }

  mTextureClientRecycler =
    new layers::TextureClientRecycleAllocator(mForwarder);
  return mTextureClientRecycler;
}

void
CompositableClient::DumpTextureClient(std::stringstream& aStream, TextureClient* aTexture)
{
  if (!aTexture) {
    return;
  }
  RefPtr<gfx::DataSourceSurface> dSurf = aTexture->GetAsSurface();
  if (!dSurf) {
    return;
  }
  aStream << gfxUtils::GetAsLZ4Base64Str(dSurf).get();
}

} 
} 
