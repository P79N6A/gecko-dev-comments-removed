




































#ifndef gfxCore_h__
#define gfxCore_h__

#include "nscore.h"


namespace mozilla {
  namespace css {
    enum Side {eSideTop, eSideRight, eSideBottom, eSideLeft};
  }
}
#define NS_SIDE_TOP     mozilla::css::eSideTop
#define NS_SIDE_RIGHT   mozilla::css::eSideRight
#define NS_SIDE_BOTTOM  mozilla::css::eSideBottom
#define NS_SIDE_LEFT    mozilla::css::eSideLeft
#define NS_GFX
#define NS_GFX_(type) type
#define NS_GFX_STATIC_MEMBER_(type) type

#endif
