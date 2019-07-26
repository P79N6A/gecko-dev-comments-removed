




#ifndef MOZILLA_GFX_CANVASCLIENT_H
#define MOZILLA_GFX_CANVASCLIENT_H

#include "mozilla/Assertions.h"         
#include "mozilla/Attributes.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/layers/CompositableClient.h"  
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersSurfaces.h"  
#include "mozilla/layers/TextureClient.h"  
#include "mozilla/mozalloc.h"           

#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Types.h"          

namespace mozilla {
namespace layers {

class ClientCanvasLayer;
class CompositableForwarder;




class CanvasClient : public CompositableClient
{
public:
  




  enum CanvasClientType {
    CanvasClientSurface,
    CanvasClientGLContext,
  };
  static TemporaryRef<CanvasClient> CreateCanvasClient(CanvasClientType aType,
                                                       CompositableForwarder* aFwd,
                                                       TextureFlags aFlags);

  CanvasClient(CompositableForwarder* aFwd, TextureFlags aFlags)
    : CompositableClient(aFwd)
  {
    mTextureInfo.mTextureFlags = aFlags;
  }

  virtual ~CanvasClient() {}

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer) = 0;

  virtual void Updated() { }

protected:
  TextureInfo mTextureInfo;
};


class CanvasClient2D : public CanvasClient
{
public:
  CanvasClient2D(CompositableForwarder* aLayerForwarder,
                 TextureFlags aFlags)
    : CanvasClient(aLayerForwarder, aFlags)
  {
  }

  TextureInfo GetTextureInfo() const
  {
    return TextureInfo(COMPOSITABLE_IMAGE);
  }

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer) MOZ_OVERRIDE;

  virtual bool AddTextureClient(TextureClient* aTexture) MOZ_OVERRIDE
  {
    MOZ_ASSERT((mTextureInfo.mTextureFlags & aTexture->GetFlags()) == mTextureInfo.mTextureFlags);
    return CompositableClient::AddTextureClient(aTexture);
  }

  virtual TemporaryRef<BufferTextureClient>
  CreateBufferTextureClient(gfx::SurfaceFormat aFormat) MOZ_OVERRIDE;

  virtual void OnDetach() MOZ_OVERRIDE
  {
    mBuffer = nullptr;
  }

private:
  RefPtr<TextureClient> mBuffer;
};
class DeprecatedCanvasClient2D : public CanvasClient
{
public:
  DeprecatedCanvasClient2D(CompositableForwarder* aLayerForwarder,
                           TextureFlags aFlags);

  TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return mTextureInfo;
  }

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer);
  virtual void Updated() MOZ_OVERRIDE;

  virtual void SetDescriptorFromReply(TextureIdentifier aTextureId,
                                      const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE
  {
    mDeprecatedTextureClient->SetDescriptorFromReply(aDescriptor);
  }

private:
  RefPtr<DeprecatedTextureClient> mDeprecatedTextureClient;
};



class DeprecatedCanvasClientSurfaceStream : public CanvasClient
{
public:
  DeprecatedCanvasClientSurfaceStream(CompositableForwarder* aFwd,
                                      TextureFlags aFlags);

  TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return mTextureInfo;
  }

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer);
  virtual void Updated() MOZ_OVERRIDE;

  virtual void SetDescriptorFromReply(TextureIdentifier aTextureId,
                                      const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE
  {
    mDeprecatedTextureClient->SetDescriptorFromReply(aDescriptor);
  }

private:
  RefPtr<DeprecatedTextureClient> mDeprecatedTextureClient;
};

}
}

#endif
