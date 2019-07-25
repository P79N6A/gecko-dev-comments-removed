








































#include "prtypes.h"
#include "nsAlgorithm.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsMargin.h"
#include "nsGkAtoms.h"
#include "nsEventStates.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsClassHashtable.h"
#include "nsIObserver.h"
#include "mozilla/TimeStamp.h"

class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsPresContext;

class nsNativeTheme :
  public nsITimerCallback,
  public nsIObserver
{
 protected:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIOBSERVER

  enum ScrollbarButtonType {
    eScrollbarButton_UpTop   = 0,
    eScrollbarButton_Down    = 1 << 0,
    eScrollbarButton_Bottom  = 1 << 1
  };

  enum TreeSortDirection {
    eTreeSortDirection_Descending,
    eTreeSortDirection_Natural,
    eTreeSortDirection_Ascending
  };

  nsNativeTheme();

  
  nsEventStates GetContentState(nsIFrame* aFrame, PRUint8 aWidgetType);

  
  
  
  bool IsWidgetStyled(nsPresContext* aPresContext, nsIFrame* aFrame,
                        PRUint8 aWidgetType);                                              

  

  bool IsDisabled(nsIFrame* aFrame, nsEventStates aEventStates);

  
  bool IsFrameRTL(nsIFrame* aFrame);

  
  bool IsDefaultButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsGkAtoms::_default);
  }

  bool IsButtonTypeMenu(nsIFrame* aFrame);

  
  bool IsChecked(nsIFrame* aFrame) {
    return GetCheckedOrSelected(aFrame, false);
  }

  
  bool IsSelected(nsIFrame* aFrame) {
    return GetCheckedOrSelected(aFrame, true);
  }
  
  bool IsFocused(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsGkAtoms::focused);
  }
  
  
  PRInt32 GetScrollbarButtonType(nsIFrame* aFrame);

  
  bool IsSelectedTab(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsGkAtoms::selected);
  }
  
  bool IsNextToSelectedTab(nsIFrame* aFrame, PRInt32 aOffset);
  
  bool IsBeforeSelectedTab(nsIFrame* aFrame) {
    return IsNextToSelectedTab(aFrame, -1);
  }
  
  bool IsAfterSelectedTab(nsIFrame* aFrame) {
    return IsNextToSelectedTab(aFrame, 1);
  }

  bool IsLeftToSelectedTab(nsIFrame* aFrame) {
    return IsFrameRTL(aFrame) ? IsAfterSelectedTab(aFrame) : IsBeforeSelectedTab(aFrame);
  }

  bool IsRightToSelectedTab(nsIFrame* aFrame) {
    return IsFrameRTL(aFrame) ? IsBeforeSelectedTab(aFrame) : IsAfterSelectedTab(aFrame);
  }

  
  bool IsCheckedButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsGkAtoms::checked);
  }

  bool IsSelectedButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsGkAtoms::checked) ||
           CheckBooleanAttr(aFrame, nsGkAtoms::selected);
  }

  bool IsOpenButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsGkAtoms::open);
  }

  bool IsPressedButton(nsIFrame* aFrame);

  
  TreeSortDirection GetTreeSortDirection(nsIFrame* aFrame);
  bool IsLastTreeHeaderCell(nsIFrame* aFrame);

  
  bool IsBottomTab(nsIFrame* aFrame);
  bool IsFirstTab(nsIFrame* aFrame);
  
  bool IsHorizontal(nsIFrame* aFrame);

  
  bool IsIndeterminateProgress(nsIFrame* aFrame, nsEventStates aEventStates);
  bool IsVerticalProgress(nsIFrame* aFrame);

  
  bool IsReadOnly(nsIFrame* aFrame) {
      return CheckBooleanAttr(aFrame, nsGkAtoms::readonly);
  }

  
  bool IsSubmenu(nsIFrame* aFrame, bool* aLeftOfParent);

  
  bool IsRegularMenuItem(nsIFrame *aFrame);

  bool IsMenuListEditable(nsIFrame *aFrame);

  nsIPresShell *GetPresShell(nsIFrame* aFrame);
  PRInt32 CheckIntAttr(nsIFrame* aFrame, nsIAtom* aAtom, PRInt32 defaultValue);
  bool CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom);

  bool GetCheckedOrSelected(nsIFrame* aFrame, bool aCheckSelected);
  bool GetIndeterminate(nsIFrame* aFrame);

  nsIFrame* GetAdjacentSiblingFrameWithSameAppearance(nsIFrame* aFrame,
                                                      bool aNextSibling);

  
  bool QueueAnimatedContentForRefresh(nsIContent* aContent,
                                      PRUint32 aMinimumFrameRate);

  















  
  typedef enum FadeState {
    FADE_NOTACTIVE = 0,   
    FADE_IN = 1,          
    FADE_IN_FINISHED = 2, 
    FADE_OUT = 3,         
  };

  














  bool
  QueueAnimatedContentRefreshForFade(nsIContent* aContent,
                                     FadeState aFadeDirection,
                                     PRUint32 aMinimumFrameRate,
                                     PRUint32 aMilliseconds,
                                     PRUint32 aUserData = 0);

  
  #define TICK_MAX 100.0

  
  class FadeData
  {
  public:
    





    FadeData(TimeStamp aTimeout, FadeState aState, PRUint32 aUserData) :
      mTimeout(aTimeout),
      mStartTime(TimeStamp::Now()),
      mState(aState),
      mUserData(aUserData) {
    }
    ~FadeData() {}

    




    void Reset(TimeDuration aTimeout, FadeState aState) { 
      NS_ASSERTION((aState == FADE_IN || aState == FADE_OUT),
                   "Bad fade direction.");
      mStartTime = TimeStamp::Now();
      mTimeout = TimeStamp::Now() + aTimeout;
      mState = aState;
    }

    




    PRUint32 GetTicks() {
      TimeStamp now = TimeStamp::Now();
      if (now >= mTimeout) {
        return (mState == FADE_OUT ? 0 : (PRUint32)TICK_MAX);
      }
      TimeDuration diff = now - mStartTime;
      PRUint32 tick =
        (PRUint32)ceil((diff / (mTimeout - mStartTime)) * TICK_MAX);
      
      if (mState == FADE_OUT) {
        tick = (PRUint32)abs(tick - TICK_MAX);
      }
      return tick;
    }

    



    TimeDuration TimeoutUsed() {
      TimeDuration used = TimeStamp::Now() - mStartTime;
      TimeDuration totalTime = mTimeout - mStartTime;
      return NS_MIN(used, totalTime);
    }

    


    TimeStamp GetTimeout() { return mTimeout; }
    FadeState GetState() { return mState; }
    void FadeInFinished() { mState = FADE_IN_FINISHED; }
    PRUint32 GetUserData() { return mUserData; }
    void SetUserData(PRUint32 aUserData) { mUserData = aUserData; }

  private:
    TimeStamp mTimeout;
    TimeStamp mStartTime;
    FadeState mState;
    PRUint32 mUserData;
  };

  



  
  FadeData* GetFade(nsIContent* aContent);
  
  FadeState GetFadeState(nsIContent* aContent);
  
  
  
  PRUint32 GetFadeTicks(nsIContent* aContent);
  
  
  double GetFadeAlpha(nsIContent* aContent);
  
  PRUint32 GetFadeUserData(nsIContent* aContent);
  void SetFadeUserData(nsIContent* aContent, PRUint32 aUserData);
  
  void CancelFade(nsIContent* aContent);
  
  void FinishFadeIn(nsIContent* aContent);

 private:
  nsresult InitFadeList();

  PRUint32 mAnimatedContentTimeout;
  nsCOMPtr<nsITimer> mAnimatedContentTimer;
  
  
  
  nsAutoTArray<nsCOMPtr<nsIContent>, 20> mAnimatedContentList;
  
  
  nsClassHashtable<nsISupportsHashKey, FadeData> mAnimatedFadesList;
};
