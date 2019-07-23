




































#include <Appearance.h>

#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsILookAndFeel.h"
#include "nsIDeviceContext.h"
#include "nsNativeTheme.h"

class nsNativeThemeMac : private nsNativeTheme,
                         public nsITheme
{
public:
  nsNativeThemeMac();
  virtual ~nsNativeThemeMac();

  NS_DECL_ISUPPORTS

  
  NS_IMETHOD DrawWidgetBackground(nsIRenderingContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  const nsRect& aRect,
                                  const nsRect& aClipRect);
  NS_IMETHOD GetWidgetBorder(nsIDeviceContext* aContext, 
                             nsIFrame* aFrame,
                             PRUint8 aWidgetType,
                             nsMargin* aResult);

  virtual PRBool GetWidgetPadding(nsIDeviceContext* aContext,
                                  nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsMargin* aResult);

  NS_IMETHOD GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsSize* aResult, PRBool* aIsOverridable);
  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                nsIAtom* aAttribute, PRBool* aShouldRepaint);
  NS_IMETHOD ThemeChanged();
  PRBool ThemeSupportsWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType);
  PRBool WidgetIsContainer(PRUint8 aWidgetType);

protected:

    
  enum {
    kAquaPushButtonEndcaps = 14,
    kAquaPushButtonTopBottom = 2,
    kAquaSmallPushButtonEndcaps = 10,

    kAquaDropdownLeftEndcap = 9,
    kAquaDropwdonRightEndcap = 20     
  };
  
  nsresult GetSystemColor(PRUint8 aWidgetType, nsILookAndFeel::nsColorID& aColorID);
  nsresult GetSystemFont(PRUint8 aWidgetType, nsSystemFontID& aFont);


  
  void DrawCheckbox ( const Rect& inBoxRect, PRBool inChecked, PRBool inDisabled, PRInt32 inState ) ;
  void DrawSmallCheckbox ( const Rect& inBoxRect, PRBool inChecked, PRBool inDisabled, PRInt32 inState );
  void DrawRadio ( const Rect& inBoxRect, PRBool inChecked, PRBool inDisabled, PRInt32 inState ) ;
  void DrawSmallRadio ( const Rect& inBoxRect, PRBool inChecked, PRBool inDisabled, PRInt32 inState );
  void DrawToolbar ( const Rect& inBoxRect ) ;
  void DrawEditText ( const Rect& inBoxRect, PRBool inIsDisabled ) ;
  void DrawListBox ( const Rect& inBoxRect, PRBool inIsDisabled ) ;
  void DrawProgress ( const Rect& inBoxRect, PRBool inIsDisabled, PRBool inIsIndeterminate, 
                        PRBool inIsHorizontal, PRInt32 inValue ) ;
  void DrawTab ( const Rect& inBoxRect, PRBool inIsDisabled, PRBool inIsFrontmost, 
                  PRBool inIsHorizontal, PRBool inTabBottom, PRInt32 inState ) ;
  void DrawTabPanel ( const Rect& inBoxRect, PRBool inIsDisabled ) ;
  void DrawScale ( const Rect& inBoxRect, PRBool inIsDisabled, PRInt32 inState,
                   PRBool inDirection, PRInt32 inCurrentValue,
                   PRInt32 inMinValue, PRInt32 inMaxValue ) ;
  void DrawSeparator ( const Rect& inBoxRect, PRBool inIsDisabled ) ;

  
  void DrawButton ( ThemeButtonKind inKind, const Rect& inBoxRect, PRBool inIsDefault, 
                      PRBool inDisabled, ThemeButtonValue inValue, ThemeButtonAdornment inAdornment, PRInt32 inState ) ;
  void DrawSpinButtons ( ThemeButtonKind inKind, const Rect& inBoxRect,
                         PRBool inDisabled, ThemeDrawState inDrawState,
                         ThemeButtonAdornment inAdornment, PRInt32 inState ) ;
  void DrawCheckboxRadio ( ThemeButtonKind inKind, const Rect& inBoxRect, PRBool inChecked, 
                              PRBool inDisabled, PRInt32 inState ) ;
  void DrawMenu ( const Rect& inBoxRect, PRBool inIsDisabled ) ;
  void DrawMenuItem ( const Rect& inBoxRect, ThemeMenuItemType itemType, PRBool inIsDisabled,
                        PRBool inHover) ;

private:

  ThemeEraseUPP mEraseProc;
};
