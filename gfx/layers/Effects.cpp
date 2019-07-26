




#include "Effects.h"
#include "LayersLogging.h"              
#include "nsAString.h"
#include "nsPrintfCString.h"            
#include "nsString.h"                   

using namespace mozilla::layers;

void
TexturedEffect::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("%s (0x%p)", Name(), this);
  AppendToString(aTo, mTextureCoords, " [texture-coords=", "]");

  if (mPremultiplied) {
    aTo += " [premultiplied]";
  } else {
    aTo += " [not-premultiplied]";
  }

  AppendToString(aTo, mFilter, " [filter=", "]");
}

void
EffectMask::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("EffectMask (0x%p)", this);
  AppendToString(aTo, mSize, " [size=", "]");
  AppendToString(aTo, mMaskTransform, " [mask-transform=", "]");

  if (mIs3D) {
    aTo += " [is-3d]";
  }
}

void
EffectRenderTarget::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  TexturedEffect::PrintInfo(aTo, aPrefix);
  aTo += nsPrintfCString(" [render-target=%p]", mRenderTarget.get());
}

void
EffectSolidColor::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("EffectSolidColor (0x%p) [color=%x]", this, mColor.ToABGR());
}

void
EffectBlendMode::PrintInfo(nsACString& aTo, const char* aPrefix)
{
  aTo += aPrefix;
  aTo += nsPrintfCString("EffectBlendMode (0x%p) [blendmode=%i]", this, (int)mBlendMode);
}

