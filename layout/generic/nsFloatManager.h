







#ifndef nsFloatManager_h_
#define nsFloatManager_h_

#include "mozilla/Attributes.h"
#include "mozilla/WritingModes.h"
#include "nsCoord.h"
#include "nsFrameList.h" 
#include "nsIntervalSet.h"
#include "nsTArray.h"

class nsIPresShell;
class nsIFrame;
struct nsHTMLReflowState;
class nsPresContext;








struct nsFlowAreaRect {
  mozilla::LogicalRect mRect;
  bool mHasFloats;

  nsFlowAreaRect(mozilla::WritingMode aWritingMode,
                 nscoord aICoord, nscoord aBCoord,
                 nscoord aISize, nscoord aBSize,
                 bool aHasFloats)
    : mRect(aWritingMode, aICoord, aBCoord, aISize, aBSize)
    , mHasFloats(aHasFloats) {}
};

#define NS_FLOAT_MANAGER_CACHE_SIZE 4

class nsFloatManager {
public:
  explicit nsFloatManager(nsIPresShell* aPresShell, mozilla::WritingMode aWM);
  ~nsFloatManager();

  void* operator new(size_t aSize) CPP_THROW_NEW;
  void operator delete(void* aPtr, size_t aSize);

  static void Shutdown();

  




  static mozilla::LogicalRect GetRegionFor(mozilla::WritingMode aWM,
                                           nsIFrame* aFloatFrame,
                                           nscoord aContainerWidth);
  






  static mozilla::LogicalRect CalculateRegionFor(
                                mozilla::WritingMode aWM,
                                nsIFrame* aFloatFrame,
                                const mozilla::LogicalMargin& aMargin,
                                nscoord aContainerWidth);
  




  static void StoreRegionFor(mozilla::WritingMode aWM,
                             nsIFrame* aFloat,
                             const mozilla::LogicalRect& aRegion,
                             nscoord aContainerWidth);

  
  
  struct SavedState {
    explicit SavedState() {}
  private:
    uint32_t mFloatInfoCount;
    nscoord mLineLeft, mBlockStart;
    bool mPushedLeftFloatPastBreak;
    bool mPushedRightFloatPastBreak;
    bool mSplitLeftFloatAcrossBreak;
    bool mSplitRightFloatAcrossBreak;

    friend class nsFloatManager;
  };

  





  void Translate(nscoord aLineLeft, nscoord aBlockStart)
  {
    mLineLeft += aLineLeft;
    mBlockStart += aBlockStart;
  }

  




  void GetTranslation(nscoord& aLineLeft, nscoord& aBlockStart) const
  {
    aLineLeft = mLineLeft;
    aBlockStart = mBlockStart;
  }

  





































  enum BandInfoType { BAND_FROM_POINT, WIDTH_WITHIN_HEIGHT };
  nsFlowAreaRect GetFlowArea(mozilla::WritingMode aWM,
                             nscoord aBCoord, BandInfoType aInfoType,
                             nscoord aBSize, mozilla::LogicalRect aContentArea,
                             SavedState* aState, nscoord mContainerWidth) const;

  







  nsresult AddFloat(nsIFrame* aFloatFrame,
                    const mozilla::LogicalRect& aMarginRect,
                    mozilla::WritingMode aWM, nscoord aContainerWidth);

  






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

  void IncludeInDamage(mozilla::WritingMode aWM,
                       nscoord aIntervalBegin, nscoord aIntervalEnd)
  {
    mFloatDamage.IncludeInterval(aIntervalBegin + mBlockStart,
                                 aIntervalEnd + mBlockStart);
  }

  bool IntersectsDamage(mozilla::WritingMode aWM,
                        nscoord aIntervalBegin, nscoord aIntervalEnd) const
  {
    return mFloatDamage.Intersects(aIntervalBegin + mBlockStart,
                                   aIntervalEnd + mBlockStart);
  }

  


  void PushState(SavedState* aState);

  









  void PopState(SavedState* aState);

  






  nscoord GetLowestFloatTop() const;

  






  enum {
    
    
    DONT_CLEAR_PUSHED_FLOATS = (1<<0)
  };
  nscoord ClearFloats(nscoord aBCoord, uint8_t aBreakType,
                      uint32_t aFlags = 0) const;

  



  bool ClearContinues(uint8_t aBreakType) const;

  void AssertStateMatches(SavedState *aState) const
  {
    NS_ASSERTION(aState->mLineLeft == mLineLeft &&
                 aState->mBlockStart == mBlockStart &&
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
    
    
    nscoord mLeftBEnd, mRightBEnd;

    FloatInfo(nsIFrame* aFrame, nscoord aLineLeft, nscoord aBStart,
              nscoord aISize, nscoord aBSize);

    nscoord LineLeft() const { return mRect.x; }
    nscoord LineRight() const { return mRect.XMost(); }
    nscoord ISize() const { return mRect.width; }
    nscoord BStart() const { return mRect.y; }
    nscoord BEnd() const { return mRect.YMost(); }
    nscoord BSize() const { return mRect.height; }
    bool IsEmpty() const { return mRect.IsEmpty(); }

#ifdef NS_BUILD_REFCNT_LOGGING
    FloatInfo(const FloatInfo& aOther);
    ~FloatInfo();
#endif

  private:
    
    
    
    
    
    
    nsRect mRect;
  };

  mozilla::DebugOnly<mozilla::WritingMode> mWritingMode;

  
  nscoord mLineLeft, mBlockStart;
  nsTArray<FloatInfo> mFloats;
  nsIntervalSet   mFloatDamage;

  
  
  
  
  
  
  bool mPushedLeftFloatPastBreak;
  bool mPushedRightFloatPastBreak;

  
  
  
  bool mSplitLeftFloatAcrossBreak;
  bool mSplitRightFloatAcrossBreak;

  static int32_t sCachedFloatManagerCount;
  static void* sCachedFloatManagers[NS_FLOAT_MANAGER_CACHE_SIZE];

  nsFloatManager(const nsFloatManager&) = delete;
  void operator=(const nsFloatManager&) = delete;
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
