




#ifndef mozilla_image_src_ImageRegion_h
#define mozilla_image_src_ImageRegion_h

#include "gfxRect.h"

namespace mozilla {
namespace image {











class ImageRegion
{
public:
  static ImageRegion Empty()
  {
    return ImageRegion(gfxRect());
  }

  static ImageRegion Create(const gfxRect& aRect)
  {
    return ImageRegion(aRect);
  }

  static ImageRegion Create(const gfxSize& aSize)
  {
    return ImageRegion(gfxRect(0, 0, aSize.width, aSize.height));
  }

  static ImageRegion Create(const nsIntSize& aSize)
  {
    return ImageRegion(gfxRect(0, 0, aSize.width, aSize.height));
  }

  static ImageRegion CreateWithSamplingRestriction(const gfxRect& aRect,
                                                   const gfxRect& aRestriction)
  {
    return ImageRegion(aRect, aRestriction);
  }

  bool IsRestricted() const { return mIsRestricted; }
  const gfxRect& Rect() const { return mRect; }

  const gfxRect& Restriction() const
  {
    MOZ_ASSERT(mIsRestricted);
    return mRestriction;
  }

  bool RestrictionContains(const gfxRect& aRect) const
  {
    if (!mIsRestricted) {
      return true;
    }
    return mRestriction.Contains(aRect);
  }

  ImageRegion Intersect(const gfxRect& aRect) const
  {
    if (mIsRestricted) {
      return CreateWithSamplingRestriction(aRect.Intersect(mRect),
                                           aRect.Intersect(mRestriction));
    }
    return Create(aRect.Intersect(mRect));
  }

  gfxRect IntersectAndRestrict(const gfxRect& aRect) const
  {
    gfxRect intersection = mRect.Intersect(aRect);
    if (mIsRestricted) {
      intersection = mRestriction.Intersect(intersection);
    }
    return intersection;
  }

  void MoveBy(gfxFloat dx, gfxFloat dy)
  {
    mRect.MoveBy(dx, dy);
    if (mIsRestricted) {
      mRestriction.MoveBy(dx, dy);
    }
  }

  void Scale(gfxFloat sx, gfxFloat sy)
  {
    mRect.Scale(sx, sy);
    if (mIsRestricted) {
      mRestriction.Scale(sx, sy);
    }
  }

  void TransformBy(const gfxMatrix& aMatrix)
  {
    mRect = aMatrix.Transform(mRect);
    if (mIsRestricted) {
      mRestriction = aMatrix.Transform(mRestriction);
    }
  }

  void TransformBoundsBy(const gfxMatrix& aMatrix)
  {
    mRect = aMatrix.TransformBounds(mRect);
    if (mIsRestricted) {
      mRestriction = aMatrix.TransformBounds(mRestriction);
    }
  }

  ImageRegion operator-(const gfxPoint& aPt) const
  {
    if (mIsRestricted) {
      return CreateWithSamplingRestriction(mRect - aPt, mRestriction - aPt);
    }
    return Create(mRect - aPt);
  }

  ImageRegion operator+(const gfxPoint& aPt) const
  {
    if (mIsRestricted) {
      return CreateWithSamplingRestriction(mRect + aPt, mRestriction + aPt);
    }
    return Create(mRect + aPt);
  }

  

private:
  explicit ImageRegion(const gfxRect& aRect)
    : mRect(aRect)
    , mIsRestricted(false)
  { }

  ImageRegion(const gfxRect& aRect, const gfxRect& aRestriction)
    : mRect(aRect)
    , mRestriction(aRestriction)
    , mIsRestricted(true)
  { }

  gfxRect mRect;
  gfxRect mRestriction;
  bool    mIsRestricted;
};

}  
}  

#endif 
