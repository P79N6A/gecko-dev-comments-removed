




#ifndef GFX_PREFS_H
#define GFX_PREFS_H

#include <stdint.h>
#include "mozilla/Assertions.h"
#include "mozilla/TypedEnum.h"










































#define DECL_GFX_PREF(Update, Pref, Name, Type, Default)                     \
public:                                                                       \
static Type Name() { MOZ_ASSERT(Exists()); return One().mPref##Name.mValue; } \
private:                                                                      \
static const char* Get##Name##PrefName() { return Pref; }                     \
PrefTemplate<UpdatePolicy::Update, Type, Default, Get##Name##PrefName> mPref##Name

class gfxPrefs;
class gfxPrefs MOZ_FINAL
{
private:
  
  MOZ_BEGIN_NESTED_ENUM_CLASS(UpdatePolicy)
    Skip, 
    Once, 
    Live  
  MOZ_END_NESTED_ENUM_CLASS(UpdatePolicy)

  
    template <MOZ_ENUM_CLASS_ENUM_TYPE(UpdatePolicy) Update, class T, T Default, const char* Pref(void)>
  class PrefTemplate
  {
  public:
    PrefTemplate()
    : mValue(Default)
    {
      Register(Update, Pref());
    }
    void Register(UpdatePolicy aUpdate, const char* aPreference)
    {
      switch(aUpdate) {
        case UpdatePolicy::Skip:
          break;
        case UpdatePolicy::Once:
          mValue = PrefGet(aPreference, mValue);
          break;
        case UpdatePolicy::Live:
          PrefAddVarCache(&mValue,aPreference, mValue);
          break;
        default:
          MOZ_CRASH();
          break;
      }
    }
    T mValue;
  };

  
  
  

  DECL_GFX_PREF(Live, "gfx.canvas.azure.accelerated",          CanvasAzureAccelerated, bool, false);
  DECL_GFX_PREF(Once, "gfx.canvas.skiagl.dynamic-cache",       CanvasSkiaGLDynamicCache, bool, false);
  DECL_GFX_PREF(Once, "gfx.canvas.skiagl.cache-size",          CanvasSkiaGLCacheSize, int32_t, 96);
  DECL_GFX_PREF(Once, "gfx.canvas.skiagl.cache-items",         CanvasSkiaGLCacheItems, int32_t, 256);

  DECL_GFX_PREF(Once, "gfx.direct2d.disabled",                 Direct2DDisabled, bool, false);
  DECL_GFX_PREF(Once, "gfx.direct2d.force-enabled",            Direct2DForceEnabled, bool, false);
  DECL_GFX_PREF(Once, "gfx.work-around-driver-bugs",           WorkAroundDriverBugs, bool, true);

  DECL_GFX_PREF(Live, "gl.msaa-level",                         MSAALevel, uint32_t, 2);

  DECL_GFX_PREF(Once, "layers.acceleration.disabled",          LayersAccelerationDisabled, bool, false);
  DECL_GFX_PREF(Live, "layers.acceleration.draw-fps",          LayersDrawFPS, bool, true);
  DECL_GFX_PREF(Once, "layers.acceleration.force-enabled",     LayersAccelerationForceEnabled, bool, false);
#ifdef XP_WIN
  
  DECL_GFX_PREF(Skip, "layers.async-video.enabled",            AsyncVideoEnabled, bool, false);
#else
  DECL_GFX_PREF(Once, "layers.async-video.enabled",            AsyncVideoEnabled, bool, false);
#endif
  DECL_GFX_PREF(Once, "layers.bufferrotation.enabled",         BufferRotationEnabled, bool, true);
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
  
  
  DECL_GFX_PREF(Skip, "layers.componentalpha.enabled",         ComponentAlphaEnabled, bool, false);
#else
  
  
  DECL_GFX_PREF(Once, "layers.componentalpha.enabled",         ComponentAlphaEnabled, bool, true);
#endif
  DECL_GFX_PREF(Live, "layers.draw-bigimage-borders",          DrawBigImageBorders, bool, false);
  DECL_GFX_PREF(Live, "layers.draw-borders",                   DrawLayerBorders, bool, false);
  DECL_GFX_PREF(Live, "layers.draw-tile-borders",              DrawTileBorders, bool, false);
  DECL_GFX_PREF(Once, "layers.dump",                           LayersDump, bool, false);
  DECL_GFX_PREF(Once, "layers.enable-tiles",                   LayersTilesEnabled, bool, false);
  DECL_GFX_PREF(Live, "layers.frame-counter",                  DrawFrameCounter, bool, false);
  DECL_GFX_PREF(Live, "layers.low-precision-buffer",           UseLowPrecisionBuffer, bool, false);
  DECL_GFX_PREF(Live, "layers.low-precision-resolution",       LowPrecisionResolution, int32_t, 250);
  DECL_GFX_PREF(Once, "layers.offmainthreadcomposition.enabled", LayersOffMainThreadCompositionEnabled, bool, false);
  DECL_GFX_PREF(Live, "layers.offmainthreadcomposition.frame-rate", LayersCompositionFrameRate, int32_t,-1);
  DECL_GFX_PREF(Once, "layers.offmainthreadcomposition.force-enabled", LayersOffMainThreadCompositionForceEnabled, bool, false);
  DECL_GFX_PREF(Once, "layers.offmainthreadcomposition.testing.enabled", LayersOffMainThreadCompositionTestingEnabled, bool, false);
  DECL_GFX_PREF(Live, "layers.orientation.sync.timeout",       OrientationSyncMillis, uint32_t, (uint32_t)0);
  DECL_GFX_PREF(Once, "layers.prefer-d3d9",                    LayersPreferD3D9, bool, false);
  DECL_GFX_PREF(Once, "layers.prefer-opengl",                  LayersPreferOpenGL, bool, false);
  DECL_GFX_PREF(Once, "layers.progressive-paint",              UseProgressiveTilePainting, bool, false);
  DECL_GFX_PREF(Once, "layers.scroll-graph",                   LayersScrollGraph, bool, false);

  DECL_GFX_PREF(Once, "layout.frame_rate",                     LayoutFrameRate, int32_t, -1);

  DECL_GFX_PREF(Live, "nglayout.debug.widget_update_flashing", WidgetUpdateFlashing, bool, false);

  DECL_GFX_PREF(Once, "webgl.force-layers-readback",           WebGLForceLayersReadback, bool, false);

public:
  
  static gfxPrefs& One()
  {
    if (!sInstance) {
      sInstance = new gfxPrefs;
    }
    return *sInstance;
  }
  static void Destroy();
  static bool Exists();

private:
  static gfxPrefs* sInstance;

private:
  
  static void PrefAddVarCache(bool*, const char*, bool);
  static void PrefAddVarCache(int32_t*, const char*, int32_t);
  static void PrefAddVarCache(uint32_t*, const char*, uint32_t);
  static bool PrefGet(const char*, bool);
  static int32_t PrefGet(const char*, int32_t);
  static uint32_t PrefGet(const char*, uint32_t);

  gfxPrefs();
  ~gfxPrefs();
  gfxPrefs(const gfxPrefs&) MOZ_DELETE;
  gfxPrefs& operator=(const gfxPrefs&) MOZ_DELETE;
};

#undef DECL_GFX_PREF /* Don't need it outside of this file */

#endif 
