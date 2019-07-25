



































#include "nsRegion.h"
#include "nsISupportsImpl.h"
#include "nsTArray.h"









#ifdef NS_COORD_IS_FLOAT
#define NS_COORD_LESS_SENTINEL nscoord_MIN
#define NS_COORD_GREATER_SENTINEL nscoord_MAX
#else
#define NS_COORD_LESS_SENTINEL PR_INT32_MIN
#define NS_COORD_GREATER_SENTINEL PR_INT32_MAX
#endif




inline PRBool nsRegion::nsRectFast::Contains (const nsRect& aRect) const
{
  return (PRBool) ((aRect.x >= x) && (aRect.y >= y) &&
                   (aRect.XMost () <= XMost ()) && (aRect.YMost () <= YMost ()));
}

inline PRBool nsRegion::nsRectFast::Intersects (const nsRect& aRect) const
{
  return (PRBool) ((x < aRect.XMost ()) && (y < aRect.YMost ()) &&
                   (aRect.x < XMost ()) && (aRect.y < YMost ()));
}

inline PRBool nsRegion::nsRectFast::IntersectRect (const nsRect& aRect1, const nsRect& aRect2)
{
  const nscoord xmost = NS_MIN (aRect1.XMost (), aRect2.XMost ());
  x = NS_MAX (aRect1.x, aRect2.x);
  width = xmost - x;
  if (width <= 0) return PR_FALSE;

  const nscoord ymost = NS_MIN (aRect1.YMost (), aRect2.YMost ());
  y = NS_MAX (aRect1.y, aRect2.y);
  height = ymost - y;
  if (height <= 0) return PR_FALSE;

  return PR_TRUE;
}

inline void nsRegion::nsRectFast::UnionRect (const nsRect& aRect1, const nsRect& aRect2)
{
  const nscoord xmost = NS_MAX (aRect1.XMost (), aRect2.XMost ());
  const nscoord ymost = NS_MAX (aRect1.YMost (), aRect2.YMost ());
  x = PR_MIN (aRect1.x, aRect2.x);
  y = PR_MIN (aRect1.y, aRect2.y);
  width  = xmost - x;
  height = ymost - y;
}







#define INIT_MEM_CHUNK_ENTRIES 100
#define INCR_MEM_CHUNK_ENTRIES 100

class RgnRectMemoryAllocator
{
  nsRegion::RgnRect*  mFreeListHead;
  PRUint32  mFreeEntries;
  void*     mChunkListHead;
#if defined (DEBUG)
  NS_DECL_OWNINGTHREAD

  void InitLock ()    { NS_ASSERT_OWNINGTHREAD (RgnRectMemoryAllocator); }
  void DestroyLock () { NS_ASSERT_OWNINGTHREAD (RgnRectMemoryAllocator); }
  void Lock ()        { NS_ASSERT_OWNINGTHREAD (RgnRectMemoryAllocator); }
  void Unlock ()      { NS_ASSERT_OWNINGTHREAD (RgnRectMemoryAllocator); }
#else
  void InitLock ()    { }
  void DestroyLock () { }
  void Lock ()        { }
  void Unlock ()      { }
#endif

  void* AllocChunk (PRUint32 aEntries, void* aNextChunk, nsRegion::RgnRect* aTailDest)
  {
    PRUint8* pBuf = new PRUint8 [aEntries * sizeof (nsRegion::RgnRect) + sizeof (void*)];
    *reinterpret_cast<void**>(pBuf) = aNextChunk;
    nsRegion::RgnRect* pRect = reinterpret_cast<nsRegion::RgnRect*>(pBuf + sizeof (void*));

    for (PRUint32 cnt = 0 ; cnt < aEntries - 1 ; cnt++)
      pRect [cnt].next = &pRect [cnt + 1];

    pRect [aEntries - 1].next = aTailDest;

    return pBuf;
  }

  void FreeChunk (void* aChunk) {  delete [] (PRUint8 *) aChunk;  }
  void* NextChunk (void* aThisChunk) const { return *static_cast<void**>(aThisChunk); }

  nsRegion::RgnRect* ChunkHead (void* aThisChunk) const
  {   return reinterpret_cast<nsRegion::RgnRect*>(static_cast<PRUint8*>(aThisChunk) + sizeof (void*));  }

public:
  RgnRectMemoryAllocator (PRUint32 aNumOfEntries);
 ~RgnRectMemoryAllocator ();

  nsRegion::RgnRect* Alloc ();
  void Free (nsRegion::RgnRect* aRect);
};


RgnRectMemoryAllocator::RgnRectMemoryAllocator (PRUint32 aNumOfEntries)
{
  InitLock ();
  mChunkListHead = AllocChunk (aNumOfEntries, nsnull, nsnull);
  mFreeEntries   = aNumOfEntries;
  mFreeListHead  = ChunkHead (mChunkListHead);
}

RgnRectMemoryAllocator::~RgnRectMemoryAllocator ()
{
  while (mChunkListHead)
  {
    void* tmp = mChunkListHead;
    mChunkListHead = NextChunk (mChunkListHead);
    FreeChunk (tmp);
  }

#if 0
  











  DestroyLock ();
#endif
}

nsRegion::RgnRect* RgnRectMemoryAllocator::Alloc ()
{
  Lock ();

  if (mFreeEntries == 0)
  {
    mChunkListHead = AllocChunk (INCR_MEM_CHUNK_ENTRIES, mChunkListHead, mFreeListHead);
    mFreeEntries   = INCR_MEM_CHUNK_ENTRIES;
    mFreeListHead  = ChunkHead (mChunkListHead);
  }

  nsRegion::RgnRect* tmp = mFreeListHead;
  mFreeListHead = mFreeListHead->next;
  mFreeEntries--;
  Unlock ();

  return tmp;
}

void RgnRectMemoryAllocator::Free (nsRegion::RgnRect* aRect)
{
  Lock ();
  mFreeEntries++;
  aRect->next = mFreeListHead;
  mFreeListHead = aRect;
  Unlock ();
}



static RgnRectMemoryAllocator* gRectPool;

nsresult nsRegion::InitStatic()
{
  gRectPool = new RgnRectMemoryAllocator(INIT_MEM_CHUNK_ENTRIES);
  return !gRectPool ? NS_ERROR_OUT_OF_MEMORY : NS_OK;
}

void nsRegion::ShutdownStatic()
{
    delete gRectPool;
}

void* nsRegion::RgnRect::operator new (size_t) CPP_THROW_NEW
{
  return gRectPool->Alloc ();
}

void nsRegion::RgnRect::operator delete (void* aRect, size_t)
{
  gRectPool->Free (static_cast<RgnRect*>(aRect));
}



void nsRegion::Init()
{
  mRectListHead.prev = mRectListHead.next = &mRectListHead;
  mCurRect = &mRectListHead;
  mRectCount = 0;
  mBoundRect.SetRect (0, 0, 0, 0);
}

inline void nsRegion::InsertBefore (RgnRect* aNewRect, RgnRect* aRelativeRect)
{
  aNewRect->prev = aRelativeRect->prev;
  aNewRect->next = aRelativeRect;
  aRelativeRect->prev->next = aNewRect;
  aRelativeRect->prev = aNewRect;
  mCurRect = aNewRect;
  mRectCount++;
}

inline void nsRegion::InsertAfter (RgnRect* aNewRect, RgnRect* aRelativeRect)
{
  aNewRect->prev = aRelativeRect;
  aNewRect->next = aRelativeRect->next;
  aRelativeRect->next->prev = aNewRect;
  aRelativeRect->next = aNewRect;
  mCurRect = aNewRect;
  mRectCount++;
}





void nsRegion::SetToElements (PRUint32 aCount)
{
  if (mRectCount < aCount)        
  {
    PRUint32 InsertCount = aCount - mRectCount;
    mRectCount = aCount;
    RgnRect* pPrev = &mRectListHead;
    RgnRect* pNext = mRectListHead.next;

    while (InsertCount--)
    {
      mCurRect = new RgnRect;
      mCurRect->prev = pPrev;
      pPrev->next = mCurRect;
      pPrev = mCurRect;
    }

    pPrev->next = pNext;
    pNext->prev = pPrev;
  } else
  if (mRectCount > aCount)        
  {
    PRUint32 RemoveCount = mRectCount - aCount;
    mRectCount = aCount;
    mCurRect = mRectListHead.next;

    while (RemoveCount--)
    {
      RgnRect* tmp = mCurRect;
      mCurRect = mCurRect->next;
      delete tmp;
    }

    mRectListHead.next = mCurRect;
    mCurRect->prev = &mRectListHead;
  }
}








inline void nsRegion::SaveLinkChain ()
{
  RgnRect* pRect = &mRectListHead;

  do
  {
    pRect->prev = pRect->next;
    pRect = pRect->next;
  } while (pRect != &mRectListHead);
}


inline void nsRegion::RestoreLinkChain ()
{
  RgnRect* pPrev = &mRectListHead;
  RgnRect* pRect = mRectListHead.next = mRectListHead.prev;

  while (pRect != &mRectListHead)
  {
    pRect->next = pRect->prev;
    pRect->prev = pPrev;
    pPrev = pRect;
    pRect = pRect->next;
  }

  mRectListHead.prev = pPrev;
}






void nsRegion::InsertInPlace (RgnRect* aRect, PRBool aOptimizeOnFly)
{
  if (mRectCount == 0)
    InsertAfter (aRect, &mRectListHead);
  else
  {
    if (aRect->y > mCurRect->y)
    {
      mRectListHead.y = NS_COORD_GREATER_SENTINEL;
      while (aRect->y > mCurRect->next->y)
        mCurRect = mCurRect->next;

      mRectListHead.x = NS_COORD_GREATER_SENTINEL;
      while (aRect->y == mCurRect->next->y && aRect->x > mCurRect->next->x)
        mCurRect = mCurRect->next;

      InsertAfter (aRect, mCurRect);
    } else
    if (aRect->y < mCurRect->y)
    {
      mRectListHead.y = NS_COORD_LESS_SENTINEL;
      while (aRect->y < mCurRect->prev->y)
        mCurRect = mCurRect->prev;

      mRectListHead.x = NS_COORD_LESS_SENTINEL;
      while (aRect->y == mCurRect->prev->y && aRect->x < mCurRect->prev->x)
        mCurRect = mCurRect->prev;

      InsertBefore (aRect, mCurRect);
    } else
    {
      if (aRect->x > mCurRect->x)
      {
        mRectListHead.x = NS_COORD_GREATER_SENTINEL;
        while (aRect->y == mCurRect->next->y && aRect->x > mCurRect->next->x)
          mCurRect = mCurRect->next;

        InsertAfter (aRect, mCurRect);
      } else
      {
        mRectListHead.x = NS_COORD_LESS_SENTINEL;
        while (aRect->y == mCurRect->prev->y && aRect->x < mCurRect->prev->x)
          mCurRect = mCurRect->prev;

        InsertBefore (aRect, mCurRect);
      }
    }
  }


  if (aOptimizeOnFly)
  {
    if (mRectCount == 1)
      mBoundRect = *mCurRect;
    else
    {
      mBoundRect.UnionRect (mBoundRect, *mCurRect);

      
      if ((mCurRect->y == mCurRect->prev->y && mCurRect->height == mCurRect->prev->height &&
           mCurRect->x == mCurRect->prev->XMost ()) ||
          (mCurRect->x == mCurRect->prev->x && mCurRect->width == mCurRect->prev->width &&
           mCurRect->y == mCurRect->prev->YMost ()) )
        mCurRect = mCurRect->prev;

      
      while (mCurRect->y == mCurRect->next->y && mCurRect->height == mCurRect->next->height &&
             mCurRect->XMost () == mCurRect->next->x)
      {
        mCurRect->width += mCurRect->next->width;
        delete Remove (mCurRect->next);
      }

      
      while (mCurRect->x == mCurRect->next->x && mCurRect->width == mCurRect->next->width &&
             mCurRect->YMost () == mCurRect->next->y)
      {
        mCurRect->height += mCurRect->next->height;
        delete Remove (mCurRect->next);
      }
    }
  }
}


nsRegion::RgnRect* nsRegion::Remove (RgnRect* aRect)
{
  aRect->prev->next = aRect->next;
  aRect->next->prev = aRect->prev;
  mRectCount--;

  if (mCurRect == aRect)
    mCurRect = (aRect->next != &mRectListHead) ? aRect->next : aRect->prev;

  return aRect;
}






void nsRegion::Optimize ()
{
  if (mRectCount == 0)
    mBoundRect.SetRect (0, 0, 0, 0);
  else
  {
    RgnRect* pRect = mRectListHead.next;
    PRInt32 xmost = mRectListHead.prev->XMost ();
    PRInt32 ymost = mRectListHead.prev->YMost ();
    mBoundRect.x = mRectListHead.next->x;
    mBoundRect.y = mRectListHead.next->y;

    while (pRect != &mRectListHead)
    {
      
      while (pRect->y == pRect->next->y && pRect->height == pRect->next->height &&
             pRect->XMost () == pRect->next->x)
      {
        pRect->width += pRect->next->width;
        delete Remove (pRect->next);
      }

      
      while (pRect->x == pRect->next->x && pRect->width == pRect->next->width &&
             pRect->YMost () == pRect->next->y)
      {
        pRect->height += pRect->next->height;
        delete Remove (pRect->next);
      }

      
      if (pRect->x < mBoundRect.x) mBoundRect.x = pRect->x;
      if (pRect->XMost () > xmost) xmost = pRect->XMost ();
      if (pRect->YMost () > ymost) ymost = pRect->YMost ();

      pRect = pRect->next;
    }

    mBoundRect.width  = xmost - mBoundRect.x;
    mBoundRect.height = ymost - mBoundRect.y;
  }
}






void nsRegion::MoveInto (nsRegion& aDestRegion, const RgnRect* aStartRect)
{
  RgnRect* pRect = const_cast<RgnRect*>(aStartRect);
  RgnRect* pPrev = pRect->prev;

  while (pRect != &mRectListHead)
  {
    RgnRect* next = pRect->next;
    aDestRegion.InsertInPlace (pRect);

    mRectCount--;
    pRect = next;
  }

  pPrev->next = &mRectListHead;
  mRectListHead.prev = pPrev;
  mCurRect = mRectListHead.next;
}





void nsRegion::Merge (const nsRegion& aRgn1, const nsRegion& aRgn2)
{
  if (aRgn1.mRectCount == 0)            
    Copy (aRgn2);
  else
  if (aRgn2.mRectCount == 0)            
    Copy (aRgn1);
  if (aRgn1.mRectCount == 1)            
  {
    RgnRect* TmpRect = new RgnRect (*aRgn1.mRectListHead.next);
    Copy (aRgn2);
    InsertInPlace (TmpRect, PR_TRUE);
  } else
  if (aRgn2.mRectCount == 1)            
  {
    RgnRect* TmpRect = new RgnRect (*aRgn2.mRectListHead.next);
    Copy (aRgn1);
    InsertInPlace (TmpRect, PR_TRUE);
  } else
  {
    const nsRegion* pCopyRegion, *pInsertRegion;

    
    if (aRgn1.mRectCount >= aRgn2.mRectCount)
    {
      pCopyRegion = &aRgn1;
      pInsertRegion = &aRgn2;
    } else
    {
      pCopyRegion = &aRgn2;
      pInsertRegion = &aRgn1;
    }

    if (pInsertRegion == this)          
      pInsertRegion = pCopyRegion;
    else
      Copy (*pCopyRegion);

    const RgnRect* pSrcRect = pInsertRegion->mRectListHead.next;

    while (pSrcRect != &pInsertRegion->mRectListHead)
    {
      InsertInPlace (new RgnRect (*pSrcRect));

      pSrcRect = pSrcRect->next;
    }

    Optimize ();
  }
}


nsRegion& nsRegion::Copy (const nsRegion& aRegion)
{
  if (&aRegion == this)
    return *this;

  if (aRegion.mRectCount == 0)
    SetEmpty ();
  else
  {
    SetToElements (aRegion.mRectCount);

    const RgnRect* pSrc = aRegion.mRectListHead.next;
    RgnRect* pDest = mRectListHead.next;

    while (pSrc != &aRegion.mRectListHead)
    {
      *pDest = *pSrc;

      pSrc  = pSrc->next;
      pDest = pDest->next;
    }

    mCurRect = mRectListHead.next;
    mBoundRect = aRegion.mBoundRect;
  }

  return *this;
}


nsRegion& nsRegion::Copy (const nsRect& aRect)
{
  if (aRect.IsEmpty ())
    SetEmpty ();
  else
  {
    SetToElements (1);
    *mRectListHead.next = static_cast<const RgnRect&>(aRect);
    mBoundRect = static_cast<const nsRectFast&>(aRect);
  }

  return *this;
}


nsRegion& nsRegion::And (const nsRegion& aRgn1, const nsRegion& aRgn2)
{
  if (&aRgn1 == &aRgn2)                                       
    Copy (aRgn1);
  else
  if (aRgn1.mRectCount == 0 || aRgn2.mRectCount == 0)         
    SetEmpty ();
  else
  {
    nsRectFast TmpRect;

    if (aRgn1.mRectCount == 1 && aRgn2.mRectCount == 1)       
    {
      TmpRect.IntersectRect (*aRgn1.mRectListHead.next, *aRgn2.mRectListHead.next);
      Copy (TmpRect);
    } else
    {
      if (!aRgn1.mBoundRect.Intersects (aRgn2.mBoundRect))    
        SetEmpty ();
      else
      {
        
        if (aRgn1.mRectCount == 1 && aRgn1.mBoundRect.Contains (aRgn2.mBoundRect))
          Copy (aRgn2);
        else
        
        if (aRgn2.mRectCount == 1 && aRgn2.mBoundRect.Contains (aRgn1.mBoundRect))
          Copy (aRgn1);
        else
        {
          nsRegion TmpRegion;
          nsRegion* pSrcRgn1 = const_cast<nsRegion*>(&aRgn1);
          nsRegion* pSrcRgn2 = const_cast<nsRegion*>(&aRgn2);

          if (&aRgn1 == this)     
          {
            TmpRegion.Copy (aRgn1);
            pSrcRgn1 = &TmpRegion;
          }

          if (&aRgn2 == this)     
          {
            TmpRegion.Copy (aRgn2);
            pSrcRgn2 = &TmpRegion;
          }

          
          if (pSrcRgn2->mRectListHead.prev->y >= pSrcRgn1->mBoundRect.YMost ())
          {
            nsRegion* Tmp = pSrcRgn1;
            pSrcRgn1 = pSrcRgn2;
            pSrcRgn2 = Tmp;
          }


          SetToElements (0);
          pSrcRgn2->SaveLinkChain ();

          pSrcRgn1->mRectListHead.y = NS_COORD_GREATER_SENTINEL;
          pSrcRgn2->mRectListHead.y = NS_COORD_GREATER_SENTINEL;

          for (RgnRect* pSrcRect1 = pSrcRgn1->mRectListHead.next ;
               pSrcRect1->y < pSrcRgn2->mBoundRect.YMost () ; pSrcRect1 = pSrcRect1->next)
          {
            if (pSrcRect1->Intersects (pSrcRgn2->mBoundRect))   
            {
              RgnRect* pPrev2 = &pSrcRgn2->mRectListHead;

              for (RgnRect* pSrcRect2 = pSrcRgn2->mRectListHead.next ;
                   pSrcRect2->y < pSrcRect1->YMost () ; pSrcRect2 = pSrcRect2->next)
              {
                if (pSrcRect2->YMost () <= pSrcRect1->y)        
                {                                               
                  pPrev2->next = pSrcRect2->next;               
                  continue;
                }

                if (pSrcRect1->Contains (*pSrcRect2))           
                {                                               
                  pPrev2->next = pSrcRect2->next;               
                  InsertInPlace (new RgnRect (*pSrcRect2));
                  continue;
                }


                if (TmpRect.IntersectRect (*pSrcRect1, *pSrcRect2))
                  InsertInPlace (new RgnRect (TmpRect));

                pPrev2 = pSrcRect2;
              }
            }
          }

          pSrcRgn2->RestoreLinkChain ();
          Optimize ();
        }
      }
    }
  }

  return *this;
}


nsRegion& nsRegion::And (const nsRegion& aRegion, const nsRect& aRect)
{
  
  if (aRegion.mRectCount == 0 || aRect.IsEmpty ())
    SetEmpty ();
  else                            
  {
    const nsRectFast& aRectFast = static_cast<const nsRectFast&>(aRect);
    nsRectFast TmpRect;

    if (aRegion.mRectCount == 1)  
    {
      TmpRect.IntersectRect (*aRegion.mRectListHead.next, aRectFast);
      Copy (TmpRect);
    } else                        
    {
      if (!aRectFast.Intersects (aRegion.mBoundRect))   
        SetEmpty ();
      else
      {
        if (aRectFast.Contains (aRegion.mBoundRect))    
          Copy (aRegion);
        else
        {
          nsRegion TmpRegion;
          nsRegion* pSrcRegion = const_cast<nsRegion*>(&aRegion);

          if (&aRegion == this)   
          {
            TmpRegion.Copy (aRegion);
            pSrcRegion = &TmpRegion;
          }

          SetToElements (0);
          pSrcRegion->mRectListHead.y = NS_COORD_GREATER_SENTINEL;

          for (const RgnRect* pSrcRect = pSrcRegion->mRectListHead.next ;
               pSrcRect->y < aRectFast.YMost () ; pSrcRect = pSrcRect->next)
          {
            if (TmpRect.IntersectRect (*pSrcRect, aRectFast))
              InsertInPlace (new RgnRect (TmpRect));
          }

          Optimize ();
        }
      }
    }
  }

  return *this;
}


nsRegion& nsRegion::Or (const nsRegion& aRgn1, const nsRegion& aRgn2)
{
  if (&aRgn1 == &aRgn2)                 
    Copy (aRgn1);
  else
  if (aRgn1.mRectCount == 0)            
    Copy (aRgn2);
  else
  if (aRgn2.mRectCount == 0)            
    Copy (aRgn1);
  else
  {
    if (!aRgn1.mBoundRect.Intersects (aRgn2.mBoundRect))  
      Merge (aRgn1, aRgn2);
    else
    {
      
      if (aRgn1.mRectCount == 1 && aRgn1.mBoundRect.Contains (aRgn2.mBoundRect))
        Copy (aRgn1);
      else
      
      if (aRgn2.mRectCount == 1 && aRgn2.mBoundRect.Contains (aRgn1.mBoundRect))
        Copy (aRgn2);
      else
      {
        nsRegion TmpRegion;
        aRgn1.SubRegion (aRgn2, TmpRegion);               
        Copy (aRgn2);
        TmpRegion.MoveInto (*this);
        Optimize ();
      }
    }
  }

  return *this;
}


nsRegion& nsRegion::Or (const nsRegion& aRegion, const nsRect& aRect)
{
  if (aRegion.mRectCount == 0)          
    Copy (aRect);
  else
  if (aRect.IsEmpty ())                 
    Copy (aRegion);
  else
  {
    const nsRectFast& aRectFast = static_cast<const nsRectFast&>(aRect);

    if (!aRectFast.Intersects (aRegion.mBoundRect))     
    {
      Copy (aRegion);
      InsertInPlace (new RgnRect (aRectFast), PR_TRUE);
    } else
    {
      
      if (aRegion.mRectCount == 1 && aRegion.mBoundRect.Contains (aRectFast))
        Copy (aRegion);
      else
      if (aRectFast.Contains (aRegion.mBoundRect))      
        Copy (aRectFast);
      else
      {
        aRegion.SubRect (aRectFast, *this);             
        InsertInPlace (new RgnRect (aRectFast));
        Optimize ();
      }
    }
  }

  return *this;
}


nsRegion& nsRegion::Xor (const nsRegion& aRgn1, const nsRegion& aRgn2)
{
  if (&aRgn1 == &aRgn2)                 
    SetEmpty ();
  else
  if (aRgn1.mRectCount == 0)            
    Copy (aRgn2);
  else
  if (aRgn2.mRectCount == 0)            
    Copy (aRgn1);
  else
  {
    if (!aRgn1.mBoundRect.Intersects (aRgn2.mBoundRect))      
      Merge (aRgn1, aRgn2);
    else
    {
      
      if (aRgn1.mRectCount == 1 && aRgn1.mBoundRect.Contains (aRgn2.mBoundRect))
      {
        aRgn1.SubRegion (aRgn2, *this);
        Optimize ();
      } else
      
      if (aRgn2.mRectCount == 1 && aRgn2.mBoundRect.Contains (aRgn1.mBoundRect))
      {
        aRgn2.SubRegion (aRgn1, *this);
        Optimize ();
      } else
      {
        nsRegion TmpRegion;
        aRgn1.SubRegion (aRgn2, TmpRegion);
        aRgn2.SubRegion (aRgn1, *this);
        TmpRegion.MoveInto (*this);
        Optimize ();
      }
    }
  }

  return *this;
}


nsRegion& nsRegion::Xor (const nsRegion& aRegion, const nsRect& aRect)
{
  if (aRegion.mRectCount == 0)          
    Copy (aRect);
  else
  if (aRect.IsEmpty ())                 
    Copy (aRegion);
  else
  {
    const nsRectFast& aRectFast = static_cast<const nsRectFast&>(aRect);

    if (!aRectFast.Intersects (aRegion.mBoundRect))     
    {
      Copy (aRegion);
      InsertInPlace (new RgnRect (aRectFast), PR_TRUE);
    } else
    {
      
      if (aRegion.mRectCount == 1 && aRegion.mBoundRect.Contains (aRectFast))
      {
        aRegion.SubRect (aRectFast, *this);
        Optimize ();
      } else
      if (aRectFast.Contains (aRegion.mBoundRect))      
      {
        nsRegion TmpRegion;
        TmpRegion.Copy (aRectFast);
        TmpRegion.SubRegion (aRegion, *this);
        Optimize ();
      } else
      {
        nsRegion TmpRegion;
        TmpRegion.Copy (aRectFast);
        TmpRegion.SubRegion (aRegion, TmpRegion);
        aRegion.SubRect (aRectFast, *this);
        TmpRegion.MoveInto (*this);
        Optimize ();
      }
    }
  }

  return *this;
}


nsRegion& nsRegion::Sub (const nsRegion& aRgn1, const nsRegion& aRgn2)
{
  if (&aRgn1 == &aRgn2)         
    SetEmpty ();
  else
  if (aRgn1.mRectCount == 0)    
    SetEmpty ();
  else
  if (aRgn2.mRectCount == 0)    
    Copy (aRgn1);
  else
  {
    if (!aRgn1.mBoundRect.Intersects (aRgn2.mBoundRect))   
      Copy (aRgn1);
    else
    {
      aRgn1.SubRegion (aRgn2, *this);
      Optimize ();
    }
  }

  return *this;
}


nsRegion& nsRegion::Sub (const nsRegion& aRegion, const nsRect& aRect)
{
  if (aRegion.mRectCount == 0)    
    SetEmpty ();
  else
  if (aRect.IsEmpty ())           
    Copy (aRegion);
  else
  {
    const nsRectFast& aRectFast = static_cast<const nsRectFast&>(aRect);

    if (!aRectFast.Intersects (aRegion.mBoundRect))   
      Copy (aRegion);
    else
    {
      if (aRectFast.Contains (aRegion.mBoundRect))    
        SetEmpty ();
      else
      {
        aRegion.SubRect (aRectFast, *this);
        Optimize ();
      }
    }
  }

  return *this;
}

PRBool nsRegion::Contains (const nsRect& aRect) const
{
  if (aRect.IsEmpty())
    return PR_TRUE;
  if (IsEmpty())
    return PR_FALSE;
  if (!IsComplex())
    return mBoundRect.Contains (aRect);

  nsRegion tmpRgn;
  tmpRgn.Sub(aRect, *this);
  return tmpRgn.IsEmpty();
}

PRBool nsRegion::Contains (const nsRegion& aRgn) const
{
  
  nsRegionRectIterator iter(aRgn);
  while (const nsRect* r = iter.Next()) {
    if (!Contains (*r)) {
      return PR_FALSE;
    }
  }
  return PR_TRUE;
}

PRBool nsRegion::Intersects (const nsRect& aRect) const
{
  if (aRect.IsEmpty() || IsEmpty())
    return PR_FALSE;

  const RgnRect* r = mRectListHead.next;
  while (r != &mRectListHead)
  {
    if (r->Intersects(aRect))
      return PR_TRUE;
    r = r->next;
  }
  return PR_FALSE;
}






void nsRegion::SubRegion (const nsRegion& aRegion, nsRegion& aResult) const
{
  if (aRegion.mRectCount == 1)    
  {
    if (aRegion.mBoundRect.Contains (mBoundRect))
      aResult.SetEmpty ();
    else
      SubRect (*aRegion.mRectListHead.next, aResult);
  } else
  {
    nsRegion TmpRegion, CompletedRegion;
    const nsRegion* pSubRgn = &aRegion;

    if (&aResult == &aRegion)     
    {
      TmpRegion.Copy (aRegion);
      pSubRgn = &TmpRegion;
    }

    const RgnRect* pSubRect = pSubRgn->mRectListHead.next;

    SubRect (*pSubRect, aResult, CompletedRegion);
    pSubRect = pSubRect->next;

    while (pSubRect != &pSubRgn->mRectListHead)
    {
      aResult.SubRect (*pSubRect, aResult, CompletedRegion);
      pSubRect = pSubRect->next;
    }

    CompletedRegion.MoveInto (aResult);
  }
}












void nsRegion::SubRect (const nsRectFast& aRect, nsRegion& aResult, nsRegion& aCompleted) const
{
  nsRegion TmpRegion;
  const nsRegion* pSrcRegion = this;

  if (&aResult == this)           
  {
    TmpRegion.Copy (*this);
    pSrcRegion = &TmpRegion;
  }

  aResult.SetToElements (0);

  const_cast<nsRegion*>(pSrcRegion)->mRectListHead.y = NS_COORD_GREATER_SENTINEL;
  const RgnRect* pSrcRect = pSrcRegion->mRectListHead.next;

  for ( ; pSrcRect->y < aRect.YMost () ; pSrcRect = pSrcRect->next)
  {
    nsRectFast TmpRect;

    
    
    
    if (pSrcRect->YMost () <= aRect.y)
    {
      aCompleted.InsertInPlace (new RgnRect (*pSrcRect));
      continue;
    }

    if (!TmpRect.IntersectRect (*pSrcRect, aRect))
      aResult.InsertInPlace (new RgnRect (*pSrcRect));
    else
    {
      
      const nscoord ax  = pSrcRect->x;
      const nscoord axm = pSrcRect->XMost ();
      const nscoord aw  = pSrcRect->width;
      const nscoord ay  = pSrcRect->y;
      const nscoord aym = pSrcRect->YMost ();
      const nscoord ah  = pSrcRect->height;
      
      const nscoord bx  = aRect.x;
      const nscoord bxm = aRect.XMost ();
      const nscoord by  = aRect.y;
      const nscoord bym = aRect.YMost ();
      
      const nscoord ix  = TmpRect.x;
      const nscoord ixm = TmpRect.XMost ();
      const nscoord iy  = TmpRect.y;
      const nscoord iym = TmpRect.YMost ();
      const nscoord ih  = TmpRect.height;

      

      if (bx <= ax && by <= ay)
      {
        if (bxm < axm && bym < aym)     
        {
          aResult.InsertInPlace (new RgnRect (ixm, ay, axm - ixm, ih));
          aResult.InsertInPlace (new RgnRect (ax, iym, aw, aym - iym));
        } else
        if (bxm >= axm && bym < aym)    
        {
          aResult.InsertInPlace (new RgnRect (ax, iym, aw, aym - iym));
        } else
        if (bxm < axm && bym >= aym)    
        {
          aResult.InsertInPlace (new RgnRect (ixm, ay, axm - ixm, ah));
        } else
        if (pSrcRect->IsEqualInterior(aRect)) 
        {                               
          pSrcRect = pSrcRect->next;    
          break;                        
        }
      } else
      if (bx > ax && by <= ay)
      {
        if (bxm < axm && bym < aym)     
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, ix - ax, ih));
          aResult.InsertInPlace (new RgnRect (ixm, ay, axm - ixm, ih));
          aResult.InsertInPlace (new RgnRect (ax, iym, aw, aym - iym));
        } else
        if (bxm >= axm && bym < aym)    
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, ix - ax, ih));
          aResult.InsertInPlace (new RgnRect (ax, iym, aw, aym - iym));
        } else
        if (bxm < axm && bym >= aym)    
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, ix - ax, ah));
          aResult.InsertInPlace (new RgnRect (ixm, ay, axm - ixm, ah));
        } else
        if (bxm >= axm && bym >= aym)   
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, ix - ax, ah));
        }
      } else
      if (bx <= ax && by > ay)
      {
        if (bxm < axm && bym < aym)     
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, aw, iy - ay));
          aResult.InsertInPlace (new RgnRect (ixm, iy, axm - ixm, ih));
          aResult.InsertInPlace (new RgnRect (ax, iym, aw, aym - iym));
        } else
        if (bxm >= axm && bym < aym)    
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, aw, iy - ay));
          aResult.InsertInPlace (new RgnRect (ax, iym, aw, aym - iym));
        } else
        if (bxm < axm && bym >= aym)    
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, aw, iy - ay));
          aResult.InsertInPlace (new RgnRect (ixm, iy, axm - ixm, ih));
        } else
        if (bxm >= axm && bym >= aym)   
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, aw, iy - ay));
        }
      } else
      if (bx > ax && by > ay)
      {
        if (bxm < axm && bym < aym)     
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, aw, iy - ay));
          aResult.InsertInPlace (new RgnRect (ax, iy, ix - ax, ih));
          aResult.InsertInPlace (new RgnRect (ixm, iy, axm - ixm, ih));
          aResult.InsertInPlace (new RgnRect (ax, iym, aw, aym - iym));

          
          pSrcRect = pSrcRect->next;    
          break;
        } else
        if (bxm >= axm && bym < aym)    
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, aw, iy - ay));
          aResult.InsertInPlace (new RgnRect (ax, iy, ix - ax, ih));
          aResult.InsertInPlace (new RgnRect (ax, iym, aw, aym - iym));
        } else
        if (bxm < axm && bym >= aym)    
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, aw, iy - ay));
          aResult.InsertInPlace (new RgnRect (ax, iy, ix - ax, ih));
          aResult.InsertInPlace (new RgnRect (ixm, iy, axm - ixm, ih));
        } else
        if (bxm >= axm && bym >= aym)   
        {
          aResult.InsertInPlace (new RgnRect (ax, ay, aw, iy - ay));
          aResult.InsertInPlace (new RgnRect (ax, iy, ix - ax, ih));
        }
      }
    }
  }

  
  
  if (pSrcRegion == &TmpRegion)
    TmpRegion.MoveInto (aResult, pSrcRect);
  else
  {
    while (pSrcRect != &pSrcRegion->mRectListHead)
    {
      aResult.InsertInPlace (new RgnRect (*pSrcRect));
      pSrcRect = pSrcRect->next;
    }
  }
}


PRBool nsRegion::IsEqual (const nsRegion& aRegion) const
{
  if (mRectCount == 0)
    return (aRegion.mRectCount == 0) ? PR_TRUE : PR_FALSE;

  if (aRegion.mRectCount == 0)
    return (mRectCount == 0) ? PR_TRUE : PR_FALSE;

  if (mRectCount == 1 && aRegion.mRectCount == 1) 
    return (mRectListHead.next->IsEqualInterior(*aRegion.mRectListHead.next));
  else                                            
  {
    if (!mBoundRect.IsEqualInterior(aRegion.mBoundRect)) 
      return PR_FALSE;
    else
    {
      nsRegion TmpRegion;
      TmpRegion.Xor (*this, aRegion);             

      return (TmpRegion.mRectCount == 0);
    }
  }
}


void nsRegion::MoveBy (nsPoint aPt)
{
  if (aPt.x || aPt.y)
  {
    RgnRect* pRect = mRectListHead.next;

    while (pRect != &mRectListHead)
    {
      pRect->MoveBy (aPt.x, aPt.y);
      pRect = pRect->next;
    }

    mBoundRect.MoveBy (aPt.x, aPt.y);
  }
}

nsRegion& nsRegion::ScaleRoundOut (float aXScale, float aYScale)
{
  nsRegion region;
  nsRegionRectIterator iter(*this);
  for (;;) {
    const nsRect* r = iter.Next();
    if (!r)
      break;
    nsRect rect = *r;
    rect.ScaleRoundOut(aXScale, aYScale);
    region.Or(region, rect);
  }
  *this = region;
  return *this;
}

nsRegion& nsRegion::ScaleInverseRoundOut (float aXScale, float aYScale)
{
  nsRegion region;
  nsRegionRectIterator iter(*this);
  for (;;) {
    const nsRect* r = iter.Next();
    if (!r)
      break;
    nsRect rect = *r;
    rect.ScaleInverseRoundOut(aXScale, aYScale);
    region.Or(region, rect);
  }
  *this = region;
  return *this;
}

nsRegion nsRegion::ConvertAppUnitsRoundOut (PRInt32 aFromAPP, PRInt32 aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }
  
  
  nsRegion region;
  nsRegionRectIterator iter(*this);
  for (;;) {
    const nsRect* r = iter.Next();
    if (!r)
      break;
    nsRect rect = r->ConvertAppUnitsRoundOut(aFromAPP, aToAPP);
    region.Or(region, rect);
  }
  return region;
}

nsRegion nsRegion::ConvertAppUnitsRoundIn (PRInt32 aFromAPP, PRInt32 aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }
  
  
  nsRegion region;
  nsRegionRectIterator iter(*this);
  for (;;) {
    const nsRect* r = iter.Next();
    if (!r)
      break;
    nsRect rect = r->ConvertAppUnitsRoundIn(aFromAPP, aToAPP);
    region.Or(region, rect);
  }
  return region;
}

nsIntRegion nsRegion::ToPixels (nscoord aAppUnitsPerPixel, bool aOutsidePixels) const
{
  nsIntRegion result;
  nsRegionRectIterator rgnIter(*this);
  const nsRect* currentRect;
  while ((currentRect = rgnIter.Next())) {
    nsIntRect deviceRect;
    if (aOutsidePixels)
      deviceRect = currentRect->ToOutsidePixels(aAppUnitsPerPixel);
    else
      deviceRect = currentRect->ToNearestPixels(aAppUnitsPerPixel);
    result.Or(result, deviceRect);
  }
  return result;
}

nsIntRegion nsRegion::ToOutsidePixels (nscoord aAppUnitsPerPixel) const
{
  return ToPixels(aAppUnitsPerPixel, true);
}

nsIntRegion nsRegion::ToNearestPixels (nscoord aAppUnitsPerPixel) const
{
  return ToPixels(aAppUnitsPerPixel, false);
}

nsIntRegion nsRegion::ScaleToOutsidePixels (float aScaleX, float aScaleY,
                                            nscoord aAppUnitsPerPixel) const
{
  nsIntRegion result;
  nsRegionRectIterator rgnIter(*this);
  const nsRect* currentRect;
  while ((currentRect = rgnIter.Next())) {
    nsIntRect deviceRect =
      currentRect->ScaleToOutsidePixels(aScaleX, aScaleY, aAppUnitsPerPixel);
    result.Or(result, deviceRect);
  }
  return result;
}


















































































namespace {
  
  
  class AxisPartition {
  public:
    
    
    void InsertCoord(nscoord c) {
      PRUint32 i;
      if (!mStops.GreatestIndexLtEq(c, i)) {
        mStops.InsertElementAt(i, c);
      }
    }

    
    
    PRInt32 IndexOf(nscoord p) const {
      return mStops.BinaryIndexOf(p);
    }

    
    
    nscoord StopAt(PRInt32 index) const {
      return mStops[index];
    }

    
    
    
    nscoord StopSize(PRInt32 index) const {
      return mStops[index+1] - mStops[index];
    }

    
    PRInt32 GetNumStops() const { return mStops.Length(); }

  private:
    nsTArray<nscoord> mStops;
  };

  const PRInt64 kVeryLargeNegativeNumber = 0xffff000000000000ll;

  struct SizePair {
    PRInt64 mSizeContainingRect;
    PRInt64 mSize;

    SizePair() : mSizeContainingRect(0), mSize(0) {}

    static SizePair VeryLargeNegative() {
      SizePair result;
      result.mSize = result.mSizeContainingRect = kVeryLargeNegativeNumber;
      return result;
    }
    SizePair& operator=(const SizePair& aOther) {
      mSizeContainingRect = aOther.mSizeContainingRect;
      mSize = aOther.mSize;
      return *this;
    }
    PRBool operator<(const SizePair& aOther) const {
      if (mSizeContainingRect < aOther.mSizeContainingRect)
        return PR_TRUE;
      if (mSizeContainingRect > aOther.mSizeContainingRect)
        return PR_FALSE;
      return mSize < aOther.mSize;
    }
    PRBool operator>(const SizePair& aOther) const {
      return aOther.operator<(*this);
    }
    SizePair operator+(const SizePair& aOther) const {
      SizePair result = *this;
      result.mSizeContainingRect += aOther.mSizeContainingRect;
      result.mSize += aOther.mSize;
      return result;
    }
    SizePair operator-(const SizePair& aOther) const {
      SizePair result = *this;
      result.mSizeContainingRect -= aOther.mSizeContainingRect;
      result.mSize -= aOther.mSize;
      return result;
    }
  };

  
  
  SizePair MaxSum1D(const nsTArray<SizePair> &A, PRInt32 n,
                    PRInt32 *minIdx, PRInt32 *maxIdx) {
    
    SizePair min, max;
    PRInt32 currentMinIdx = 0;

    *minIdx = 0;
    *maxIdx = 0;

    
    
    for(PRInt32 i = 1; i < n; i++) {
      SizePair cand = A[i] - min;
      if (cand > max) {
        max = cand;
        *minIdx = currentMinIdx;
        *maxIdx = i;
      }
      if (min > A[i]) {
        min = A[i];
        currentMinIdx = i;
      }
    }

    return max;
  }
}

nsRect nsRegion::GetLargestRectangle (const nsRect& aContainingRect) const {
  nsRect bestRect;

  if (mRectCount <= 1) {
    bestRect = mBoundRect;
    return bestRect;
  }

  AxisPartition xaxis, yaxis;

  
  nsRegionRectIterator iter(*this);
  const nsRect *currentRect;
  while ((currentRect = iter.Next())) {
    xaxis.InsertCoord(currentRect->x);
    xaxis.InsertCoord(currentRect->XMost());
    yaxis.InsertCoord(currentRect->y);
    yaxis.InsertCoord(currentRect->YMost());
  }
  if (!aContainingRect.IsEmpty()) {
    xaxis.InsertCoord(aContainingRect.x);
    xaxis.InsertCoord(aContainingRect.XMost());
    yaxis.InsertCoord(aContainingRect.y);
    yaxis.InsertCoord(aContainingRect.YMost());
  }

  
  
  
  PRInt32 matrixHeight = yaxis.GetNumStops() - 1;
  PRInt32 matrixWidth = xaxis.GetNumStops() - 1;
  PRInt32 matrixSize = matrixHeight * matrixWidth;
  nsTArray<SizePair> areas(matrixSize);
  areas.SetLength(matrixSize);

  iter.Reset();
  while ((currentRect = iter.Next())) {
    PRInt32 xstart = xaxis.IndexOf(currentRect->x);
    PRInt32 xend = xaxis.IndexOf(currentRect->XMost());
    PRInt32 y = yaxis.IndexOf(currentRect->y);
    PRInt32 yend = yaxis.IndexOf(currentRect->YMost());

    for (; y < yend; y++) {
      nscoord height = yaxis.StopSize(y);
      for (PRInt32 x = xstart; x < xend; x++) {
        nscoord width = xaxis.StopSize(x);
        PRInt64 size = width*PRInt64(height);
        if (currentRect->Intersects(aContainingRect)) {
          areas[y*matrixWidth+x].mSizeContainingRect = size;
        }
        areas[y*matrixWidth+x].mSize = size;
      }
    }
  }

  
  {
    
    PRInt32 m = matrixHeight + 1;
    PRInt32 n = matrixWidth + 1;
    nsTArray<SizePair> pareas(m*n);
    pareas.SetLength(m*n);
    for (PRInt32 y = 1; y < m; y++) {
      for (PRInt32 x = 1; x < n; x++) {
        SizePair area = areas[(y-1)*matrixWidth+x-1];
        if (!area.mSize) {
          area = SizePair::VeryLargeNegative();
        }
        area = area + pareas[    y*n+x-1]
                    + pareas[(y-1)*n+x  ]
                    - pareas[(y-1)*n+x-1];
        pareas[y*n+x] = area;
      }
    }

    
    areas.SetLength(0);

    SizePair bestArea;
    struct {
      PRInt32 left, top, right, bottom;
    } bestRectIndices = { 0, 0, 0, 0 };
    for (PRInt32 m1 = 0; m1 < m; m1++) {
      for (PRInt32 m2 = m1+1; m2 < m; m2++) {
        nsTArray<SizePair> B;
        B.SetLength(n);
        for (PRInt32 i = 0; i < n; i++) {
          B[i] = pareas[m2*n+i] - pareas[m1*n+i];
        }
        PRInt32 minIdx, maxIdx;
        SizePair area = MaxSum1D(B, n, &minIdx, &maxIdx);
        if (area > bestArea) {
          bestRectIndices.left = minIdx;
          bestRectIndices.top = m1;
          bestRectIndices.right = maxIdx;
          bestRectIndices.bottom = m2;
          bestArea = area;
        }
      }
    }

    bestRect.MoveTo(xaxis.StopAt(bestRectIndices.left),
                    yaxis.StopAt(bestRectIndices.top));
    bestRect.SizeTo(xaxis.StopAt(bestRectIndices.right) - bestRect.x,
                    yaxis.StopAt(bestRectIndices.bottom) - bestRect.y);
  }

  return bestRect;
}

void nsRegion::SimplifyOutward (PRUint32 aMaxRects)
{
  NS_ASSERTION(aMaxRects >= 1, "Invalid max rect count");
  
  if (mRectCount <= aMaxRects)
    return;

  
  RgnRect* pRect = mRectListHead.next;
  while (pRect != &mRectListHead)
  {
    
    
    
    
    
    
    
    
    
    while (pRect->next != &mRectListHead &&
           pRect->YMost () >= pRect->next->y)
    {
      pRect->UnionRect(*pRect, *pRect->next);
      delete Remove (pRect->next);
    }

    pRect = pRect->next;
  }

  if (mRectCount <= aMaxRects)
    return;

  *this = GetBounds();
}

void nsRegion::SimplifyInward (PRUint32 aMaxRects)
{
  NS_ASSERTION(aMaxRects >= 1, "Invalid max rect count");

  if (mRectCount <= aMaxRects)
    return;

  SetEmpty();
}

void nsRegion::SimpleSubtract (const nsRect& aRect)
{
  if (aRect.IsEmpty())
    return;

  
  nsRect param = aRect;
  RgnRect* r = mRectListHead.next;
  while (r != &mRectListHead)
  {
    RgnRect* next = r->next;
    if (param.Contains(*r)) {
      delete Remove(r);
    }
    r = next;
  }
  
  Optimize();
}

void nsRegion::SimpleSubtract (const nsRegion& aRegion)
{
  if (aRegion.IsEmpty())
    return;

  if (&aRegion == this) {
    SetEmpty();
    return;
  }

  const RgnRect* r = aRegion.mRectListHead.next;
  while (r != &aRegion.mRectListHead)
  {
    SimpleSubtract(*r);
    r = r->next;
  }

  Optimize();
}

nsRegion nsIntRegion::ToAppUnits (nscoord aAppUnitsPerPixel) const
{
  nsRegion result;
  nsIntRegionRectIterator rgnIter(*this);
  const nsIntRect* currentRect;
  while ((currentRect = rgnIter.Next())) {
    nsRect appRect = currentRect->ToAppUnits(aAppUnitsPerPixel);
    result.Or(result, appRect);
  }
  return result;
}
