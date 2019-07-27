




#ifndef MOZILLA_GFX_BUFFERCLIENT_H
#define MOZILLA_GFX_BUFFERCLIENT_H

#include <stdint.h>                     
#include <vector>                       
#include <map>                          
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/AsyncTransactionTracker.h" 
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersTypes.h"  
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/layers/TextureClientRecycleAllocator.h" 
#include "nsISupportsImpl.h"            

namespace mozilla {
namespace layers {

class CompositableClient;
class BufferTextureClient;
class ImageBridgeChild;
class CompositableForwarder;
class CompositableChild;
class PCompositableChild;




class RemoveTextureFromCompositableTracker : public AsyncTransactionTracker {
public:
  RemoveTextureFromCompositableTracker()
  {
    MOZ_COUNT_CTOR(RemoveTextureFromCompositableTracker);
  }

protected:
  ~RemoveTextureFromCompositableTracker()
  {
    MOZ_COUNT_DTOR(RemoveTextureFromCompositableTracker);
    ReleaseTextureClient();
  }

public:
  virtual void Complete() override
  {
    ReleaseTextureClient();
  }

  virtual void Cancel() override
  {
    ReleaseTextureClient();
  }

  virtual void SetTextureClient(TextureClient* aTextureClient) override
  {
    ReleaseTextureClient();
    mTextureClient = aTextureClient;
  }

  virtual void SetReleaseFenceHandle(FenceHandle& aReleaseFenceHandle) override
  {
    if (mTextureClient) {
      mTextureClient->SetReleaseFenceHandle(aReleaseFenceHandle);
    }
  }

protected:
  void ReleaseTextureClient();

private:
  RefPtr<TextureClient> mTextureClient;
};







































class CompositableClient
{
protected:
  virtual ~CompositableClient();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositableClient)

  explicit CompositableClient(CompositableForwarder* aForwarder, TextureFlags aFlags = TextureFlags::NO_FLAGS);

  virtual TextureInfo GetTextureInfo() const = 0;

  LayersBackend GetCompositorBackendType() const;

  TemporaryRef<BufferTextureClient>
  CreateBufferTextureClient(gfx::SurfaceFormat aFormat,
                            gfx::IntSize aSize,
                            gfx::BackendType aMoz2dBackend = gfx::BackendType::NONE,
                            TextureFlags aFlags = TextureFlags::DEFAULT);

  TemporaryRef<TextureClient>
  CreateTextureClientForDrawing(gfx::SurfaceFormat aFormat,
                                gfx::IntSize aSize,
                                gfx::BackendType aMoz2DBackend,
                                TextureFlags aTextureFlags,
                                TextureAllocationFlags aAllocFlags = ALLOC_DEFAULT);

  


  virtual bool Connect();

  void Destroy();

  PCompositableChild* GetIPDLActor() const;

  
  virtual void SetIPDLActor(CompositableChild* aChild);

  CompositableForwarder* GetForwarder() const
  {
    return mForwarder;
  }

  







  uint64_t GetAsyncID() const;

  


  virtual bool AddTextureClient(TextureClient* aClient);

  


  virtual void OnTransaction();

  


  virtual void OnDetach() {}

  



  virtual void ClearCachedResources();

  






  virtual void RemoveTexture(TextureClient* aTexture);

  static CompositableClient* FromIPDLActor(PCompositableChild* aActor);

  







  static PCompositableChild* CreateIPDLActor();

  static bool DestroyIPDLActor(PCompositableChild* actor);

  void InitIPDLActor(PCompositableChild* aActor, uint64_t aAsyncID = 0);

  static void TransactionCompleteted(PCompositableChild* aActor, uint64_t aTransactionId);

  static void HoldUntilComplete(PCompositableChild* aActor, AsyncTransactionTracker* aTracker);

  static uint64_t GetTrackersHolderId(PCompositableChild* aActor);

  TextureFlags GetTextureFlags() const { return mTextureFlags; }

  TextureClientRecycleAllocator* GetTextureClientRecycler();

  static void DumpTextureClient(std::stringstream& aStream, TextureClient* aTexture);
protected:
  CompositableChild* mCompositableChild;
  CompositableForwarder* mForwarder;
  
  
  TextureFlags mTextureFlags;
  RefPtr<TextureClientRecycleAllocator> mTextureClientRecycler;

  friend class CompositableChild;
};




struct AutoRemoveTexture
{
  explicit AutoRemoveTexture(CompositableClient* aCompositable,
                             TextureClient* aTexture = nullptr)
    : mTexture(aTexture)
    , mCompositable(aCompositable)
  {}

  ~AutoRemoveTexture()
  {
    if (mCompositable && mTexture) {
      mCompositable->RemoveTexture(mTexture);
    }
  }

  RefPtr<TextureClient> mTexture;
private:
  CompositableClient* mCompositable;
};

} 
} 

#endif
