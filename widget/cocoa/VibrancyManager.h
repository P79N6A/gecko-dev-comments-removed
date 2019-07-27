





#ifndef VibrancyManager_h
#define VibrancyManager_h

#include "mozilla/Assertions.h"
#include "mozilla/TypedEnum.h"
#include "nsClassHashtable.h"
#include "nsRegion.h"
#include "nsTArray.h"

#import <Foundation/NSGeometry.h>

@class NSView;
class nsChildView;
class nsIntRegion;

namespace mozilla {

MOZ_BEGIN_ENUM_CLASS(VibrancyType)
  LIGHT,
  DARK
MOZ_END_ENUM_CLASS(VibrancyType)












class VibrancyManager {
public:
  









  VibrancyManager(const nsChildView& aCoordinateConverter,
                  NSView* aContainerView)
    : mCoordinateConverter(aCoordinateConverter)
    , mContainerView(aContainerView)
  {
    MOZ_ASSERT(SystemSupportsVibrancy(),
               "Don't instantiate this if !SystemSupportsVibrancy()");
  }

  






  void UpdateVibrantRegion(VibrancyType aType, const nsIntRegion& aRegion);

  





  void ClearVibrantAreas() const;

  




  static bool SystemSupportsVibrancy();

  
  
  struct VibrantRegion {
    nsIntRegion region;
    nsTArray<NSView*> effectViews;
  };
  void ClearVibrantRegion(const VibrantRegion& aVibrantRegion) const;

protected:
  NSView* CreateEffectView(VibrancyType aType, NSRect aRect);

  const nsChildView& mCoordinateConverter;
  NSView* mContainerView;
  nsClassHashtable<nsUint32HashKey, VibrantRegion> mVibrantRegions;
};

}

#endif 
