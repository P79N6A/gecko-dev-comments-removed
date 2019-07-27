




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
class SharedSurfaceTextureClient;




class CanvasClient : public CompositableClient
{
public:
  




  enum CanvasClientType {
    CanvasClientSurface,
    CanvasClientGLContext,
    CanvasClientTypeShSurf,
  };
  static already_AddRefed<CanvasClient> CreateCanvasClient(CanvasClientType aType,
                                                       CompositableForwarder* aFwd,
                                                       TextureFlags aFlags);

  CanvasClient(CompositableForwarder* aFwd, TextureFlags aFlags)
    : CompositableClient(aFwd, aFlags)
  {
    mTextureFlags = aFlags;
  }

  virtual ~CanvasClient() {}

  virtual void Clear() {};

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer) = 0;

  virtual void Updated() { }
};


class CanvasClient2D : public CanvasClient
{
public:
  CanvasClient2D(CompositableForwarder* aLayerForwarder,
                 TextureFlags aFlags)
    : CanvasClient(aLayerForwarder, aFlags)
  {
  }

  TextureInfo GetTextureInfo() const override
  {
    return TextureInfo(CompositableType::IMAGE, mTextureFlags);
  }

  virtual void Clear() override
  {
    mBuffer = nullptr;
  }

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer) override;

  virtual bool AddTextureClient(TextureClient* aTexture) override
  {
    MOZ_ASSERT((mTextureFlags & aTexture->GetFlags()) == mTextureFlags);
    return CompositableClient::AddTextureClient(aTexture);
  }

  virtual void OnDetach() override
  {
    mBuffer = nullptr;
  }

private:
  already_AddRefed<TextureClient>
    CreateTextureClientForCanvas(gfx::SurfaceFormat aFormat,
                                 gfx::IntSize aSize,
                                 TextureFlags aFlags,
                                 ClientCanvasLayer* aLayer);

  RefPtr<TextureClient> mBuffer;
};



class CanvasClientSharedSurface : public CanvasClient
{
private:
  RefPtr<SharedSurfaceTextureClient> mShSurfClient;
  RefPtr<TextureClient> mReadbackClient;
  RefPtr<TextureClient> mFront;

  void ClearSurfaces();

public:
  CanvasClientSharedSurface(CompositableForwarder* aLayerForwarder,
                            TextureFlags aFlags);

  ~CanvasClientSharedSurface();

  virtual TextureInfo GetTextureInfo() const override {
    return TextureInfo(CompositableType::IMAGE);
  }

  virtual void Clear() override {
    ClearSurfaces();
  }

  virtual void Update(gfx::IntSize aSize,
                      ClientCanvasLayer* aLayer) override;

  virtual void OnDetach() override {
    ClearSurfaces();
  }
};

}
}

#endif
