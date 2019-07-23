








































#ifndef nsFloatManager_h_
#define nsFloatManager_h_

#include "nsIntervalSet.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsTArray.h"

class nsIPresShell;
class nsIFrame;
struct nsHTMLReflowState;
class nsPresContext;








struct nsFlowAreaRect {
  nsRect mRect;
  PRPackedBool mHasFloats;

  nsFlowAreaRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                 PRBool aHasFloats)
    : mRect(aX, aY, aWidth, aHeight), mHasFloats(aHasFloats) {}
};

#define NS_FLOAT_MANAGER_CACHE_SIZE 4

class nsFloatManager {
public:
  nsFloatManager(nsIPresShell* aPresShell);
  ~nsFloatManager();

  void* operator new(size_t aSize) CPP_THROW_NEW;
  void operator delete(void* aPtr, size_t aSize);

  static void Shutdown();

  




  static nsRect GetRegionFor(nsIFrame* aFloatFrame);
  






  static nsRect CalculateRegionFor(nsIFrame* aFloatFrame,
                                   const nsMargin& aMargin);
  




  static nsresult StoreRegionFor(nsIFrame* aFloat,
                                 nsRect&   aRegion);

  
  
  struct SavedState;
  friend struct SavedState;
  struct SavedState {
  private:
    PRUint32 mFloatInfoCount;
    nscoord mX, mY;
    
    friend class nsFloatManager;
  };

  




  void Translate(nscoord aDx, nscoord aDy) { mX += aDx; mY += aDy; }

  




  void GetTranslation(nscoord& aX, nscoord& aY) const { aX = mX; aY = mY; }

  





































  enum BandInfoType { BAND_FROM_POINT, WIDTH_WITHIN_HEIGHT };
  nsFlowAreaRect GetFlowArea(nscoord aY, BandInfoType aInfoType,
                             nscoord aHeight, nscoord aContentAreaWidth,
                             SavedState* aState) const;

  






  nsresult AddFloat(nsIFrame* aFloatFrame, const nsRect& aMarginRect);

  







  nsresult RemoveTrailingRegions(nsIFrame* aFrameList);

private:
  struct FloatInfo;
public:

  PRBool HasAnyFloats() const { return !mFloats.IsEmpty(); }

  



  PRBool HasFloatDamage() const
  {
    return !mFloatDamage.IsEmpty();
  }

  void IncludeInDamage(nscoord aIntervalBegin, nscoord aIntervalEnd)
  {
    mFloatDamage.IncludeInterval(aIntervalBegin + mY, aIntervalEnd + mY);
  }

  PRBool IntersectsDamage(nscoord aIntervalBegin, nscoord aIntervalEnd) const
  {
    return mFloatDamage.Intersects(aIntervalBegin + mY, aIntervalEnd + mY);
  }

  


  void PushState(SavedState* aState);

  









  void PopState(SavedState* aState);

  






  nscoord GetLowestFloatTop() const;

  





  nscoord ClearFloats(nscoord aY, PRUint8 aBreakType) const;

  



  PRBool ClearContinues(PRUint8 aBreakType) const;

  void AssertStateMatches(SavedState *aState) const
  {
    NS_ASSERTION(aState->mX == mX && aState->mY == mY &&
                 aState->mFloatInfoCount == mFloats.Length(),
                 "float manager state should match saved state");
  }

#ifdef DEBUG
  


  nsresult List(FILE* out) const;
#endif

private:

  struct FloatInfo {
    nsIFrame *const mFrame;
    nsRect mRect;
    
    nscoord mLeftYMost, mRightYMost;

    FloatInfo(nsIFrame* aFrame, const nsRect& aRect);
#ifdef NS_BUILD_REFCNT_LOGGING
    FloatInfo(const FloatInfo& aOther);
    ~FloatInfo();
#endif
  };

  nscoord         mX, mY;     
  nsTArray<FloatInfo> mFloats;
  nsIntervalSet   mFloatDamage;

  static PRInt32 sCachedFloatManagerCount;
  static void* sCachedFloatManagers[NS_FLOAT_MANAGER_CACHE_SIZE];

  nsFloatManager(const nsFloatManager&);  
  void operator=(const nsFloatManager&);  
};






class nsAutoFloatManager {
public:
  nsAutoFloatManager(nsHTMLReflowState& aReflowState)
    : mReflowState(aReflowState),
      mNew(nsnull),
      mOld(nsnull) {}

  ~nsAutoFloatManager();

  




  nsresult
  CreateFloatManager(nsPresContext *aPresContext);

protected:
  nsHTMLReflowState &mReflowState;
  nsFloatManager *mNew;
  nsFloatManager *mOld;
};

#endif 
