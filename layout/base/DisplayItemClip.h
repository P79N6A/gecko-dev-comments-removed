




#ifndef DISPLAYITEMCLIP_H_
#define DISPLAYITEMCLIP_H_

#include "mozilla/RefPtr.h"
#include "nsRect.h"
#include "nsTArray.h"
#include "nsStyleConsts.h"

class gfxContext;
class nsPresContext;
class nsRegion;

namespace mozilla {
namespace gfx {
class DrawTarget;
class Path;
}
}

namespace mozilla {







class DisplayItemClip {
  typedef mozilla::gfx::Color Color;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::Path Path;

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

  
  DisplayItemClip() : mHaveClipRect(false) {}

  void SetTo(const nsRect& aRect);
  void SetTo(const nsRect& aRect, const nscoord* aRadii);
  void SetTo(const nsRect& aRect, const nsRect& aRoundedRect, const nscoord* aRadii);
  void IntersectWith(const DisplayItemClip& aOther);

  
  
  
  void ApplyTo(gfxContext* aContext, nsPresContext* aPresContext,
               uint32_t aBegin = 0, uint32_t aEnd = UINT32_MAX);

  void ApplyRectTo(gfxContext* aContext, int32_t A2D) const;
  
  
  
  void ApplyRoundedRectClipsTo(gfxContext* aContext, int32_t A2DPRInt32,
                               uint32_t aBegin, uint32_t aEnd) const;

  
  void FillIntersectionOfRoundedRectClips(gfxContext* aContext,
                                          const Color& aColor,
                                          int32_t aAppUnitsPerDevPixel,
                                          uint32_t aBegin,
                                          uint32_t aEnd) const;
  
  mozilla::TemporaryRef<Path> MakeRoundedRectPath(DrawTarget& aDrawTarget,
                                                  int32_t A2D,
                                                  const RoundedRect &aRoundRect) const;

  
  
  
  
  
  bool MayIntersect(const nsRect& aRect) const;

  
  
  
  nsRect ApproximateIntersectInward(const nsRect& aRect) const;

  






  bool ComputeRegionInClips(DisplayItemClip* aOldClip,
                            const nsPoint& aShift,
                            nsRegion* aCombined) const;

  
  
  
  
  bool IsRectClippedByRoundedCorner(const nsRect& aRect) const;

  
  
  bool IsRectAffectedByClip(const nsRect& aRect) const;
  bool IsRectAffectedByClip(const nsIntRect& aRect, float aXScale, float aYScale, int32_t A2D) const;

  
  nsRect NonRoundedIntersection() const;

  
  
  nsRect ApplyNonRoundedIntersection(const nsRect& aRect) const;

  
  void RemoveRoundedCorners();

  
  
  void AddOffsetAndComputeDifference(uint32_t aStart, const nsPoint& aPoint, const nsRect& aBounds,
                                     const DisplayItemClip& aOther, uint32_t aOtherStart, const nsRect& aOtherBounds,
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

  nsCString ToString() const;

  



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
};

}

#endif 
