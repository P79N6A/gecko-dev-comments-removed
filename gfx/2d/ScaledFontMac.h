




































#ifndef MOZILLA_GFX_SCALEDFONTMAC_H_
#define MOZILLA_GFX_SCALEDFONTMAC_H_

#import <ApplicationServices/ApplicationServices.h>
#include "2D.h"

#include "ScaledFontBase.h"

namespace mozilla {
namespace gfx {

class ScaledFontMac : public ScaledFontBase
{
public:
  ScaledFontMac(CGFontRef aFont, Float aSize);
  virtual ~ScaledFontMac();

  virtual FontType GetType() const { return FONT_MAC; }
#ifdef USE_SKIA
  virtual SkTypeface* GetSkTypeface();
#endif
  virtual TemporaryRef<Path> GetPathForGlyphs(const GlyphBuffer &aBuffer, const DrawTarget *aTarget);
private:
  friend class DrawTargetCG;
  CGFontRef mFont;
};

}
}

#endif 
