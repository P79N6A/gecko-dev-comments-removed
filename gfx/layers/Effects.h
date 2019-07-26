




#ifndef MOZILLA_LAYERS_EFFECTS_H
#define MOZILLA_LAYERS_EFFECTS_H

#include "mozilla/Assertions.h"         
#include "mozilla/RefPtr.h"             
#include "mozilla/gfx/Matrix.h"         
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/Types.h"          
#include "mozilla/layers/CompositorTypes.h"  
#include "mozilla/layers/LayersTypes.h"
#include "mozilla/layers/TextureHost.h"  
#include "mozilla/mozalloc.h"           
#include "nscore.h"                     

namespace mozilla {
namespace layers {

















struct Effect : public RefCounted<Effect>
{
  Effect(EffectTypes aType) : mType(aType) {}

  EffectTypes mType;

  virtual ~Effect() {}
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix) =0;
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

  virtual const char* Name() = 0;
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);

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

  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);

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

  virtual const char* Name() { return "EffectRenderTarget"; }
  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);

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

  virtual const char* Name() { return "EffectBGRX"; }
};

struct EffectRGBX : public TexturedEffect
{
  EffectRGBX(TextureSource *aRGBXTexture,
             bool aPremultiplied,
             gfx::Filter aFilter)
    : TexturedEffect(EFFECT_RGBX, aRGBXTexture, aPremultiplied, aFilter)
  {}

  virtual const char* Name() { return "EffectRGBX"; }
};

struct EffectBGRA : public TexturedEffect
{
  EffectBGRA(TextureSource *aBGRATexture,
             bool aPremultiplied,
             gfx::Filter aFilter)
    : TexturedEffect(EFFECT_BGRA, aBGRATexture, aPremultiplied, aFilter)
  {}

  virtual const char* Name() { return "EffectBGRA"; }
};

struct EffectRGBA : public TexturedEffect
{
  EffectRGBA(TextureSource *aRGBATexture,
             bool aPremultiplied,
             gfx::Filter aFilter)
    : TexturedEffect(EFFECT_RGBA, aRGBATexture, aPremultiplied, aFilter)
  {}

  virtual const char* Name() { return "EffectRGBA"; }
};

struct EffectYCbCr : public TexturedEffect
{
  EffectYCbCr(TextureSource *aSource, gfx::Filter aFilter)
    : TexturedEffect(EFFECT_YCBCR, aSource, false, aFilter)
  {}

  virtual const char* Name() { return "EffectYCbCr"; }
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

  virtual const char* Name() { return "EffectComponentAlpha"; }

  TextureSource* mOnBlack;
  TextureSource* mOnWhite;
};

struct EffectSolidColor : public Effect
{
  EffectSolidColor(const gfx::Color &aColor)
    : Effect(EFFECT_SOLID_COLOR)
    , mColor(aColor)
  {}

  virtual void PrintInfo(nsACString& aTo, const char* aPrefix);

  gfx::Color mColor;
};

struct EffectChain
{
  RefPtr<Effect> mPrimaryEffect;
  RefPtr<Effect> mSecondaryEffects[EFFECT_MAX_SECONDARY];
};










inline TemporaryRef<TexturedEffect>
CreateTexturedEffect(gfx::SurfaceFormat aFormat,
                     TextureSource* aSource,
                     const gfx::Filter& aFilter)
{
  MOZ_ASSERT(aSource);
  RefPtr<TexturedEffect> result;
  switch (aFormat) {
  case gfx::FORMAT_B8G8R8A8:
    result = new EffectBGRA(aSource, true, aFilter);
    break;
  case gfx::FORMAT_B8G8R8X8:
    result = new EffectBGRX(aSource, true, aFilter);
    break;
  case gfx::FORMAT_R8G8B8X8:
    result = new EffectRGBX(aSource, true, aFilter);
    break;
  case gfx::FORMAT_R5G6B5:
    result = new EffectRGBX(aSource, true, aFilter);
    break;
  case gfx::FORMAT_R8G8B8A8:
    result = new EffectRGBA(aSource, true, aFilter);
    break;
  case gfx::FORMAT_YUV:
    result = new EffectYCbCr(aSource, aFilter);
    break;
  default:
    NS_WARNING("unhandled program type");
    break;
  }

  return result;
}







inline TemporaryRef<TexturedEffect>
CreateTexturedEffect(TextureSource* aSource,
                     TextureSource* aSourceOnWhite,
                     const gfx::Filter& aFilter)
{
  MOZ_ASSERT(aSource);
  if (aSourceOnWhite) {
    MOZ_ASSERT(aSource->GetFormat() == gfx::FORMAT_R8G8B8X8 ||
               aSourceOnWhite->GetFormat() == gfx::FORMAT_B8G8R8X8);
    return new EffectComponentAlpha(aSource, aSourceOnWhite, aFilter);
  }

  return CreateTexturedEffect(aSource->GetFormat(), aSource, aFilter);
}






inline TemporaryRef<TexturedEffect>
CreateTexturedEffect(TextureSource *aTexture,
                     const gfx::Filter& aFilter)
{
  return CreateTexturedEffect(aTexture, nullptr, aFilter);
}


} 
} 

#endif
