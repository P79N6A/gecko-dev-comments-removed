







#include "nsAlgorithm.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsMargin.h"
#include "nsGkAtoms.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsIContent.h"

class nsIFrame;
class nsIPresShell;
class nsPresContext;

namespace mozilla {
class EventStates;
} 

class nsNativeTheme : public nsITimerCallback
{
 protected:
  virtual ~nsNativeTheme() {}

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

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

  
  mozilla::EventStates GetContentState(nsIFrame* aFrame, uint8_t aWidgetType);

  
  
  
  bool IsWidgetStyled(nsPresContext* aPresContext, nsIFrame* aFrame,
                        uint8_t aWidgetType);                                              

  

  bool IsDisabled(nsIFrame* aFrame, mozilla::EventStates aEventStates);

  
  bool IsFrameRTL(nsIFrame* aFrame);

  bool IsHTMLContent(nsIFrame *aFrame);
  
  
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

  
  int32_t GetScrollbarButtonType(nsIFrame* aFrame);

  
  bool IsSelectedTab(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsGkAtoms::visuallyselected);
  }
  
  bool IsNextToSelectedTab(nsIFrame* aFrame, int32_t aOffset);
  
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

  
  bool IsIndeterminateProgress(nsIFrame* aFrame,
                               mozilla::EventStates aEventStates);
  bool IsVerticalProgress(nsIFrame* aFrame);

  
  bool IsVerticalMeter(nsIFrame* aFrame);

  
  bool IsReadOnly(nsIFrame* aFrame) {
      return CheckBooleanAttr(aFrame, nsGkAtoms::readonly);
  }

  
  bool IsSubmenu(nsIFrame* aFrame, bool* aLeftOfParent);

  
  bool IsRegularMenuItem(nsIFrame *aFrame);

  bool IsMenuListEditable(nsIFrame *aFrame);

  nsIPresShell *GetPresShell(nsIFrame* aFrame);
  static bool CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom);
  static int32_t CheckIntAttr(nsIFrame* aFrame, nsIAtom* aAtom, int32_t defaultValue);

  
  static double GetProgressValue(nsIFrame* aFrame);
  static double GetProgressMaxValue(nsIFrame* aFrame);

  bool GetCheckedOrSelected(nsIFrame* aFrame, bool aCheckSelected);
  bool GetIndeterminate(nsIFrame* aFrame);

  bool QueueAnimatedContentForRefresh(nsIContent* aContent,
                                        uint32_t aMinimumFrameRate);

  nsIFrame* GetAdjacentSiblingFrameWithSameAppearance(nsIFrame* aFrame,
                                                      bool aNextSibling);

  bool IsRangeHorizontal(nsIFrame* aFrame);

  
  bool IsDarkBackground(nsIFrame* aFrame);

 private:
  uint32_t mAnimatedContentTimeout;
  nsCOMPtr<nsITimer> mAnimatedContentTimer;
  nsAutoTArray<nsCOMPtr<nsIContent>, 20> mAnimatedContentList;
};
