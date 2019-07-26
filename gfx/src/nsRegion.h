




#ifndef nsRegion_h__
#define nsRegion_h__

#include "nsRect.h"
#include "nsPoint.h"
#include "nsString.h"

class nsIntRegion;








class NS_GFX nsRegion
{
  friend class nsRegionRectIterator;
  friend class RgnRectMemoryAllocator;








  struct nsRectFast : public nsRect
  {
    nsRectFast () {}      
    nsRectFast (int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight) : nsRect (aX, aY, aWidth, aHeight) {}
    nsRectFast (const nsRect& aRect) : nsRect (aRect) {}

    
    inline bool Contains (const nsRect& aRect) const;
    inline bool Intersects (const nsRect& aRect) const;
    inline bool IntersectRect (const nsRect& aRect1, const nsRect& aRect2);
    inline void UnionRect (const nsRect& aRect1, const nsRect& aRect2);
  };


  struct RgnRect : public nsRectFast
  {
    RgnRect* prev;
    RgnRect* next;

    RgnRect () {}                           
    RgnRect (int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight) : nsRectFast (aX, aY, aWidth, aHeight) {}
    RgnRect (const nsRectFast& aRect) : nsRectFast (aRect) {}

    void* operator new (size_t) CPP_THROW_NEW;
    void  operator delete (void* aRect, size_t);

    RgnRect& operator = (const RgnRect& aRect)      
    {
      x = aRect.x;
      y = aRect.y;
      width = aRect.width;
      height = aRect.height;
      return *this;
    }
  };


public:
  nsRegion () { Init (); }
  nsRegion (const nsRect& aRect) { Init (); Copy (aRect); }
  nsRegion (const nsRegion& aRegion) { Init (); Copy (aRegion); }
 ~nsRegion () { SetToElements (0); }
  nsRegion& operator = (const nsRect& aRect) { Copy (aRect); return *this; }
  nsRegion& operator = (const nsRegion& aRegion) { Copy (aRegion); return *this; }


  nsRegion& And  (const nsRegion& aRgn1,   const nsRegion& aRgn2);
  nsRegion& And  (const nsRegion& aRegion, const nsRect& aRect);
  nsRegion& And  (const nsRect& aRect, const nsRegion& aRegion)
  {
    return  And  (aRegion, aRect);
  }
  nsRegion& And  (const nsRect& aRect1, const nsRect& aRect2)
  {
    nsRect TmpRect;

    TmpRect.IntersectRect (aRect1, aRect2);
    return Copy (TmpRect);
  }

  nsRegion& Or   (const nsRegion& aRgn1,   const nsRegion& aRgn2);
  nsRegion& Or   (const nsRegion& aRegion, const nsRect& aRect);
  nsRegion& Or   (const nsRect& aRect, const nsRegion& aRegion)
  {
    return  Or   (aRegion, aRect);
  }
  nsRegion& Or   (const nsRect& aRect1, const nsRect& aRect2)
  {
    Copy (aRect1);
    return Or (*this, aRect2);
  }

  nsRegion& Xor  (const nsRegion& aRgn1,   const nsRegion& aRgn2);
  nsRegion& Xor  (const nsRegion& aRegion, const nsRect& aRect);
  nsRegion& Xor  (const nsRect& aRect, const nsRegion& aRegion)
  {
    return  Xor  (aRegion, aRect);
  }
  nsRegion& Xor  (const nsRect& aRect1, const nsRect& aRect2)
  {
    Copy (aRect1);
    return Xor (*this, aRect2);
  }

  nsRegion& Sub  (const nsRegion& aRgn1,   const nsRegion& aRgn2);
  nsRegion& Sub  (const nsRegion& aRegion, const nsRect& aRect);
  nsRegion& Sub  (const nsRect& aRect, const nsRegion& aRegion)
  {
    return Sub (nsRegion (aRect), aRegion);
  }
  nsRegion& Sub  (const nsRect& aRect1, const nsRect& aRect2)
  {
    Copy (aRect1);
    return Sub (*this, aRect2);
  }

  bool Contains (const nsRect& aRect) const;
  bool Contains (const nsRegion& aRgn) const;
  bool Intersects (const nsRect& aRect) const;

  void MoveBy (int32_t aXOffset, int32_t aYOffset)
  {
    MoveBy (nsPoint (aXOffset, aYOffset));
  }
  void MoveBy (nsPoint aPt);
  void SetEmpty ()
  {
    SetToElements (0);
    mBoundRect.SetRect (0, 0, 0, 0);
  }

  bool IsEmpty () const { return mRectCount == 0; }
  bool IsComplex () const { return mRectCount > 1; }
  bool IsEqual (const nsRegion& aRegion) const;
  uint32_t GetNumRects () const { return mRectCount; }
  const nsRect& GetBounds () const { return mBoundRect; }
  uint64_t Area () const;
  
  
  
  nsRegion ConvertAppUnitsRoundOut (int32_t aFromAPP, int32_t aToAPP) const;
  nsRegion ConvertAppUnitsRoundIn (int32_t aFromAPP, int32_t aToAPP) const;
  nsRegion& ScaleRoundOut(float aXScale, float aYScale);
  nsRegion& ScaleInverseRoundOut(float aXScale, float aYScale);
  nsIntRegion ScaleToOutsidePixels (float aXScale, float aYScale, nscoord aAppUnitsPerPixel) const;
  nsIntRegion ScaleToInsidePixels (float aXScale, float aYScale, nscoord aAppUnitsPerPixel) const;
  nsIntRegion ScaleToNearestPixels (float aXScale, float aYScale, nscoord aAppUnitsPerPixel) const;
  nsIntRegion ToOutsidePixels (nscoord aAppUnitsPerPixel) const;
  nsIntRegion ToNearestPixels (nscoord aAppUnitsPerPixel) const;

  





  nsRect GetLargestRectangle (const nsRect& aContainingRect = nsRect()) const;

  





  void SimplifyOutward (uint32_t aMaxRects);
  




  void SimplifyInward (uint32_t aMaxRects);
  






  void SimpleSubtract (const nsRect& aRect);
  







  void SimpleSubtract (const nsRegion& aRegion);

  nsCString ToString() const;

  


  static nsresult InitStatic();

  


  static void ShutdownStatic();

private:
  uint32_t    mRectCount;
  RgnRect*    mCurRect;
  RgnRect     mRectListHead;
  nsRectFast  mBoundRect;

  void Init ();
  nsRegion& Copy (const nsRegion& aRegion);
  nsRegion& Copy (const nsRect& aRect);
  void InsertBefore (RgnRect* aNewRect, RgnRect* aRelativeRect);
  void InsertAfter (RgnRect* aNewRect, RgnRect* aRelativeRect);
  void SetToElements (uint32_t aCount);
  RgnRect* Remove (RgnRect* aRect);
  void InsertInPlace (RgnRect* aRect, bool aOptimizeOnFly = false);
  inline void SaveLinkChain ();
  inline void RestoreLinkChain ();
  void Optimize ();
  void SubRegion (const nsRegion& aRegion, nsRegion& aResult) const;
  void SubRect (const nsRectFast& aRect, nsRegion& aResult, nsRegion& aCompleted) const;
  void SubRect (const nsRectFast& aRect, nsRegion& aResult) const
  {    SubRect (aRect, aResult, aResult);  }
  void Merge (const nsRegion& aRgn1, const nsRegion& aRgn2);
  void MoveInto (nsRegion& aDestRegion, const RgnRect* aStartRect);
  void MoveInto (nsRegion& aDestRegion)
  {    MoveInto (aDestRegion, mRectListHead.next);  }
  nsIntRegion ToPixels(nscoord aAppUnitsPerPixel, bool aOutsidePixels) const;
};





class NS_GFX nsRegionRectIterator
{
  const nsRegion*  mRegion;
  const nsRegion::RgnRect* mCurPtr;

public:
  nsRegionRectIterator (const nsRegion& aRegion)
  {
    mRegion = &aRegion;
    mCurPtr = &aRegion.mRectListHead;
  }

  const nsRect* Next ()
  {
    mCurPtr = mCurPtr->next;
    return (mCurPtr != &mRegion->mRectListHead) ? mCurPtr : nullptr;
  }

  const nsRect* Prev ()
  {
    mCurPtr = mCurPtr->prev;
    return (mCurPtr != &mRegion->mRectListHead) ? mCurPtr : nullptr;
  }

  void Reset ()
  {
    mCurPtr = &mRegion->mRectListHead;
  }
};




class NS_GFX nsIntRegion
{
  friend class nsIntRegionRectIterator;

public:
  nsIntRegion () {}
  nsIntRegion (const nsIntRect& aRect) : mImpl (ToRect(aRect)) {}
  nsIntRegion (const nsIntRegion& aRegion) : mImpl (aRegion.mImpl) {}
  nsIntRegion& operator = (const nsIntRect& aRect) { mImpl = ToRect (aRect); return *this; }
  nsIntRegion& operator = (const nsIntRegion& aRegion) { mImpl = aRegion.mImpl; return *this; }

  bool operator==(const nsIntRegion& aRgn) const
  {
    return IsEqual(aRgn);
  }

  nsIntRegion& And  (const nsIntRegion& aRgn1,   const nsIntRegion& aRgn2)
  {
    mImpl.And (aRgn1.mImpl, aRgn2.mImpl);
    return *this;
  }
  nsIntRegion& And  (const nsIntRegion& aRegion, const nsIntRect& aRect)
  {
    mImpl.And (aRegion.mImpl, ToRect (aRect));
    return *this;
  }
  nsIntRegion& And  (const nsIntRect& aRect, const nsIntRegion& aRegion)
  {
    return  And  (aRegion, aRect);
  }
  nsIntRegion& And  (const nsIntRect& aRect1, const nsIntRect& aRect2)
  {
    nsIntRect TmpRect;

    TmpRect.IntersectRect (aRect1, aRect2);
    mImpl = ToRect (TmpRect);
    return *this;
  }

  nsIntRegion& Or   (const nsIntRegion& aRgn1,   const nsIntRegion& aRgn2)
  {
    mImpl.Or (aRgn1.mImpl, aRgn2.mImpl);
    return *this;
  }
  nsIntRegion& Or   (const nsIntRegion& aRegion, const nsIntRect& aRect)
  {
    mImpl.Or (aRegion.mImpl, ToRect (aRect));
    return *this;
  }
  nsIntRegion& Or   (const nsIntRect& aRect, const nsIntRegion& aRegion)
  {
    return  Or   (aRegion, aRect);
  }
  nsIntRegion& Or   (const nsIntRect& aRect1, const nsIntRect& aRect2)
  {
    mImpl = ToRect (aRect1);
    return Or (*this, aRect2);
  }

  nsIntRegion& Xor  (const nsIntRegion& aRgn1,   const nsIntRegion& aRgn2)
  {
    mImpl.Xor (aRgn1.mImpl, aRgn2.mImpl);
    return *this;
  }
  nsIntRegion& Xor  (const nsIntRegion& aRegion, const nsIntRect& aRect)
  {
    mImpl.Xor (aRegion.mImpl, ToRect (aRect));
    return *this;
  }
  nsIntRegion& Xor  (const nsIntRect& aRect, const nsIntRegion& aRegion)
  {
    return  Xor  (aRegion, aRect);
  }
  nsIntRegion& Xor  (const nsIntRect& aRect1, const nsIntRect& aRect2)
  {
    mImpl = ToRect (aRect1);
    return Xor (*this, aRect2);
  }

  nsIntRegion& Sub  (const nsIntRegion& aRgn1,   const nsIntRegion& aRgn2)
  {
    mImpl.Sub (aRgn1.mImpl, aRgn2.mImpl);
    return *this;
  }
  nsIntRegion& Sub  (const nsIntRegion& aRegion, const nsIntRect& aRect)
  {
    mImpl.Sub (aRegion.mImpl, ToRect (aRect));
    return *this;
  }
  nsIntRegion& Sub  (const nsIntRect& aRect, const nsIntRegion& aRegion)
  {
    return Sub (nsIntRegion (aRect), aRegion);
  }
  nsIntRegion& Sub  (const nsIntRect& aRect1, const nsIntRect& aRect2)
  {
    mImpl = ToRect (aRect1);
    return Sub (*this, aRect2);
  }

  bool Contains (const nsIntRect& aRect) const
  {
    return mImpl.Contains (ToRect (aRect));
  }
  bool Contains (const nsIntRegion& aRgn) const
  {
    return mImpl.Contains (aRgn.mImpl);
  }
  bool Intersects (const nsIntRect& aRect) const
  {
    return mImpl.Intersects (ToRect (aRect));
  }

  void MoveBy (int32_t aXOffset, int32_t aYOffset)
  {
    MoveBy (nsIntPoint (aXOffset, aYOffset));
  }
  void MoveBy (nsIntPoint aPt)
  {
    mImpl.MoveBy (aPt.x, aPt.y);
  }
  void SetEmpty ()
  {
    mImpl.SetEmpty  ();
  }

  bool IsEmpty () const { return mImpl.IsEmpty (); }
  bool IsComplex () const { return mImpl.IsComplex (); }
  bool IsEqual (const nsIntRegion& aRegion) const
  {
    return mImpl.IsEqual (aRegion.mImpl);
  }
  uint32_t GetNumRects () const { return mImpl.GetNumRects (); }
  nsIntRect GetBounds () const { return FromRect (mImpl.GetBounds ()); }
  nsRegion ToAppUnits (nscoord aAppUnitsPerPixel) const;
  nsIntRect GetLargestRectangle (const nsIntRect& aContainingRect = nsIntRect()) const
  {
    return FromRect (mImpl.GetLargestRectangle( ToRect(aContainingRect) ));
  }

  nsIntRegion& ScaleRoundOut (float aXScale, float aYScale)
  {
    mImpl.ScaleRoundOut(aXScale, aYScale);
    return *this;
  }

  





  void SimplifyOutward (uint32_t aMaxRects)
  {
    mImpl.SimplifyOutward (aMaxRects);
  }
  




  void SimplifyInward (uint32_t aMaxRects)
  {
    mImpl.SimplifyInward (aMaxRects);
  }
  






  void SimpleSubtract (const nsIntRect& aRect)
  {
    mImpl.SimpleSubtract (ToRect (aRect));
  }
  







  void SimpleSubtract (const nsIntRegion& aRegion)
  {
    mImpl.SimpleSubtract (aRegion.mImpl);
  }

  nsCString ToString() const { return mImpl.ToString(); }

private:
  nsRegion mImpl;

  static nsRect ToRect(const nsIntRect& aRect)
  {
    return nsRect (aRect.x, aRect.y, aRect.width, aRect.height);
  }
  static nsIntRect FromRect(const nsRect& aRect)
  {
    return nsIntRect (aRect.x, aRect.y, aRect.width, aRect.height);
  }
};

class NS_GFX nsIntRegionRectIterator
{
  nsRegionRectIterator mImpl;
  nsIntRect mTmp;

public:
  nsIntRegionRectIterator (const nsIntRegion& aRegion) : mImpl (aRegion.mImpl) {}

  const nsIntRect* Next ()
  {
    const nsRect* r = mImpl.Next();
    if (!r)
      return nullptr;
    mTmp = nsIntRegion::FromRect (*r);
    return &mTmp;
  }

  const nsIntRect* Prev ()
  {
    const nsRect* r = mImpl.Prev();
    if (!r)
      return nullptr;
    mTmp = nsIntRegion::FromRect (*r);
    return &mTmp;
  }

  void Reset ()
  {
    mImpl.Reset ();
  }
};

#endif
