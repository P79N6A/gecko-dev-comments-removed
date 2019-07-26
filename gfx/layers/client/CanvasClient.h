




#ifndef MOZILLA_GFX_CANVASCLIENT_H
#define MOZILLA_GFX_CANVASCLIENT_H

#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/CompositableClient.h"

namespace mozilla {

namespace layers {

class ClientCanvasLayer;




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

  virtual void Updated();

  virtual void SetDescriptorFromReply(TextureIdentifier aTextureId,
                                      const SurfaceDescriptor& aDescriptor) MOZ_OVERRIDE
  {
    mDeprecatedTextureClient->SetDescriptorFromReply(aDescriptor);
  }
protected:
  RefPtr<DeprecatedTextureClient> mDeprecatedTextureClient;
  TextureInfo mTextureInfo;
};


class CanvasClient2D : public CanvasClient
{
public:
  CanvasClient2D(CompositableForwarder* aLayerForwarder,
                 TextureFlags aFlags);

  TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return mTextureInfo;
  }

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer);
};



class CanvasClientSurfaceStream : public CanvasClient
{
public:
  CanvasClientSurfaceStream(CompositableForwarder* aFwd,
                            TextureFlags aFlags);

  TextureInfo GetTextureInfo() const MOZ_OVERRIDE
  {
    return mTextureInfo;
  }

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer);
  virtual void Updated() MOZ_OVERRIDE;

private:
  bool mNeedsUpdate;
};

}
}

#endif
