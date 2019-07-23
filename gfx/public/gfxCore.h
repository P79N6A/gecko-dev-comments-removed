




































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

#if defined(MOZ_ENABLE_LIBXUL) || !defined(MOZILLA_INTERNAL_API)
#  define NS_GFX
#  define NS_GFX_(type) type
#  define NS_GFX_STATIC_MEMBER_(type) type
#elif defined(_IMPL_NS_GFX)
#  define NS_GFX NS_EXPORT
#  define NS_GFX_(type) NS_EXPORT_(type)
#  define NS_GFX_STATIC_MEMBER_(type) NS_EXPORT_STATIC_MEMBER_(type)
#else
#  define NS_GFX NS_IMPORT
#  define NS_GFX_(type) NS_IMPORT_(type)
#  define NS_GFX_STATIC_MEMBER_(type) NS_IMPORT_STATIC_MEMBER_(type)
#endif

#endif
