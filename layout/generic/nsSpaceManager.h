










































#ifndef nsSpaceManager_h___
#define nsSpaceManager_h___

#include "prclist.h"
#include "nsIntervalSet.h"
#include "nsISupports.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsVoidArray.h"

class nsIPresShell;
class nsIFrame;
struct nsSize;
struct nsHTMLReflowState;
class nsPresContext;

#define NS_SPACE_MANAGER_CACHE_SIZE 4










struct nsBandTrapezoid {
  nscoord   mTopY, mBottomY;            
  nscoord   mTopLeftX, mBottomLeftX;    
  nscoord   mTopRightX, mBottomRightX;  
  const nsSmallVoidArray* mFrames; 

  
  nscoord GetHeight() const {return mBottomY - mTopY;}

  
  inline void GetRect(nsRect& aRect) const;

  
  inline void operator=(const nsRect& aRect);

  
  inline PRBool EqualGeometry(const nsBandTrapezoid& aTrap) const;

  nsBandTrapezoid()
    : mTopY(0),
      mBottomY(0),
      mTopLeftX(0),
      mBottomLeftX(0),
      mTopRightX(0),
      mBottomRightX(0),
      mFrames(nsnull)
  {
  }
};

inline void nsBandTrapezoid::GetRect(nsRect& aRect) const
{
  aRect.x = PR_MIN(mTopLeftX, mBottomLeftX);
  aRect.y = mTopY;
  aRect.width = PR_MAX(mTopRightX, mBottomRightX);
  if (NS_MAXSIZE != aRect.width) {
    aRect.width -= aRect.x;
  }
  aRect.height = (NS_MAXSIZE == mBottomY) ? NS_MAXSIZE : mBottomY - mTopY;
}

inline void nsBandTrapezoid::operator=(const nsRect& aRect)
{
  mTopLeftX = mBottomLeftX = aRect.x;
  mTopRightX = mBottomRightX = aRect.XMost();
  mTopY = aRect.y;
  mBottomY = aRect.YMost();
}

inline PRBool nsBandTrapezoid::EqualGeometry(const nsBandTrapezoid& aTrap) const
{
  return (
    mTopLeftX == aTrap.mTopLeftX &&
    mBottomLeftX == aTrap.mBottomLeftX &&
    mTopRightX == aTrap.mTopRightX &&
    mBottomRightX == aTrap.mBottomRightX &&
    mTopY == aTrap.mTopY &&
    mBottomY == aTrap.mBottomY
  );
}





struct nsBandData {
  PRInt32 mCount; 
  PRInt32 mSize; 
  nsBandTrapezoid* mTrapezoids; 
};






class nsSpaceManager {
public:
  nsSpaceManager(nsIPresShell* aPresShell, nsIFrame* aFrame);
  ~nsSpaceManager();

  void* operator new(size_t aSize) CPP_THROW_NEW;
  void operator delete(void* aPtr, size_t aSize);

  static void Shutdown();

  







  nsIFrame* GetFrame() const { return mFrame; }

  




  void Translate(nscoord aDx, nscoord aDy) { mX += aDx; mY += aDy; }

  




  void GetTranslation(nscoord& aX, nscoord& aY) const { aX = mX; aY = mY; }

  





  PRBool XMost(nscoord& aXMost) const;

  




  PRBool YMost(nscoord& aYMost) const;

  






















  nsresult GetBandData(nscoord       aYOffset,
                       const nsSize& aMaxSize,
                       nsBandData&   aBandData) const;

  










  nsresult AddRectRegion(nsIFrame*     aFrame,
                         const nsRect& aUnavailableSpace);

  










  nsresult RemoveTrailingRegions(nsIFrame* aFrameList);

protected:
  







  nsresult RemoveRegion(nsIFrame* aFrame);

public:
  
  
  struct SavedState {
  private:
    nsIFrame *mLastFrame;
    nscoord mX, mY;
    nscoord mLowestTop;
    nscoord mMaximalLeftYMost;
    nscoord mMaximalRightYMost;
    PRPackedBool mHaveCachedLeftYMost;
    PRPackedBool mHaveCachedRightYMost;
    
    friend class nsSpaceManager;
  };

  


  void ClearRegions();

  PRBool HasAnyFloats() { return mFrameInfoMap != nsnull; }

  



  PRBool HasFloatDamage()
  {
    return !mFloatDamage.IsEmpty();
  }

  void IncludeInDamage(nscoord aIntervalBegin, nscoord aIntervalEnd)
  {
    mFloatDamage.IncludeInterval(aIntervalBegin + mY, aIntervalEnd + mY);
  }

  PRBool IntersectsDamage(nscoord aIntervalBegin, nscoord aIntervalEnd)
  {
    return mFloatDamage.Intersects(aIntervalBegin + mY, aIntervalEnd + mY);
  }

  


  void PushState(SavedState* aState);

  







  void PopState(SavedState* aState);

  




  nscoord GetLowestRegionTop();

  



  nscoord ClearFloats(nscoord aY, PRUint8 aBreakType);

#ifdef DEBUG
  


  nsresult List(FILE* out);
#endif

protected:
  
  
  struct FrameInfo {
    nsIFrame* const mFrame;
    nsRect          mRect;       
    FrameInfo*      mNext;

    FrameInfo(nsIFrame* aFrame, const nsRect& aRect);
#ifdef NS_BUILD_REFCNT_LOGGING
    ~FrameInfo();
#endif
  };

public:
  
  struct BandRect : PRCListStr {
    nscoord   mLeft, mTop;
    nscoord   mRight, mBottom;
    nsSmallVoidArray mFrames;  

    BandRect(nscoord aLeft, nscoord aTop,
             nscoord aRight, nscoord aBottom,
             nsIFrame* aFrame);
    BandRect(nscoord aLeft, nscoord aTop,
             nscoord aRight, nscoord aBottom,
             nsSmallVoidArray& frames);
    ~BandRect();

    
    BandRect* Next() const {return (BandRect*)PR_NEXT_LINK(this);}
    BandRect* Prev() const {return (BandRect*)PR_PREV_LINK(this);}
    void      InsertBefore(BandRect* aBandRect) {PR_INSERT_BEFORE(aBandRect, this);}
    void      InsertAfter(BandRect* aBandRect) {PR_INSERT_AFTER(aBandRect, this);}
    void      Remove() {PR_REMOVE_LINK(this);}

    
    
    
    
    
    BandRect* SplitVertically(nscoord aBottom);

    
    
    
    
    
    BandRect* SplitHorizontally(nscoord aRight);

    
    PRBool  IsOccupiedBy(const nsIFrame*) const;
    void    AddFrame(const nsIFrame* aFrame) {
      mFrames.AppendElement((void*)aFrame);
    }
    void    RemoveFrame(const nsIFrame* aFrame) {
      mFrames.RemoveElement((void*)aFrame);
    }
    nsIFrame * FrameAt(PRInt32 index) {
      return NS_STATIC_CAST(nsIFrame*, mFrames.FastElementAt(index));
    }
    PRBool  HasSameFrameList(const BandRect* aBandRect) const;
    PRInt32 Length() const;
  };

  
  struct BandList : BandRect {
    BandList();

    
    PRBool    IsEmpty() const {return PR_CLIST_IS_EMPTY((PRCListStr*)this);}
    BandRect* Head() const {return (BandRect*)PR_LIST_HEAD(this);}
    BandRect* Tail() const {return (BandRect*)PR_LIST_TAIL(this);}

    
    void      Append(BandRect* aBandRect) {PR_APPEND_LINK(aBandRect, this);}

    
    void      Clear();
  };

protected:
  nsIFrame* const mFrame;     
  nscoord         mX, mY;     
  BandList        mBandList;  
  nscoord         mLowestTop;  
  FrameInfo*      mFrameInfoMap;
  nsIntervalSet   mFloatDamage;
  PRPackedBool    mHaveCachedLeftYMost; 
  PRPackedBool    mHaveCachedRightYMost; 
  nscoord         mMaximalLeftYMost;  
                                      
                                      
                                      
  nscoord         mMaximalRightYMost; 
                                      
                                      
                                      
  
  
  
  BandRect*       mCachedBandPosition;

protected:
  FrameInfo* GetFrameInfoFor(nsIFrame* aFrame);
  FrameInfo* CreateFrameInfo(nsIFrame* aFrame, const nsRect& aRect);
  void       DestroyFrameInfo(FrameInfo*);

  void       ClearFrameInfo();
  void       ClearBandRects();

  BandRect*  GetNextBand(const BandRect* aBandRect) const;
  BandRect*  GetPrevBand(const BandRect* aBandRect) const;
  void       DivideBand(BandRect* aBand, nscoord aBottom);
  PRBool     CanJoinBands(BandRect* aBand, BandRect* aPrevBand);
  PRBool     JoinBands(BandRect* aBand, BandRect* aPrevBand);
  void       AddRectToBand(BandRect* aBand, BandRect* aBandRect);
  void       InsertBandRect(BandRect* aBandRect);

  nsresult   GetBandAvailableSpace(const BandRect* aBand,
                                   nscoord         aY,
                                   const nsSize&   aMaxSize,
                                   nsBandData&     aAvailableSpace) const;

  
  
  
  
  
  BandRect*  GuessBandWithTopAbove(nscoord aYOffset) const;

  void SetCachedBandPosition(BandRect* aBandRect) {
    NS_ASSERTION(!aBandRect ||
                 aBandRect == mBandList.Head() ||
                 aBandRect->Prev()->mBottom != aBandRect->mBottom,
                 "aBandRect should be first rect within its band");
    mCachedBandPosition = aBandRect;
  }


private:
  static PRInt32 sCachedSpaceManagerCount;
  static void* sCachedSpaceManagers[NS_SPACE_MANAGER_CACHE_SIZE];

  nsSpaceManager(const nsSpaceManager&);  
  void operator=(const nsSpaceManager&);  
};






class nsAutoSpaceManager {
public:
  nsAutoSpaceManager(nsHTMLReflowState& aReflowState)
    : mReflowState(aReflowState),
#ifdef DEBUG
      mOwns(PR_TRUE),
#endif
      mNew(nsnull),
      mOld(nsnull) {}

  ~nsAutoSpaceManager();

  




  nsresult
  CreateSpaceManagerFor(nsPresContext *aPresContext,
                        nsIFrame *aFrame);

#ifdef DEBUG
  




  void DebugOrphanSpaceManager() { mOwns = PR_FALSE; }
#endif

protected:
  nsHTMLReflowState &mReflowState;
#ifdef DEBUG
  PRBool mOwns;
#endif
  nsSpaceManager *mNew;
  nsSpaceManager *mOld;
};

#endif 

