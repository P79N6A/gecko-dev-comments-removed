




#include "Effects.h"
#include "LayersLogging.h"              
#include "nsAString.h"
#include "nsPrintfCString.h"            
#include "nsString.h"                   

using namespace mozilla::layers;

void
TexturedEffect::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("%s (0x%p)", Name(), this).get();
  AppendToString(aStream, mTextureCoords, " [texture-coords=", "]");

  if (mPremultiplied) {
    aStream << " [premultiplied]";
  } else {
    aStream << " [not-premultiplied]";
  }

  AppendToString(aStream, mFilter, " [filter=", "]");
}

void
EffectMask::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("EffectMask (0x%p)", this).get();
  AppendToString(aStream, mSize, " [size=", "]");
  AppendToString(aStream, mMaskTransform, " [mask-transform=", "]");

  if (mIs3D) {
    aStream << " [is-3d]";
  }
}

void
EffectRenderTarget::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  TexturedEffect::PrintInfo(aStream, aPrefix);
  aStream << nsPrintfCString(" [render-target=%p]", mRenderTarget.get()).get();
}

void
EffectSolidColor::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("EffectSolidColor (0x%p) [color=%x]", this, mColor.ToABGR()).get();
}

void
EffectBlendMode::PrintInfo(std::stringstream& aStream, const char* aPrefix)
{
  aStream << aPrefix;
  aStream << nsPrintfCString("EffectBlendMode (0x%p) [blendmode=%i]", this, (int)mBlendMode).get();
}

