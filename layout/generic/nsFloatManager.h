







#ifndef nsFloatManager_h_
#define nsFloatManager_h_

#include "mozilla/Attributes.h"

#include "nsIntervalSet.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsTArray.h"
#include "nsFrameList.h" 

class nsIPresShell;
class nsIFrame;
struct nsHTMLReflowState;
class nsPresContext;








struct nsFlowAreaRect {
  nsRect mRect;
  bool mHasFloats;

  nsFlowAreaRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                 bool aHasFloats)
    : mRect(aX, aY, aWidth, aHeight), mHasFloats(aHasFloats) {}
};

#define NS_FLOAT_MANAGER_CACHE_SIZE 4

class nsFloatManager {
public:
  explicit nsFloatManager(nsIPresShell* aPresShell);
  ~nsFloatManager();

  void* operator new(size_t aSize) CPP_THROW_NEW;
  void operator delete(void* aPtr, size_t aSize);

  static void Shutdown();

  




  static nsRect GetRegionFor(nsIFrame* aFloatFrame);
  






  static nsRect CalculateRegionFor(nsIFrame* aFloatFrame,
                                   const nsMargin& aMargin);
  




  static void StoreRegionFor(nsIFrame* aFloat, nsRect& aRegion);

  
  
  struct SavedState;
  friend struct SavedState;
  struct SavedState {
  private:
    uint32_t mFloatInfoCount;
    nscoord mX, mY;
    bool mPushedLeftFloatPastBreak;
    bool mPushedRightFloatPastBreak;
    bool mSplitLeftFloatAcrossBreak;
    bool mSplitRightFloatAcrossBreak;

    friend class nsFloatManager;
  };

  




  void Translate(nscoord aDx, nscoord aDy) { mX += aDx; mY += aDy; }

  




  void GetTranslation(nscoord& aX, nscoord& aY) const { aX = mX; aY = mY; }

  




































  enum BandInfoType { BAND_FROM_POINT, WIDTH_WITHIN_HEIGHT };
  nsFlowAreaRect GetFlowArea(nscoord aY, BandInfoType aInfoType,
                             nscoord aHeight, nsRect aContentArea,
                             SavedState* aState) const;

  






  nsresult AddFloat(nsIFrame* aFloatFrame, const nsRect& aMarginRect);

  






  void SetPushedLeftFloatPastBreak()
    { mPushedLeftFloatPastBreak = true; }
  void SetPushedRightFloatPastBreak()
    { mPushedRightFloatPastBreak = true; }

  




  void SetSplitLeftFloatAcrossBreak()
    { mSplitLeftFloatAcrossBreak = true; }
  void SetSplitRightFloatAcrossBreak()
    { mSplitRightFloatAcrossBreak = true; }

  







  nsresult RemoveTrailingRegions(nsIFrame* aFrameList);

private:
  struct FloatInfo;
public:

  bool HasAnyFloats() const { return !mFloats.IsEmpty(); }

  



  bool HasFloatDamage() const
  {
    return !mFloatDamage.IsEmpty();
  }

  void IncludeInDamage(nscoord aIntervalBegin, nscoord aIntervalEnd)
  {
    mFloatDamage.IncludeInterval(aIntervalBegin + mY, aIntervalEnd + mY);
  }

  bool IntersectsDamage(nscoord aIntervalBegin, nscoord aIntervalEnd) const
  {
    return mFloatDamage.Intersects(aIntervalBegin + mY, aIntervalEnd + mY);
  }

  


  void PushState(SavedState* aState);

  









  void PopState(SavedState* aState);

  






  nscoord GetLowestFloatTop() const;

  





  enum {
    
    
    DONT_CLEAR_PUSHED_FLOATS = (1<<0)
  };
  nscoord ClearFloats(nscoord aY, uint8_t aBreakType, uint32_t aFlags = 0) const;

  



  bool ClearContinues(uint8_t aBreakType) const;

  void AssertStateMatches(SavedState *aState) const
  {
    NS_ASSERTION(aState->mX == mX && aState->mY == mY &&
                 aState->mPushedLeftFloatPastBreak ==
                   mPushedLeftFloatPastBreak &&
                 aState->mPushedRightFloatPastBreak ==
                   mPushedRightFloatPastBreak &&
                 aState->mSplitLeftFloatAcrossBreak ==
                   mSplitLeftFloatAcrossBreak &&
                 aState->mSplitRightFloatAcrossBreak ==
                   mSplitRightFloatAcrossBreak &&
                 aState->mFloatInfoCount == mFloats.Length(),
                 "float manager state should match saved state");
  }

#ifdef DEBUG_FRAME_DUMP
  


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

  
  
  
  
  
  
  bool mPushedLeftFloatPastBreak;
  bool mPushedRightFloatPastBreak;

  
  
  
  bool mSplitLeftFloatAcrossBreak;
  bool mSplitRightFloatAcrossBreak;

  static int32_t sCachedFloatManagerCount;
  static void* sCachedFloatManagers[NS_FLOAT_MANAGER_CACHE_SIZE];

  nsFloatManager(const nsFloatManager&) MOZ_DELETE;
  void operator=(const nsFloatManager&) MOZ_DELETE;
};






class nsAutoFloatManager {
public:
  explicit nsAutoFloatManager(nsHTMLReflowState& aReflowState)
    : mReflowState(aReflowState),
      mNew(nullptr),
      mOld(nullptr) {}

  ~nsAutoFloatManager();

  




  nsresult
  CreateFloatManager(nsPresContext *aPresContext);

protected:
  nsHTMLReflowState &mReflowState;
  nsFloatManager *mNew;
  nsFloatManager *mOld;
};

#endif 
