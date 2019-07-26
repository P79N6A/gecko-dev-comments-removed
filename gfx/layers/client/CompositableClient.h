




#ifndef MOZILLA_GFX_BUFFERCLIENT_H
#define MOZILLA_GFX_BUFFERCLIENT_H

#include "mozilla/layers/PCompositableChild.h"
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace layers {

class CompositableChild;
class CompositableClient;
class TextureClient;
class ShadowLayersChild;
class ImageBridgeChild;
class ShadowableLayer;
class CompositableForwarder;
class CompositableChild;







































class CompositableClient : public RefCounted<CompositableClient>
{
public:
  CompositableClient(CompositableForwarder* aForwarder)
  : mCompositableChild(nullptr), mForwarder(aForwarder)
  {
    MOZ_COUNT_CTOR(CompositableClient);
  }

  virtual ~CompositableClient();

  virtual CompositableType GetType() const
  {
    NS_WARNING("This method should be overridden");
    return BUFFER_UNKNOWN;
  }

  LayersBackend GetCompositorBackendType() const;

  TemporaryRef<TextureClient>
  CreateTextureClient(TextureClientType aTextureClientType,
                      TextureFlags aFlags);

  


  virtual bool Connect();

  void Destroy();

  CompositableChild* GetIPDLActor() const;

  
  void SetIPDLActor(CompositableChild* aChild);

  CompositableForwarder* GetForwarder() const
  {
    return mForwarder;
  }

  




  uint64_t GetAsyncID() const;

protected:
  CompositableChild* mCompositableChild;
  CompositableForwarder* mForwarder;
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

  virtual PTextureChild* AllocPTexture(const TextureInfo& aInfo) MOZ_OVERRIDE;
  virtual bool DeallocPTexture(PTextureChild* aActor) MOZ_OVERRIDE;

  void Destroy();

  void SetClient(CompositableClient* aClient)
  {
    mCompositableClient = aClient;
  }

  CompositableClient* GetCompositableClient() const
  {
    return mCompositableClient;
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
