




#ifndef GFX_CANVASLAYERD3D10_H
#define GFX_CANVASLAYERD3D10_H

#include "LayerManagerD3D10.h"

#include "mozilla/Preferences.h"

namespace mozilla {

namespace gl {
class GLContext;
}

namespace layers {

class CanvasLayerD3D10 : public CanvasLayer,
                         public LayerD3D10
{
public:
  CanvasLayerD3D10(LayerManagerD3D10 *aManager);
  ~CanvasLayerD3D10();

  
  virtual void Initialize(const Data& aData);

  
  virtual Layer* GetLayer();
  virtual void RenderLayer();

private:
  typedef mozilla::gl::GLContext GLContext;

  void UpdateSurface();

  RefPtr<gfx::SourceSurface> mSurface;
  mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;
  nsRefPtr<GLContext> mGLContext;
  nsRefPtr<ID3D10Texture2D> mTexture;
  nsRefPtr<ID3D10ShaderResourceView> mUploadSRView;
  nsRefPtr<ID3D10ShaderResourceView> mSRView;

  bool mDataIsPremultiplied;
  bool mNeedsYFlip;
  bool mIsD2DTexture;
  bool mHasAlpha;

  nsAutoArrayPtr<uint8_t> mCachedTempBlob;
  uint32_t mCachedTempBlob_Size;

  uint8_t* GetTempBlob(const uint32_t aSize)
  {
    if (!mCachedTempBlob || aSize != mCachedTempBlob_Size) {
      mCachedTempBlob = new uint8_t[aSize];
      mCachedTempBlob_Size = aSize;
    }

    return mCachedTempBlob;
  }

  void DiscardTempBlob()
  {
    mCachedTempBlob = nullptr;
  }
};

} 
} 
#endif 
