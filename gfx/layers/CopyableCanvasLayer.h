




#ifndef GFX_COPYABLECANVASLAYER_H
#define GFX_COPYABLECANVASLAYER_H

#include <stdint.h>                     
#include "GLContextTypes.h"             
#include "Layers.h"                     
#include "gfxASurface.h"                
#include "gfxContext.h"                 
#include "gfxImageSurface.h"            
#include "gfxPlatform.h"                
#include "gfxPoint.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/2D.h"             
#include "mozilla/mozalloc.h"           
#include "nsAutoPtr.h"                  
#include "nsTraceRefcnt.h"              

using namespace mozilla::gfx;

namespace mozilla {
namespace layers {

class CanvasClientWebGL;





class CopyableCanvasLayer : public CanvasLayer
{
public:
  CopyableCanvasLayer(LayerManager* aLayerManager, void *aImplData);
  virtual ~CopyableCanvasLayer();

  virtual void Initialize(const Data& aData);
  

protected:
  void PaintWithOpacity(gfxContext* aContext,
                        float aOpacity,
                        Layer* aMaskLayer,
                        gfxContext::GraphicsOperator aOperator = gfxContext::OPERATOR_OVER);

  void UpdateSurface(gfxASurface* aDestSurface = nullptr, Layer* aMaskLayer = nullptr);

  nsRefPtr<gfxASurface> mSurface;
  nsRefPtr<mozilla::gl::GLContext> mGLContext;
  mozilla::RefPtr<mozilla::gfx::DrawTarget> mDrawTarget;

  uint32_t mCanvasFramebuffer;

  bool mIsGLAlphaPremult;
  bool mNeedsYFlip;
  bool mForceReadback;

  nsRefPtr<gfxImageSurface> mCachedTempSurface;
  gfxIntSize mCachedSize;
  gfxImageFormat mCachedFormat;

  gfxImageSurface* GetTempSurface(const gfxIntSize& aSize, const gfxImageFormat aFormat)
  {
    if (!mCachedTempSurface ||
        aSize.width != mCachedSize.width ||
        aSize.height != mCachedSize.height ||
        aFormat != mCachedFormat)
    {
      mCachedTempSurface = new gfxImageSurface(aSize, aFormat);
      mCachedSize = aSize;
      mCachedFormat = aFormat;
    }

    MOZ_ASSERT(mCachedTempSurface->Stride() == mCachedTempSurface->Width() * 4);
    return mCachedTempSurface;
  }

  void DiscardTempSurface()
  {
    mCachedTempSurface = nullptr;
  }
};

}
}

#endif
