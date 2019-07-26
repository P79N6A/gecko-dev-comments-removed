




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

  virtual TextureInfo GetTextureInfo() const
  {
    MOZ_NOT_REACHED("This method should be overridden");
    return TextureInfo();
  }

  LayersBackend GetCompositorBackendType() const;

  TemporaryRef<TextureClient>
  CreateTextureClient(TextureClientType aTextureClientType);

  virtual void SetDescriptorFromReply(TextureIdentifier aTextureId,
                                      const SurfaceDescriptor& aDescriptor)
  {
    MOZ_NOT_REACHED("If you want to call this, you should have implemented it");
  }

  


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
