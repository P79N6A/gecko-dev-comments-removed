








































#include "prtypes.h"
#include "nsIAtom.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsMargin.h"
#include "nsILookAndFeel.h"
#include "nsWidgetAtoms.h"

class nsIFrame;
class nsIPresShell;
class nsPresContext;

class nsNativeTheme
{
 protected:

  enum TreeSortDirection {
    eTreeSortDirection_Descending,
    eTreeSortDirection_Natural,
    eTreeSortDirection_Ascending
  };

  nsNativeTheme();

  
  PRInt32 GetContentState(nsIFrame* aFrame, PRUint8 aWidgetType);

  
  
  
  PRBool IsWidgetStyled(nsPresContext* aPresContext, nsIFrame* aFrame,
                        PRUint8 aWidgetType);                                              

  

  
  PRBool IsDisabled(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::disabled);
  }

  
  PRBool IsDefaultButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::_default);
  }

  
  PRBool IsChecked(nsIFrame* aFrame) {
    return GetCheckedOrSelected(aFrame, PR_FALSE);
  }

  
  PRBool IsSelected(nsIFrame* aFrame) {
    return GetCheckedOrSelected(aFrame, PR_TRUE);
  }
  
  PRBool IsFocused(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::focused);
  }

  
  PRBool IsSelectedTab(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::selected);
  }

  
  PRBool IsCheckedButton(nsIFrame* aFrame) {
    return CheckBooleanAttr(aFrame, nsWidgetAtoms::checked);
  }
  
  
  TreeSortDirection GetTreeSortDirection(nsIFrame* aFrame);

  
  PRBool IsBottomTab(nsIFrame* aFrame);
  PRBool IsFirstTab(nsIFrame* aFrame);
  PRBool IsLastTab(nsIFrame* aFrame);

  
  PRBool IsIndeterminateProgress(nsIFrame* aFrame);

  PRInt32 GetProgressValue(nsIFrame* aFrame) {
    return CheckIntAttr(aFrame, nsWidgetAtoms::value);
  }

  
  PRBool IsReadOnly(nsIFrame* aFrame) {
      return CheckBooleanAttr(aFrame, nsWidgetAtoms::readonly);
  }

  
  nsIPresShell *GetPresShell(nsIFrame* aFrame);
  PRInt32 CheckIntAttr(nsIFrame* aFrame, nsIAtom* aAtom);
  PRBool CheckBooleanAttr(nsIFrame* aFrame, nsIAtom* aAtom);

  PRBool GetCheckedOrSelected(nsIFrame* aFrame, PRBool aCheckSelected);

  
  
  static nsMargin                  sButtonBorderSize;
  static nsMargin                  sButtonDisabledBorderSize;
  static PRUint8                   sButtonActiveBorderStyle;
  static PRUint8                   sButtonInactiveBorderStyle;
  static nsILookAndFeel::nsColorID sButtonBorderColorID;
  static nsILookAndFeel::nsColorID sButtonDisabledBorderColorID;
  static nsILookAndFeel::nsColorID sButtonBGColorID;
  static nsILookAndFeel::nsColorID sButtonDisabledBGColorID;
  static nsMargin                  sTextfieldBorderSize;
  static PRUint8                   sTextfieldBorderStyle;
  static nsILookAndFeel::nsColorID sTextfieldBorderColorID;
  static PRBool                    sTextfieldBGTransparent;
  static nsILookAndFeel::nsColorID sTextfieldBGColorID;
  static nsILookAndFeel::nsColorID sTextfieldDisabledBGColorID;
  static nsMargin                  sListboxBorderSize;
  static PRUint8                   sListboxBorderStyle;
  static nsILookAndFeel::nsColorID sListboxBorderColorID;
  static PRBool                    sListboxBGTransparent;
  static nsILookAndFeel::nsColorID sListboxBGColorID;
  static nsILookAndFeel::nsColorID sListboxDisabledBGColorID;
};
