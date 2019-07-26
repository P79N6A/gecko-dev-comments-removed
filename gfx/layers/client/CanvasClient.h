




#ifndef MOZILLA_GFX_CANVASCLIENT_H
#define MOZILLA_GFX_CANVASCLIENT_H

#include "mozilla/layers/TextureClient.h"
#include "mozilla/layers/CompositableClient.h"

namespace mozilla {

namespace layers {

class BasicCanvasLayer;
class TextureIdentifier;




class CanvasClient : public CompositableClient
{
public:
  




  static TemporaryRef<CanvasClient> CreateCanvasClient(LayersBackend aBackendType,
                                                       CompositableType aImageHostType,
                                                       CompositableForwarder* aFwd,
                                                       TextureFlags aFlags);

  CanvasClient(CompositableForwarder* aFwd, TextureFlags aFlags)
  : CompositableClient(aFwd), mFlags(aFlags)
  {}

  virtual ~CanvasClient() {}

  virtual void Update(gfx::IntSize aSize, BasicCanvasLayer* aLayer) = 0;

  virtual void SetBuffer(const TextureIdentifier& aTextureIdentifier,
                         const SurfaceDescriptor& aBuffer);
  virtual void Updated()
  {
    mTextureClient->Updated();
  }
protected:
  RefPtr<TextureClient> mTextureClient;
  TextureFlags mFlags;
};


class CanvasClient2D : public CanvasClient
{
public:
  CanvasClient2D(CompositableForwarder* aLayerForwarder,
                 TextureFlags aFlags);

  CompositableType GetType() const MOZ_OVERRIDE
  {
    return BUFFER_IMAGE_SINGLE;
  }

  virtual void Update(gfx::IntSize aSize, BasicCanvasLayer* aLayer);
};



class CanvasClientWebGL : public CanvasClient
{
public:
  CanvasClientWebGL(CompositableForwarder* aFwd,
                    TextureFlags aFlags);

  CompositableType GetType() const MOZ_OVERRIDE
  {
    return BUFFER_IMAGE_BUFFERED;
  }

  virtual void Update(gfx::IntSize aSize, BasicCanvasLayer* aLayer);
};

}
}

#endif
