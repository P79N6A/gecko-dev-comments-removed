




#ifndef MOZILLA_GFX_BUFFERCLIENT_H
#define MOZILLA_GFX_BUFFERCLIENT_H

#include <stdint.h>                     
#include <vector>                       
#include <map>                          
#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"
#include "mozilla/layers/LayersTypes.h"  
#include "nsISupportsImpl.h"            

namespace mozilla {
namespace layers {

class AsyncTransactionTracker;
class CompositableClient;
class TextureClient;
class BufferTextureClient;
class ImageBridgeChild;
class CompositableForwarder;
class CompositableChild;
class SurfaceDescriptor;
class PCompositableChild;






































class CompositableClient
{
protected:
  virtual ~CompositableClient();

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(CompositableClient)

  CompositableClient(CompositableForwarder* aForwarder, TextureFlags aFlags = TextureFlags::NO_FLAGS);

  virtual TextureInfo GetTextureInfo() const = 0;

  LayersBackend GetCompositorBackendType() const;

  TemporaryRef<BufferTextureClient>
  CreateBufferTextureClient(gfx::SurfaceFormat aFormat,
                            TextureFlags aFlags = TextureFlags::DEFAULT,
                            gfx::BackendType aMoz2dBackend = gfx::BackendType::NONE);

  TemporaryRef<TextureClient>
  CreateTextureClientForDrawing(gfx::SurfaceFormat aFormat,
                                TextureFlags aTextureFlags,
                                gfx::BackendType aMoz2dBackend,
                                const gfx::IntSize& aSizeHint);

  virtual void SetDescriptorFromReply(TextureIdentifier aTextureId,
                                      const SurfaceDescriptor& aDescriptor)
  {
    MOZ_CRASH("If you want to call this, you should have implemented it");
  }

  


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

  



  virtual void ClearCachedResources() {}

  






  virtual void RemoveTexture(TextureClient* aTexture);

  static CompositableClient* FromIPDLActor(PCompositableChild* aActor);

  







  static PCompositableChild* CreateIPDLActor();

  static bool DestroyIPDLActor(PCompositableChild* actor);

  void InitIPDLActor(PCompositableChild* aActor, uint64_t aAsyncID = 0);

  static void TransactionCompleteted(PCompositableChild* aActor, uint64_t aTransactionId);

  static void HoldUntilComplete(PCompositableChild* aActor, AsyncTransactionTracker* aTracker);

protected:
  CompositableChild* mCompositableChild;
  CompositableForwarder* mForwarder;
  
  
  TextureFlags mTextureFlags;

  friend class CompositableChild;
};




struct AutoRemoveTexture MOZ_STACK_CLASS {
  AutoRemoveTexture(CompositableClient* aCompositable,
                    TextureClient* aTexture = nullptr)
    : mCompositable(aCompositable)
    , mTexture(aTexture)
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
