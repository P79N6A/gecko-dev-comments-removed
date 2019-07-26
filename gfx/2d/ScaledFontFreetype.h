




#ifndef MOZILLA_GFX_SCALEDFONTFREETYPE_H_
#define MOZILLA_GFX_SCALEDFONTFREETYPE_H_

#include "ScaledFontBase.h"

namespace mozilla {
namespace gfx {

class ScaledFontFreetype : public ScaledFontBase
{
public:

  ScaledFontFreetype(FontOptions* aFont, Float aSize);
};

}
}

#endif 
