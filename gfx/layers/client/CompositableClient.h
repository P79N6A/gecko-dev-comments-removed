




#ifndef MOZILLA_GFX_BUFFERCLIENT_H
#define MOZILLA_GFX_BUFFERCLIENT_H

#include "mozilla/layers/PCompositableChild.h"
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace layers {

class CompositableChild;
class CompositableClient;
class DeprecatedTextureClient;
class TextureClient;
class BufferTextureClient;
class ImageBridgeChild;
class ShadowableLayer;
class CompositableForwarder;
class CompositableChild;
class SurfaceDescriptor;







































class CompositableClient : public RefCounted<CompositableClient>
{
public:
  CompositableClient(CompositableForwarder* aForwarder);

  virtual ~CompositableClient();

  virtual TextureInfo GetTextureInfo() const = 0;

  LayersBackend GetCompositorBackendType() const;

  TemporaryRef<DeprecatedTextureClient>
  CreateDeprecatedTextureClient(DeprecatedTextureClientType aDeprecatedTextureClientType);

  TemporaryRef<BufferTextureClient>
  CreateBufferTextureClient(gfx::SurfaceFormat aFormat, TextureFlags aFlags);

  virtual TemporaryRef<BufferTextureClient>
  CreateBufferTextureClient(gfx::SurfaceFormat aFormat);

  virtual void SetDescriptorFromReply(TextureIdentifier aTextureId,
                                      const SurfaceDescriptor& aDescriptor)
  {
    MOZ_CRASH("If you want to call this, you should have implemented it");
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

  


  virtual void AddTextureClient(TextureClient* aClient);

  



  virtual void RemoveTextureClient(TextureClient* aClient);

  


  virtual void OnTransaction();

  


  virtual void Detach() {}

protected:
  
  std::vector<uint64_t> mTexturesToRemove;
  uint64_t mNextTextureID;
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
