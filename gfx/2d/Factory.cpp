




































#include "2D.h"

#ifdef USE_CAIRO
#include "DrawTargetCairo.h"
#include "ScaledFontBase.h"
#endif

#ifdef USE_SKIA
#include "DrawTargetSkia.h"
#include "ScaledFontBase.h"
#include "ScaledFontFreetype.h"
#endif

#ifdef WIN32
#include "ScaledFontWin.h"
#endif

#ifdef XP_MACOSX
#include "ScaledFontMac.h"
#endif


#ifdef XP_MACOSX
#include "DrawTargetCG.h"
#endif

#ifdef WIN32
#include "DrawTargetD2D.h"
#include "ScaledFontDWrite.h"
#include <d3d10_1.h>
#endif

#include "DrawTargetDual.h"

#include "Logging.h"

#ifdef PR_LOGGING
PRLogModuleInfo *sGFX2DLog = PR_NewLogModule("gfx2d");
#endif

namespace mozilla {
namespace gfx {


int sGfxLogLevel = LOG_DEBUG;

#ifdef WIN32
ID3D10Device1 *Factory::mD3D10Device;
#endif

TemporaryRef<DrawTarget>
Factory::CreateDrawTarget(BackendType aBackend, const IntSize &aSize, SurfaceFormat aFormat)
{
  switch (aBackend) {
#ifdef WIN32
  case BACKEND_DIRECT2D:
    {
      RefPtr<DrawTargetD2D> newTarget;
      newTarget = new DrawTargetD2D();
      if (newTarget->Init(aSize, aFormat)) {
        return newTarget;
      }
      break;
    }
#elif defined XP_MACOSX || defined ANDROID || defined LINUX
#ifdef USE_SKIA
  case BACKEND_SKIA:
    {
      RefPtr<DrawTargetSkia> newTarget;
      newTarget = new DrawTargetSkia();
      if (newTarget->Init(aSize, aFormat)) {
        return newTarget;
      }
      break;
    }
#endif
#ifdef XP_MACOSX
  case BACKEND_COREGRAPHICS:
    {
      RefPtr<DrawTargetCG> newTarget;
      newTarget = new DrawTargetCG();
      if (newTarget->Init(aSize, aFormat)) {
        return newTarget;
      }
      break;
    }
#endif
#endif
  default:
    gfxDebug() << "Invalid draw target type specified.";
    return NULL;
  }

  gfxDebug() << "Failed to create DrawTarget, Type: " << aBackend << " Size: " << aSize;
  
  return NULL;
}

TemporaryRef<DrawTarget>
Factory::CreateDrawTargetForData(BackendType aBackend, 
                                 unsigned char *aData, 
                                 const IntSize &aSize, 
                                 int32_t aStride, 
                                 SurfaceFormat aFormat)
{
  switch (aBackend) {
#ifdef USE_SKIA
  case BACKEND_SKIA:
    {
      RefPtr<DrawTargetSkia> newTarget;
      newTarget = new DrawTargetSkia();
      newTarget->Init(aData, aSize, aStride, aFormat);
      return newTarget;
    }
#endif
  default:
    gfxDebug() << "Invalid draw target type specified.";
    return NULL;
  }

  gfxDebug() << "Failed to create DrawTarget, Type: " << aBackend << " Size: " << aSize;
  
  return NULL;
}

TemporaryRef<ScaledFont>
Factory::CreateScaledFontForNativeFont(const NativeFont &aNativeFont, Float aSize)
{
  switch (aNativeFont.mType) {
#ifdef WIN32
  case NATIVE_FONT_DWRITE_FONT_FACE:
    {
      return new ScaledFontDWrite(static_cast<IDWriteFontFace*>(aNativeFont.mFont), aSize);
    }
#endif
#ifdef XP_MACOSX
  case NATIVE_FONT_MAC_FONT_FACE:
    {
      return new ScaledFontMac(static_cast<CGFontRef>(aNativeFont.mFont), aSize);
    }
#endif
#ifdef USE_SKIA
#ifdef WIN32
  case NATIVE_FONT_GDI_FONT_FACE:
    {
      return new ScaledFontWin(static_cast<gfxGDIFont*>(aNativeFont.mFont), aSize);
    }
#endif
  case NATIVE_FONT_SKIA_FONT_FACE:
    {
      return new ScaledFontFreetype(static_cast<gfxFont*>(aNativeFont.mFont), aSize);
    }
#endif
#ifdef USE_CAIRO
  case NATIVE_FONT_CAIRO_FONT_FACE:
    {
      return new ScaledFontBase(aSize);
    }
#endif
  default:
    gfxWarning() << "Invalid native font type specified.";
    return NULL;
  }
}

TemporaryRef<ScaledFont>
Factory::CreateScaledFontWithCairo(const NativeFont& aNativeFont, Float aSize, cairo_scaled_font_t* aScaledFont)
{
#ifdef USE_CAIRO
  
  
  
  
  RefPtr<ScaledFont> font = CreateScaledFontForNativeFont(aNativeFont, aSize);
  static_cast<ScaledFontBase*>(font.get())->SetCairoScaledFont(aScaledFont);
  return font;
#else
  return NULL;
#endif
}

#ifdef WIN32
TemporaryRef<DrawTarget>
Factory::CreateDrawTargetForD3D10Texture(ID3D10Texture2D *aTexture, SurfaceFormat aFormat)
{
  RefPtr<DrawTargetD2D> newTarget;

  newTarget = new DrawTargetD2D();
  if (newTarget->Init(aTexture, aFormat)) {
    return newTarget;
  }

  gfxWarning() << "Failed to create draw target for D3D10 texture.";

  
  return NULL;
}

TemporaryRef<DrawTarget>
Factory::CreateDualDrawTargetForD3D10Textures(ID3D10Texture2D *aTextureA,
                                              ID3D10Texture2D *aTextureB,
                                              SurfaceFormat aFormat)
{
  RefPtr<DrawTargetD2D> newTargetA;
  RefPtr<DrawTargetD2D> newTargetB;

  newTargetA = new DrawTargetD2D();
  if (!newTargetA->Init(aTextureA, aFormat)) {
    gfxWarning() << "Failed to create draw target for D3D10 texture.";
    return NULL;
  }

  newTargetB = new DrawTargetD2D();
  if (!newTargetB->Init(aTextureB, aFormat)) {
    gfxWarning() << "Failed to create draw target for D3D10 texture.";
    return NULL;
  }

  RefPtr<DrawTarget> newTarget =
    new DrawTargetDual(newTargetA, newTargetB);

  return newTarget;
}

void
Factory::SetDirect3D10Device(ID3D10Device1 *aDevice)
{
  mD3D10Device = aDevice;
}

ID3D10Device1*
Factory::GetDirect3D10Device()
{
  return mD3D10Device;
}

TemporaryRef<GlyphRenderingOptions>
Factory::CreateDWriteGlyphRenderingOptions(IDWriteRenderingParams *aParams)
{
  RefPtr<GlyphRenderingOptions> options =
    new GlyphRenderingOptionsDWrite(aParams);

  return options;
}

#endif 

#ifdef USE_CAIRO
TemporaryRef<DrawTarget>
Factory::CreateDrawTargetForCairoSurface(cairo_surface_t* aSurface)
{
  RefPtr<DrawTargetCairo> newTarget = new DrawTargetCairo();
  if (newTarget->Init(aSurface)) {
    return newTarget;
  }

  return NULL;
}
#endif

}
}
