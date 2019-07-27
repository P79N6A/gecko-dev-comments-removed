




#ifndef mozilla_RegionTyped_h
#define mozilla_RegionTyped_h

#include "nsRegion.h"
#include "mozilla/gfx/Rect.h"

namespace mozilla {

namespace gfx {

template <class units>
class IntRegionTyped :
    public BaseIntRegion<IntRegionTyped<units>, IntRectTyped<units>, IntPointTyped<units>, IntMarginTyped<units>>
{
  typedef BaseIntRegion<IntRegionTyped<units>, IntRectTyped<units>, IntPointTyped<units>, IntMarginTyped<units>> Super;
public:
  
  IntRegionTyped() {}
  MOZ_IMPLICIT IntRegionTyped(const IntRectTyped<units>& aRect) : Super(aRect) {}
  IntRegionTyped(const IntRegionTyped& aRegion) : Super(aRegion) {}
  IntRegionTyped(IntRegionTyped&& aRegion) : Super(mozilla::Move(aRegion)) {}

  
  
  IntRegionTyped& operator=(const IntRegionTyped& aRegion)
  {
    return Super::operator=(aRegion);
  }
  IntRegionTyped& operator=(IntRegionTyped&& aRegion)
  {
    return Super::operator=(mozilla::Move(aRegion));
  }

  static IntRegionTyped FromUntyped(const nsIntRegion& aRegion)
  {
    return IntRegionTyped(aRegion.Impl());
  }
private:
  
  explicit IntRegionTyped(const nsRegion& aRegion) : Super(aRegion) {}
};

} 

} 



#endif 
