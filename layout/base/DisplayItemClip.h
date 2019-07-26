




#ifndef DISPLAYITEMCLIP_H_
#define DISPLAYITEMCLIP_H_

#include "nsRect.h"
#include "nsTArray.h"
#include "nsStyleConsts.h"

class gfxContext;
class nsDisplayItem;
class nsPresContext;
class nsRegion;

namespace mozilla {







class DisplayItemClip {
public:
  struct RoundedRect {
    nsRect mRect;
    
    nscoord mRadii[8];

    RoundedRect operator+(const nsPoint& aOffset) const {
      RoundedRect r = *this;
      r.mRect += aOffset;
      return r;
    }
    bool operator==(const RoundedRect& aOther) const {
      if (!mRect.IsEqualInterior(aOther.mRect)) {
        return false;
      }

      NS_FOR_CSS_HALF_CORNERS(corner) {
        if (mRadii[corner] != aOther.mRadii[corner]) {
          return false;
        }
      }
      return true;
    }
    bool operator!=(const RoundedRect& aOther) const {
      return !(*this == aOther);
    }
  };

  
  DisplayItemClip() : mHaveClipRect(false), mHasBeenDestroyed(false) {}
  ~DisplayItemClip() { mHasBeenDestroyed = true; }

  void MaybeDestroy() const
  {
    if (!mHasBeenDestroyed) {
      this->~DisplayItemClip();
    }
  }

  void SetTo(const nsRect& aRect);
  void SetTo(const nsRect& aRect, const nscoord* aRadii);
  void IntersectWith(const DisplayItemClip& aOther);

  
  
  
  void ApplyTo(gfxContext* aContext, nsPresContext* aPresContext,
               uint32_t aBegin = 0, uint32_t aEnd = UINT32_MAX);

  void ApplyRectTo(gfxContext* aContext, int32_t A2D) const;
  
  
  
  void ApplyRoundedRectsTo(gfxContext* aContext, int32_t A2DPRInt32,
                           uint32_t aBegin, uint32_t aEnd) const;

  
  void DrawRoundedRectsTo(gfxContext* aContext, int32_t A2D,
                          uint32_t aBegin, uint32_t aEnd) const;
  
  void AddRoundedRectPathTo(gfxContext* aContext, int32_t A2D,
                            const RoundedRect &aRoundRect) const;

  
  
  
  
  
  bool MayIntersect(const nsRect& aRect) const;
  
  
  
  nsRect ApproximateIntersectInward(const nsRect& aRect) const;

  






  bool ComputeRegionInClips(DisplayItemClip* aOldClip,
                            const nsPoint& aShift,
                            nsRegion* aCombined) const;

  
  
  
  
  bool IsRectClippedByRoundedCorner(const nsRect& aRect) const;

  
  
  bool IsRectAffectedByClip(const nsRect& aRect) const;

  
  nsRect NonRoundedIntersection() const;

  
  
  nsRect ApplyNonRoundedIntersection(const nsRect& aRect) const;

  
  void RemoveRoundedCorners();

  
  
  void AddOffsetAndComputeDifference(const nsPoint& aPoint, const nsRect& aBounds,
                                     const DisplayItemClip& aOther, const nsRect& aOtherBounds,
                                     nsRegion* aDifference);

  bool operator==(const DisplayItemClip& aOther) const {
    return mHaveClipRect == aOther.mHaveClipRect &&
           (!mHaveClipRect || mClipRect.IsEqualInterior(aOther.mClipRect)) &&
           mRoundedClipRects == aOther.mRoundedClipRects;
  }
  bool operator!=(const DisplayItemClip& aOther) const {
    return !(*this == aOther);
  }

  bool HasClip() const { return mHaveClipRect; }
  const nsRect& GetClipRect() const
  {
    NS_ASSERTION(HasClip(), "No clip rect!");
    return mClipRect;
  }

  void MoveBy(nsPoint aPoint);

#ifdef MOZ_DUMP_PAINTING
  nsCString ToString() const;
#endif

  



  uint32_t GetCommonRoundedRectCount(const DisplayItemClip& aOther,
                                     uint32_t aMax) const;
  uint32_t GetRoundedRectCount() const { return mRoundedClipRects.Length(); }
  void AppendRoundedRects(nsTArray<RoundedRect>* aArray, uint32_t aCount) const;

  static const DisplayItemClip& NoClip();

  static void Shutdown();

private:
  nsRect mClipRect;
  nsTArray<RoundedRect> mRoundedClipRects;
  
  
  bool mHaveClipRect;
  
  
  bool mHasBeenDestroyed;
};

}

#endif 
