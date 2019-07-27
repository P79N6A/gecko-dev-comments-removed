





#ifndef VibrancyManager_h
#define VibrancyManager_h

#include "mozilla/Assertions.h"
#include "nsClassHashtable.h"
#include "nsRegion.h"
#include "nsTArray.h"

#import <Foundation/NSGeometry.h>

@class NSColor;
@class NSView;
class nsChildView;
class nsIntRegion;

namespace mozilla {

enum class VibrancyType {
  LIGHT,
  DARK,
  TOOLTIP,
  MENU,
  HIGHLIGHTED_MENUITEM
};












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

  





  NSColor* VibrancyFillColorForType(VibrancyType aType);

  



  NSColor* VibrancyFontSmoothingBackgroundColorForType(VibrancyType aType);

  




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
