




































#ifndef MOZILLA_GFX_SCALEDFONTMAC_H_
#define MOZILLA_GFX_SCALEDFONTMAC_H_

#include "ScaledFontSkia.h"
#import <ApplicationServices/ApplicationServices.h>


namespace mozilla {
namespace gfx {

class ScaledFontMac : public ScaledFontSkia
{
public:
  ScaledFontMac(CGFontRef aFont, Float aSize);
  virtual ~ScaledFontMac();

  virtual FontType GetType() const { return FONT_MAC; }

private:
  friend class DrawTargetSkia;

  CTFontRef mFontFace;
};

}
}

#endif 
