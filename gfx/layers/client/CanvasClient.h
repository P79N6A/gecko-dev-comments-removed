




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
namespace gl {
class SharedSurface;
class ShSurfHandle;
}
}

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
    CanvasClientTypeShSurf,
  };
  static TemporaryRef<CanvasClient> CreateCanvasClient(CanvasClientType aType,
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

  TextureInfo GetTextureInfo() const
  {
    return TextureInfo(CompositableType::IMAGE, mTextureFlags);
  }

  virtual void Clear() MOZ_OVERRIDE
  {
    mBuffer = nullptr;
  }

  virtual void Update(gfx::IntSize aSize, ClientCanvasLayer* aLayer) MOZ_OVERRIDE;

  virtual bool AddTextureClient(TextureClient* aTexture) MOZ_OVERRIDE
  {
    MOZ_ASSERT((mTextureFlags & aTexture->GetFlags()) == mTextureFlags);
    return CompositableClient::AddTextureClient(aTexture);
  }

  virtual void OnDetach() MOZ_OVERRIDE
  {
    mBuffer = nullptr;
  }

private:
  TemporaryRef<TextureClient>
    CreateTextureClientForCanvas(gfx::SurfaceFormat aFormat,
                                 gfx::IntSize aSize,
                                 TextureFlags aFlags,
                                 ClientCanvasLayer* aLayer);

  RefPtr<TextureClient> mBuffer;
};



class CanvasClientSharedSurface : public CanvasClient
{
private:
  RefPtr<gl::ShSurfHandle> mFront;
  RefPtr<gl::ShSurfHandle> mPrevFront;

  RefPtr<TextureClient> mFrontTex;

public:
  CanvasClientSharedSurface(CompositableForwarder* aLayerForwarder,
                            TextureFlags aFlags);

  virtual TextureInfo GetTextureInfo() const MOZ_OVERRIDE {
    return TextureInfo(CompositableType::IMAGE);
  }

  virtual void Clear() MOZ_OVERRIDE {
    mFront = nullptr;
    mPrevFront = nullptr;
    mFrontTex = nullptr;
  }

  virtual void Update(gfx::IntSize aSize,
                      ClientCanvasLayer* aLayer) MOZ_OVERRIDE;

  virtual void OnDetach() MOZ_OVERRIDE {
    CanvasClientSharedSurface::Clear();
  }
};

}
}

#endif
