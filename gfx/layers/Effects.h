




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
#include "mozilla/EnumeratedArray.h"

namespace mozilla {
namespace layers {

















struct Effect
{
  NS_INLINE_DECL_REFCOUNTING(Effect)

  explicit Effect(EffectTypes aType) : mType(aType) {}

  EffectTypes mType;

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix) = 0;

protected:
  virtual ~Effect() {}
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
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  gfx::Rect mTextureCoords;
  TextureSource* mTexture;
  bool mPremultiplied;
  gfx::Filter mFilter;
};


struct EffectMask : public Effect
{
  EffectMask(TextureSource *aMaskTexture,
             gfx::IntSize aSize,
             const gfx::Matrix4x4 &aMaskTransform)
    : Effect(EffectTypes::MASK)
    , mMaskTexture(aMaskTexture)
    , mIs3D(false)
    , mSize(aSize)
    , mMaskTransform(aMaskTransform)
  {}

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  TextureSource* mMaskTexture;
  bool mIs3D;
  gfx::IntSize mSize;
  gfx::Matrix4x4 mMaskTransform;
};

struct EffectBlendMode : public Effect
{
  explicit EffectBlendMode(gfx::CompositionOp aBlendMode)
    : Effect(EffectTypes::BLEND_MODE)
    , mBlendMode(aBlendMode)
  { }

  virtual const char* Name() { return "EffectBlendMode"; }
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  gfx::CompositionOp mBlendMode;
};


struct EffectRenderTarget : public TexturedEffect
{
  explicit EffectRenderTarget(CompositingRenderTarget *aRenderTarget)
    : TexturedEffect(EffectTypes::RENDER_TARGET, aRenderTarget, true, gfx::Filter::LINEAR)
    , mRenderTarget(aRenderTarget)
  {}

  virtual const char* Name() { return "EffectRenderTarget"; }
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  RefPtr<CompositingRenderTarget> mRenderTarget;

protected:
  EffectRenderTarget(EffectTypes aType, CompositingRenderTarget *aRenderTarget)
    : TexturedEffect(aType, aRenderTarget, true, gfx::Filter::LINEAR)
    , mRenderTarget(aRenderTarget)
  {}

};


struct EffectColorMatrix : public Effect
{
  explicit EffectColorMatrix(gfx::Matrix5x4 aMatrix)
    : Effect(EffectTypes::COLOR_MATRIX)
    , mColorMatrix(aMatrix)
  {}

  virtual const char* Name() { return "EffectColorMatrix"; }
  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);
  const gfx::Matrix5x4 mColorMatrix;
};


struct EffectRGB : public TexturedEffect
{
  EffectRGB(TextureSource *aTexture,
            bool aPremultiplied,
            gfx::Filter aFilter,
            bool aFlipped = false)
    : TexturedEffect(EffectTypes::RGB, aTexture, aPremultiplied, aFilter)
  {}

  virtual const char* Name() { return "EffectRGB"; }
};

struct EffectYCbCr : public TexturedEffect
{
  EffectYCbCr(TextureSource *aSource, gfx::Filter aFilter)
    : TexturedEffect(EffectTypes::YCBCR, aSource, false, aFilter)
  {}

  virtual const char* Name() { return "EffectYCbCr"; }
};

struct EffectComponentAlpha : public TexturedEffect
{
  EffectComponentAlpha(TextureSource *aOnBlack,
                       TextureSource *aOnWhite,
                       gfx::Filter aFilter)
    : TexturedEffect(EffectTypes::COMPONENT_ALPHA, nullptr, false, aFilter)
    , mOnBlack(aOnBlack)
    , mOnWhite(aOnWhite)
  {}

  virtual const char* Name() { return "EffectComponentAlpha"; }

  TextureSource* mOnBlack;
  TextureSource* mOnWhite;
};

struct EffectSolidColor : public Effect
{
  explicit EffectSolidColor(const gfx::Color &aColor)
    : Effect(EffectTypes::SOLID_COLOR)
    , mColor(aColor)
  {}

  virtual void PrintInfo(std::stringstream& aStream, const char* aPrefix);

  gfx::Color mColor;
};

struct EffectChain
{
  EffectChain() : mLayerRef(nullptr) {}
  explicit EffectChain(void* aLayerRef) : mLayerRef(aLayerRef) {}

  RefPtr<Effect> mPrimaryEffect;
  EnumeratedArray<EffectTypes, EffectTypes::MAX_SECONDARY, RefPtr<Effect>>
    mSecondaryEffects;
  void* mLayerRef; 
};










inline TemporaryRef<TexturedEffect>
CreateTexturedEffect(gfx::SurfaceFormat aFormat,
                     TextureSource* aSource,
                     const gfx::Filter& aFilter,
                     bool isAlphaPremultiplied)
{
  MOZ_ASSERT(aSource);
  RefPtr<TexturedEffect> result;
  switch (aFormat) {
  case gfx::SurfaceFormat::B8G8R8A8:
  case gfx::SurfaceFormat::B8G8R8X8:
  case gfx::SurfaceFormat::R8G8B8X8:
  case gfx::SurfaceFormat::R5G6B5:
  case gfx::SurfaceFormat::R8G8B8A8:
    result = new EffectRGB(aSource, isAlphaPremultiplied, aFilter);
    break;
  case gfx::SurfaceFormat::YUV:
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
                     const gfx::Filter& aFilter,
                     bool isAlphaPremultiplied)
{
  MOZ_ASSERT(aSource);
  if (aSourceOnWhite) {
    MOZ_ASSERT(aSource->GetFormat() == gfx::SurfaceFormat::R8G8B8X8 ||
               aSourceOnWhite->GetFormat() == gfx::SurfaceFormat::B8G8R8X8);
    return new EffectComponentAlpha(aSource, aSourceOnWhite, aFilter);
  }

  return CreateTexturedEffect(aSource->GetFormat(),
                              aSource,
                              aFilter,
                              isAlphaPremultiplied);
}






inline TemporaryRef<TexturedEffect>
CreateTexturedEffect(TextureSource *aTexture,
                     const gfx::Filter& aFilter)
{
  return CreateTexturedEffect(aTexture, nullptr, aFilter, true);
}


} 
} 

#endif
