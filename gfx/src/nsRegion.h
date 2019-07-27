




#ifndef nsRegion_h__
#define nsRegion_h__

#include <stddef.h>                     
#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxCore.h"                    
#include "mozilla/ToString.h"           
#include "nsCoord.h"                    
#include "nsError.h"                    
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsMargin.h"                   
#include "nsStringGlue.h"               
#include "xpcom-config.h"               
#include "mozilla/Move.h"               

class nsIntRegion;
class gfx3DMatrix;

#include "pixman.h"















enum class VisitSide {
	TOP,
	BOTTOM,
	LEFT,
	RIGHT
};

class nsRegionRectIterator;

class nsRegion
{

  friend class nsRegionRectIterator;

public:
  nsRegion () { pixman_region32_init(&mImpl); }
  MOZ_IMPLICIT nsRegion (const nsRect& aRect) { pixman_region32_init_rect(&mImpl,
                                                                          aRect.x,
                                                                          aRect.y,
                                                                          aRect.width,
                                                                          aRect.height); }
  nsRegion (const nsRegion& aRegion) { pixman_region32_init(&mImpl); pixman_region32_copy(&mImpl,aRegion.Impl()); }
  nsRegion (nsRegion&& aRegion) { mImpl = aRegion.mImpl; pixman_region32_init(&aRegion.mImpl); }
  nsRegion& operator = (nsRegion&& aRegion) {
      pixman_region32_fini(&mImpl);
      mImpl = aRegion.mImpl;
      pixman_region32_init(&aRegion.mImpl);
      return *this;
  }
 ~nsRegion () { pixman_region32_fini(&mImpl); }
  nsRegion& operator = (const nsRect& aRect) { Copy (aRect); return *this; }
  nsRegion& operator = (const nsRegion& aRegion) { Copy (aRegion); return *this; }
  bool operator==(const nsRegion& aRgn) const
  {
    return IsEqual(aRgn);
  }
  bool operator!=(const nsRegion& aRgn) const
  {
    return !(*this == aRgn);
  }

  friend std::ostream& operator<<(std::ostream& stream, const nsRegion& m);

  void Swap(nsRegion* aOther)
  {
    pixman_region32_t tmp = mImpl;
    mImpl = aOther->mImpl;
    aOther->mImpl = tmp;
  }

  static
  nsresult InitStatic()
  {
    return NS_OK;
  }

  static
  void ShutdownStatic() {}

  void AndWith(const nsRegion& aOther)
  {
    And(*this, aOther);
  }
  void AndWith(const nsRect& aOther)
  {
    And(*this, aOther);
  }
  nsRegion& And(const nsRegion& aRgn1,   const nsRegion& aRgn2)
  {
    pixman_region32_intersect(&mImpl, aRgn1.Impl(), aRgn2.Impl());
    return *this;
  }
  nsRegion& And(const nsRect& aRect, const nsRegion& aRegion)
  {
    return And(aRegion, aRect);
  }
  nsRegion& And(const nsRegion& aRegion, const nsRect& aRect)
  {
    pixman_region32_intersect_rect(&mImpl, aRegion.Impl(), aRect.x, aRect.y, aRect.width, aRect.height);
    return *this;
  }
  nsRegion& And(const nsRect& aRect1, const nsRect& aRect2)
  {
    nsRect TmpRect;

    TmpRect.IntersectRect(aRect1, aRect2);
    return Copy(TmpRect);
  }

  nsRegion& OrWith(const nsRegion& aOther)
  {
    return Or(*this, aOther);
  }
  nsRegion& OrWith(const nsRect& aOther)
  {
    return Or(*this, aOther);
  }
  nsRegion& Or(const nsRegion& aRgn1, const nsRegion& aRgn2)
  {
    pixman_region32_union(&mImpl, aRgn1.Impl(), aRgn2.Impl());
    return *this;
  }
  nsRegion& Or(const nsRegion& aRegion, const nsRect& aRect)
  {
    pixman_region32_union_rect(&mImpl, aRegion.Impl(), aRect.x, aRect.y, aRect.width, aRect.height);
    return *this;
  }
  nsRegion& Or(const nsRect& aRect, const nsRegion& aRegion)
  {
    return  Or(aRegion, aRect);
  }
  nsRegion& Or(const nsRect& aRect1, const nsRect& aRect2)
  {
    Copy (aRect1);
    return Or (*this, aRect2);
  }

  nsRegion& XorWith(const nsRegion& aOther)
  {
    return Xor(*this, aOther);
  }
  nsRegion& XorWith(const nsRect& aOther)
  {
    return Xor(*this, aOther);
  }
  nsRegion& Xor(const nsRegion& aRgn1,   const nsRegion& aRgn2)
  {
    
    
    nsRegion p;
    p.Sub(aRgn1, aRgn2);
    nsRegion q;
    q.Sub(aRgn2, aRgn1);
    return Or(p, q);
  }
  nsRegion& Xor(const nsRegion& aRegion, const nsRect& aRect)
  {
    return Xor(aRegion, nsRegion(aRect));
  }
  nsRegion& Xor(const nsRect& aRect, const nsRegion& aRegion)
  {
    return Xor(nsRegion(aRect), aRegion);
  }
  nsRegion& Xor(const nsRect& aRect1, const nsRect& aRect2)
  {
    return Xor(nsRegion(aRect1), nsRegion(aRect2));
  }

  nsRegion ToAppUnits (nscoord aAppUnitsPerPixel) const;

  nsRegion& SubOut(const nsRegion& aOther)
  {
    return Sub(*this, aOther);
  }
  nsRegion& SubOut(const nsRect& aOther)
  {
    return Sub(*this, aOther);
  }
  nsRegion& Sub(const nsRegion& aRgn1, const nsRegion& aRgn2)
  {
    pixman_region32_subtract(&mImpl, aRgn1.Impl(), aRgn2.Impl());
    return *this;
  }
  nsRegion& Sub(const nsRegion& aRegion, const nsRect& aRect)
  {
    return Sub(aRegion, nsRegion(aRect));
  }
  nsRegion& Sub(const nsRect& aRect, const nsRegion& aRegion)
  {
    return Sub(nsRegion(aRect), aRegion);
  }
  nsRegion& Sub(const nsRect& aRect1, const nsRect& aRect2)
  {
    Copy(aRect1);
    return Sub(*this, aRect2);
  }

  




  bool Contains (int aX, int aY) const
  {
    return pixman_region32_contains_point(Impl(), aX, aY, nullptr);
  }
  bool Contains (const nsRect& aRect) const
  {
    pixman_box32_t box = RectToBox(aRect);
    return pixman_region32_contains_rectangle(Impl(), &box) == PIXMAN_REGION_IN;
  }
  bool Contains (const nsRegion& aRgn) const;
  bool Intersects (const nsRect& aRect) const;

  void MoveBy (int32_t aXOffset, int32_t aYOffset)
  {
    MoveBy (nsPoint (aXOffset, aYOffset));
  }
  void MoveBy (nsPoint aPt) { pixman_region32_translate(&mImpl, aPt.x, aPt.y); }
  void SetEmpty ()
  {
    pixman_region32_clear(&mImpl);
  }

  nsRegion MovedBy(int32_t aXOffset, int32_t aYOffset) const
  {
    return MovedBy(nsPoint(aXOffset, aYOffset));
  }
  nsRegion MovedBy(const nsPoint& aPt) const
  {
    nsRegion copy(*this);
    copy.MoveBy(aPt);
    return copy;
  }

  nsRegion Intersect(const nsRegion& aOther) const
  {
    nsRegion intersection;
    intersection.And(*this, aOther);
    return intersection;
  }

  void Inflate(const nsMargin& aMargin);

  nsRegion Inflated(const nsMargin& aMargin) const
  {
    nsRegion copy(*this);
    copy.Inflate(aMargin);
    return copy;
  }

  bool IsEmpty () const { return !pixman_region32_not_empty(Impl()); }
  bool IsComplex () const { return GetNumRects() > 1; }
  bool IsEqual (const nsRegion& aRegion) const
  {
    return pixman_region32_equal(Impl(), aRegion.Impl());
  }
  uint32_t GetNumRects () const
  {
    
    
    uint32_t result = pixman_region32_n_rects(Impl());
    return (result == 1 && GetBounds().IsEmpty()) ? 0 : result;
  }
  const nsRect GetBounds () const { return BoxToRect(mImpl.extents); }
  uint64_t Area () const;

  






  MOZ_WARN_UNUSED_RESULT nsRegion
    ScaleToOtherAppUnitsRoundOut (int32_t aFromAPP, int32_t aToAPP) const;
  MOZ_WARN_UNUSED_RESULT nsRegion
    ScaleToOtherAppUnitsRoundIn (int32_t aFromAPP, int32_t aToAPP) const;
  nsRegion& ScaleRoundOut(float aXScale, float aYScale);
  nsRegion& ScaleInverseRoundOut(float aXScale, float aYScale);
  nsRegion& Transform (const gfx3DMatrix &aTransform);
  nsIntRegion ScaleToOutsidePixels (float aXScale, float aYScale, nscoord aAppUnitsPerPixel) const;
  nsIntRegion ScaleToInsidePixels (float aXScale, float aYScale, nscoord aAppUnitsPerPixel) const;
  nsIntRegion ScaleToNearestPixels (float aXScale, float aYScale, nscoord aAppUnitsPerPixel) const;
  nsIntRegion ToOutsidePixels (nscoord aAppUnitsPerPixel) const;
  nsIntRegion ToNearestPixels (nscoord aAppUnitsPerPixel) const;

  





  nsRect GetLargestRectangle (const nsRect& aContainingRect = nsRect()) const;

  





  void SimplifyOutward (uint32_t aMaxRects);
  





  void SimplifyOutwardByArea(uint32_t aThreshold);
  




  void SimplifyInward (uint32_t aMaxRects);

  











  typedef void (*visitFn)(void *closure, VisitSide side, int x1, int y1, int x2, int y2);
  void VisitEdges(visitFn, void *closure);

  nsCString ToString() const;
private:
  pixman_region32_t mImpl;

#ifndef MOZ_TREE_PIXMAN
  
  static inline void
  pixman_region32_clear(pixman_region32_t *region)
  {
    pixman_region32_fini(region);
    pixman_region32_init(region);
  }
#endif

  nsIntRegion ToPixels(nscoord aAppUnitsPerPixel, bool aOutsidePixels) const;

  nsRegion& Copy (const nsRegion& aRegion)
  {
    pixman_region32_copy(&mImpl, aRegion.Impl());
    return *this;
  }

  nsRegion& Copy (const nsRect& aRect)
  {
    
    
    
    
    
    if (aRect.IsEmpty()) {
      pixman_region32_clear(&mImpl);
    } else {
      pixman_box32_t box = RectToBox(aRect);
      pixman_region32_reset(&mImpl, &box);
    }
    return *this;
  }

  static inline pixman_box32_t RectToBox(const nsRect &aRect)
  {
    pixman_box32_t box = { aRect.x, aRect.y, aRect.XMost(), aRect.YMost() };
    return box;
  }

  static inline pixman_box32_t RectToBox(const nsIntRect &aRect)
  {
    pixman_box32_t box = { aRect.x, aRect.y, aRect.XMost(), aRect.YMost() };
    return box;
  }


  static inline nsRect BoxToRect(const pixman_box32_t &aBox)
  {
    return nsRect(aBox.x1, aBox.y1,
                  aBox.x2 - aBox.x1,
                  aBox.y2 - aBox.y1);
  }

  pixman_region32_t* Impl() const
  {
    return const_cast<pixman_region32_t*>(&mImpl);
  }

};


class NS_GFX nsRegionRectIterator
{
  const nsRegion*  mRegion;
  int i;
  int n;
  nsRect rect;
  pixman_box32_t *boxes;

public:
  explicit nsRegionRectIterator (const nsRegion& aRegion)
  {
    mRegion = &aRegion;
    i = 0;
    boxes = pixman_region32_rectangles(aRegion.Impl(), &n);
    
    
    if (n == 1 && nsRegion::BoxToRect(boxes[0]).IsEmpty()) {
      n = 0;
    }
  }

  const nsRect* Next ()
  {
    if (i == n)
      return nullptr;
    rect = nsRegion::BoxToRect(boxes[i]);
    NS_ASSERTION(!rect.IsEmpty(), "Shouldn't return empty rect");
    i++;
    return &rect;
  }

  const nsRect* Prev ()
  {
    if (i == -1)
      return nullptr;
    rect = nsRegion::BoxToRect(boxes[i]);
    NS_ASSERTION(!rect.IsEmpty(), "Shouldn't return empty rect");
    i--;
    return &rect;
  }

  void Reset ()
  {
    i = 0;
  }
};

namespace mozilla {
namespace gfx {




template <typename Derived, typename Rect, typename Point, typename Margin>
class NS_GFX BaseIntRegion
{
  friend class ::nsRegion;

public:
  typedef Rect RectType;
  typedef Point PointType;
  typedef Margin MarginType;

  BaseIntRegion () {}
  MOZ_IMPLICIT BaseIntRegion (const Rect& aRect) : mImpl (ToRect(aRect)) {}
  BaseIntRegion (const BaseIntRegion& aRegion) : mImpl (aRegion.mImpl) {}
  BaseIntRegion (BaseIntRegion&& aRegion) : mImpl (mozilla::Move(aRegion.mImpl)) {}
  Derived& operator = (const Rect& aRect) { mImpl = ToRect (aRect); return This(); }
  Derived& operator = (const Derived& aRegion) { mImpl = aRegion.mImpl; return This(); }
  Derived& operator = (Derived&& aRegion) { mImpl = mozilla::Move(aRegion.mImpl); return This(); }

  bool operator==(const Derived& aRgn) const
  {
    return IsEqual(aRgn);
  }
  bool operator!=(const Derived& aRgn) const
  {
    return !(*this == aRgn);
  }

  friend std::ostream& operator<<(std::ostream& stream, const Derived& m) {
    return stream << m.mImpl;
  }

  void Swap(Derived* aOther)
  {
    mImpl.Swap(&aOther->mImpl);
  }

  void AndWith(const Derived& aOther)
  {
    And(This(), aOther);
  }
  void AndWith(const Rect& aOther)
  {
    And(This(), aOther);
  }
  Derived& And  (const Derived& aRgn1,   const Derived& aRgn2)
  {
    mImpl.And (aRgn1.mImpl, aRgn2.mImpl);
    return This();
  }
  Derived& And  (const Derived& aRegion, const Rect& aRect)
  {
    mImpl.And (aRegion.mImpl, ToRect (aRect));
    return This();
  }
  Derived& And  (const Rect& aRect, const Derived& aRegion)
  {
    return  And  (aRegion, aRect);
  }
  Derived& And  (const Rect& aRect1, const Rect& aRect2)
  {
    Rect TmpRect;

    TmpRect.IntersectRect (aRect1, aRect2);
    mImpl = ToRect (TmpRect);
    return This();
  }

  Derived& OrWith(const Derived& aOther)
  {
    return Or(This(), aOther);
  }
  Derived& OrWith(const Rect& aOther)
  {
    return Or(This(), aOther);
  }
  Derived& Or   (const Derived& aRgn1,   const Derived& aRgn2)
  {
    mImpl.Or (aRgn1.mImpl, aRgn2.mImpl);
    return This();
  }
  Derived& Or   (const Derived& aRegion, const Rect& aRect)
  {
    mImpl.Or (aRegion.mImpl, ToRect (aRect));
    return This();
  }
  Derived& Or   (const Rect& aRect, const Derived& aRegion)
  {
    return  Or   (aRegion, aRect);
  }
  Derived& Or   (const Rect& aRect1, const Rect& aRect2)
  {
    mImpl = ToRect (aRect1);
    return Or (This(), aRect2);
  }

  Derived& XorWith(const Derived& aOther)
  {
    return Xor(This(), aOther);
  }
  Derived& XorWith(const Rect& aOther)
  {
    return Xor(This(), aOther);
  }
  Derived& Xor  (const Derived& aRgn1,   const Derived& aRgn2)
  {
    mImpl.Xor (aRgn1.mImpl, aRgn2.mImpl);
    return This();
  }
  Derived& Xor  (const Derived& aRegion, const Rect& aRect)
  {
    mImpl.Xor (aRegion.mImpl, ToRect (aRect));
    return This();
  }
  Derived& Xor  (const Rect& aRect, const Derived& aRegion)
  {
    return  Xor  (aRegion, aRect);
  }
  Derived& Xor  (const Rect& aRect1, const Rect& aRect2)
  {
    mImpl = ToRect (aRect1);
    return Xor (This(), aRect2);
  }

  Derived& SubOut(const Derived& aOther)
  {
    return Sub(This(), aOther);
  }
  Derived& SubOut(const Rect& aOther)
  {
    return Sub(This(), aOther);
  }
  Derived& Sub  (const Derived& aRgn1,   const Derived& aRgn2)
  {
    mImpl.Sub (aRgn1.mImpl, aRgn2.mImpl);
    return This();
  }
  Derived& Sub  (const Derived& aRegion, const Rect& aRect)
  {
    mImpl.Sub (aRegion.mImpl, ToRect (aRect));
    return This();
  }
  Derived& Sub  (const Rect& aRect, const Derived& aRegion)
  {
    return Sub (Derived (aRect), aRegion);
  }
  Derived& Sub  (const Rect& aRect1, const Rect& aRect2)
  {
    mImpl = ToRect (aRect1);
    return Sub (This(), aRect2);
  }

  




  bool Contains (int aX, int aY) const
  {
    return mImpl.Contains(aX, aY);
  }
  bool Contains (const Rect& aRect) const
  {
    return mImpl.Contains (ToRect (aRect));
  }
  bool Contains (const Derived& aRgn) const
  {
    return mImpl.Contains (aRgn.mImpl);
  }
  bool Intersects (const Rect& aRect) const
  {
    return mImpl.Intersects (ToRect (aRect));
  }

  void MoveBy (int32_t aXOffset, int32_t aYOffset)
  {
    MoveBy (Point (aXOffset, aYOffset));
  }
  void MoveBy (Point aPt)
  {
    mImpl.MoveBy (aPt.x, aPt.y);
  }
  Derived MovedBy(int32_t aXOffset, int32_t aYOffset) const
  {
    return MovedBy(Point(aXOffset, aYOffset));
  }
  Derived MovedBy(const Point& aPt) const
  {
    Derived copy(This());
    copy.MoveBy(aPt);
    return copy;
  }

  Derived Intersect(const Derived& aOther) const
  {
    Derived intersection;
    intersection.And(This(), aOther);
    return intersection;
  }

  void Inflate(const Margin& aMargin)
  {
    mImpl.Inflate(nsMargin(aMargin.top, aMargin.right, aMargin.bottom, aMargin.left));
  }
  Derived Inflated(const Margin& aMargin) const
  {
    Derived copy(This());
    copy.Inflate(aMargin);
    return copy;
  }

  void SetEmpty ()
  {
    mImpl.SetEmpty  ();
  }

  bool IsEmpty () const { return mImpl.IsEmpty (); }
  bool IsComplex () const { return mImpl.IsComplex (); }
  bool IsEqual (const Derived& aRegion) const
  {
    return mImpl.IsEqual (aRegion.mImpl);
  }
  uint32_t GetNumRects () const { return mImpl.GetNumRects (); }
  Rect GetBounds () const { return FromRect (mImpl.GetBounds ()); }
  uint64_t Area () const { return mImpl.Area(); }
  nsRegion ToAppUnits (nscoord aAppUnitsPerPixel) const
  {
    nsRegion result;
    RectIterator rgnIter(*this);
    const Rect* currentRect;
    while ((currentRect = rgnIter.Next())) {
      nsRect appRect = ::ToAppUnits(*currentRect, aAppUnitsPerPixel);
      result.Or(result, appRect);
    }
    return result;
  }
  Rect GetLargestRectangle (const Rect& aContainingRect = Rect()) const
  {
    return FromRect (mImpl.GetLargestRectangle( ToRect(aContainingRect) ));
  }

  Derived& ScaleRoundOut (float aXScale, float aYScale)
  {
    mImpl.ScaleRoundOut(aXScale, aYScale);
    return This();
  }

  Derived& Transform (const gfx3DMatrix &aTransform)
  {
    mImpl.Transform(aTransform);
    return This();
  }

  





  void SimplifyOutward (uint32_t aMaxRects)
  {
    mImpl.SimplifyOutward (aMaxRects);
  }
  void SimplifyOutwardByArea (uint32_t aThreshold)
  {
    mImpl.SimplifyOutwardByArea (aThreshold);
  }
  




  void SimplifyInward (uint32_t aMaxRects)
  {
    mImpl.SimplifyInward (aMaxRects);
  }

  typedef void (*visitFn)(void *closure, VisitSide side, int x1, int y1, int x2, int y2);
  void VisitEdges (visitFn visit, void *closure)
  {
    mImpl.VisitEdges (visit, closure);
  }

  nsCString ToString() const { return mImpl.ToString(); }

  class NS_GFX RectIterator
  {
    nsRegionRectIterator mImpl;
    Rect mTmp;

  public:
    explicit RectIterator (const BaseIntRegion& aRegion) : mImpl (aRegion.mImpl) {}

    const Rect* Next ()
    {
      const nsRect* r = mImpl.Next();
      if (!r)
        return nullptr;
      mTmp = FromRect (*r);
      return &mTmp;
    }

    const Rect* Prev ()
    {
      const nsRect* r = mImpl.Prev();
      if (!r)
        return nullptr;
      mTmp = FromRect (*r);
      return &mTmp;
    }

    void Reset ()
    {
      mImpl.Reset ();
    }
  };

protected:
  
  
  explicit BaseIntRegion(const nsRegion& aImpl) : mImpl(aImpl) {}
  const nsRegion& Impl() const { return mImpl; }
private:
  nsRegion mImpl;

  static nsRect ToRect(const Rect& aRect)
  {
    return nsRect (aRect.x, aRect.y, aRect.width, aRect.height);
  }
  static Rect FromRect(const nsRect& aRect)
  {
    return Rect (aRect.x, aRect.y, aRect.width, aRect.height);
  }

  Derived& This()
  {
    return *static_cast<Derived*>(this);
  }
  const Derived& This() const
  {
    return *static_cast<const Derived*>(this);
  }
};

}  
}  

class NS_GFX nsIntRegion : public mozilla::gfx::BaseIntRegion<nsIntRegion, nsIntRect, nsIntPoint, nsIntMargin>
{
public:
  
  nsIntRegion() {}
  MOZ_IMPLICIT nsIntRegion(const nsIntRect& aRect) : BaseIntRegion(aRect) {}
  nsIntRegion(const nsIntRegion& aRegion) : BaseIntRegion(aRegion) {}
  nsIntRegion(nsIntRegion&& aRegion) : BaseIntRegion(mozilla::Move(aRegion)) {}

  
  
  nsIntRegion& operator=(const nsIntRegion& aRegion)
  {
    return BaseIntRegion::operator=(aRegion);
  }
  nsIntRegion& operator=(nsIntRegion&& aRegion)
  {
    return BaseIntRegion::operator=(mozilla::Move(aRegion));
  }
};

typedef nsIntRegion::RectIterator nsIntRegionRectIterator;

#endif
