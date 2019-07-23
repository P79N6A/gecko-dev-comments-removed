




































#ifndef nsRegion_h__
#define nsRegion_h__






#include "nsRect.h"
#include "nsPoint.h"

class NS_GFX nsRegion
{
  friend class nsRegionRectIterator;
  friend class RgnRectMemoryAllocator;








  struct nsRectFast : public nsRect
  {
    nsRectFast () {}      
    nsRectFast (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) : nsRect (aX, aY, aWidth, aHeight) {}
    nsRectFast (const nsRect& aRect) : nsRect (aRect) {}

    
    inline PRBool Contains (const nsRect& aRect) const;
    inline PRBool Intersects (const nsRect& aRect) const;
    inline PRBool IntersectRect (const nsRect& aRect1, const nsRect& aRect2);
    inline void UnionRect (const nsRect& aRect1, const nsRect& aRect2);
  };


  struct RgnRect : public nsRectFast
  {
    RgnRect* prev;
    RgnRect* next;

    RgnRect () {}                           
    RgnRect (PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) : nsRectFast (aX, aY, aWidth, aHeight) {}
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

  PRBool Contains (const nsRect& aRect) const;
  PRBool Intersects (const nsRect& aRect) const;

  void MoveBy (PRInt32 aXOffset, PRInt32 aYOffset)
  {
    MoveBy (nsPoint (aXOffset, aYOffset));
  }
  void MoveBy (nsPoint aPt);
  void SetEmpty ()
  {
    SetToElements (0);
    mBoundRect.SetRect (0, 0, 0, 0);
  }

  PRBool IsEmpty () const { return mRectCount == 0; }
  PRBool IsComplex () const { return mRectCount > 1; }
  PRBool IsEqual (const nsRegion& aRegion) const;
  PRUint32 GetNumRects () const { return mRectCount; }
  const nsRect& GetBounds () const { return mBoundRect; }

  





  void SimplifyOutward (PRUint32 aMaxRects);
  




  void SimplifyInward (PRUint32 aMaxRects);
  






  void SimpleSubtract (const nsRect& aRect);
  







  void SimpleSubtract (const nsRegion& aRegion);

  


  static nsresult InitStatic();

  


  static void ShutdownStatic();

private:
  PRUint32    mRectCount;
  RgnRect*    mCurRect;
  RgnRect     mRectListHead;
  nsRectFast  mBoundRect;

  void Init ();
  nsRegion& Copy (const nsRegion& aRegion);
  nsRegion& Copy (const nsRect& aRect);
  void InsertBefore (RgnRect* aNewRect, RgnRect* aRelativeRect);
  void InsertAfter (RgnRect* aNewRect, RgnRect* aRelativeRect);
  void SetToElements (PRUint32 aCount);
  RgnRect* Remove (RgnRect* aRect);
  void InsertInPlace (RgnRect* aRect, PRBool aOptimizeOnFly = PR_FALSE);
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
    return (mCurPtr != &mRegion->mRectListHead) ? mCurPtr : nsnull;
  }

  const nsRect* Prev ()
  {
    mCurPtr = mCurPtr->prev;
    return (mCurPtr != &mRegion->mRectListHead) ? mCurPtr : nsnull;
  }

  void Reset ()
  {
    mCurPtr = &mRegion->mRectListHead;
  }
};


#endif
