




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
#include "nsTraceRefcnt.h"              
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
  CompositableClient(CompositableForwarder* aForwarder);

  virtual ~CompositableClient();

  virtual TextureInfo GetTextureInfo() const = 0;

  LayersBackend GetCompositorBackendType() const;

  TemporaryRef<DeprecatedTextureClient>
  CreateDeprecatedTextureClient(DeprecatedTextureClientType aDeprecatedTextureClientType,
                                gfxContentType aContentType = GFX_CONTENT_SENTINEL);

  virtual TemporaryRef<BufferTextureClient>
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

  
  virtual void SetIPDLActor(CompositableChild* aChild);

  CompositableForwarder* GetForwarder() const
  {
    return mForwarder;
  }

  




  uint64_t GetAsyncID() const;

  


  virtual void AddTextureClient(TextureClient* aClient);

  



  virtual void RemoveTextureClient(TextureClient* aClient);

  


  virtual void OnTransaction();

  


  virtual void OnDetach() {}

  void OnReplyTextureRemoved(uint64_t aTextureID);

  void FlushTexturesToRemoveCallbacks();
protected:
  struct TextureIDAndFlags {
    TextureIDAndFlags(uint64_t aID, TextureFlags aFlags)
    : mID(aID), mFlags(aFlags) {}
    uint64_t mID;
    TextureFlags mFlags;
  };
  
  nsTArray<TextureIDAndFlags> mTexturesToRemove;
  std::map<uint64_t, TextureClientData*> mTexturesToRemoveCallbacks;
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
