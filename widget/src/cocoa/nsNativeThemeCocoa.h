




































#ifndef nsNativeThemeCocoa_h_
#define nsNativeThemeCocoa_h_

#import <Carbon/Carbon.h>

#include "nsITheme.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsILookAndFeel.h"
#include "nsIDeviceContext.h"
#include "nsNativeTheme.h"

class nsNativeThemeCocoa : private nsNativeTheme,
                           public nsITheme
{
public:
  nsNativeThemeCocoa();
  virtual ~nsNativeThemeCocoa();

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

  virtual PRBool GetWidgetOverflow(nsIDeviceContext* aContext, nsIFrame* aFrame,
                                   PRUint8 aWidgetType, nsRect* aResult);

  NS_IMETHOD GetMinimumWidgetSize(nsIRenderingContext* aContext, nsIFrame* aFrame,
                                  PRUint8 aWidgetType,
                                  nsSize* aResult, PRBool* aIsOverridable);
  NS_IMETHOD WidgetStateChanged(nsIFrame* aFrame, PRUint8 aWidgetType, 
                                nsIAtom* aAttribute, PRBool* aShouldRepaint);
  NS_IMETHOD ThemeChanged();
  PRBool ThemeSupportsWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType);
  PRBool WidgetIsContainer(PRUint8 aWidgetType);
  PRBool ThemeDrawsFocusForWidget(nsPresContext* aPresContext, nsIFrame* aFrame, PRUint8 aWidgetType);
  PRBool ThemeNeedsComboboxDropmarker();

protected:

  
  static const int kAquaDropdownLeftEndcap = 9;
  static const int kAquaDropwdonRightEndcap = 20; 

  nsresult GetSystemColor(PRUint8 aWidgetType, nsILookAndFeel::nsColorID& aColorID);
  nsresult GetSystemFont(PRUint8 aWidgetType, nsSystemFontID& aFont);


  
  void DrawFrame (CGContextRef context, HIThemeFrameKind inKind,
                  const HIRect& inBoxRect, PRBool inIsDisabled,
                  PRInt32 inState);
  void DrawProgress(CGContextRef context, const HIRect& inBoxRect,
                    PRBool inIsIndeterminate, PRBool inIsHorizontal,
                    PRInt32 inValue);
  void DrawTab (CGContextRef context, const HIRect& inBoxRect,
                PRBool inIsDisabled, PRBool inIsFrontmost, 
                PRBool inIsHorizontal, PRBool inTabBottom,
                PRInt32 inState);
  void DrawTabPanel (CGContextRef context, const HIRect& inBoxRect);
  void DrawScale (CGContextRef context, const HIRect& inBoxRect,
                  PRBool inIsDisabled, PRInt32 inState,
                  PRBool inDirection, PRBool inIsReverse,
                  PRInt32 inCurrentValue,
                  PRInt32 inMinValue, PRInt32 inMaxValue);
  void DrawButton (CGContextRef context, ThemeButtonKind inKind,
                   const HIRect& inBoxRect, PRBool inIsDefault, 
                   PRBool inDisabled, ThemeButtonValue inValue,
                   ThemeButtonAdornment inAdornment, PRInt32 inState);
  void DrawSpinButtons (CGContextRef context, ThemeButtonKind inKind,
                        const HIRect& inBoxRect,
                        PRBool inDisabled, ThemeDrawState inDrawState,
                        ThemeButtonAdornment inAdornment, PRInt32 inState);
  void DrawCheckboxRadio (CGContextRef context, ThemeButtonKind inKind,
                          const HIRect& inBoxRect, PRBool inChecked, 
                          PRBool inDisabled, PRInt32 inState);
  
  void DrawScrollbar(CGContextRef aCGContext, const HIRect& aBoxRect, nsIFrame *aFrame);
  void GetScrollbarPressStates (nsIFrame *aFrame, PRInt32 aButtonStates[]);
  void GetScrollbarDrawInfo (HIThemeTrackDrawInfo& aTdi, nsIFrame *aFrame, 
                             const HIRect& aRect, PRBool aShouldGetButtonStates);
  nsIFrame* GetParentScrollbarFrame(nsIFrame *aFrame);
};

#endif 
