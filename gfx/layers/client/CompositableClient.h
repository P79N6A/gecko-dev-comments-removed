




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
#include "mozilla/layers/PCompositableChild.h"  
#include "nsISupportsImpl.h"            
#include "gfxASurface.h"                

namespace mozilla {
namespace layers {

class CompositableClient;
class DeprecatedTextureClient;
class TextureClient;
class BufferTextureClient;
class ImageBridgeChild;
class CompositableForwarder;
class CompositableChild;
class SurfaceDescriptor;
class TextureClientData;







































class CompositableClient : public AtomicRefCounted<CompositableClient>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(CompositableClient)
  CompositableClient(CompositableForwarder* aForwarder, TextureFlags aFlags = 0);

  virtual ~CompositableClient();

  virtual TextureInfo GetTextureInfo() const = 0;

  LayersBackend GetCompositorBackendType() const;

  TemporaryRef<DeprecatedTextureClient>
  CreateDeprecatedTextureClient(DeprecatedTextureClientType aDeprecatedTextureClientType,
                                gfxContentType aContentType = gfxContentType::SENTINEL);

  TemporaryRef<BufferTextureClient>
  CreateBufferTextureClient(gfx::SurfaceFormat aFormat,
                            TextureFlags aFlags = TEXTURE_FLAGS_DEFAULT);

  
  
  TemporaryRef<TextureClient>
  CreateTextureClientForDrawing(gfx::SurfaceFormat aFormat,
                                TextureFlags aTextureFlags);

  virtual void SetDescriptorFromReply(TextureIdentifier aTextureId,
                                      const SurfaceDescriptor& aDescriptor)
  {
    MOZ_CRASH("If you want to call this, you should have implemented it");
  }

  


  virtual bool Connect();

  void Destroy();

  CompositableChild* GetIPDLActor() const;

  
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

protected:
  CompositableChild* mCompositableChild;
  CompositableForwarder* mForwarder;
  
  
  TextureFlags mTextureFlags;

  friend class CompositableChild;
};







class CompositableChild : public PCompositableChild
{
public:
  CompositableChild()
  : mCompositableClient(nullptr), mID(0)
  {
    MOZ_COUNT_CTOR(CompositableChild);
  }
  ~CompositableChild()
  {
    MOZ_COUNT_DTOR(CompositableChild);
  }

  void Destroy();

  void SetClient(CompositableClient* aClient)
  {
    mCompositableClient = aClient;
  }

  CompositableClient* GetCompositableClient() const
  {
    return mCompositableClient;
  }

  virtual void ActorDestroy(ActorDestroyReason) MOZ_OVERRIDE {
    if (mCompositableClient) {
      mCompositableClient->mCompositableChild = nullptr;
    }
  }

  void SetAsyncID(uint64_t aID) { mID = aID; }
  uint64_t GetAsyncID() const
  {
    return mID;
  }
private:
  CompositableClient* mCompositableClient;
  uint64_t mID;
};

} 
} 

#endif
