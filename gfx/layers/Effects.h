




#ifndef MOZILLA_LAYERS_EFFECTS_H
#define MOZILLA_LAYERS_EFFECTS_H

#include "mozilla/gfx/Matrix.h"
#include "mozilla/layers/Compositor.h"
#include "LayersLogging.h"
#include "mozilla/RefPtr.h"

namespace mozilla {
namespace layers {


















enum EffectTypes
{
  EFFECT_MASK,
  EFFECT_MAX_SECONDARY, 
  EFFECT_BGRX,
  EFFECT_RGBX,
  EFFECT_BGRA,
  EFFECT_RGBA,
  EFFECT_YCBCR,
  EFFECT_COMPONENT_ALPHA,
  EFFECT_SOLID_COLOR,
  EFFECT_RENDER_TARGET,
  EFFECT_MAX  
};

struct Effect : public RefCounted<Effect>
{
  Effect(EffectTypes aType) : mType(aType) {}

  EffectTypes mType;

  virtual ~Effect() {}
#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix) =0;
#endif
};


struct TexturedEffect : public Effect
{
  TexturedEffect(EffectTypes aType,
                 TextureSource *aTexture,
                 bool aPremultiplied,
                 gfx::Filter aFilter)
     : Effect(aType)
     , mTextureCoords(0, 0, 1.0f, 1.0f)
     , mTexture(aTexture)
     , mPremultiplied(aPremultiplied)
     , mFilter(aFilter)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() = 0;
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

  gfx::Rect mTextureCoords;
  TextureSource* mTexture;
  bool mPremultiplied;
  gfx::Filter mFilter;;
};


struct EffectMask : public Effect
{
  EffectMask(TextureSource *aMaskTexture,
             gfx::IntSize aSize,
             const gfx::Matrix4x4 &aMaskTransform)
    : Effect(EFFECT_MASK)
    , mMaskTexture(aMaskTexture)
    , mIs3D(false)
    , mSize(aSize)
    , mMaskTransform(aMaskTransform)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

  TextureSource* mMaskTexture;
  bool mIs3D;
  gfx::IntSize mSize;
  gfx::Matrix4x4 mMaskTransform;
};


struct EffectRenderTarget : public TexturedEffect
{
  EffectRenderTarget(CompositingRenderTarget *aRenderTarget)
    : TexturedEffect(EFFECT_RENDER_TARGET, aRenderTarget, true, gfx::FILTER_LINEAR)
    , mRenderTarget(aRenderTarget)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "EffectRenderTarget"; }
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

  RefPtr<CompositingRenderTarget> mRenderTarget;
};

struct EffectBGRX : public TexturedEffect
{
  EffectBGRX(TextureSource *aBGRXTexture,
             bool aPremultiplied,
             gfx::Filter aFilter,
             bool aFlipped = false)
    : TexturedEffect(EFFECT_BGRX, aBGRXTexture, aPremultiplied, aFilter)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "EffectBGRX"; }
#endif
};

struct EffectRGBX : public TexturedEffect
{
  EffectRGBX(TextureSource *aRGBXTexture,
             bool aPremultiplied,
             gfx::Filter aFilter)
    : TexturedEffect(EFFECT_RGBX, aRGBXTexture, aPremultiplied, aFilter)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "EffectRGBX"; }
#endif
};

struct EffectBGRA : public TexturedEffect
{
  EffectBGRA(TextureSource *aBGRATexture,
             bool aPremultiplied,
             gfx::Filter aFilter)
    : TexturedEffect(EFFECT_BGRA, aBGRATexture, aPremultiplied, aFilter)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "EffectBGRA"; }
#endif
};

struct EffectRGBA : public TexturedEffect
{
  EffectRGBA(TextureSource *aRGBATexture,
             bool aPremultiplied,
             gfx::Filter aFilter)
    : TexturedEffect(EFFECT_RGBA, aRGBATexture, aPremultiplied, aFilter)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "EffectRGBA"; }
#endif
};

struct EffectYCbCr : public TexturedEffect
{
  EffectYCbCr(TextureSource *aSource, gfx::Filter aFilter)
    : TexturedEffect(EFFECT_YCBCR, aSource, false, aFilter)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "EffectYCbCr"; }
#endif
};

struct EffectComponentAlpha : public TexturedEffect
{
  EffectComponentAlpha(TextureSource *aOnBlack,
                       TextureSource *aOnWhite,
                       gfx::Filter aFilter)
    : TexturedEffect(EFFECT_COMPONENT_ALPHA, nullptr, false, aFilter)
    , mOnBlack(aOnBlack)
    , mOnWhite(aOnWhite)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual const char* Name() { return "EffectComponentAlpha"; }
#endif

  TextureSource* mOnBlack;
  TextureSource* mOnWhite;
};

struct EffectSolidColor : public Effect
{
  EffectSolidColor(const gfx::Color &aColor)
    : Effect(EFFECT_SOLID_COLOR)
    , mColor(aColor)
  {}

#ifdef MOZ_LAYERS_HAVE_LOG
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);
#endif

  gfx::Color mColor;
};

struct EffectChain
{
  RefPtr<Effect> mPrimaryEffect;
  RefPtr<Effect> mSecondaryEffects[EFFECT_MAX_SECONDARY];
};

inline TemporaryRef<TexturedEffect>
CreateTexturedEffect(DeprecatedTextureHost *aDeprecatedTextureHost,
                     DeprecatedTextureHost *aDeprecatedTextureHostOnWhite,
                     const gfx::Filter& aFilter)
{
  if (aDeprecatedTextureHostOnWhite) {
    MOZ_ASSERT(aDeprecatedTextureHost->GetFormat() == gfx::FORMAT_R8G8B8X8 ||
               aDeprecatedTextureHost->GetFormat() == gfx::FORMAT_B8G8R8X8);
    return new EffectComponentAlpha(aDeprecatedTextureHost, aDeprecatedTextureHostOnWhite, aFilter);
  }

  RefPtr<TexturedEffect> result;
  switch (aDeprecatedTextureHost->GetFormat()) {
  case gfx::FORMAT_B8G8R8A8:
    result = new EffectBGRA(aDeprecatedTextureHost, true, aFilter);
    break;
  case gfx::FORMAT_B8G8R8X8:
    result = new EffectBGRX(aDeprecatedTextureHost, true, aFilter);
    break;
  case gfx::FORMAT_R8G8B8X8:
    result = new EffectRGBX(aDeprecatedTextureHost, true, aFilter);
    break;
  case gfx::FORMAT_R5G6B5:
    result = new EffectRGBX(aDeprecatedTextureHost, true, aFilter);
    break;
  case gfx::FORMAT_R8G8B8A8:
    result = new EffectRGBA(aDeprecatedTextureHost, true, aFilter);
    break;
  case gfx::FORMAT_YUV:
    result = new EffectYCbCr(aDeprecatedTextureHost, aFilter);
    break;
  default:
    MOZ_CRASH("unhandled program type");
  }

  return result;
}

inline TemporaryRef<TexturedEffect>
CreateTexturedEffect(DeprecatedTextureHost *aDeprecatedTextureHost,
                     const gfx::Filter& aFilter)
{
  return CreateTexturedEffect(aDeprecatedTextureHost, nullptr, aFilter);
}

} 
} 

#endif
