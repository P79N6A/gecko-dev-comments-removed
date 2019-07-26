




#include "Effects.h"
#include "LayersLogging.h"
#include "nsPrintfCString.h"

using namespace mozilla::layers;

#ifdef MOZ_LAYERS_HAVE_LOG
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

  if (mMaskTexture) {
    nsAutoCString prefix(aPrefix);
    prefix += "  ";

    aTo += "\n";
    mMaskTexture->PrintInfo(aTo, prefix.get());
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

#endif 
