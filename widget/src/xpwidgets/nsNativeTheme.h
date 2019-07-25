








































#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsMargin.h"
#include "nsILookAndFeel.h"
#include "nsWidgetAtoms.h"
#include "nsEventStates.h"
#include "nsTArray.h"
#include "nsITimer.h"

class nsIContent;
class nsIFrame;
class nsIPresShell;
class nsPresContext;

class nsNativeTheme : public nsITimerCallback
{
 protected:

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

  
  nsEventStates GetContentState(nsIFrame* aFrame, PRUint8 aWidgetType);

  
  
  
  PRBool IsWidgetStyled(nsPresContext* aPresContext, nsIFrame* aFrame,
                        PRUint8 aWidgetType);                                              

  

  bool IsDisabled(nsIFrame* aFrame, nsEventStates aEventStates);

  
  PRBool IsFrameRTL(nsIFrame* aFrame);

  
  PRBool IsDefaultButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::_default);
  }

  PRBool IsButtonTypeMenu(nsIFrame* aFrame);

  
  PRBool IsChecked(nsIFrame* aFrame) {
    return GetCheckedOrSelected(aFrame, PR_FALSE);
  }

  
  PRBool IsSelected(nsIFrame* aFrame) {
    return GetCheckedOrSelected(aFrame, PR_TRUE);
  }
  
  PRBool IsFocused(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::focused);
  }
  
  
  PRInt32 GetScrollbarButtonType(nsIFrame* aFrame);

  
  PRBool IsSelectedTab(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::selected);
  }
  
  PRBool IsNextToSelectedTab(nsIFrame* aFrame, PRInt32 aOffset);
  
  PRBool IsBeforeSelectedTab(nsIFrame* aFrame) {
    return IsNextToSelectedTab(aFrame, -1);
  }
  
  PRBool IsAfterSelectedTab(nsIFrame* aFrame) {
    return IsNextToSelectedTab(aFrame, 1);
  }

  PRBool IsLeftToSelectedTab(nsIFrame* aFrame) {
    return IsFrameRTL(aFrame) ? IsAfterSelectedTab(aFrame) : IsBeforeSelectedTab(aFrame);
  }

  PRBool IsRightToSelectedTab(nsIFrame* aFrame) {
    return IsFrameRTL(aFrame) ? IsBeforeSelectedTab(aFrame) : IsAfterSelectedTab(aFrame);
  }

  
  PRBool IsCheckedButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::checked);
  }

  PRBool IsOpenButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::open);
  }

  
  TreeSortDirection GetTreeSortDirection(nsIFrame* aFrame);
  PRBool IsLastTreeHeaderCell(nsIFrame* aFrame);

  
  PRBool IsBottomTab(nsIFrame* aFrame);
  PRBool IsFirstTab(nsIFrame* aFrame);
  PRBool IsLastTab(nsIFrame* aFrame);
  
  PRBool IsHorizontal(nsIFrame* aFrame);

  
  PRBool IsIndeterminateProgress(nsIFrame* aFrame, nsEventStates aEventStates);
  PRBool IsVerticalProgress(nsIFrame* aFrame);

  
  PRBool IsReadOnly(nsIFrame* aFrame) {
      return CheckBooleanAttr(aFrame, nsWidgetAtoms::readonly);
  }

  
  PRBool IsSubmenu(nsIFrame* aFrame, PRBool* aLeftOfParent);

  nsIPresShell *GetPresShell(nsIFrame* aFrame);
  PRInt32 CheckIntAttr(nsIFrame* aFrame, nsIAtom* aAtom, PRInt32 defaultValue);
  PRBool CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom);

  PRBool GetCheckedOrSelected(nsIFrame* aFrame, PRBool aCheckSelected);
  PRBool GetIndeterminate(nsIFrame* aFrame);

  PRBool QueueAnimatedContentForRefresh(nsIContent* aContent,
                                        PRUint32 aMinimumFrameRate);

 private:
  PRUint32 mAnimatedContentTimeout;
  nsCOMPtr<nsITimer> mAnimatedContentTimer;
  nsAutoTArray<nsCOMPtr<nsIContent>, 20> mAnimatedContentList;
};
